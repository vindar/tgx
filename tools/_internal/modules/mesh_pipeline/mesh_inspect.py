from __future__ import annotations

import argparse
import math
import re
import struct
import sys
import warnings
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[4]))

from tools._internal.modules.mesh_pipeline.mesh3d_to_mesh3d2 import legacy_to_objmesh, parse_legacy_header


def _prepare_pyvista_import() -> None:
    warnings.filterwarnings("ignore", message=".*Blowfish has been deprecated.*")


@dataclass
class DecodedMaterial:
    name: str
    texture_symbol: str = ""
    color: tuple[float, float, float] = (0.75, 0.75, 0.75)
    ambiant: float = 0.1
    diffuse: float = 0.7
    specular: float = 0.6
    exponent: int = 32
    emissive_color: tuple[float, float, float] = (0.0, 0.0, 0.0)
    emissive_strength: float = 0.0
    emissive_texture_symbol: str = ""
    material_extra_flags: int = 0
    material_extra_present: bool = False


@dataclass
class DecodedMeshlet:
    index: int
    center: np.ndarray
    cone_dir: np.ndarray
    radius: float
    cone_cos: float
    payload_offset32: int
    nb_vertices: int
    nb_normals: int
    nb_texcoords: int
    material_index: int
    vertices: np.ndarray = field(default_factory=lambda: np.zeros((0, 3), dtype=np.float64))
    normals: np.ndarray = field(default_factory=lambda: np.zeros((0, 3), dtype=np.float64))
    texcoords: np.ndarray = field(default_factory=lambda: np.zeros((0, 2), dtype=np.float64))
    triangles: list[tuple[int, int, int]] = field(default_factory=list)
    triangle_texcoords: list[tuple[int, int, int]] = field(default_factory=list)
    triangle_normals: list[tuple[int, int, int]] = field(default_factory=list)
    chains: int = 0
    vertex_refs_loaded: int = 0
    face_bytes: int = 0
    payload_bytes: int = 0


@dataclass
class DecodedHeaderMesh:
    path: Path
    format: str
    symbol: str
    color_type: str
    id: int
    payload_words_count: int
    nb_meshlets_declared: int
    nb_materials_declared: int
    bbox: tuple[float, float, float, float, float, float]
    name: str
    materials: list[DecodedMaterial]
    meshlets: list[DecodedMeshlet]
    material_extras_present: bool = False
    texture_headers: dict[str, Path] = field(default_factory=dict)


@dataclass
class TextureImage:
    symbol: str
    width: int
    height: int
    rgb: np.ndarray


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    return text


def _numbers(text: str) -> list[float]:
    return [
        float(token.rstrip("fFuUlL"))
        for token in re.findall(r"[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?[fFuUlL]*", text)
    ]


def _ints(text: str) -> list[int]:
    out: list[int] = []
    for token in re.findall(r"[-+]?(?:0x[0-9a-fA-F]+|\d+)[uUlL]*", text):
        out.append(int(token.rstrip("uUlL"), 0))
    return out


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


def _extract_braced_after(text: str, start: int) -> tuple[str, int]:
    open_pos = text.find("{", start)
    if open_pos < 0:
        raise ValueError("missing opening brace")
    depth = 0
    in_string = False
    escape = False
    for i in range(open_pos, len(text)):
        ch = text[i]
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
            if depth == 0:
                return text[open_pos + 1:i], i + 1
    raise ValueError("unterminated braced initializer")


def _extract_named_array_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\[[^\]]*\]\s*(?:PROGMEM\s*)?=", text)
    if not match:
        raise ValueError(f"array {name!r} not found")
    body, _ = _extract_braced_after(text, match.end())
    return body


def _extract_named_object_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*(?:PROGMEM\s*)?=", text)
    if not match:
        raise ValueError(f"object {name!r} not found")
    body, _ = _extract_braced_after(text, match.end())
    return body


def _identifier_or_null(text: str) -> str:
    text = text.strip()
    if text in ("nullptr", "NULL", "0"):
        return ""
    text = re.sub(r"\([^)]*\)", "", text).strip()
    match = re.search(r"&?\s*(\w+)", text)
    return match.group(1) if match else ""


def _string_field(text: str) -> str:
    match = re.search(r'"(.*?)"', text, flags=re.S)
    return match.group(1) if match else ""


def _vec3(text: str) -> np.ndarray:
    vals = _numbers(text)
    if len(vals) != 3:
        raise ValueError(f"expected vec3, got {text!r}")
    return np.asarray(vals, dtype=np.float64)


def _box(text: str) -> tuple[float, float, float, float, float, float]:
    vals = _numbers(text)
    if len(vals) != 6:
        raise ValueError(f"expected box with 6 floats, got {text!r}")
    return tuple(float(v) for v in vals)  # type: ignore[return-value]


