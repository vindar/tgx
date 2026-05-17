from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.mesh3d2_converter.cones import apply_visibility_cones, auto_visibility_margin_deg
from tools.mesh3d2_converter.exporter import Mesh3D2ExportResult, export_mesh3d2_header
from tools.mesh3d2_converter.mesh import FaceVertex, Material, Meshlet, ObjMesh, Triangle
from tools.mesh3d2_converter.meshlets import sort_meshlets_by_material
from tools.mesh3d2_converter.pipeline import (
    add_build_options,
    add_visibility_options,
    build_meshlets_for_args as _common_build_meshlets_for_args,
    export_common,
    finalize_meshlets_for_export,
)
from tools.mesh3d2_converter.profiles import apply_profile_to_args
from tools.mesh3d2_converter.quality import cull_ratio_stats, fibonacci_sphere
from tools.mesh3d2_converter.stripifier import DEFAULT_LKH_EXE
from tools.mesh3d2_converter.validate import validate_mesh_for_export
from tools.mesh3d2_converter.visibility import compute_tgx_visibility


DEFAULT_TEXCOORD_QUANT_BITS = 9
DEFAULT_NORMAL_QUANT_BITS = 16
DEFAULT_VERTEX_QUANT_BITS = 20


@dataclass
class LegacyMeshDecl:
    name: str
    color_type: str
    nb_vertices: int
    nb_texcoords: int
    nb_normals: int
    nb_faces: int
    len_face: int
    vertices: str
    texcoords: str
    normals: str
    face: str
    texture: str
    color: tuple[float, float, float]
    ambiant_strength: float
    diffuse_strength: float
    specular_strength: float
    specular_exponent: int
    next_mesh: str
    bbox: tuple[float, float, float, float, float, float]
    model_name: str


@dataclass
class LegacyParseResult:
    header: Path
    includes: list[Path | str]
    vertices: dict[str, np.ndarray]
    texcoords: dict[str, np.ndarray]
    normals: dict[str, np.ndarray]
    faces: dict[str, list[int]]
    meshes: dict[str, LegacyMeshDecl]


@dataclass
class TuneResult:
    name: str
    params: dict[str, float | int | str]
    score: float
    meshlets: list[Meshlet]
    export: Mesh3D2ExportResult
    cull: dict[str, float]
    cone_source: str


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    return text


def _numbers(text: str) -> list[float]:
    return [float(x.rstrip("fFuUlL")) for x in re.findall(r"[-+]?(?:0x[0-9a-fA-F]+|\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?[fFuUlL]*", text)]


def _ints(text: str) -> list[int]:
    out: list[int] = []
    for token in re.findall(r"[-+]?(?:0x[0-9a-fA-F]+|\d+)[uUlL]*", text):
        out.append(int(token.rstrip("uUlL"), 0))
    return out


def _parse_vec_arrays(text: str, typename: str, dim: int) -> dict[str, np.ndarray]:
    pattern = re.compile(rf"(?:const\s+)?tgx::fVec{dim}\s+(\w+)\s*\[\s*(\d+)\s*\]\s*PROGMEM\s*=\s*\{{(.*?)\}};", re.S)
    arrays: dict[str, np.ndarray] = {}
    for name, count_text, body in pattern.findall(text):
        count = int(count_text)
        values = _numbers(body)
        if len(values) != count * dim:
            raise ValueError(f"{typename} array {name}: expected {count * dim} floats, found {len(values)}")
        arrays[name] = np.asarray(values, dtype=np.float64).reshape((count, dim))
    return arrays


def _parse_uint16_arrays(text: str) -> dict[str, list[int]]:
    pattern = re.compile(r"(?:const\s+)?uint16_t\s+(\w+)\s*\[\s*(\d+)\s*\]\s*PROGMEM\s*=\s*\{(.*?)\};", re.S)
    arrays: dict[str, list[int]] = {}
    for name, count_text, body in pattern.findall(text):
        count = int(count_text)
        values = _ints(body)
        if len(values) != count:
            raise ValueError(f"uint16_t array {name}: expected {count} values, found {len(values)}")
        arrays[name] = values
    return arrays


def _split_top_level(text: str) -> list[str]:
    fields: list[str] = []
    start = 0
    depth = 0
    in_string = False
    escape = False
    for i, ch in enumerate(text):
        if in_string:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        elif ch == "," and depth == 0:
            field = text[start:i].strip()
            if field:
                fields.append(field)
            start = i + 1
    tail = text[start:].strip()
    if tail:
        fields.append(tail)
    return fields


