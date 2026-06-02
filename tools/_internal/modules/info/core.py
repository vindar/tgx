from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from ..mesh_pipeline.mesh3d_to_mesh3d2 import legacy_to_objmesh, parse_legacy_header
from ..mesh_pipeline.mesh_inspect import (
    DecodedHeaderMesh,
    TextureImage,
    _parse_texture_image,
    _parse_texture_headers,
    detect_mesh_type,
    parse_mesh3d2_header,
    show_legacy_pyvista,
    show_mesh3d2_pyvista,
)


class InfoError(RuntimeError):
    """User-facing inspection error."""


@dataclass
class SourceBundle:
    input_path: Path
    files: list[Path]
    raw: str
    text: str


@dataclass
class PreviewResult:
    image: Image.Image | None = None
    view_callback: object | None = None


@dataclass
class InfoResult:
    kind: str
    title: str
    symbol: str
    files: list[Path]
    report: str
    preview: PreviewResult = field(default_factory=PreviewResult)


@dataclass
class ImageObjectInfo:
    symbol: str
    color_type: str
    data_symbol: str
    width: int
    height: int
    bytes_per_pixel: int
    memory_bytes: int
    image: Image.Image


@dataclass
class IliFontInfo:
    symbol: str
    version: int
    bpp: int
    first: int
    last: int
    bits_index: int
    bits_width: int
    bits_height: int
    bits_xoffset: int
    bits_yoffset: int
    bits_delta: int
    line_space: int
    cap_height: int
    data_bytes: int
    index_bytes: int
    glyphs: list["DecodedGlyph"]


@dataclass
class GfxFontInfo:
    symbol: str
    first: int
    last: int
    y_advance: int
    bitmap_bytes: int
    glyph_count: int
    glyphs: list["DecodedGlyph"]


@dataclass
class DecodedGlyph:
    codepoint: int
    width: int
    height: int
    x_offset: int
    y_offset: int
    x_advance: int
    levels: list[int] = field(default_factory=list)
    bpp: int = 1

    @property
    def present(self) -> bool:
        return self.width > 0 and self.height > 0 and bool(self.levels)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    return text


def read_source_bundle(path: str | Path) -> SourceBundle:
    root = Path(path).resolve()
    if not root.exists():
        raise InfoError(f"Input file not found: {root}")
    if root.suffix.lower() not in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
        raise InfoError("Please select a TGX generated .h/.hpp/.cpp file.")

    files: list[Path] = []

    def add_file(candidate: Path) -> None:
        candidate = candidate.resolve()
        if candidate.exists() and candidate not in files:
            files.append(candidate)

    add_file(root)
    if root.suffix.lower() in (".h", ".hpp"):
        add_file(root.with_suffix(".cpp"))
    elif root.suffix.lower() in (".cpp", ".cxx", ".cc"):
        add_file(root.with_suffix(".h"))
        add_file(root.with_suffix(".hpp"))

    # Follow local generated includes, but only when the file exists next to the
    # selected file. System headers and missing user includes are ignored here;
    # missing data arrays are reported later if they are actually required.
    changed = True
    while changed:
        changed = False
        for file in list(files):
            try:
                raw = file.read_text(encoding="utf-8", errors="replace")
            except OSError as exc:
                raise InfoError(f"Cannot read associated file: {file}") from exc
            for include in re.findall(r'^\s*#include\s+"([^"]+)"', raw, flags=re.M):
                if Path(include).name.lower() == "tgx.h":
                    continue
                candidate = (file.parent / include).resolve()
                if candidate.exists() and candidate not in files:
                    files.append(candidate)
                    changed = True

    chunks: list[str] = []
    for file in files:
        try:
            chunks.append(f"\n// --- TGX_INFO_FILE: {file} ---\n")
            chunks.append(file.read_text(encoding="utf-8", errors="replace"))
        except OSError as exc:
            raise InfoError(f"Cannot read associated file: {file}") from exc
    raw = "\n".join(chunks)
    return SourceBundle(root, files, raw, strip_comments(raw))


