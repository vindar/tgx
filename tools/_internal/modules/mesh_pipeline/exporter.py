from __future__ import annotations

import math
import os
import re
import struct
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from .mesh import FaceVertex, Meshlet, ObjMesh
from .progress import Heartbeat, log
from .stripifier import DEFAULT_LKH_EXE, StripifyOptions, stripify_component


@dataclass
class MeshletExportStats:
    meshlets: int
    triangles: int
    chains: int
    vertex_refs_loaded: int
    payload_bytes: int
    payload_words: int
    materials: int
    meshlet_array_bytes: int
    material_array_bytes: int
    mesh_struct_bytes: int
    static_bytes: int


@dataclass
class MeshletExportResult:
    header: str
    stats: MeshletExportStats
    source: str | None = None


@dataclass
class _EncodedMeshlet:
    material_index: int
    vertices: list[int]
    normals: list[int]
    texcoords: list[int]
    face_stream: bytes
    chain_count: int
    vertex_refs_loaded: int
    payload_offset32: int = 0


def _identifier(name: str) -> str:
    out = re.sub(r"\W+", "_", name.strip())
    if not out:
        out = "mesh"
    if out[0].isdigit():
        out = "_" + out
    return out


def _corner_edge(
    tri: tuple[FaceVertex, FaceVertex, FaceVertex],
    i: int,
) -> tuple[FaceVertex, FaceVertex]:
    if i == 0:
        return tri[0], tri[1]
    if i == 1:
        return tri[1], tri[2]
    return tri[2], tri[0]


def _cyclic_rotations(
    tri: tuple[FaceVertex, FaceVertex, FaceVertex],
) -> tuple[
    tuple[FaceVertex, FaceVertex, FaceVertex],
    tuple[FaceVertex, FaceVertex, FaceVertex],
    tuple[FaceVertex, FaceVertex, FaceVertex],
]:
    return (
        tri,
        (tri[1], tri[2], tri[0]),
        (tri[2], tri[0], tri[1]),
    )


def _same_triangle_corners(
    a: tuple[FaceVertex, FaceVertex, FaceVertex],
    b: tuple[FaceVertex, FaceVertex, FaceVertex],
) -> bool:
    return sorted(a, key=lambda c: (c.v, c.vt, c.vn)) == sorted(b, key=lambda c: (c.v, c.vt, c.vn))


def _is_chain_adjacent(
    a: tuple[FaceVertex, FaceVertex, FaceVertex],
    b: tuple[FaceVertex, FaceVertex, FaceVertex],
) -> bool:
    edges_a = {_corner_edge(a, 0), _corner_edge(a, 1), _corner_edge(a, 2)}
    return any((edge[1], edge[0]) in edges_a for edge in (_corner_edge(b, 0), _corner_edge(b, 1), _corner_edge(b, 2)))


def _with_new_corner_last(
    tri: tuple[FaceVertex, FaceVertex, FaceVertex],
    new_corner: FaceVertex,
) -> tuple[FaceVertex, FaceVertex, FaceVertex]:
    for rotated in _cyclic_rotations(tri):
        if rotated[2] == new_corner:
            return rotated
    raise RuntimeError("internal strip error: new corner is not in triangle")


def _make_chains(
    mesh: ObjMesh,
    tri_indices: list[int],
    *,
    use_lkh: bool,
    lkh_exe: str | Path,
    lkh_time_limit_s: float | None = None,
) -> list[list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]]]:
    if use_lkh:
        result = stripify_component(
            mesh,
            tri_indices,
            StripifyOptions(max_time_s=lkh_time_limit_s, lkh_exe=lkh_exe),
        )
        ordered_runs = result.chains
    else:
        ordered_runs = [tri_indices.copy()]

    if not ordered_runs:
        return []

    runs: list[list[tuple[FaceVertex, FaceVertex, FaceVertex]]] = []
    for ordered in ordered_runs:
        current: list[tuple[FaceVertex, FaceVertex, FaceVertex]] = []
        for ti in ordered:
            tri = mesh.triangles[ti].corners
            if (not current) or (len(current) >= 255) or (not _is_chain_adjacent(current[-1], tri)):
                current = [tri]
                runs.append(current)
            else:
                current.append(tri)

    chains: list[list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]]] = []
    for run in runs:
        start = 0
        while start < len(run):
            chain, consumed = _encode_chain_prefix(run[start:])
            chains.append(chain)
            start += consumed
    return chains