def _identifier_or_null(text: str) -> str:
    text = text.strip()
    if text == "nullptr" or text == "NULL" or text == "0":
        return ""
    match = re.search(r"&?\s*(\w+)", text)
    return match.group(1) if match else ""


def _parse_vec3_field(text: str) -> tuple[float, float, float]:
    values = _numbers(text)
    if len(values) != 3:
        raise ValueError(f"expected 3 floats in {text!r}")
    return float(values[0]), float(values[1]), float(values[2])


def _parse_box_field(text: str) -> tuple[float, float, float, float, float, float]:
    values = _numbers(text)
    if len(values) != 6:
        raise ValueError(f"expected 6 floats in {text!r}")
    return tuple(float(x) for x in values)  # type: ignore[return-value]


def _parse_string_field(text: str) -> str:
    match = re.search(r'"(.*?)"', text, flags=re.S)
    return match.group(1) if match else ""


def _parse_mesh_decls(text: str) -> dict[str, LegacyMeshDecl]:
    pattern = re.compile(r"(?:const\s+)?tgx::Mesh3D<\s*([^>]+?)\s*>\s+(\w+)\s*(?:PROGMEM\s*)?=\s*\{(.*?)\};", re.S)
    meshes: dict[str, LegacyMeshDecl] = {}
    for color_type, name, body in pattern.findall(text):
        fields = _split_top_level(body)
        if len(fields) < 19:
            raise ValueError(f"Mesh3D declaration {name}: expected at least 19 fields, found {len(fields)}")
        meshes[name] = LegacyMeshDecl(
            name=name,
            color_type=color_type.strip(),
            nb_vertices=int(_numbers(fields[1])[0]),
            nb_texcoords=int(_numbers(fields[2])[0]),
            nb_normals=int(_numbers(fields[3])[0]),
            nb_faces=int(_numbers(fields[4])[0]),
            len_face=int(_numbers(fields[5])[0]),
            vertices=_identifier_or_null(fields[6]),
            texcoords=_identifier_or_null(fields[7]),
            normals=_identifier_or_null(fields[8]),
            face=_identifier_or_null(fields[9]),
            texture=_identifier_or_null(fields[10]),
            color=_parse_vec3_field(fields[11]),
            ambiant_strength=float(_numbers(fields[12])[0]),
            diffuse_strength=float(_numbers(fields[13])[0]),
            specular_strength=float(_numbers(fields[14])[0]),
            specular_exponent=int(_numbers(fields[15])[0]),
            next_mesh=_identifier_or_null(fields[16]),
            bbox=_parse_box_field(fields[17]),
            model_name=_parse_string_field(fields[18]),
        )
    return meshes


def parse_legacy_header(header: str | Path) -> LegacyParseResult:
    path = Path(header).resolve()
    raw = path.read_text(encoding="utf-8", errors="replace")
    includes: list[Path | str] = []
    for inc in re.findall(r'^\s*#include\s+"([^"]+)"', raw, flags=re.M):
        if inc.replace("\\", "/").lower() == "tgx.h":
            continue
        candidate = (path.parent / inc).resolve()
        includes.append(candidate if candidate.exists() else inc)

    text = _strip_comments(raw)
    vertices = _parse_vec_arrays(text, "fVec3", 3)
    texcoords = _parse_vec_arrays(text, "fVec2", 2)
    normals = _parse_vec_arrays(text, "fVec3", 3)
    faces = _parse_uint16_arrays(text)
    meshes = _parse_mesh_decls(text)
    if not meshes:
        raise ValueError(f"no Mesh3D declarations found in {path}")
    return LegacyParseResult(path, includes, vertices, texcoords, normals, faces, meshes)


def _chain_roots(meshes: dict[str, LegacyMeshDecl]) -> list[str]:
    referenced = {m.next_mesh for m in meshes.values() if m.next_mesh}
    return sorted(name for name in meshes if name not in referenced)