def detect_asset_type(bundle: SourceBundle) -> str:
    text = bundle.text
    if re.search(r"\btgx::Mesh3Dv2\s*<", text) or re.search(r"\btgx::Mesh3D\s*<", text):
        return "mesh"
    if re.search(r"\btgx::Image\s*<\s*tgx::\w+\s*>", text):
        return "image"
    if re.search(r"\bILI9341_t3_font_t\s+\w+", text) or re.search(r"\bGFXfont\s+\w+", text):
        return "font"
    raise InfoError("No TGX mesh, image or font object was found in the selected file.")


def inspect_path(path: str | Path) -> InfoResult:
    bundle = read_source_bundle(path)
    kind = detect_asset_type(bundle)
    if kind == "mesh":
        return inspect_mesh(bundle)
    if kind == "image":
        return inspect_image(bundle)
    if kind == "font":
        return inspect_font(bundle)
    raise AssertionError(kind)


def _format_files(files: list[Path]) -> str:
    return "\n".join(f"  - {file}" for file in files)


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
        elif ch in "({[":
            depth += 1
        elif ch in ")}]":
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
        raise InfoError("Missing opening brace in generated initializer.")
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
    raise InfoError("Unterminated braced initializer in generated file.")


def _extract_named_array_body(text: str, name: str) -> str:
    pattern = rf"\b{name}\s*(?:\[[^\]]*\])?\s*(?:PROGMEM\s*)?="
    match = re.search(pattern, text)
    if not match:
        raise InfoError(f"Required generated array '{name}' was not found.")
    body, _ = _extract_braced_after(text, match.end())
    return body


def _extract_named_object_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*(?:PROGMEM\s*)?=", text)
    if not match:
        raise InfoError(f"Required generated object '{name}' was not found.")
    body, _ = _extract_braced_after(text, match.end())
    return body


def _identifier(text: str) -> str:
    text = text.strip()
    text = re.sub(r"\([^)]+\)", "", text).strip()
    match = re.search(r"&?\s*(\w+)", text)
    return match.group(1) if match else ""


def inspect_mesh(bundle: SourceBundle) -> InfoResult:
    candidates = _mesh_parse_candidates(bundle)
    try:
        mesh_type = _detect_mesh_type_from_candidates(candidates)
    except Exception as exc:
        raise InfoError(f"Could not detect or parse TGX mesh: {exc}") from exc

    if mesh_type == "Mesh3Dv2":
        try:
            mesh_path, mesh = _parse_first(candidates, parse_mesh3d2_header, "Mesh3Dv2")
        except Exception as exc:
            raise InfoError(f"Could not parse Mesh3Dv2: {exc}") from exc
        report = mesh3dv2_report(mesh, bundle.files)
        return InfoResult(
            kind="mesh",
            title=f"Mesh3Dv2: {mesh.symbol}",
            symbol=mesh.symbol,
            files=bundle.files,
            report=report,
            preview=PreviewResult(view_callback=lambda: show_mesh3d2_pyvista(mesh, "tabs")),
        )

    try:
        mesh_path, parsed = _parse_first(candidates, parse_legacy_header, "Mesh3D")
        obj, texture_symbols, _ = legacy_to_objmesh(parsed)
    except Exception as exc:
        raise InfoError(f"Could not parse legacy Mesh3D: {exc}") from exc
    report = legacy_mesh_report(parsed, obj, texture_symbols, bundle.files)
    root = next(iter(parsed.meshes.keys()), "Mesh3D")
    return InfoResult(
        kind="mesh",
        title=f"Mesh3D legacy: {root}",
        symbol=root,
        files=bundle.files,
        report=report,
        preview=PreviewResult(view_callback=lambda: show_legacy_pyvista(mesh_path)),
    )


def _mesh_parse_candidates(bundle: SourceBundle) -> list[Path]:
    headers = [p for p in bundle.files if p.suffix.lower() in (".h", ".hpp")]
    sources = [p for p in bundle.files if p.suffix.lower() in (".cpp", ".cxx", ".cc")]
    ordered = headers + sources
    return ordered or [bundle.input_path]


def _detect_mesh_type_from_candidates(candidates: list[Path]) -> str:
    last_error: Exception | None = None
    for candidate in candidates:
        try:
            return detect_mesh_type(candidate)
        except Exception as exc:
            last_error = exc
    if last_error is not None:
        raise last_error
    raise ValueError("no candidate file")