def _meshlet16b_scales(bbox: tuple[float, float, float, float, float, float]) -> tuple[np.ndarray, float, float, float]:
    bb_min = np.asarray([bbox[0], bbox[2], bbox[4]], dtype=np.float64)
    bb_max = np.asarray([bbox[1], bbox[3], bbox[5]], dtype=np.float64)
    center = 0.5 * (bb_min + bb_max)
    extent = float(np.max(bb_max - bb_min))
    if (not math.isfinite(extent)) or extent <= 1.0e-20:
        extent = 1.0
    return center, extent / 65534.0, extent / 65535.0, 1.0 / 32767.0


def _u32_to_bytes(words: list[int]) -> bytes:
    return b"".join(struct.pack("<I", w & 0xFFFFFFFF) for w in words)


def _bytes_to_f32_array(data: bytes, offset: int, count: int, dim: int) -> tuple[np.ndarray, int]:
    nbytes = count * dim * 4
    vals = struct.unpack_from("<" + "f" * (count * dim), data, offset) if count else ()
    return np.asarray(vals, dtype=np.float64).reshape((count, dim)), offset + nbytes


def _bytes_to_i16_array(data: bytes, offset: int, count: int, dim: int) -> tuple[np.ndarray, int]:
    nbytes = count * dim * 2
    vals = struct.unpack_from("<" + "h" * (count * dim), data, offset) if count else ()
    return np.asarray(vals, dtype=np.float64).reshape((count, dim)), offset + nbytes


def _rgb565_to_rgb(value: int) -> tuple[int, int, int]:
    r5 = (value >> 11) & 31
    g6 = (value >> 5) & 63
    b5 = value & 31
    return _rgb565_components_to_rgb(r5, g6, b5)