def _append_encoded_triangle(
    storage: list[FaceVertex],
    pointers: list[int],
    target: tuple[FaceVertex, FaceVertex, FaceVertex],
) -> tuple[int, FaceVertex, list[FaceVertex], list[int]] | None:
    for direction in (0, 1):
        swap_index = 0 if direction else 1
        for new_corner in target:
            new_storage = storage.copy()
            new_pointers = pointers.copy()
            new_pointers[swap_index], new_pointers[2] = new_pointers[2], new_pointers[swap_index]
            new_storage[new_pointers[2]] = new_corner
            if _same_triangle_corners(tuple(new_storage), target):
                return direction, new_corner, new_storage, new_pointers
    return None


def _encode_chain_prefix(
    run: list[tuple[FaceVertex, FaceVertex, FaceVertex]],
) -> tuple[list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]], int]:
    best_chain: list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]] | None = None
    best_consumed = 0

    for first in _cyclic_rotations(run[0]):
        storage = list(first)
        pointers = [0, 1, 2]
        chain: list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]] = [(None, first)]
        consumed = 1
        for target in run[1:]:
            encoded = _append_encoded_triangle(storage, pointers, target)
            if encoded is None:
                break
            direction, new_corner, storage, pointers = encoded
            chain.append((direction, _with_new_corner_last(target, new_corner)))
            consumed += 1
        if consumed > best_consumed:
            best_chain = chain
            best_consumed = consumed
            if consumed == len(run):
                break

    if best_chain is None:
        return [(None, run[0])], 1
    return best_chain, best_consumed


def _append_corner(
    out: bytearray,
    corner: FaceVertex,
    v_map: dict[int, int],
    vt_map: dict[int, int],
    vn_map: dict[int, int],
    *,
    direction: int = 0,
    has_texcoords: bool,
    has_normals: bool,
) -> None:
    v = v_map[corner.v]
    if v > 127:
        raise ValueError("meshlet local vertex index exceeded 127")
    out.append(v | (0x80 if direction else 0))
    if has_texcoords:
        out.append(vt_map[corner.vt])
    if has_normals:
        out.append(vn_map[corner.vn])


def _register_corner_in_stream_order(
    corner: FaceVertex,
    vertices: list[int],
    normals: list[int],
    texcoords: list[int],
    v_map: dict[int, int],
    vt_map: dict[int, int],
    vn_map: dict[int, int],
    *,
    has_texcoords: bool,
    has_normals: bool,
) -> None:
    if corner.v not in v_map:
        v_map[corner.v] = len(vertices)
        vertices.append(corner.v)
    if has_normals and corner.vn >= 0 and corner.vn not in vn_map:
        vn_map[corner.vn] = len(normals)
        normals.append(corner.vn)
    if has_texcoords and corner.vt >= 0 and corner.vt not in vt_map:
        vt_map[corner.vt] = len(texcoords)
        texcoords.append(corner.vt)


def _append_corner_stream_ordered(
    out: bytearray,
    corner: FaceVertex,
    vertices: list[int],
    normals: list[int],
    texcoords: list[int],
    v_map: dict[int, int],
    vt_map: dict[int, int],
    vn_map: dict[int, int],
    *,
    direction: int = 0,
    has_texcoords: bool,
    has_normals: bool,
) -> None:
    _register_corner_in_stream_order(
        corner,
        vertices,
        normals,
        texcoords,
        v_map,
        vt_map,
        vn_map,
        has_texcoords=has_texcoords,
        has_normals=has_normals,
    )
    _append_corner(
        out,
        corner,
        v_map,
        vt_map,
        vn_map,
        direction=direction,
        has_texcoords=has_texcoords,
        has_normals=has_normals,
    )