def _parse_first(candidates: list[Path], parser, label: str):
    last_error: Exception | None = None
    for candidate in candidates:
        try:
            return candidate, parser(candidate)
        except Exception as exc:
            last_error = exc
    if last_error is not None:
        raise InfoError(f"No parseable {label} definition found: {last_error}") from last_error
    raise InfoError(f"No parseable {label} definition found.")


def mesh3dv2_report(mesh: DecodedHeaderMesh, files: list[Path]) -> str:
    triangles = sum(len(m.triangles) for m in mesh.meshlets)
    chains = sum(m.chains for m in mesh.meshlets)
    refs = sum(m.vertex_refs_loaded for m in mesh.meshlets)
    payload_bytes = mesh.payload_words_count * 4
    local_vertices = sum(m.nb_vertices for m in mesh.meshlets)
    local_normals = sum(m.nb_normals for m in mesh.meshlets)
    local_texcoords = sum(m.nb_texcoords for m in mesh.meshlets)
    # 32-bit MCU ABI estimate, kept in sync with the Mesh3Dv2 exporter.
    # Meshlet3Dv2 is 24 bytes, MeshMaterial3Dv2 is 32 bytes with a 32-bit
    # texture pointer, MeshMaterialExtra3Dv2 is 24 bytes when present, and
    # Mesh3Dv2 itself is 52 bytes with the optional extras pointer.
    material_bytes = len(mesh.materials) * 32
    material_extra_bytes = len(mesh.materials) * 24 if mesh.material_extras_present else 0
    meshlet_bytes = len(mesh.meshlets) * 24
    mesh_struct_bytes = 52
    static_estimate = payload_bytes + material_bytes + material_extra_bytes + meshlet_bytes + mesh_struct_bytes
    textured = sum(1 for m in mesh.materials if m.texture_symbol)
    emissive = sum(1 for m in mesh.materials if m.material_extra_present)
    emissive_textured = sum(1 for m in mesh.materials if m.emissive_texture_symbol)
    texture_lines = texture_report_lines(mesh.texture_headers)
    active_cones = [m for m in mesh.meshlets if m.cone_cos > -1.0]
    cone_line = f"{len(active_cones)} active, {len(mesh.meshlets) - len(active_cones)} disabled"
    if active_cones:
        angles = [math.degrees(math.acos(max(-1.0, min(1.0, m.cone_cos)))) for m in active_cones]
        cone_line += f" (angle avg {float(np.mean(angles)):.1f} deg, min {min(angles):.1f}, max {max(angles):.1f})"
    return "\n".join(
        [
            "TGX asset type: Mesh3Dv2",
            f"Symbol       : {mesh.symbol}",
            f"Name         : {mesh.name}",
            f"Color type   : {mesh.color_type}",
            f"Files        :\n{_format_files(files)}",
            "",
            "Geometry",
            f"  triangles       : {triangles}",
            f"  meshlets        : {len(mesh.meshlets)}",
            f"  materials       : {len(mesh.materials)} ({textured} textured, {emissive} emissive, {emissive_textured} emissive textured)",
            f"  chains          : {chains} ({chains / max(1, triangles):.3f} per triangle)",
            f"  vertex refs     : {refs} ({refs / max(1, triangles):.3f} per triangle)",
            f"  local vertices  : {local_vertices}",
            f"  local normals   : {local_normals}",
            f"  local texcoords : {local_texcoords}",
            f"  bbox            : x[{mesh.bbox[0]:.6g}, {mesh.bbox[1]:.6g}] y[{mesh.bbox[2]:.6g}, {mesh.bbox[3]:.6g}] z[{mesh.bbox[4]:.6g}, {mesh.bbox[5]:.6g}]",
            "",
            "Memory estimate, excluding texture images",
            f"  payload         : {payload_bytes} bytes",
            f"  meshlets        : {meshlet_bytes} bytes",
            f"  materials       : {material_bytes} bytes",
            f"  material extras : {material_extra_bytes} bytes",
            f"  mesh structure  : {mesh_struct_bytes} bytes",
            f"  total           : {static_estimate} bytes ({static_estimate / 1024.0:.1f} KiB)",
            "",
            "Meshlet stats",
            f"  triangles       : {_distribution([len(m.triangles) for m in mesh.meshlets])}",
            f"  chains          : {_distribution([m.chains for m in mesh.meshlets])}",
            f"  vertices        : {_distribution([m.nb_vertices for m in mesh.meshlets])}",
            f"  payload bytes   : {_distribution([m.payload_bytes for m in mesh.meshlets])}",
            f"  culling cones   : {cone_line}",
            "",
            "Textures",
            *(texture_lines or ["  none"]),
        ]
    )