def _ordered_chain(meshes: dict[str, LegacyMeshDecl], root: str | None) -> list[LegacyMeshDecl]:
    if root is None:
        roots = _chain_roots(meshes)
        if len(roots) != 1:
            raise ValueError(f"cannot choose a unique root mesh; candidates are: {', '.join(roots)}")
        root = roots[0]
    if root not in meshes:
        raise ValueError(f"root mesh {root!r} was not found")
    ordered: list[LegacyMeshDecl] = []
    seen: set[str] = set()
    name = root
    while name:
        if name in seen:
            raise ValueError(f"cycle detected in Mesh3D chain at {name}")
        seen.add(name)
        mesh = meshes[name]
        ordered.append(mesh)
        name = mesh.next_mesh
    return ordered


def _material_key(mesh: LegacyMeshDecl, used: dict[tuple, str]) -> str:
    signature = (
        mesh.texture,
        tuple(round(x, 9) for x in mesh.color),
        round(mesh.ambiant_strength, 9),
        round(mesh.diffuse_strength, 9),
        round(mesh.specular_strength, 9),
        mesh.specular_exponent,
    )
    if signature in used:
        return used[signature]
    base = mesh.texture or mesh.name
    base = re.sub(r"\W+", "_", base) or "material"
    key = base
    if key in used.values():
        suffix = 2
        while f"{base}_{suffix}" in used.values():
            suffix += 1
        key = f"{base}_{suffix}"
    used[signature] = key
    return key


def _decode_faces(words: list[int], mesh: LegacyMeshDecl, material: str) -> list[Triangle]:
    elem_words = 1 + (1 if mesh.nb_texcoords else 0) + (1 if mesh.nb_normals else 0)
    triangles: list[Triangle] = []
    pos = 0
    while pos < len(words):
        nbt = words[pos]
        pos += 1
        if nbt == 0:
            break
        corners: list[tuple[int, FaceVertex]] = []
        for _ in range(nbt + 2):
            raw_v = words[pos]
            pos += 1
            vt = -1
            vn = -1
            if mesh.nb_texcoords:
                vt = words[pos]
                pos += 1
            if mesh.nb_normals:
                vn = words[pos]
                pos += 1
            corners.append(((raw_v >> 15) & 1, FaceVertex(raw_v & 32767, vt, vn)))
        current = [corners[0][1], corners[1][1], corners[2][1]]
        triangles.append(Triangle(tuple(current), material=material))
        for direction, corner in corners[3:]:
            if direction:
                current = [current[2], current[1], corner]
            else:
                current = [current[0], current[2], corner]
            triangles.append(Triangle(tuple(current), material=material))
    if pos > len(words):
        raise ValueError(f"{mesh.face}: face stream ended unexpectedly")
    if len(triangles) != mesh.nb_faces:
        raise ValueError(f"{mesh.name}: decoded {len(triangles)} triangles, expected {mesh.nb_faces}")
    expected_words = 1 + sum(1 + elem_words * (0) for _ in [])
    _ = expected_words
    return triangles


def legacy_to_objmesh(parsed: LegacyParseResult, *, root: str | None = None, name: str | None = None) -> tuple[ObjMesh, dict[str, str], list[LegacyMeshDecl]]:
    chain = _ordered_chain(parsed.meshes, root)
    first = chain[0]
    vertices = parsed.vertices[first.vertices]
    texcoords = parsed.texcoords[first.texcoords] if first.texcoords else np.zeros((0, 2), dtype=np.float64)
    normals = parsed.normals[first.normals] if first.normals else np.zeros((0, 3), dtype=np.float64)

    triangles: list[Triangle] = []
    materials: dict[str, Material] = {}
    texture_symbols: dict[str, str] = {}
    used_materials: dict[tuple, str] = {}

    for mesh in chain:
        if mesh.vertices != first.vertices:
            raise ValueError("mixed vertex arrays in one Mesh3D chain are not supported yet")
        if mesh.texcoords != first.texcoords:
            raise ValueError("mixed texcoord arrays in one Mesh3D chain are not supported yet")
        if mesh.normals != first.normals:
            raise ValueError("mixed normal arrays in one Mesh3D chain are not supported yet")
        words = parsed.faces[mesh.face]
        if len(words) != mesh.len_face:
            raise ValueError(f"{mesh.face}: declaration len_face={mesh.len_face}, array length={len(words)}")
        material = _material_key(mesh, used_materials)
        materials[material] = Material(
            name=material,
            diffuse=mesh.color,
            ambiant_strength=mesh.ambiant_strength,
            diffuse_strength=mesh.diffuse_strength,
            specular_strength=mesh.specular_strength,
            specular_exponent=mesh.specular_exponent,
        )
        if mesh.texture:
            texture_symbols[material] = mesh.texture
        triangles.extend(_decode_faces(words, mesh, material))

    model_name = name or first.model_name or first.name
    obj = ObjMesh(
        vertices=vertices.copy(),
        texcoords=texcoords.copy(),
        normals=normals.copy(),
        triangles=triangles,
        name=model_name,
        materials=materials,
        source_path=parsed.header,
    )
    if obj.normals.size:
        lengths = np.linalg.norm(obj.normals, axis=1)
        bad = np.where(np.abs(lengths - 1.0) > 1.0e-3)[0]
        if len(bad):
            raise ValueError(f"legacy normal array contains {len(bad)} non-unit normals; first index is {int(bad[0])}")
    return obj, texture_symbols, chain