def _rgb565_components_to_rgb(r5: int, g6: int, b5: int) -> tuple[int, int, int]:
    r5 = max(0, min(31, int(r5)))
    g6 = max(0, min(63, int(g6)))
    b5 = max(0, min(31, int(b5)))
    return ((r5 * 255 + 15) // 31, (g6 * 255 + 31) // 63, (b5 * 255 + 15) // 31)


def _parse_texture_headers(raw_text: str, base: Path) -> dict[str, Path]:
    out: dict[str, Path] = {}
    for inc in re.findall(r'^\s*#include\s+"([^"]+)"', raw_text, flags=re.M):
        path = (base / inc).resolve()
        if not path.exists():
            continue
        raw, text = _read_generated_pair(path)
        for symbol in re.findall(r"(?:static\s+)?(?:extern\s+)?(?:const\s+)?tgx::Image<[^>]+>\s+(\w+)\s*(?:\(|;)", text):
            out[symbol] = path
    return out


def _read_generated_pair(path: Path) -> tuple[str, str]:
    raw = path.read_text(encoding="utf-8", errors="replace")
    source = path.with_suffix(".cpp")
    if source.exists():
        raw = raw + "\n" + source.read_text(encoding="utf-8", errors="replace")
    return raw, _strip_comments(raw)


def _parse_texture_image(path: Path, symbol: str) -> TextureImage | None:
    raw, text = _read_generated_pair(path)
    image_match = re.search(
        rf"(?:static\s+)?(?:const\s+)?tgx::Image<\s*tgx::(\w+)\s*>\s+{re.escape(symbol)}\s*\((.*?)\)\s*;",
        text,
        flags=re.S,
    )
    if not image_match:
        return None
    color_type = image_match.group(1)
    args = _split_top_level(image_match.group(2))
    if len(args) < 3:
        return None
    data_symbol = _identifier_or_null(args[0])
    dims = _ints(",".join(args[1:3]))
    if len(dims) < 2:
        return None
    width, height = int(dims[0]), int(dims[1])
    try:
        body = _extract_named_array_body(text, data_symbol)
    except Exception:
        return None
    try:
        rgb_values = _decode_tgx_image_pixels(body, color_type, width * height)
    except Exception:
        return None
    if len(rgb_values) < width * height:
        return None
    rgb = np.zeros((height, width, 3), dtype=np.uint8)
    for i, value in enumerate(rgb_values[: width * height]):
        y = i // width
        x = i % width
        rgb[y, x] = value
    # TGX texture headers generated from OBJ files store rows bottom-to-top to
    # match the historical renderer convention.  PyVista/NumPy expects row 0 at
    # the top of the image, so flip the decoded preview image vertically.
    rgb = np.flipud(rgb)
    return TextureImage(symbol=symbol, width=width, height=height, rgb=rgb)


def _decode_tgx_image_pixels(body: str, color_type: str, count: int) -> list[tuple[int, int, int]]:
    calls = re.findall(r"\bC\s*\((.*?)\)", body, flags=re.S)
    if not calls:
        values = _ints(body)
        if color_type == "RGB565":
            return [_rgb565_to_rgb(v) for v in values[:count]]
        if color_type in ("RGB24", "RGB32", "RGB64", "RGBf"):
            # Non-macro generated forms for these color types are uncommon in
            # mesh textures.  Refuse them instead of guessing constructor arity.
            raise ValueError(f"unsupported non-macro {color_type} texture data")
        raise ValueError(f"unsupported texture color type {color_type}")

    out: list[tuple[int, int, int]] = []
    for call in calls[:count]:
        vals = _numbers(call)
        ints = _ints(call)
        if color_type == "RGB565":
            if len(ints) >= 3:
                out.append(_rgb565_components_to_rgb(ints[0], ints[1], ints[2]))
            elif ints:
                out.append(_rgb565_to_rgb(ints[0]))
            else:
                raise ValueError("invalid RGB565 texture literal")
        elif color_type in ("RGB24", "RGB32"):
            if len(vals) < 3:
                raise ValueError(f"invalid {color_type} texture literal")
            out.append(tuple(max(0, min(255, int(round(v)))) for v in vals[:3]))  # type: ignore[arg-type]
        elif color_type == "RGB64":
            if len(vals) < 3:
                raise ValueError("invalid RGB64 texture literal")
            out.append(tuple(max(0, min(255, int(round(v / 257.0)))) for v in vals[:3]))  # type: ignore[arg-type]
        elif color_type == "RGBf":
            if len(vals) < 3:
                raise ValueError("invalid RGBf texture literal")
            out.append(tuple(max(0, min(255, int(round(v * 255.0)))) for v in vals[:3]))  # type: ignore[arg-type]
        else:
            raise ValueError(f"unsupported texture color type {color_type}")
    return out


def _parse_materials(text: str, array_name: str) -> list[DecodedMaterial]:
    body = _extract_named_array_body(text, array_name)
    entries = _split_top_level(body)
    materials: list[DecodedMaterial] = []
    for i, entry in enumerate(entries):
        fields = _split_top_level(entry.strip().strip("{}").strip())
        if len(fields) < 6:
            continue
        texture = _identifier_or_null(fields[0])
        color_vals = _numbers(fields[1])
        color = tuple(float(v) for v in color_vals[:3]) if len(color_vals) >= 3 else (0.75, 0.75, 0.75)
        materials.append(
            DecodedMaterial(
                name=f"mat{i}",
                texture_symbol=texture,
                color=color,  # type: ignore[arg-type]
                ambiant=float(_numbers(fields[2])[0]),
                diffuse=float(_numbers(fields[3])[0]),
                specular=float(_numbers(fields[4])[0]),
                exponent=int(_numbers(fields[5])[0]),
            )
        )
    return materials


def _apply_material_extras(text: str, array_name: str, materials: list[DecodedMaterial]) -> bool:
    if not array_name:
        return False
    body = _extract_named_array_body(text, array_name)
    entries = _split_top_level(body)
    for i, entry in enumerate(entries[: len(materials)]):
        fields = _split_top_level(entry.strip().strip("{}").strip())
        if len(fields) < 4:
            continue
        color_vals = _numbers(fields[0])
        color = tuple(float(v) for v in color_vals[:3]) if len(color_vals) >= 3 else (0.0, 0.0, 0.0)
        strength_vals = _numbers(fields[1])
        flag_vals = _ints(fields[3])
        materials[i].emissive_color = color  # type: ignore[assignment]
        materials[i].emissive_strength = float(strength_vals[0]) if strength_vals else 0.0
        materials[i].emissive_texture_symbol = _identifier_or_null(fields[2])
        materials[i].material_extra_flags = int(flag_vals[0]) if flag_vals else 0
        materials[i].material_extra_present = True
    return True


def _parse_meshlet_headers(text: str, array_name: str, fmt: str = "Mesh3Dv2", bbox: tuple[float, float, float, float, float, float] | None = None) -> list[DecodedMeshlet]:
    body = _extract_named_array_body(text, array_name)
    entries = _split_top_level(body)
    meshlets: list[DecodedMeshlet] = []
    if bbox is None:
        raise ValueError("Mesh3Dv2 parsing requires the parent mesh bounding box")
    bbox_center, center_scale, radius_scale, snorm_scale = _meshlet16b_scales(bbox)
    for index, entry in enumerate(entries):
        fields = _split_top_level(entry.strip().strip("{}").strip())
        if fmt == "Mesh3Dv2":
            if len(fields) < 8:
                continue
            q_center = _ints(fields[0])
            q_dir = _ints(fields[1])
            if len(q_center) != 3 or len(q_dir) != 3:
                raise ValueError(f"Mesh3Dv2 meshlet {index}: invalid quantized center/cone fields")
            center = bbox_center + np.asarray(q_center, dtype=np.float64) * center_scale
            meshlets.append(
                DecodedMeshlet(
                    index=index,
                    center=center,
                    cone_dir=np.asarray(q_dir, dtype=np.float64) * snorm_scale,
                    radius=float(_ints(fields[2])[0]) * radius_scale,
                    cone_cos=float(_ints(fields[3])[0]) * snorm_scale,
                    payload_offset32=int(_ints(fields[4])[0]),
                    nb_vertices=int(_numbers(fields[5])[0]),
                    nb_normals=int(_numbers(fields[6])[0]),
                    nb_texcoords=int(_numbers(fields[7])[0]),
                    material_index=int(_numbers(fields[8])[0]) if len(fields) > 8 else 0,
                )
            )
    return meshlets


def _read_face_element(face: bytes, pos: int, has_tex: bool, has_norm: bool) -> tuple[int, int, int, int]:
    v = face[pos]
    pos += 1
    vt = -1
    vn = -1
    if has_tex:
        vt = face[pos]
        pos += 1
    if has_norm:
        vn = face[pos]
        pos += 1
    return v, vt, vn, pos


def _decode_face_stream(face: bytes, has_tex: bool, has_norm: bool) -> tuple[
    list[tuple[int, int, int]],
    list[tuple[int, int, int]],
    list[tuple[int, int, int]],
    int,
    int,
    int,
]:
    pos = 0
    chains = 0
    vertex_refs = 0
    triangles: list[tuple[int, int, int]] = []
    tri_tex: list[tuple[int, int, int]] = []
    tri_norm: list[tuple[int, int, int]] = []
    while pos < len(face):
        nbt = face[pos]
        pos += 1
        if nbt == 0:
            break
        chains += 1
        vertex_refs += nbt + 2
        corners = []
        for _ in range(3):
            v, vt, vn, pos = _read_face_element(face, pos, has_tex, has_norm)
            corners.append([v & 127, vt, vn])
        triangles.append((corners[0][0], corners[1][0], corners[2][0]))
        tri_tex.append((corners[0][1], corners[1][1], corners[2][1]))
        tri_norm.append((corners[0][2], corners[1][2], corners[2][2]))
        for _ in range(nbt - 1):
            nv, vt, vn, pos = _read_face_element(face, pos, has_tex, has_norm)
            if nv & 128:
                corners[0], corners[2] = corners[2], corners[0]
            else:
                corners[1], corners[2] = corners[2], corners[1]
            corners[2] = [nv & 127, vt, vn]
            triangles.append((corners[0][0], corners[1][0], corners[2][0]))
            tri_tex.append((corners[0][1], corners[1][1], corners[2][1]))
            tri_norm.append((corners[0][2], corners[1][2], corners[2][2]))
    return triangles, tri_tex, tri_norm, chains, vertex_refs, pos


def _decode_payload(mesh: DecodedHeaderMesh, payload_words: list[int]) -> None:
    data = _u32_to_bytes(payload_words)
    next_offsets = [m.payload_offset32 * 4 for m in mesh.meshlets[1:]] + [mesh.payload_words_count * 4]
    for meshlet, next_offset in zip(mesh.meshlets, next_offsets):
        start = meshlet.payload_offset32 * 4
        pos = start
        if mesh.format == "Mesh3Dv2":
            qv, pos = _bytes_to_i16_array(data, pos, meshlet.nb_vertices, 3)
            qn, pos = _bytes_to_i16_array(data, pos, meshlet.nb_normals, 3)
            qt, pos = _bytes_to_i16_array(data, pos, meshlet.nb_texcoords, 2)
            meshlet.vertices = meshlet.center + qv * (meshlet.radius / 32767.0)
            meshlet.normals = qn * (1.0 / 32767.0)
            meshlet.texcoords = qt * (4.0 / 32767.0)
        face = data[pos:next_offset]
        has_tex = meshlet.nb_texcoords != 0
        has_norm = meshlet.nb_normals != 0
        tris, tri_tex, tri_norm, chains, refs, face_len = _decode_face_stream(face, has_tex, has_norm)
        meshlet.triangles = tris
        meshlet.triangle_texcoords = tri_tex
        meshlet.triangle_normals = tri_norm
        meshlet.chains = chains
        meshlet.vertex_refs_loaded = refs
        meshlet.face_bytes = face_len
        meshlet.payload_bytes = next_offset - start


def parse_mesh3d2_header(path: str | Path) -> DecodedHeaderMesh:
    header = Path(path).resolve()
    raw, text = _read_generated_pair(header)
    decl = re.search(r"const\s+tgx::(Mesh3Dv2)<\s*([^>]+?)\s*>\s+(\w+)\s*(?:PROGMEM\s*)?=", text)
    if not decl:
        raise ValueError("no Mesh3Dv2 declaration found")
    fmt, color_type, symbol = decl.group(1), decl.group(2).strip(), decl.group(3)
    body = _extract_named_object_body(text, symbol)
    fields = _split_top_level(body)
    if len(fields) < 8:
        raise ValueError(f"{symbol}: expected mesh fields, found {len(fields)}")
    material_array = _identifier_or_null(fields[3])
    meshlet_array = _identifier_or_null(fields[4])
    payload_field = 5
    bbox_field = 6
    name_field = 7
    material_extra_array = _identifier_or_null(fields[8]) if len(fields) >= 9 else ""
    payload_array = _identifier_or_null(fields[payload_field])
    payload_words = _ints(_extract_named_array_body(text, payload_array))
    materials = _parse_materials(text, material_array)
    material_extras_present = _apply_material_extras(text, material_extra_array, materials)
    mesh = DecodedHeaderMesh(
        path=header,
        format=fmt,
        symbol=symbol,
        color_type=color_type,
        id=int(_numbers(fields[0])[0]),
        payload_words_count=len(payload_words),
        nb_meshlets_declared=int(_numbers(fields[1])[0]),
        nb_materials_declared=int(_numbers(fields[2])[0]),
        bbox=_box(fields[bbox_field]),
        name=_string_field(fields[name_field]),
        materials=materials,
        meshlets=_parse_meshlet_headers(text, meshlet_array, fmt, _box(fields[bbox_field])),
        material_extras_present=material_extras_present,
        texture_headers=_parse_texture_headers(raw, header.parent),
    )
    _decode_payload(mesh, payload_words)
    return mesh


def detect_mesh_type(path: str | Path) -> str:
    raw, _ = _read_generated_pair(Path(path).resolve())
    text = raw
    if re.search(r"tgx::Mesh3Dv2\s*<", text):
        return "Mesh3Dv2"
    if re.search(r"tgx::Mesh3D\s*<", text):
        return "Mesh3D"
    raise ValueError("could not detect TGX mesh type")


def _summarize_distribution(values: list[int] | list[float]) -> str:
    if not values:
        return "n/a"
    arr = np.asarray(values, dtype=np.float64)
    return f"min {arr.min():.0f}, avg {arr.mean():.2f}, median {np.median(arr):.1f}, max {arr.max():.0f}"


def print_mesh3d2_stats(mesh: DecodedHeaderMesh) -> None:
    triangles = sum(len(m.triangles) for m in mesh.meshlets)
    chains = sum(m.chains for m in mesh.meshlets)
    refs = sum(m.vertex_refs_loaded for m in mesh.meshlets)
    payload_bytes = mesh.payload_words_count * 4
    vertices_local = sum(m.nb_vertices for m in mesh.meshlets)
    normals_local = sum(m.nb_normals for m in mesh.meshlets)
    texcoords_local = sum(m.nb_texcoords for m in mesh.meshlets)
    textured = sum(1 for m in mesh.materials if m.texture_symbol)
    emissive = sum(1 for m in mesh.materials if m.material_extra_present)
    emissive_textured = sum(1 for m in mesh.materials if m.emissive_texture_symbol)
    print(f"file           : {mesh.path}")
    print(f"type           : {mesh.format} ({mesh.color_type})")
    print(f"symbol/name    : {mesh.symbol} / {mesh.name}")
    print(f"id             : {mesh.id}")
    print(f"bbox           : x[{mesh.bbox[0]:.6g}, {mesh.bbox[1]:.6g}] y[{mesh.bbox[2]:.6g}, {mesh.bbox[3]:.6g}] z[{mesh.bbox[4]:.6g}, {mesh.bbox[5]:.6g}]")
    print(f"materials      : {len(mesh.materials)} ({textured} textured, {emissive} emissive, {emissive_textured} emissive textured)")
    for i, mat in enumerate(mesh.materials):
        tex = mat.texture_symbol or "none"
        emit_tex = mat.emissive_texture_symbol or "none"
        print(f"  mat {i:3d}: tex={tex}, color=({mat.color[0]:.3g}, {mat.color[1]:.3g}, {mat.color[2]:.3g}), ka/kd/ks={mat.ambiant:.3g}/{mat.diffuse:.3g}/{mat.specular:.3g}, exp={mat.exponent}, emit=({mat.emissive_color[0]:.3g}, {mat.emissive_color[1]:.3g}, {mat.emissive_color[2]:.3g})*{mat.emissive_strength:.3g}, emit_tex={emit_tex}, flags={mat.material_extra_flags}")
    print(f"meshlets       : {len(mesh.meshlets)} declared {mesh.nb_meshlets_declared}")
    print(f"triangles      : {triangles} decoded")
    print(f"chains         : {chains} ({100.0 * chains / max(1, triangles):.2f}% of triangles)")
    print(f"vertex refs    : {refs} ({refs / max(1, triangles):.3f} per tri)")
    print(f"local arrays   : vertices {vertices_local}, normals {normals_local}, texcoords {texcoords_local}")
    print(f"payload        : {payload_bytes} bytes ({mesh.payload_words_count} uint32 words, {payload_bytes / max(1, triangles):.2f} bytes/tri)")
    print("meshlet distribution:")
    print(f"  triangles    : {_summarize_distribution([len(m.triangles) for m in mesh.meshlets])}")
    print(f"  chains       : {_summarize_distribution([m.chains for m in mesh.meshlets])}")
    print(f"  vertices     : {_summarize_distribution([m.nb_vertices for m in mesh.meshlets])}")
    print(f"  normals      : {_summarize_distribution([m.nb_normals for m in mesh.meshlets])}")
    print(f"  texcoords    : {_summarize_distribution([m.nb_texcoords for m in mesh.meshlets])}")
    print(f"  payload bytes: {_summarize_distribution([m.payload_bytes for m in mesh.meshlets])}")
    active_cones = [m for m in mesh.meshlets if m.cone_cos > -1.0]
    print(f"culling cones   : {len(active_cones)} active, {len(mesh.meshlets) - len(active_cones)} disabled")
    if active_cones:
        angles = [math.degrees(math.acos(max(-1.0, min(1.0, m.cone_cos)))) for m in active_cones]
        print(f"  angle         : min {min(angles):.2f} deg, avg {float(np.mean(angles)):.2f} deg, max {max(angles):.2f} deg")
        print(f"  cos           : min {min(m.cone_cos for m in active_cones):.6g}, avg {float(np.mean([m.cone_cos for m in active_cones])):.6g}, max {max(m.cone_cos for m in active_cones):.6g}")
    if mesh.texture_headers:
        print("texture headers:")
        for symbol, path in sorted(mesh.texture_headers.items()):
            print(f"  {symbol}: {path}")


def print_legacy_stats(path: str | Path) -> None:
    parsed = parse_legacy_header(path)
    roots = []
    referenced = {m.next_mesh for m in parsed.meshes.values() if m.next_mesh}
    for name in parsed.meshes:
        if name not in referenced:
            roots.append(name)
    print(f"file           : {parsed.header}")
    print("type           : Mesh3D legacy")
    print(f"decls          : {len(parsed.meshes)} mesh declarations")
    print(f"chain roots    : {', '.join(roots) if roots else 'n/a'}")
    print(f"arrays         : vertices {len(parsed.vertices)}, texcoords {len(parsed.texcoords)}, normals {len(parsed.normals)}, faces {len(parsed.faces)}")
    total_tris = sum(m.nb_faces for m in parsed.meshes.values())
    total_face_len = sum(m.len_face for m in parsed.meshes.values())
    total_vertices = sum(m.nb_vertices for m in parsed.meshes.values())
    total_tex = sum(m.nb_texcoords for m in parsed.meshes.values())
    total_normals = sum(m.nb_normals for m in parsed.meshes.values())
    print(f"total meshes   : {len(parsed.meshes)}")
    print(f"total triangles: {total_tris}")
    print(f"total arrays   : vertices {total_vertices}, texcoords {total_tex}, normals {total_normals}, face entries {total_face_len}")
    print("submeshes:")
    for name, mesh in parsed.meshes.items():
        faces = parsed.faces.get(mesh.face, [])
        chains, refs = _legacy_chain_stats(faces, mesh.texcoords != "", mesh.normals != "")
        print(f"  {name}: tris={mesh.nb_faces}, chains={chains}, verts={mesh.nb_vertices}, tex={mesh.nb_texcoords}, normals={mesh.nb_normals}, texture={mesh.texture or 'none'}, next={mesh.next_mesh or 'none'}")


def _legacy_chain_stats(face: list[int], has_texcoords: bool, has_normals: bool) -> tuple[int, int]:
    pos = 0
    chains = 0
    refs = 0
    stride = 1 + int(has_texcoords) + int(has_normals)
    while pos < len(face):
        nbt = face[pos]
        pos += 1
        if nbt == 0:
            break
        chains += 1
        refs += nbt + 2
        pos += (nbt + 2) * stride
    return chains, refs


def show_legacy_pyvista(path: str | Path) -> None:
    _prepare_pyvista_import()
    import pyvista as pv

    parsed = parse_legacy_header(path)
    obj, texture_symbols, _ = legacy_to_objmesh(parsed)
    texture_headers = _parse_texture_headers(Path(path).read_text(encoding="utf-8", errors="replace"), Path(path).resolve().parent)
    plotter = pv.Plotter()

    by_material: dict[str, list[int]] = {}
    for ti, tri in enumerate(obj.triangles):
        by_material.setdefault(tri.material, []).append(ti)

    for material, tri_indices in sorted(by_material.items()):
        points = []
        tcoords = []
        faces = []
        for ti in tri_indices:
            tri = obj.triangles[ti]
            base = len(points)
            for corner in tri.corners:
                points.append(obj.vertices[corner.v])
                if corner.vt >= 0 and len(obj.texcoords) > corner.vt:
                    tcoords.append((float(obj.texcoords[corner.vt][0]), float(obj.texcoords[corner.vt][1])))
                else:
                    tcoords.append((0.0, 0.0))
            faces.extend([3, base, base + 1, base + 2])

        pv_mesh = pv.PolyData(np.asarray(points, dtype=np.float64), np.asarray(faces, dtype=np.int64))
        texture = None
        symbol = texture_symbols.get(material, "")
        if symbol and symbol in texture_headers:
            tex_img = _parse_texture_image(texture_headers[symbol], symbol)
            if tex_img is not None:
                pv_mesh.active_texture_coordinates = np.asarray(tcoords, dtype=np.float64)
                texture = pv.Texture(tex_img.rgb)
        if texture is not None:
            plotter.add_mesh(pv_mesh, texture=texture, show_edges=False, lighting=True)
        else:
            mat = obj.materials.get(material)
            color = mat.diffuse if mat is not None else (0.70, 0.72, 0.76)
            plotter.add_mesh(pv_mesh, color=color, show_edges=True, edge_color=(0.05, 0.05, 0.05), line_width=0.25, lighting=True)

    plotter.add_axes()
    plotter.add_title(f"{obj.name}: Mesh3D legacy / {len(obj.triangles)} triangles", font_size=12)
    plotter.reset_camera()
    plotter.camera_position = "iso"
    plotter.show()


def _make_pyvista_polydata(mesh: DecodedHeaderMesh, *, textured: bool):
    _prepare_pyvista_import()
    import pyvista as pv

    points: list[np.ndarray] = []
    faces: list[int] = []
    cell_meshlet: list[float] = []
    tcoords: list[tuple[float, float]] = []
    cell_material: list[int] = []
    for m in mesh.meshlets:
        for tri, tri_tex in zip(m.triangles, m.triangle_texcoords):
            base = len(points)
            for vi, ti in zip(tri, tri_tex):
                points.append(m.vertices[vi])
                if textured and ti >= 0 and len(m.texcoords) > ti:
                    tcoords.append((float(m.texcoords[ti][0]), float(m.texcoords[ti][1])))
                else:
                    tcoords.append((0.0, 0.0))
            faces.extend([3, base, base + 1, base + 2])
            cell_meshlet.append(float(m.index))
            cell_material.append(m.material_index)
    pv_mesh = pv.PolyData(np.asarray(points, dtype=np.float64), np.asarray(faces, dtype=np.int64))
    pv_mesh.cell_data["meshlet"] = np.asarray(cell_meshlet, dtype=np.float64)
    pv_mesh.cell_data["material"] = np.asarray(cell_material, dtype=np.float64)
    if textured and tcoords:
        pv_mesh.active_texture_coordinates = np.asarray(tcoords, dtype=np.float64)
    return pv_mesh


def _make_pyvista_polydata_for_material(mesh: DecodedHeaderMesh, material_index: int, *, textured: bool):
    _prepare_pyvista_import()
    import pyvista as pv

    points: list[np.ndarray] = []
    faces: list[int] = []
    tcoords: list[tuple[float, float]] = []
    for meshlet in mesh.meshlets:
        if meshlet.material_index != material_index:
            continue
        for tri, tri_tex in zip(meshlet.triangles, meshlet.triangle_texcoords):
            base = len(points)
            for vi, ti in zip(tri, tri_tex):
                points.append(meshlet.vertices[vi])
                if textured and ti >= 0 and len(meshlet.texcoords) > ti:
                    tcoords.append((float(meshlet.texcoords[ti][0]), float(meshlet.texcoords[ti][1])))
                else:
                    tcoords.append((0.0, 0.0))
            faces.extend([3, base, base + 1, base + 2])
    if not points:
        return None
    pv_mesh = pv.PolyData(np.asarray(points, dtype=np.float64), np.asarray(faces, dtype=np.int64))
    if textured and tcoords:
        pv_mesh.active_texture_coordinates = np.asarray(tcoords, dtype=np.float64)
    return pv_mesh


def _add_cones(plotter, pv, mesh: DecodedHeaderMesh, *, scale: float = 0.18) -> None:
    bb = np.asarray([[mesh.bbox[0], mesh.bbox[2], mesh.bbox[4]], [mesh.bbox[1], mesh.bbox[3], mesh.bbox[5]]], dtype=np.float64)
    diag = float(np.linalg.norm(bb[1] - bb[0]))
    length_factor = max(diag * scale, 1e-6)
    centers = []
    line_points: list[np.ndarray] = []
    lines: list[int] = []
    for m in mesh.meshlets:
        if m.cone_cos <= -1.0:
            continue
        centers.append(m.center)
        axis = m.cone_dir / max(float(np.linalg.norm(m.cone_dir)), 1e-12)
        length = max(m.radius * 2.0, length_factor * 0.15)
        base = len(line_points)
        line_points.append(m.center)
        line_points.append(m.center + axis * length)
        lines.extend([2, base, base + 1])
    if line_points:
        line_mesh = pv.PolyData(np.asarray(line_points, dtype=np.float64), lines=np.asarray(lines, dtype=np.int64))
        plotter.add_mesh(line_mesh, color="orange", line_width=2.0, opacity=0.75)
    if centers:
        points = pv.PolyData(np.asarray(centers, dtype=np.float64))
        points["center"] = np.arange(len(centers), dtype=np.float64)
        plotter.add_mesh(points, color="black", point_size=5.0, render_points_as_spheres=True, opacity=0.65)


def show_mesh3d2_pyvista(mesh: DecodedHeaderMesh, mode: str = "tabs") -> None:
    _prepare_pyvista_import()
    import pyvista as pv

    plotter = pv.Plotter()
    actors: dict[str, list[object]] = {"texture": [], "meshlets": [], "cones": []}
    pv_mesh = _make_pyvista_polydata(mesh, textured=False)

    for material_index, material in enumerate(mesh.materials):
        tex_img: TextureImage | None = None
        tex_symbol = material.texture_symbol
        tex_path = mesh.texture_headers.get(tex_symbol)
        if tex_symbol and tex_path is not None:
            tex_img = _parse_texture_image(tex_path, tex_symbol)
        material_mesh = _make_pyvista_polydata_for_material(mesh, material_index, textured=tex_img is not None)
        if material_mesh is None:
            continue
        if tex_img is not None:
            actors["texture"].append(plotter.add_mesh(material_mesh, texture=pv.Texture(tex_img.rgb), show_edges=False, lighting=True))
        else:
            actors["texture"].append(plotter.add_mesh(material_mesh, color=material.color, show_edges=False, lighting=True))
    if not actors["texture"]:
        actors["texture"].append(plotter.add_mesh(pv_mesh, color=(0.70, 0.72, 0.76), show_edges=False, lighting=True))

    def ensure_meshlets() -> None:
        if actors["meshlets"]:
            return
        actors["meshlets"].append(
            plotter.add_mesh(
                pv_mesh,
                scalars="meshlet",
                cmap="tab20",
                show_edges=True,
                edge_color=(0.04, 0.04, 0.04),
                line_width=0.25,
                lighting=True,
            )
        )

    def ensure_cones() -> None:
        if actors["cones"]:
            return
        actors["cones"].append(
            plotter.add_mesh(
                pv_mesh,
                scalars="meshlet",
                cmap="tab20",
                show_edges=False,
                opacity=0.45,
                lighting=True,
            )
        )
        before = set(plotter.actors.keys())
        _add_cones(plotter, pv, mesh)
        for key in set(plotter.actors.keys()) - before:
            actors["cones"].append(plotter.actors[key])

    def set_mode(name: str) -> None:
        if name == "meshlets":
            ensure_meshlets()
        elif name == "cones":
            ensure_cones()
        for group, group_actors in actors.items():
            visible = group == name
            for actor in group_actors:
                actor.SetVisibility(visible)
        plotter.add_text(
            f"{mesh.name or mesh.symbol} | mode: {name} | Press [1] for texture, [2] for meshlets, [3] for visibility cones",
            name="mode_label",
            position="upper_left",
            font_size=10,
        )
        plotter.render()

    plotter.add_key_event("1", lambda: set_mode("texture"))
    plotter.add_key_event("2", lambda: set_mode("meshlets"))
    plotter.add_key_event("3", lambda: set_mode("cones"))
    plotter.add_axes()
    plotter.reset_camera()
    plotter.camera_position = "iso"
    set_mode("texture" if mode == "tabs" else mode)
    plotter.show()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect TGX Mesh3D and meshlet generated headers")
    parser.add_argument("input", help="TGX mesh header to inspect")
    parser.add_argument("--view", action="store_true", help="open an interactive PyVista viewer for meshlet formats")
    parser.add_argument("--view-mode", choices=("tabs", "texture", "meshlets", "cones"), default="tabs", help="initial PyVista view mode; tabs enables keys 1/2/3")
    args = parser.parse_args(argv)

    mesh_type = detect_mesh_type(args.input)
    if mesh_type == "Mesh3D":
        print_legacy_stats(args.input)
        if args.view:
            show_legacy_pyvista(args.input)
        return 0

    mesh = parse_mesh3d2_header(args.input)
    print_mesh3d2_stats(mesh)
    if args.view:
        show_mesh3d2_pyvista(mesh, args.view_mode)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