def legacy_mesh_report(parsed, obj, texture_symbols: dict[str, str], files: list[Path]) -> str:
    total_tris = sum(m.nb_faces for m in parsed.meshes.values())
    total_face_len = sum(m.len_face for m in parsed.meshes.values())
    vertex_arrays = {m.vertices for m in parsed.meshes.values() if m.vertices}
    texcoord_arrays = {m.texcoords for m in parsed.meshes.values() if m.texcoords}
    normal_arrays = {m.normals for m in parsed.meshes.values() if m.normals}
    vertex_count = sum(int(parsed.vertices[name].shape[0]) for name in vertex_arrays if name in parsed.vertices)
    texcoord_count = sum(int(parsed.texcoords[name].shape[0]) for name in texcoord_arrays if name in parsed.texcoords)
    normal_count = sum(int(parsed.normals[name].shape[0]) for name in normal_arrays if name in parsed.normals)
    memory = vertex_count * 12 + texcoord_count * 8 + normal_count * 12 + total_face_len * 2
    texture_headers = _parse_texture_headers(parsed.header.read_text(encoding="utf-8", errors="replace"), parsed.header.parent)
    return "\n".join(
        [
            "TGX asset type: Mesh3D legacy",
            f"File         : {parsed.header}",
            f"Files        :\n{_format_files(files)}",
            "",
            "Geometry",
            f"  mesh declarations : {len(parsed.meshes)}",
            f"  triangles         : {total_tris}",
            f"  vertices          : {vertex_count}",
            f"  normals           : {normal_count}",
            f"  texcoords         : {texcoord_count}",
            f"  face entries      : {total_face_len}",
            f"  materials/groups  : {len(obj.materials)}",
            "",
            "Memory estimate, excluding texture images",
            f"  arrays            : {memory} bytes ({memory / 1024.0:.1f} KiB)",
            "",
            "Submeshes",
            *[
                f"  {name}: triangles={mesh.nb_faces}, vertices={mesh.nb_vertices}, texcoords={mesh.nb_texcoords}, normals={mesh.nb_normals}, texture={mesh.texture or 'none'}, next={mesh.next_mesh or 'none'}"
                for name, mesh in parsed.meshes.items()
            ],
            "",
            "Textures",
            *(texture_report_lines(texture_headers) or ["  none"]),
        ]
    )


def texture_report_lines(texture_headers: dict[str, Path]) -> list[str]:
    lines: list[str] = []
    for symbol, path in sorted(texture_headers.items()):
        tex = _parse_texture_image(path, symbol)
        if tex is None:
            lines.append(f"  {symbol}: {path} (could not parse dimensions)")
        else:
            lines.append(f"  {symbol}: {tex.width} x {tex.height}, RGB565, {tex.width * tex.height * 2} bytes, {path}")
    return lines


def _distribution(values: list[int]) -> str:
    if not values:
        return "n/a"
    arr = np.asarray(values, dtype=np.float64)
    return f"min {arr.min():.0f}, avg {arr.mean():.2f}, median {np.median(arr):.1f}, max {arr.max():.0f}"


def inspect_image(bundle: SourceBundle) -> InfoResult:
    image = parse_image_object(bundle)
    report = "\n".join(
        [
            "TGX asset type: Image",
            f"Symbol       : {image.symbol}",
            f"Color type   : {image.color_type}",
            f"Data symbol  : {image.data_symbol}",
            f"Dimensions   : {image.width} x {image.height}",
            f"Pixels       : {image.width * image.height}",
            f"Memory       : {image.memory_bytes} bytes ({image.memory_bytes / 1024.0:.1f} KiB)",
            f"Files        :\n{_format_files(bundle.files)}",
        ]
    )
    return InfoResult("image", f"Image: {image.symbol}", image.symbol, bundle.files, report, PreviewResult(image=image.image))