def _relative_includes(includes: list[Path | str], output: Path) -> list[str]:
    out_dir = output.resolve().parent
    rels: list[str] = []
    seen: set[str] = set()
    for inc in includes:
        if isinstance(inc, Path):
            try:
                text = inc.resolve().relative_to(out_dir)
            except ValueError:
                text = Path(__import__("os").path.relpath(inc.resolve(), out_dir))
            value = str(text).replace("\\", "/")
        else:
            value = inc.replace("\\", "/")
        if value not in seen:
            rels.append(value)
            seen.add(value)
    return rels


def _print_stats(mesh: ObjMesh, meshlets, result: Mesh3D2ExportResult, chain: list[LegacyMeshDecl], cone_source: str) -> None:
    target = max(len(mesh.materials), int(np.ceil(len(mesh.triangles) / 64.0)))
    print("Legacy Mesh3D input:")
    print(f"  submeshes      : {len(chain)}")
    print(f"  triangles      : {len(mesh.triangles)}")
    print(f"  vertices       : {len(mesh.vertices)}")
    print(f"  texcoords      : {len(mesh.texcoords)}")
    print(f"  normals        : {len(mesh.normals)}")
    print(f"  materials      : {len(mesh.materials)}")
    print(f"  target meshlets: {target} (heuristic: max(materials, triangles/64))")
    stats = result.stats
    print("Mesh3D2 output:")
    print(f"  meshlets       : {stats.meshlets}")
    print(f"  chains         : {stats.chains} ({100.0 * stats.chains / max(1, stats.triangles):.2f}% of triangles)")
    print(f"  vertex refs    : {stats.vertex_refs_loaded} ({stats.vertex_refs_loaded / max(1, stats.triangles):.3f} per tri)")
    print(f"  payload        : {stats.payload_bytes} bytes ({stats.payload_words} uint32 words)")
    if cone_source == "nocull":
        print("Meshlet culling estimate:")
        print("  cone source    : disabled")
        return
    q = cull_ratio_stats(meshlets, samples=128, meshlet_cost=3.0, cone_source=cone_source)
    print("Meshlet culling estimate:")
    print(f"  cone source    : {cone_source}")
    print(f"  gross culled   : {100.0 * q['gross_mean']:.1f}% avg")
    print(f"  net gain       : {100.0 * q['net_mean']:.1f}% avg")