def _encode_meshlet(mesh: ObjMesh, meshlet: Meshlet, material_index: int, *, use_lkh: bool, lkh_exe: str | Path) -> _EncodedMeshlet:
    chains = _make_chains(mesh, meshlet.triangles, use_lkh=use_lkh, lkh_exe=lkh_exe)

    vertices: list[int] = []
    normals: list[int] = []
    texcoords: list[int] = []
    v_map: dict[int, int] = {}
    vn_map: dict[int, int] = {}
    vt_map: dict[int, int] = {}

    has_normals = mesh.has_normals()
    has_texcoords = mesh.has_texcoords()

    stream = bytearray()
    vertex_refs_loaded = 0
    for chain in chains:
        stream.append(len(chain))
        vertex_refs_loaded += len(chain) + 2
        first = chain[0][1]
        _append_corner_stream_ordered(stream, first[0], vertices, normals, texcoords, v_map, vt_map, vn_map, has_texcoords=has_texcoords, has_normals=has_normals)
        _append_corner_stream_ordered(stream, first[1], vertices, normals, texcoords, v_map, vt_map, vn_map, has_texcoords=has_texcoords, has_normals=has_normals)
        _append_corner_stream_ordered(stream, first[2], vertices, normals, texcoords, v_map, vt_map, vn_map, has_texcoords=has_texcoords, has_normals=has_normals)
        for direction, tri in chain[1:]:
            assert direction is not None
            _append_corner_stream_ordered(stream, tri[2], vertices, normals, texcoords, v_map, vt_map, vn_map, direction=direction, has_texcoords=has_texcoords, has_normals=has_normals)
    stream.append(0)

    if has_normals and len(normals) > 255:
        raise ValueError("meshlet local normal index exceeded 255")
    if has_texcoords and len(texcoords) > 255:
        raise ValueError("meshlet local texture coordinate index exceeded 255")

    return _EncodedMeshlet(material_index, vertices, normals, texcoords, bytes(stream), len(chains), vertex_refs_loaded)


def _clamp_i16(value: int) -> int:
    return max(-32767, min(32767, value))


def _clamp_u16(value: int) -> int:
    return max(0, min(65535, value))


def _pack_i16(values) -> bytes:
    items = [int(x) for x in values]
    return struct.pack("<" + "h" * len(items), *items)


def _q_snorm16(value: float) -> int:
    return _clamp_i16(int(round(max(-1.0, min(1.0, float(value))) * 32767.0)))


def _q_texcoord16(value: float) -> int:
    return _clamp_i16(int(round(max(-4.0, min(4.0, float(value))) * (32767.0 / 4.0))))


def _payload16_for_meshlet(mesh: ObjMesh, meshlet: Meshlet, encoded: _EncodedMeshlet, *, center: np.ndarray | None = None, radius: float | None = None) -> bytes:
    payload = bytearray()
    center = meshlet.center if center is None else np.asarray(center, dtype=np.float64)
    radius = float(meshlet.radius if radius is None else radius)
    if (not math.isfinite(radius)) or radius <= 0.0:
        raise ValueError("Mesh3Dv2 export requires each meshlet to have a positive finite bounding sphere radius")
    vertex_scale = 32767.0 / radius

    for index in encoded.vertices:
        rel = mesh.vertices[index] - center
        if float(np.linalg.norm(rel)) > radius * (1.0 + 1e-6):
            raise ValueError("Mesh3Dv2 meshlet bounding sphere does not contain all local vertices")
        q = np.rint(rel * vertex_scale).astype(np.int64)
        payload.extend(_pack_i16(_clamp_i16(int(x)) for x in q))
    for index in encoded.normals:
        payload.extend(_pack_i16(_q_snorm16(x) for x in mesh.normals[index]))
    for index in encoded.texcoords:
        payload.extend(_pack_i16(_q_texcoord16(x) for x in mesh.texcoords[index]))
    payload.extend(encoded.face_stream)
    while len(payload) % 4:
        payload.append(0)
    return bytes(payload)


def _meshlet16b_scales(mesh: ObjMesh) -> tuple[np.ndarray, float, float, float]:
    bb_min, bb_max = mesh.bounding_box()
    bbox_center = 0.5 * (bb_min + bb_max)
    extent = float(np.max(bb_max - bb_min))
    if (not math.isfinite(extent)) or extent <= 1.0e-20:
        extent = 1.0
    return bbox_center, extent / 65534.0, extent / 65535.0, 1.0 / 32767.0