def parse_image_object(bundle: SourceBundle) -> ImageObjectInfo:
    decls = list(
        re.finditer(
            r"(?:static\s+)?(?:const\s+)?tgx::Image\s*<\s*tgx::(\w+)\s*>\s+(\w+)\s*\((.*?)\)\s*;",
            bundle.text,
            flags=re.S,
        )
    )
    if not decls:
        raise InfoError("No complete tgx::Image object definition was found. If this is a split file, select the .h or .cpp next to its companion file.")
    color_type, symbol, args_text = decls[0].group(1), decls[0].group(2), decls[0].group(3)
    args = _split_top_level(args_text)
    if len(args) < 3:
        raise InfoError(f"Image object '{symbol}' has an unsupported initializer.")
    data_symbol = _identifier(args[0])
    dims = _ints(",".join(args[1:3]))
    if len(dims) < 2:
        raise InfoError(f"Image object '{symbol}' does not contain constant dimensions.")
    width, height = dims[0], dims[1]
    body = _extract_named_array_body(bundle.text, data_symbol)
    pixels = decode_image_pixels(body, color_type, width, height)
    bpp = color_type_size(color_type)
    return ImageObjectInfo(symbol, color_type, data_symbol, width, height, bpp, width * height * bpp, pixels)


def color_type_size(color_type: str) -> int:
    sizes = {"RGB565": 2, "RGB24": 3, "RGB32": 4, "RGB64": 8, "RGBf": 12}
    if color_type not in sizes:
        raise InfoError(f"Unsupported TGX image color type: {color_type}")
    return sizes[color_type]


def decode_image_pixels(body: str, color_type: str, width: int, height: int) -> Image.Image:
    calls = re.findall(r"\bC\s*\((.*?)\)", body, flags=re.S)
    if not calls:
        # Accept older explicit constructors such as tgx::RGB565((uint16_t)0xffff).
        if color_type == "RGB565":
            values = _ints(body)
            rgb = [_rgb565_to_rgb(v) for v in values[: width * height]]
        else:
            raise InfoError("Image data array is not in a supported generated TGX form.")
    else:
        rgb = []
        for call in calls[: width * height]:
            vals = _numbers(call)
            if color_type == "RGB565":
                ints = _ints(call)
                if not ints:
                    raise InfoError("Invalid RGB565 image literal.")
                if len(ints) >= 3:
                    # Legacy image_converter.py emitted C(R,G,B) where R/G/B
                    # are already 5/6/5-bit components, matching
                    # tgx::RGB565(int r, int g, int b).
                    rgb.append(_rgb565_components_to_rgb(ints[0], ints[1], ints[2]))
                else:
                    rgb.append(_rgb565_to_rgb(ints[0]))
            elif color_type in ("RGB24", "RGB32"):
                if len(vals) < 3:
                    raise InfoError(f"Invalid {color_type} image literal.")
                rgb.append(tuple(max(0, min(255, int(round(v)))) for v in vals[:3]))
            elif color_type == "RGB64":
                if len(vals) < 3:
                    raise InfoError("Invalid RGB64 image literal.")
                rgb.append(tuple(max(0, min(255, int(round(v / 257.0)))) for v in vals[:3]))
            elif color_type == "RGBf":
                if len(vals) < 3:
                    raise InfoError("Invalid RGBf image literal.")
                rgb.append(tuple(max(0, min(255, int(round(v * 255.0)))) for v in vals[:3]))
            else:
                raise InfoError(f"Unsupported TGX image color type: {color_type}")
    if len(rgb) < width * height:
        raise InfoError(f"Image data has only {len(rgb)} pixels, expected {width * height}.")
    image = Image.new("RGB", (width, height))
    image.putdata(rgb[: width * height])
    return image


def _rgb565_to_rgb(value: int) -> tuple[int, int, int]:
    value &= 0xFFFF
    r5 = (value >> 11) & 31
    g6 = (value >> 5) & 63
    b5 = value & 31
    return _rgb565_components_to_rgb(r5, g6, b5)