def preprocess_legacy_mesh(
    mesh: ObjMesh,
    *,
    vertex_quant_bits: int,
    texcoord_quant_bits: int,
    texcoord_wrap: bool,
    normal_quant_bits: int,
) -> ObjMesh:
    """Merge near-identical vertices/UVs/normals before meshlet construction.

    Vertices are snapped relative to the mesh bounding box. UVs are snapped on an
    unbounded grid, so coordinates outside [0,1] keep their tile number. Normals are
    snapped in signed fixed point and then renormalized.
    """
    vertices = mesh.vertices.astype(np.float64, copy=True)
    texcoords = mesh.texcoords.astype(np.float64, copy=True)
    normals = mesh.normals.astype(np.float64, copy=True)
    v_remap = {i: i for i in range(len(vertices))}
    vt_remap = {i: i for i in range(len(texcoords))}
    vn_remap = {i: i for i in range(len(normals))}

    if vertex_quant_bits >= 0 and len(vertices):
        bb_min = np.min(vertices, axis=0)
        bb_max = np.max(vertices, axis=0)
        extent = bb_max - bb_min
        scale = (1 << vertex_quant_bits) - 1
        safe_extent = np.where(extent > 0.0, extent, 1.0)
        quant = np.rint(((vertices - bb_min) / safe_extent) * scale).astype(np.int64)
        quant[:, extent <= 0.0] = 0
        seen_v: dict[tuple[int, int, int], int] = {}
        new_vertices: list[np.ndarray] = []
        v_remap = {}
        for i, key in enumerate(map(tuple, quant)):
            if key not in seen_v:
                seen_v[key] = len(new_vertices)
                new_vertices.append(bb_min + (quant[i].astype(np.float64) / scale) * extent)
            v_remap[i] = seen_v[key]
        vertices = np.asarray(new_vertices, dtype=np.float64).reshape((-1, 3))

    if texcoord_quant_bits >= 0 and len(texcoords):
        scale = 1 << texcoord_quant_bits
        uv = np.mod(texcoords, 1.0) if texcoord_wrap else texcoords.copy()
        quant = np.rint(uv * scale).astype(np.int64)
        if texcoord_wrap:
            quant %= scale
        seen: dict[tuple[int, int], int] = {}
        new_texcoords: list[np.ndarray] = []
        vt_remap = {}
        for i, key in enumerate(map(tuple, quant)):
            if key not in seen:
                seen[key] = len(new_texcoords)
                new_texcoords.append(quant[i].astype(np.float64) / scale)
            vt_remap[i] = seen[key]
        texcoords = np.asarray(new_texcoords, dtype=np.float64).reshape((-1, 2))

    if normal_quant_bits >= 0 and len(normals):
        maxq = (1 << (normal_quant_bits - 1)) - 1
        quant = np.rint(np.clip(normals, -1.0, 1.0) * maxq).astype(np.int64)
        seen_n: dict[tuple[int, int, int], int] = {}
        new_normals: list[np.ndarray] = []
        vn_remap = {}
        for i, key in enumerate(map(tuple, quant)):
            if key not in seen_n:
                seen_n[key] = len(new_normals)
                n = quant[i].astype(np.float64) / maxq
                length = float(np.linalg.norm(n))
                if length > 0.0:
                    n /= length
                new_normals.append(n)
            vn_remap[i] = seen_n[key]
        normals = np.asarray(new_normals, dtype=np.float64).reshape((-1, 3))

    triangles: list[Triangle] = []
    for tri in mesh.triangles:
        triangles.append(
            Triangle(
                tuple(
                    FaceVertex(
                        v_remap.get(c.v, c.v),
                        vt_remap.get(c.vt, c.vt),
                        vn_remap.get(c.vn, c.vn),
                    )
                    for c in tri.corners
                ),
                tri.material,
                tri.group,
            )
        )

    return ObjMesh(vertices, texcoords, normals, triangles, mesh.name, mesh.materials.copy(), mesh.source_path)


def _apply_profile(args: argparse.Namespace, mesh: ObjMesh) -> None:
    apply_profile_to_args(args, mesh, source="legacy")


def _build_meshlets_for_args(args: argparse.Namespace, mesh: ObjMesh) -> list[Meshlet]:
    meshlets, _ = _common_build_meshlets_for_args(args, mesh, source="legacy")
    return meshlets