def _quantize_meshlet16b_metadata(
    mesh: ObjMesh,
    meshlet: Meshlet,
    encoded: _EncodedMeshlet,
    cone_source: str,
    bbox_center: np.ndarray,
    center_scale: float,
    radius_scale: float,
    snorm_scale: float,
) -> tuple[list[int], list[int], int, int, np.ndarray, float]:
    q_center = [
        _clamp_i16(int(round(float((meshlet.center[i] - bbox_center[i]) / center_scale))))
        for i in range(3)
    ]
    decoded_center = bbox_center + np.asarray(q_center, dtype=np.float64) * center_scale

    if encoded.vertices:
        radius_needed = max(float(np.linalg.norm(mesh.vertices[index] - decoded_center)) for index in encoded.vertices)
    else:
        radius_needed = float(meshlet.radius)
    radius_needed = max(radius_needed, float(meshlet.radius), 1.0e-12)
    q_radius = _clamp_u16(int(math.ceil(radius_needed / radius_scale)))
    decoded_radius = max(float(q_radius) * radius_scale, 1.0e-12)

    cone_dir, cone_cos = meshlet.selected_cull_cone(cone_source)
    if cone_cos > 1.0:
        cone_cos = -2.0
    q_dir = [_q_snorm16(float(x)) for x in cone_dir]
    if cone_cos <= -1.0:
        q_cos = -32768
    else:
        q_cos = _clamp_i16(int(math.floor((max(-1.0, min(1.0, float(cone_cos))) - 0.0015) / snorm_scale)))

    return q_center, q_dir, q_radius, q_cos, decoded_center, decoded_radius


def _fmt_float(v: float) -> str:
    if not math.isfinite(v):
        return "0.0f"
    text = f"{float(v):.9g}"
    if "e" not in text and "E" not in text and "." not in text:
        text += ".0"
    return text + "f"


def _fmt_comment_float(v: float) -> str:
    if not math.isfinite(v):
        return "0"
    return f"{float(v):.9g}"


def _format_u32_array(values: list[int]) -> str:
    lines = []
    for i in range(0, len(values), 8):
        lines.append("    " + ", ".join(f"0x{v:08x}u" for v in values[i : i + 8]) + ",")
    return "\n".join(lines)


def _format_meshlet_initializer(
    q_center: list[int],
    q_dir: list[int],
    q_radius: int,
    q_cos: int,
    item: _EncodedMeshlet,
) -> str:
    return (
        f"    {{ {{ {q_center[0]}, {q_center[1]}, {q_center[2]} }}, "
        f"{{ {q_dir[0]}, {q_dir[1]}, {q_dir[2]} }}, "
        f"{q_radius}u, {q_cos}, {item.payload_offset32}u, "
        f"{len(item.vertices)}, {len(item.normals)}, {len(item.texcoords)}, {item.material_index} }},"
    )


def _embedded_memory_estimate(
    *,
    payload_bytes: int,
    meshlets: int,
    materials: int,
) -> tuple[int, int, int, int]:
    # 32-bit MCU ABI estimate. Meshlet3Dv2 is 24 bytes, MeshMaterial3Dv2 is
    # 32 bytes with a 32-bit texture pointer, Mesh3Dv2 itself is 48 bytes.
    meshlet_array_bytes = meshlets * 24
    material_array_bytes = materials * 32
    mesh_struct_bytes = 48
    return (
        meshlet_array_bytes,
        material_array_bytes,
        mesh_struct_bytes,
        payload_bytes + meshlet_array_bytes + material_array_bytes + mesh_struct_bytes,
    )