def _rgb565_components_to_rgb(r5: int, g6: int, b5: int) -> tuple[int, int, int]:
    r5 = max(0, min(31, int(r5)))
    g6 = max(0, min(63, int(g6)))
    b5 = max(0, min(31, int(b5)))
    return ((r5 * 255 + 15) // 31, (g6 * 255 + 31) // 63, (b5 * 255 + 15) // 31)


def inspect_font(bundle: SourceBundle) -> InfoResult:
    fonts = parse_fonts(bundle)
    if not fonts:
        raise InfoError("No complete TGX font object definition was found.")
    lines = [
        "TGX asset type: Font",
        f"Objects      : {len(fonts)}",
        f"Files        :\n{_format_files(bundle.files)}",
        "",
    ]
    for font in fonts:
        if isinstance(font, IliFontInfo):
            aa = f"AA{font.bpp if font.bpp > 1 else 0}" if font.version == 23 else "mono"
            lines.extend(
                [
                    f"ILI9341_t3_font_t {font.symbol}",
                    f"  version      : {'v2.3' if font.version == 23 else 'v1'}",
                    f"  antialiasing : {aa}",
                    f"  codepoints   : {font.first}..{font.last} ({font.last - font.first + 1} slots)",
                    f"  glyphs drawn : {sum(1 for g in font.glyphs if g.present)}",
                    f"  line/cap     : {font.line_space} / {font.cap_height}",
                    f"  data/index   : {font.data_bytes} / {font.index_bytes} bytes",
                    f"  memory       : {font.data_bytes + font.index_bytes} bytes ({(font.data_bytes + font.index_bytes) / 1024.0:.1f} KiB)",
                    "",
                ]
            )
        else:
            lines.extend(
                [
                    f"GFXfont {font.symbol}",
                    f"  format       : Adafruit GFX",
                    f"  codepoints   : {font.first}..{font.last} ({font.glyph_count} slots)",
                    f"  glyphs drawn : {sum(1 for g in font.glyphs if g.present)}",
                    f"  yAdvance     : {font.y_advance}",
                    f"  bitmap       : {font.bitmap_bytes} bytes",
                    f"  memory       : {font.bitmap_bytes + font.glyph_count * 7} bytes ({(font.bitmap_bytes + font.glyph_count * 7) / 1024.0:.1f} KiB)",
                    "",
                ]
            )
    preview = render_font_preview(fonts)
    return InfoResult("font", f"Font: {fonts[0].symbol}", fonts[0].symbol, bundle.files, "\n".join(lines), PreviewResult(image=preview))


def parse_fonts(bundle: SourceBundle) -> list[IliFontInfo | GfxFontInfo]:
    fonts: list[IliFontInfo | GfxFontInfo] = []
    for match in re.finditer(r"(?:static\s+)?const\s+ILI9341_t3_font_t\s+(\w+)\s*(?:PROGMEM\s*)?=", bundle.text):
        fonts.append(parse_ili_font(bundle.text, match.group(1)))
    for match in re.finditer(r"(?:static\s+)?const\s+GFXfont\s+(\w+)\s*(?:PROGMEM\s*)?=", bundle.text):
        fonts.append(parse_gfx_font(bundle.text, match.group(1)))
    return fonts


class BitReader:
    def __init__(self, data: bytes, bitpos: int = 0):
        self.data = data
        self.bitpos = bitpos

    def read_unsigned(self, bits: int) -> int:
        value = 0
        for _ in range(bits):
            byte = self.data[self.bitpos // 8] if self.bitpos // 8 < len(self.data) else 0
            bit = (byte >> (7 - (self.bitpos % 8))) & 1
            self.bitpos += 1
            value = (value << 1) | bit
        return value

    def read_signed(self, bits: int) -> int:
        value = self.read_unsigned(bits)
        if bits > 0 and value >= (1 << (bits - 1)):
            value -= 1 << bits
        return value

    def align_byte(self) -> None:
        if self.bitpos % 8:
            self.bitpos += 8 - (self.bitpos % 8)


def parse_ili_font(text: str, symbol: str) -> IliFontInfo:
    body = _extract_named_object_body(text, symbol)
    fields = _split_top_level(body)
    if len(fields) < 17:
        raise InfoError(f"ILI font '{symbol}' has an unsupported initializer.")
    index_symbol = _identifier(fields[0])
    data_symbol = _identifier(fields[2])
    values = [_ints(field)[0] if _ints(field) else 0 for field in fields[3:17]]
    version, reserved, first, last, _, _, bits_index, bits_width, bits_height, bits_xoffset, bits_yoffset, bits_delta, line_space, cap_height = values[:14]
    data = bytes(_ints(_extract_named_array_body(text, data_symbol)))
    index = bytes(_ints(_extract_named_array_body(text, index_symbol)))
    bpp = 1
    if version == 23:
        bpp = {0: 1, 1: 2, 2: 4, 3: 8}.get(reserved, 1)
    glyphs = decode_ili_glyphs(data, index, version, bpp, first, last, bits_index, bits_width, bits_height, bits_xoffset, bits_yoffset, bits_delta)
    return IliFontInfo(symbol, version, bpp, first, last, bits_index, bits_width, bits_height, bits_xoffset, bits_yoffset, bits_delta, line_space, cap_height, len(data), len(index), glyphs)


def decode_ili_glyphs(
    data: bytes,
    index: bytes,
    version: int,
    bpp: int,
    first: int,
    last: int,
    bits_index: int,
    bits_width: int,
    bits_height: int,
    bits_xoffset: int,
    bits_yoffset: int,
    bits_delta: int,
) -> list[DecodedGlyph]:
    offsets = []
    reader = BitReader(index)
    for _ in range(first, last + 1):
        offsets.append(reader.read_unsigned(bits_index))
    glyphs: list[DecodedGlyph] = []
    for cp, offset in zip(range(first, last + 1), offsets):
        r = BitReader(data, offset * 8)
        _ = r.read_unsigned(3)
        width = r.read_unsigned(bits_width)
        height = r.read_unsigned(bits_height)
        x_offset = r.read_signed(bits_xoffset)
        y_offset = r.read_signed(bits_yoffset)
        x_advance = r.read_unsigned(bits_delta)
        levels: list[int] = []
        if width > 0 and height > 0:
            if bpp == 1 and version == 1:
                y = 0
                rows: list[list[int]] = []
                while y < height:
                    compressed = r.read_unsigned(1)
                    repeat = 0
                    if compressed:
                        repeat = r.read_unsigned(3) + 1
                    line = [r.read_unsigned(1) for _ in range(width)]
                    rows.append(line)
                    for _ in range(repeat):
                        if len(rows) < height:
                            rows.append(list(line))
                    y = len(rows)
                levels = [value for row in rows[:height] for value in row]
            else:
                r.align_byte()
                levels = [r.read_unsigned(bpp) for _ in range(width * height)]
        glyphs.append(DecodedGlyph(cp, width, height, x_offset, y_offset, x_advance, levels, bpp))
    return glyphs


def parse_gfx_font(text: str, symbol: str) -> GfxFontInfo:
    body = _extract_named_object_body(text, symbol)
    fields = _split_top_level(body)
    if len(fields) < 5:
        raise InfoError(f"GFX font '{symbol}' has an unsupported initializer.")
    bitmap_symbol = _identifier(fields[0])
    glyph_symbol = _identifier(fields[1])
    first = _ints(fields[2])[0]
    last = _ints(fields[3])[0]
    y_advance = _ints(fields[4])[0]
    bitmap = bytes(_ints(_extract_named_array_body(text, bitmap_symbol)))
    glyph_rows = _split_top_level(_extract_named_array_body(text, glyph_symbol))
    glyphs: list[DecodedGlyph] = []
    for cp, row in zip(range(first, last + 1), glyph_rows):
        vals = _ints(row)
        if len(vals) < 6:
            continue
        offset, width, height, x_advance, x_offset, y_offset = vals[:6]
        if x_offset >= 128:
            x_offset -= 256
        if y_offset >= 128:
            y_offset -= 256
        levels = read_gfx_bitmap(bitmap, offset, width * height) if width and height else []
        glyphs.append(DecodedGlyph(cp, width, height, x_offset, y_offset, x_advance, levels, 1))
    return GfxFontInfo(symbol, first, last, y_advance, len(bitmap), last - first + 1, glyphs)


def read_gfx_bitmap(bitmap: bytes, offset: int, pixels: int) -> list[int]:
    r = BitReader(bitmap, offset * 8)
    return [r.read_unsigned(1) for _ in range(pixels)]


def render_font_preview(fonts: list[IliFontInfo | GfxFontInfo]) -> Image.Image:
    columns = 16
    sections = []
    width = 1100
    total_h = 20
    for font in fonts:
        max_gw = max((g.width for g in font.glyphs), default=0)
        max_gh = max((g.height for g in font.glyphs), default=0)
        cell_w = max(42, max_gw + 10)
        cell_h = max(42, max_gh + 18)
        rows = math.ceil(len(font.glyphs) / columns)
        section_h = 36 + rows * cell_h + 70
        sections.append((font, cell_w, cell_h, rows, section_h))
        width = max(width, 20 + columns * cell_w + 20)
        total_h += section_h + 16
    image = Image.new("RGB", (width, max(220, total_h)), (30, 34, 42))
    draw = ImageDraw.Draw(image)
    y = 16
    for font, cell_w, cell_h, _rows, section_h in sections:
        draw.text((12, y), font_title(font), fill=(235, 240, 248))
        sample_y = y + 30
        draw.text((12, sample_y), "A lazy fox jumps over gypq 0123456789.", fill=(160, 175, 200))
        render_font_text(draw, font, "A lazy fox jumps over gypq 0123456789.", 12, sample_y + 34, (245, 245, 230))
        grid_y = y + 86
        for i, glyph in enumerate(font.glyphs):
            gx = 12 + (i % columns) * cell_w
            gy = grid_y + (i // columns) * cell_h
            fill = (38, 44, 56) if glyph.present else (28, 30, 36)
            outline = (65, 74, 92)
            draw.rectangle((gx, gy, gx + cell_w - 2, gy + cell_h - 2), fill=fill, outline=outline)
            label = chr(glyph.codepoint) if 32 <= glyph.codepoint <= 126 else f"{glyph.codepoint:02X}"
            draw.text((gx + 3, gy + 2), label, fill=(160, 170, 185))
            if glyph.present:
                baseline = gy + cell_h - 8
                draw.line((gx + 2, baseline, gx + cell_w - 4, baseline), fill=(92, 54, 60))
                paste_glyph(draw, glyph, gx + 5 + glyph.x_offset, baseline - glyph.y_offset, (235, 245, 245))
        y += section_h + 16
    return image


def font_title(font: IliFontInfo | GfxFontInfo) -> str:
    if isinstance(font, IliFontInfo):
        aa = f"AA{font.bpp if font.bpp > 1 else 0}" if font.version == 23 else "mono"
        return f"{font.symbol} - ILI9341 {'v2.3' if font.version == 23 else 'v1'} - {aa} - {font.first}..{font.last}"
    return f"{font.symbol} - Adafruit GFX - {font.first}..{font.last}"


def render_font_text(draw: ImageDraw.ImageDraw, font: IliFontInfo | GfxFontInfo, text: str, x: int, baseline: int, color: tuple[int, int, int]) -> None:
    cursor = x
    by_cp = {g.codepoint: g for g in font.glyphs}
    for ch in text:
        glyph = by_cp.get(ord(ch))
        if glyph is None:
            cursor += 6
            continue
        if glyph.present:
            paste_glyph(draw, glyph, cursor + glyph.x_offset, baseline - glyph.y_offset, color)
        cursor += glyph.x_advance


def paste_glyph(draw: ImageDraw.ImageDraw, glyph: DecodedGlyph, x: int, y_bottom: int, color: tuple[int, int, int]) -> None:
    if not glyph.present:
        return
    max_level = max(1, (1 << glyph.bpp) - 1)
    top = y_bottom - glyph.height
    for gy in range(glyph.height):
        row = gy * glyph.width
        for gx in range(glyph.width):
            level = glyph.levels[row + gx]
            if level <= 0:
                continue
            alpha = level / max_level
            c = tuple(int(round(v * alpha)) for v in color)
            draw.point((x + gx, top + gy), fill=c)