def _candidate_params(mesh: ObjMesh, mode: str) -> list[tuple[str, dict[str, float | int | str]]]:
    has_many_materials = len(mesh.materials) >= 4
    if has_many_materials:
        base = [
            ("material_coarse", dict(target_vertices=112, max_triangles=220, max_normal_angle=125.0, radius_weight=0.5, normal_weight=0.5, vertex_weight=0.1, stop_score=999.0, merge_passes=2, smooth_passes=0, merge_max_normal_angle=140.0)),
            ("material_medium", dict(target_vertices=96, max_triangles=170, max_normal_angle=115.0, radius_weight=0.8, normal_weight=0.8, vertex_weight=0.1, stop_score=999.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle=128.0)),
            ("material_fine", dict(target_vertices=78, max_triangles=132, max_normal_angle=102.0, radius_weight=1.3, normal_weight=1.3, vertex_weight=0.1, stop_score=999.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle=112.0)),
        ]
        if mode == "thorough":
            base.extend([
                ("material_xcoarse", dict(target_vertices=127, max_triangles=320, max_normal_angle=145.0, radius_weight=0.25, normal_weight=0.25, vertex_weight=0.1, stop_score=999.0, merge_passes=2, smooth_passes=0, merge_max_normal_angle=155.0)),
                ("material_dense", dict(target_vertices=64, max_triangles=105, max_normal_angle=92.0, radius_weight=1.8, normal_weight=1.8, vertex_weight=0.1, stop_score=999.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle=104.0)),
                ("material_medium_smooth", dict(target_vertices=96, max_triangles=170, max_normal_angle=115.0, radius_weight=0.8, normal_weight=0.8, vertex_weight=0.1, stop_score=999.0, merge_passes=1, smooth_passes=1, merge_max_normal_angle=128.0)),
            ])
        return base

    base = [
        ("balanced", dict(target_vertices=52, max_triangles=70, max_normal_angle=42.0, radius_weight=7.0, normal_weight=18.0, vertex_weight=2.0, stop_score=4.8, merge_passes=1, smooth_passes=0, merge_max_normal_angle=48.0)),
        ("balanced_wide", dict(target_vertices=64, max_triangles=95, max_normal_angle=50.0, radius_weight=4.0, normal_weight=10.0, vertex_weight=1.0, stop_score=8.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle=56.0)),
        ("coarse", dict(target_vertices=96, max_triangles=160, max_normal_angle=70.0, radius_weight=1.5, normal_weight=4.0, vertex_weight=0.4, stop_score=40.0, merge_passes=1, smooth_passes=0, merge_max_normal_angle=78.0)),
        ("xcoarse", dict(target_vertices=112, max_triangles=220, max_normal_angle=90.0, radius_weight=0.7, normal_weight=1.5, vertex_weight=0.2, stop_score=120.0, merge_passes=2, smooth_passes=0, merge_max_normal_angle=100.0)),
    ]
    if mode == "thorough":
        base.extend([
            ("fine", dict(target_vertices=42, max_triangles=55, max_normal_angle=34.0, radius_weight=9.0, normal_weight=22.0, vertex_weight=2.5, stop_score=3.5, merge_passes=1, smooth_passes=0, merge_max_normal_angle=40.0)),
            ("wide_smooth", dict(target_vertices=64, max_triangles=95, max_normal_angle=50.0, radius_weight=4.0, normal_weight=10.0, vertex_weight=1.0, stop_score=8.0, merge_passes=1, smooth_passes=1, merge_max_normal_angle=56.0)),
        ])
    return base


def _score_candidate(stats: Mesh3D2ExportStats, cull: dict[str, float], texture_heavy: bool) -> float:
    tri = max(1, stats.triangles)
    predicted_tri_cost = tri * (1.0 - cull["net_mean"])
    if texture_heavy:
        chain_cost = 2.2 * stats.chains
        vertex_ref_cost = 0.18 * stats.vertex_refs_loaded
        payload_cost = stats.payload_bytes / 96.0
    else:
        chain_cost = 0.75 * stats.chains
        vertex_ref_cost = 0.12 * stats.vertex_refs_loaded
        payload_cost = stats.payload_bytes / 256.0
    return predicted_tri_cost + chain_cost + vertex_ref_cost + payload_cost


def _evaluate_candidate(
    args: argparse.Namespace,
    mesh: ObjMesh,
    name: str,
    params: dict[str, float | int | str],
    *,
    texture_symbols: dict[str, str],
    color_type: str,
    use_visibility: bool,
    visibility_samples: int,
    visibility_size: int,
    visibility_margin: float,
) -> TuneResult:
    trial = argparse.Namespace(**vars(args))
    trial.profile = "custom"
    for key, value in params.items():
        setattr(trial, key, value)
    meshlets = _build_meshlets_for_args(trial, mesh)
    cone_source = "normal"
    if use_visibility:
        views = fibonacci_sphere(visibility_samples)
        margin = visibility_margin if visibility_margin >= 0.0 else auto_visibility_margin_deg(len(views))
        visibility = compute_tgx_visibility(mesh, meshlets, views, width=visibility_size, height=visibility_size, helper=args.visibility_helper)
        meshlets = apply_visibility_cones(meshlets, views, visibility, margin_deg=margin)
        cone_source = "visibility"
    if args.sort_by_material:
        meshlets = sort_meshlets_by_material(meshlets)
    validate_mesh_for_export(mesh, meshlets)
    exported = export_mesh3d2_header(
        mesh,
        meshlets,
        name=args.name or (Path(args.output).stem),
        color_type=color_type,
        use_lkh=not args.no_lkh,
        lkh_exe=args.lkh,
        texture_symbols=texture_symbols,
        extra_includes=[],
    )
    cull = cull_ratio_stats(meshlets, samples=args.auto_tune_cull_samples, meshlet_cost=args.auto_tune_meshlet_cost, cone_source=cone_source)
    return TuneResult(name, dict(params), _score_candidate(exported.stats, cull, bool(texture_symbols)), meshlets, exported, cull, cone_source)


