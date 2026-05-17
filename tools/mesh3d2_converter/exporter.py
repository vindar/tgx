from __future__ import annotations

import math
import re
import struct
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from .mesh import FaceVertex, Meshlet, ObjMesh
from .stripifier import DEFAULT_LKH_EXE, _read_lkh_tour, _write_lkh_problem


@dataclass
class Mesh3D2ExportStats:
    meshlets: int
    triangles: int
    chains: int
    vertex_refs_loaded: int
    payload_bytes: int
    payload_words: int
    materials: int


@dataclass
class Mesh3D2ExportResult:
    header: str
    stats: Mesh3D2ExportStats


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


def _lkh_order(mesh: ObjMesh, tri_indices: list[int], lkh_exe: str | Path) -> list[int]:
    if len(tri_indices) <= 2:
        return tri_indices.copy()
    lkh_exe = Path(lkh_exe)
    if not lkh_exe.exists():
        return tri_indices.copy()
    with tempfile.TemporaryDirectory(prefix="tgx_lkh_export_") as tmp:
        workdir = Path(tmp)
        _write_lkh_problem(workdir, mesh, tri_indices)
        completed = subprocess.run(
            [str(lkh_exe), "meshlet.par"],
            cwd=workdir,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
        if completed.returncode != 0 or not (workdir / "meshlet_res.txt").exists():
            return tri_indices.copy()
        return _read_lkh_tour(workdir, tri_indices)


def _make_chains(
    mesh: ObjMesh,
    tri_indices: list[int],
    *,
    use_lkh: bool,
    lkh_exe: str | Path,
) -> list[list[tuple[int | None, tuple[FaceVertex, FaceVertex, FaceVertex]]]]:
    ordered = _lkh_order(mesh, tri_indices, lkh_exe) if use_lkh else tri_indices.copy()
    triangles = [mesh.triangles[i].corners for i in ordered]
    if not triangles:
        return []

    # LKH returns a cyclic tour. Rotate it at a rupture when one exists, so the
    # arbitrary tour start does not add a spurious extra chain.
    if len(triangles) > 1:
        for i in range(len(triangles)):
            if not _is_chain_adjacent(triangles[i - 1], triangles[i]):
                triangles = triangles[i:] + triangles[:i]
                break

    runs: list[list[tuple[FaceVertex, FaceVertex, FaceVertex]]] = []
    current: list[tuple[FaceVertex, FaceVertex, FaceVertex]] = []
    for tri in triangles:
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
        raise ValueError("Mesh3D2 local vertex index exceeded 127")
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
        raise ValueError("Mesh3D2 local normal index exceeded 255")
    if has_texcoords and len(texcoords) > 255:
        raise ValueError("Mesh3D2 local texture coordinate index exceeded 255")

    return _EncodedMeshlet(material_index, vertices, normals, texcoords, bytes(stream), len(chains), vertex_refs_loaded)


def _pack_f32(values) -> bytes:
    return struct.pack("<" + "f" * len(values), *[float(x) for x in values])


def _payload_for_meshlet(mesh: ObjMesh, encoded: _EncodedMeshlet) -> bytes:
    payload = bytearray()
    for index in encoded.vertices:
        payload.extend(_pack_f32(mesh.vertices[index]))
    for index in encoded.normals:
        payload.extend(_pack_f32(mesh.normals[index]))
    for index in encoded.texcoords:
        payload.extend(_pack_f32(mesh.texcoords[index]))
    payload.extend(encoded.face_stream)
    while len(payload) % 4:
        payload.append(0)
    return bytes(payload)


def _fmt_float(v: float) -> str:
    if not math.isfinite(v):
        return "0.0f"
    text = f"{float(v):.9g}"
    if "e" not in text and "E" not in text and "." not in text:
        text += ".0"
    return text + "f"


def _fmt_vec3(v: np.ndarray) -> str:
    return "{ " + ", ".join(_fmt_float(float(x)) for x in v) + " }"


def _format_u32_array(values: list[int]) -> str:
    lines = []
    for i in range(0, len(values), 8):
        lines.append("    " + ", ".join(f"0x{v:08x}u" for v in values[i : i + 8]) + ",")
    return "\n".join(lines)


def export_mesh3d2_header(
    mesh: ObjMesh,
    meshlets: list[Meshlet],
    *,
    name: str | None = None,
    color_type: str = "tgx::RGB565",
    use_lkh: bool = True,
    lkh_exe: str | Path = DEFAULT_LKH_EXE,
    texture_symbols: dict[str, str] | None = None,
    extra_includes: list[str] | None = None,
) -> Mesh3D2ExportResult:
    symbol = _identifier(name or mesh.name or "mesh")
    materials = sorted({m.material for m in meshlets})
    if not materials:
        materials = [""]
    material_index = {mat: i for i, mat in enumerate(materials)}
    texture_symbols = texture_symbols or {}
    extra_includes = extra_includes or []

    encoded: list[_EncodedMeshlet] = []
    payload = bytearray()
    chains = 0
    vertex_refs_loaded = 0
    for meshlet in meshlets:
        item = _encode_meshlet(mesh, meshlet, material_index[meshlet.material], use_lkh=use_lkh, lkh_exe=lkh_exe)
        item.payload_offset32 = len(payload) // 4
        block = _payload_for_meshlet(mesh, item)
        payload.extend(block)
        encoded.append(item)
        chains += item.chain_count
        vertex_refs_loaded += item.vertex_refs_loaded

    payload_words = list(struct.unpack("<" + "I" * (len(payload) // 4), payload))
    bb_min, bb_max = mesh.bounding_box()

    text: list[str] = []
    text.append("#pragma once")
    text.append("")
    text.append("#include <tgx.h>")
    for include in extra_includes:
        text.append(f'#include "{include}"')
    text.append("")
    text.append(f"// Mesh3D2 generated by tools/mesh3d2_converter.")
    text.append(f"// Source model: {mesh.name}")
    text.append(f"// Triangles: {len(mesh.triangles)}, meshlets: {len(meshlets)}, chains: {chains}.")
    text.append("")
    text.append(f"const uint32_t {symbol}_payload[{len(payload_words)}] PROGMEM = {{")
    text.append(_format_u32_array(payload_words))
    text.append("};")
    text.append("")
    text.append(f"const tgx::Meshlet3D2 {symbol}_meshlets[{len(meshlets)}] PROGMEM = {{")
    for meshlet, item in zip(meshlets, encoded):
        cone_dir = meshlet.visibility_axis if meshlet.visibility_axis is not None else np.asarray([0.0, 0.0, 1.0], dtype=np.float64)
        cone_cos = meshlet.visibility_cos if meshlet.visibility_axis is not None else -1.0
        text.append("    {")
        text.append(f"        {_fmt_vec3(meshlet.center)}, {_fmt_vec3(cone_dir)},")
        text.append(f"        {_fmt_float(meshlet.radius)}, {_fmt_float(cone_cos)},")
        text.append(f"        {item.payload_offset32}u,")
        text.append(f"        {len(item.vertices)}, {len(item.normals)}, {len(item.texcoords)}, {item.material_index}")
        text.append("    },")
    text.append("};")
    text.append("")
    text.append(f"const tgx::MeshMaterial3D2<{color_type}> {symbol}_materials[{len(materials)}] PROGMEM = {{")
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
        text.append(f"    {{ {texture_ptr}, {{ {_fmt_float(color[0])}, {_fmt_float(color[1])}, {_fmt_float(color[2])} }}, {_fmt_float(ambiant)}, {_fmt_float(diffuse)}, {_fmt_float(specular)}, {int(exponent)} }},{suffix}")
    text.append("};")
    text.append("")
    text.append(f"const tgx::Mesh3D2<{color_type}> {symbol} PROGMEM =")
    text.append("    {")
    text.append("    2,")
    text.append(f"    {len(payload_words)}u,")
    text.append(f"    {len(meshlets)},")
    text.append(f"    {len(mesh.triangles)},")
    text.append(f"    {len(materials)},")
    text.append(f"    {symbol}_materials,")
    text.append(f"    {symbol}_meshlets,")
    text.append(f"    {symbol}_payload,")
    text.append("    {")
    text.append(f"    {_fmt_float(bb_min[0])}, {_fmt_float(bb_max[0])},")
    text.append(f"    {_fmt_float(bb_min[1])}, {_fmt_float(bb_max[1])},")
    text.append(f"    {_fmt_float(bb_min[2])}, {_fmt_float(bb_max[2])}")
    text.append("    },")
    text.append(f"    \"{symbol}\"")
    text.append("    };")
    text.append("")

    stats = Mesh3D2ExportStats(
        meshlets=len(meshlets),
        triangles=len(mesh.triangles),
        chains=chains,
        vertex_refs_loaded=vertex_refs_loaded,
        payload_bytes=len(payload),
        payload_words=len(payload_words),
        materials=len(materials),
    )
    return Mesh3D2ExportResult("\n".join(text), stats)