def _export_meshlet_header_impl(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    *,
    name: str | None = None,
    color_type: str = "tgx::RGB565",
    use_lkh: bool = True,
    lkh_exe: str | Path = DEFAULT_LKH_EXE,
    texture_symbols: dict[str, str] | None = None,
    extra_includes: list[str] | None = None,
    mesh_type: str = "Mesh3Dv2",
    meshlet_type: str = "Meshlet3Dv2",
    material_type: str = "MeshMaterial3Dv2",
    mesh_id: int = 2162,
    cone_source: str = "normal",
    header_filename: str | None = None,
    single_header: bool = False,
) -> MeshletExportResult:
    symbol = _identifier(name or mesh.name or "mesh")
    materials = sorted({m.material for m in meshlets})
    if not materials:
        materials = [""]
    material_index = {mat: i for i, mat in enumerate(materials)}
    texture_symbols = texture_symbols or {}
    extra_includes = extra_includes or []

    encoded: list[_EncodedMeshlet] = [None] * len(meshlets)  # type: ignore[list-item]
    heartbeat = Heartbeat("encode meshlets")
    done_count = 0
    done_chains = 0
    workers = max(1, min(len(meshlets), (os.cpu_count() or 1) // 2))
    if len(meshlets) >= 32:
        log(f"encode meshlets: {len(meshlets)} meshlets, {workers} workers")

    def encode_one(index: int, meshlet: Meshlet) -> tuple[int, _EncodedMeshlet]:
        item = _encode_meshlet(mesh, meshlet, material_index[meshlet.material], use_lkh=use_lkh, lkh_exe=lkh_exe)
        return index, item

    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures = [pool.submit(encode_one, index, meshlet) for index, meshlet in enumerate(meshlets)]
        for future in as_completed(futures):
            index, item = future.result()
            encoded[index] = item
            done_count += 1
            done_chains += item.chain_count
            if len(meshlets) >= 32:
                if done_count == 1 or done_count == len(meshlets) or (done_count % 25) == 0:
                    log(f"encode meshlets: {done_count}/{len(meshlets)}, chains {done_chains}")
                else:
                    heartbeat.ping(f"{done_count}/{len(meshlets)}, chains {done_chains}")

    payload = bytearray()
    chains = 0
    vertex_refs_loaded = 0
    metadata16b: list[tuple[list[int], list[int], int, int, np.ndarray, float]] = []
    bbox_center, center_scale, radius_scale, snorm_scale = _meshlet16b_scales(mesh)
    for item, meshlet in zip(encoded, meshlets):
        item.payload_offset32 = len(payload) // 4
        meta = _quantize_meshlet16b_metadata(mesh, meshlet, item, cone_source, bbox_center, center_scale, radius_scale, snorm_scale)
        metadata16b.append(meta)
        block = _payload16_for_meshlet(mesh, meshlet, item, center=meta[4], radius=meta[5])
        payload.extend(block)
        chains += item.chain_count
        vertex_refs_loaded += item.vertex_refs_loaded

    payload_words = list(struct.unpack("<" + "I" * (len(payload) // 4), payload))
    bb_min, bb_max = mesh.bounding_box()
    meshlet_array_bytes, material_array_bytes, mesh_struct_bytes, static_bytes = _embedded_memory_estimate(
        payload_bytes=len(payload),
        meshlets=len(meshlets),
        materials=len(materials),
    )

    header: list[str] = []
    header.append("#pragma once")
    header.append("")
    header.append("#include <tgx.h>")
    header.append("")
    header.append(f"// {mesh_type} generated by tgx_mesh.")
    header.append(f"// Source model : {mesh.name}")
    header.append(f"// Symbol       : {symbol}")
    header.append(f"// Color type   : {color_type}")
    header.append(f"// Triangles    : {len(mesh.triangles)}")
    header.append(f"// Meshlets     : {len(meshlets)}")
    header.append(f"// Materials    : {len(materials)}")
    header.append(f"// Chains       : {chains} ({100.0 * chains / max(1, len(mesh.triangles)):.2f}% of triangles)")
    header.append(f"// Vertex refs  : {vertex_refs_loaded} ({vertex_refs_loaded / max(1, len(mesh.triangles)):.3f} per triangle)")
    header.append(f"// Payload      : {len(payload)} bytes ({len(payload_words)} uint32 words)")
    header.append("// Static memory estimate, excluding texture images:")
    header.append(f"//   payload array : {len(payload)} bytes")
    header.append(f"//   meshlet array : {meshlet_array_bytes} bytes")
    header.append(f"//   material array: {material_array_bytes} bytes")
    header.append(f"//   mesh structure: {mesh_struct_bytes} bytes")
    header.append(f"//   total         : {static_bytes} bytes")
    header.append(
        f"// Bounding box : x[{_fmt_comment_float(bb_min[0])}, {_fmt_comment_float(bb_max[0])}] "
        f"y[{_fmt_comment_float(bb_min[1])}, {_fmt_comment_float(bb_max[1])}] "
        f"z[{_fmt_comment_float(bb_min[2])}, {_fmt_comment_float(bb_max[2])}]"
    )
    definitions: list[str] = []
    definitions.append(f"const uint32_t {symbol}_payload[{len(payload_words)}] PROGMEM = {{")
    definitions.append(_format_u32_array(payload_words))
    definitions.append("};")
    definitions.append("")
    definitions.append(f"const tgx::{meshlet_type} {symbol}_meshlets[{len(meshlets)}] PROGMEM = {{")
    for mi, item in enumerate(encoded):
        q_center, q_dir, q_radius, q_cos, _, _ = metadata16b[mi]
        definitions.append(_format_meshlet_initializer(q_center, q_dir, q_radius, q_cos, item))
    definitions.append("};")
    definitions.append("")
    definitions.append(f"const tgx::{material_type}<{color_type}> {symbol}_materials[{len(materials)}] PROGMEM = {{")
    for mat in materials:
        texture = texture_symbols.get(mat, "")
        texture_ptr = f"&{texture}" if texture else "nullptr"
        material = mesh.materials.get(mat)
        color = material.diffuse if material is not None else (0.75, 0.75, 0.75)
        ambiant = material.ambiant_strength if material is not None else 0.1
        diffuse = material.diffuse_strength if material is not None else 0.7
        specular = material.specular_strength if material is not None else 0.6
        exponent = material.specular_exponent if material is not None else 32
        suffix = f" // {mat}" if mat else ""
        definitions.append(f"    {{ {texture_ptr}, {{ {_fmt_float(color[0])}, {_fmt_float(color[1])}, {_fmt_float(color[2])} }}, {_fmt_float(ambiant)}, {_fmt_float(diffuse)}, {_fmt_float(specular)}, {int(exponent)} }},{suffix}")
    definitions.append("};")
    definitions.append("")
    object_prefix = "const" if single_header else "extern const"
    definitions.append(f"{object_prefix} tgx::{mesh_type}<{color_type}> {symbol} PROGMEM =")
    definitions.append("    {")
    definitions.append(f"    {mesh_id},")
    definitions.append(f"    {len(meshlets)},")
    definitions.append(f"    {len(materials)},")
    definitions.append(f"    {symbol}_materials,")
    definitions.append(f"    {symbol}_meshlets,")
    definitions.append(f"    {symbol}_payload,")
    definitions.append("    {")
    definitions.append(f"    {_fmt_float(bb_min[0])}, {_fmt_float(bb_max[0])},")
    definitions.append(f"    {_fmt_float(bb_min[1])}, {_fmt_float(bb_max[1])},")
    definitions.append(f"    {_fmt_float(bb_min[2])}, {_fmt_float(bb_max[2])}")
    definitions.append("    },")
    definitions.append(f"    \"{symbol}\"")
    definitions.append("    };")
    definitions.append("")

    source: list[str] | None
    if single_header:
        for include in extra_includes:
            header.append(f'#include "{include}"')
        header.append("")
        header.extend(definitions)
        source = None
    else:
        header.append("")
        header.append(f"extern const tgx::{mesh_type}<{color_type}> {symbol};")
        header.append("")
        source = [f'#include "{header_filename or (symbol + ".h")}"']
        for include in extra_includes:
            source.append(f'#include "{include}"')
        source.append("")
        source.extend(definitions)

    stats = MeshletExportStats(
        meshlets=len(meshlets),
        triangles=len(mesh.triangles),
        chains=chains,
        vertex_refs_loaded=vertex_refs_loaded,
        payload_bytes=len(payload),
        payload_words=len(payload_words),
        materials=len(materials),
        meshlet_array_bytes=meshlet_array_bytes,
        material_array_bytes=material_array_bytes,
        mesh_struct_bytes=mesh_struct_bytes,
        static_bytes=static_bytes,
    )
    return MeshletExportResult("\n".join(header), stats, None if source is None else "\n".join(source))




def export_mesh3dv2_header(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    *,
    name: str | None = None,
    color_type: str = "tgx::RGB565",
    use_lkh: bool = True,
    lkh_exe: str | Path = DEFAULT_LKH_EXE,
    texture_symbols: dict[str, str] | None = None,
    extra_includes: list[str] | None = None,
    cone_source: str = "normal",
    header_filename: str | None = None,
    single_header: bool = False,
) -> MeshletExportResult:
    return _export_meshlet_header_impl(
        mesh,
        meshlets,
        name=name,
        color_type=color_type,
        use_lkh=use_lkh,
        lkh_exe=lkh_exe,
        texture_symbols=texture_symbols,
        extra_includes=extra_includes,
        mesh_type="Mesh3Dv2",
        meshlet_type="Meshlet3Dv2",
        material_type="MeshMaterial3Dv2",
        mesh_id=2162,
        cone_source=cone_source,
        header_filename=header_filename,
        single_header=single_header,
    )