def _print_tune_result(result: TuneResult, prefix: str = "  ") -> None:
    stats = result.export.stats
    print(
        f"{prefix}{result.name}: score={result.score:.1f}, meshlets={stats.meshlets}, "
        f"chains={stats.chains}, payload={stats.payload_bytes}, net={100.0 * result.cull['net_mean']:.1f}%, "
        f"gross={100.0 * result.cull['gross_mean']:.1f}% ({result.cone_source})"
    )


def _auto_tune(args: argparse.Namespace, mesh: ObjMesh, texture_symbols: dict[str, str], color_type: str) -> TuneResult:
    candidates = _candidate_params(mesh, args.auto_tune)
    print(f"Auto-tune: {len(candidates)} candidates ({args.auto_tune})")
    first_pass: list[TuneResult] = []
    for name, params in candidates:
        result = _evaluate_candidate(
            args,
            mesh,
            name,
            params,
            texture_symbols=texture_symbols,
            color_type=color_type,
            use_visibility=False,
            visibility_samples=0,
            visibility_size=0,
            visibility_margin=args.visibility_margin,
        )
        first_pass.append(result)
        _print_tune_result(result)

    first_pass.sort(key=lambda r: r.score)
    finalists = first_pass[: max(1, min(args.auto_tune_finalists, len(first_pass)))]
    if args.visibility_cones:
        print(
            f"Auto-tune: visibility pass on {len(finalists)} finalist(s), "
            f"{args.auto_tune_visibility_samples} views, {args.auto_tune_visibility_size}x{args.auto_tune_visibility_size}"
        )
        visible_results: list[TuneResult] = []
        for finalist in finalists:
            result = _evaluate_candidate(
                args,
                mesh,
                finalist.name,
                finalist.params,
                texture_symbols=texture_symbols,
                color_type=color_type,
                use_visibility=True,
                visibility_samples=args.auto_tune_visibility_samples,
                visibility_size=args.auto_tune_visibility_size,
                visibility_margin=args.visibility_margin,
            )
            visible_results.append(result)
            _print_tune_result(result)
        visible_results.sort(key=lambda r: r.score)
        best = visible_results[0]
    else:
        best = finalists[0]

    if args.auto_tune_report:
        report = {
            "input": str(args.input),
            "mode": args.auto_tune,
            "best": {
                "name": best.name,
                "score": best.score,
                "params": best.params,
                "stats": best.export.stats.__dict__,
                "cull": best.cull,
                "cone_source": best.cone_source,
            },
            "first_pass": [
                {
                    "name": r.name,
                    "score": r.score,
                    "params": r.params,
                    "stats": r.export.stats.__dict__,
                    "cull": r.cull,
                    "cone_source": r.cone_source,
                }
                for r in first_pass
            ],
        }
        Path(args.auto_tune_report).parent.mkdir(parents=True, exist_ok=True)
        Path(args.auto_tune_report).write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"Auto-tune winner: {best.name}")
    _print_tune_result(best)
    return best


def convert(args: argparse.Namespace) -> None:
    parsed = parse_legacy_header(args.input)
    mesh, texture_symbols, chain = legacy_to_objmesh(parsed, root=args.root, name=args.name)
    if args.preprocess_legacy:
        mesh = preprocess_legacy_mesh(
            mesh,
            vertex_quant_bits=args.vertex_quant_bits,
            texcoord_quant_bits=args.texcoord_quant_bits,
            texcoord_wrap=args.texcoord_wrap,
            normal_quant_bits=args.normal_quant_bits,
        )

    output = Path(args.output)
    includes = _relative_includes(parsed.includes, output)
    color_type = args.color_type or chain[0].color_type
    if args.auto_tune != "off":
        tuned = _auto_tune(args, mesh, texture_symbols, color_type)
        if args.visibility_cones and (
            args.visibility_samples != args.auto_tune_visibility_samples
            or args.visibility_size != args.auto_tune_visibility_size
        ):
            print(
                f"Auto-tune final visibility: recomputing winner with "
                f"{args.visibility_samples} views, {args.visibility_size}x{args.visibility_size}"
            )
            tuned = _evaluate_candidate(
                args,
                mesh,
                tuned.name,
                tuned.params,
                texture_symbols=texture_symbols,
                color_type=color_type,
                use_visibility=True,
                visibility_samples=args.visibility_samples,
                visibility_size=args.visibility_size,
                visibility_margin=args.visibility_margin,
            )
            _print_tune_result(tuned)
        meshlets = tuned.meshlets
        result = export_common(
            args,
            mesh,
            meshlets,
            output=output,
            color_type=color_type,
            texture_symbols=texture_symbols,
            extra_includes=includes,
        )
        cone_source = tuned.cone_source
    else:
        _apply_profile(args, mesh)
        meshlets = _build_meshlets_for_args(args, mesh)
        meshlets, cone_source = finalize_meshlets_for_export(args, mesh, meshlets)
        result = export_common(
            args,
            mesh,
            meshlets,
            output=output,
            color_type=color_type,
            texture_symbols=texture_symbols,
            extra_includes=includes,
        )
    print(f"wrote {output}")
    _print_stats(mesh, meshlets, result, chain, cone_source)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Convert a TGX legacy Mesh3D header into a Mesh3D2 header")
    parser.add_argument("input", help="legacy Mesh3D .h file")
    parser.add_argument("-o", "--output", required=True, help="output Mesh3D2 header")
    parser.add_argument("--root", help="root Mesh3D symbol; auto-detected for a single chain")
    parser.add_argument("--name", help="output C++ symbol")
    parser.add_argument("--color-type", help="override color type; defaults to the legacy Mesh3D color type")
    add_build_options(parser, source="legacy")
    parser.add_argument("--no-lkh", action="store_true")
    parser.add_argument("--lkh", default=str(DEFAULT_LKH_EXE))
    add_visibility_options(parser, default_samples=1024, default_size=512)
    parser.add_argument("--no-preprocess-legacy", dest="preprocess_legacy", action="store_false", help="keep legacy vertex/normal/UV indices exactly as-is")
    parser.add_argument("--vertex-quant-bits", type=int, default=DEFAULT_VERTEX_QUANT_BITS, help="quantize and merge vertices to this many bits per bounding-box coordinate; negative disables")
    parser.add_argument("--texcoord-quant-bits", type=int, default=DEFAULT_TEXCOORD_QUANT_BITS, help="snap and merge UVs to a 1/(2^bits) grid; negative disables")
    parser.add_argument("--texcoord-wrap", action="store_true", help="identify UVs modulo 1 during UV quantization; only use when this is known to preserve the model texture mapping")
    parser.add_argument("--normal-quant-bits", type=int, default=DEFAULT_NORMAL_QUANT_BITS, help="quantize and merge normals to signed fixed-point bits per coordinate; negative disables")
    parser.add_argument("--auto-tune", choices=("off", "fast", "thorough"), default="off", help="try several meshlet profiles and keep the best estimated runtime/profile tradeoff")
    parser.add_argument("--auto-tune-finalists", type=int, default=3, help="number of first-pass candidates that get the slower TGX visibility pass")
    parser.add_argument("--auto-tune-cull-samples", type=int, default=256, help="view samples used to score candidate culling quality")
    parser.add_argument("--auto-tune-meshlet-cost", type=float, default=3.0, help="estimated runtime cost of testing one meshlet, in triangle equivalents")
    parser.add_argument("--auto-tune-visibility-samples", type=int, default=2048, help="TGX visibility views used for auto-tune finalists")
    parser.add_argument("--auto-tune-visibility-size", type=int, default=512, help="TGX visibility render size used for auto-tune finalists")
    parser.add_argument("--auto-tune-report", help="optional JSON report for auto-tune candidates")
    parser.add_argument("--no-sort-by-material", dest="sort_by_material", action="store_false", help="keep meshlets in builder order instead of grouping them by material")
    parser.add_argument("--no-merge-small-materials", dest="merge_small_materials", action="store_false", help="do not merge material groups that fit in one Mesh3D2 local table")
    parser.set_defaults(preprocess_legacy=True)
    parser.set_defaults(sort_by_material=True)
    parser.set_defaults(merge_small_materials=True)
    args = parser.parse_args(argv)
    convert(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
