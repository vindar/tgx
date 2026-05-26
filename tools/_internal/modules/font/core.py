from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

try:
    from fontTools.ttLib import TTFont
except ImportError as exc:  # pragma: no cover - user-facing dependency error
    raise RuntimeError("fonttools is required. Install it with: python -m pip install fonttools") from exc

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError as exc:  # pragma: no cover - user-facing dependency error
    raise RuntimeError("Pillow is required. Install it with: python -m pip install pillow") from exc


SUPPORTED_FONT_FORMATS = ("ili9341-v1", "ili9341-v23", "adafruit-gfx")
SUPPORTED_AA = ("none", "1", "2", "4", "8")
SUPPORTED_LAYOUTS = ("header", "split")
CHARACTER_PRESETS = (
    "none",
    "all",
    "ascii",
    "printable",
    "latin1",
    "letters-digits",
    "letters",
    "digits",
)


@dataclass
class FontGlyph:
    codepoint: int
    selected: bool
    present: bool
    width: int
    height: int
    x_offset: int
    y_offset_ili: int
    y_offset_gfx: int
    x_advance: int
    levels: list[int] = field(default_factory=list)


@dataclass
class RasterFont:
    source_path: Path
    symbol_base: str
    size: int
    line_space: int
    cap_height: int
    glyphs: dict[int, FontGlyph]
    selected_codepoints: set[int]
    exported_codepoints: list[int]
    bpp: int


@dataclass
class FontExportOptions:
    input_path: Path
    output: Path
    symbol_base: str | None = None
    sizes: tuple[int, ...] = (12, 16, 24)
    output_format: str = "ili9341-v23"
    antialias: str = "4"
    chars: str = "ascii"
    layout: str = "header"
    progmem: bool = True
    preview_png: Path | None = None


@dataclass
class FontExportResult:
    header_path: Path
    cpp_path: Path | None
    symbols: list[str]
    glyph_count: int
    data_bytes: int
    index_bytes: int
    bitmap_bytes: int
    preview_path: Path | None = None


def sanitize_identifier(name: str, fallback: str = "font") -> str:
    out = re.sub(r"\W+", "_", name.strip())
    out = out.strip("_")
    if not out:
        out = fallback
    if out[0].isdigit():
        out = "_" + out
    return out


def parse_sizes(text: str | Iterable[int]) -> tuple[int, ...]:
    if isinstance(text, str):
        values = [int(part.strip()) for part in re.split(r"[,; ]+", text.strip()) if part.strip()]
    else:
        values = [int(v) for v in text]
    values = sorted(set(values))
    if not values or any(v <= 0 for v in values):
        raise ValueError("font sizes must be positive integers")
    return tuple(values)


def available_codepoints(font_path: Path) -> set[int]:
    with TTFont(str(font_path), lazy=True) as font:
        out: set[int] = set()
        for table in font["cmap"].tables:
            out.update(cp for cp in table.cmap.keys() if 0 <= cp <= 255)
        return out


def font_family_name(font_path: Path) -> str:
    try:
        with TTFont(str(font_path), lazy=True) as font:
            names = font["name"].names
            for name_id in (4, 1):
                for item in names:
                    if item.nameID == name_id:
                        try:
                            text = item.toUnicode()
                        except Exception:
                            continue
                        if text.strip():
                            return text.strip()
    except Exception:
        pass
    return font_path.stem


def parse_character_selection(text: str, available: set[int] | None = None) -> set[int]:
    available = set(range(256)) if available is None else {cp for cp in available if 0 <= cp <= 255}
    raw = (text or "ascii").strip()
    key = raw.lower().replace("_", "-").replace(" ", "")
    if key in ("none", "aucun", "empty"):
        return set()
    if key in ("all", "tout"):
        return set(available)
    if key in ("ascii", "127", "upto127", "jusqua127", "0-127"):
        return {cp for cp in available if 0 <= cp <= 127}
    if key in ("printable", "ascii-printable", "32-126"):
        return {cp for cp in available if 32 <= cp <= 126}
    if key in ("latin1", "latin-1", "0-255"):
        return {cp for cp in available if 0 <= cp <= 255}
    if key in ("letters-digits", "lettersdigits", "alnum", "lettresetchiffres", "lettres-chiffres"):
        return {cp for cp in available if (48 <= cp <= 57) or (65 <= cp <= 90) or (97 <= cp <= 122)}
    if key in ("digits", "chiffres"):
        return {cp for cp in available if 48 <= cp <= 57}
    if key in ("letters", "lettres"):
        return {cp for cp in available if (65 <= cp <= 90) or (97 <= cp <= 122)}
    if key in ("latin1-letters-digits", "latin1alnum", "latin-1-letters-digits"):
        return {cp for cp in available if chr(cp).isalnum()}
    if key in ("latin1-letters", "latin1letters", "latin-1-letters"):
        return {cp for cp in available if chr(cp).isalpha()}

    result: set[int] = set()
    for token in re.split(r"[,;]+", raw):
        token = token.strip()
        if not token:
            continue
        if len(token) == 1 and not token.isdigit():
            cp = ord(token)
            if 0 <= cp <= 255:
                result.add(cp)
            continue
        match = re.match(r"^(?:0x([0-9a-fA-F]+)|(\d+))\s*-\s*(?:0x([0-9a-fA-F]+)|(\d+))$", token)
        if match:
            start = int(match.group(1) or match.group(2), 16 if match.group(1) else 10)
            end = int(match.group(3) or match.group(4), 16 if match.group(3) else 10)
            if start > end:
                start, end = end, start
            result.update(cp for cp in range(max(0, start), min(255, end) + 1))
            continue
        if token.lower().startswith("0x"):
            cp = int(token, 16)
        else:
            cp = int(token, 10)
        if 0 <= cp <= 255:
            result.add(cp)
    return result & available


def exported_range(selected: set[int], output_format: str) -> list[int]:
    if not selected:
        raise ValueError("no characters selected")
    first = min(selected)
    last = max(selected)
    if output_format == "adafruit-gfx" and (first < 0 or last > 255):
        raise ValueError("Adafruit GFX fonts support only codepoints 0..255")
    return list(range(first, last + 1))


def normalize_format(value: str) -> str:
    key = value.strip().lower().replace("_", "-")
    aliases = {
        "ili": "ili9341-v23",
        "ili9341": "ili9341-v23",
        "ili9341-v1": "ili9341-v1",
        "ili9341-t3-v1": "ili9341-v1",
        "v1": "ili9341-v1",
        "ili9341-v23": "ili9341-v23",
        "ili9341-v2.3": "ili9341-v23",
        "packedbdf": "ili9341-v23",
        "v23": "ili9341-v23",
        "adafruit": "adafruit-gfx",
        "adafruit-gfx": "adafruit-gfx",
        "gfx": "adafruit-gfx",
        "gfxfont": "adafruit-gfx",
    }
    if key not in aliases:
        raise ValueError("unsupported font format '{}'; choose {}".format(value, ", ".join(SUPPORTED_FONT_FORMATS)))
    return aliases[key]


def normalize_aa(value: str, output_format: str) -> int:
    key = str(value).strip().lower()
    if key in ("none", "no", "off", "0", "1"):
        bpp = 1
    elif key in ("2", "aa2"):
        bpp = 2
    elif key in ("4", "aa4"):
        bpp = 4
    elif key in ("8", "aa8"):
        bpp = 8
    else:
        raise ValueError("unsupported antialiasing '{}'; choose none, 2, 4, or 8".format(value))
    if output_format in ("ili9341-v1", "adafruit-gfx"):
        return 1
    return bpp


def _font_getbbox(font: ImageFont.FreeTypeFont, ch: str) -> tuple[int, int, int, int]:
    try:
        return tuple(int(round(v)) for v in font.getbbox(ch, anchor="ls"))
    except TypeError:
        left, top, right, bottom = font.getbbox(ch)
        ascent, _ = font.getmetrics()
        return int(left), int(top - ascent), int(right), int(bottom - ascent)


def _font_advance(font: ImageFont.FreeTypeFont, ch: str) -> int:
    try:
        return max(0, int(round(font.getlength(ch))))
    except Exception:
        return max(0, _font_getbbox(font, ch)[2] - _font_getbbox(font, ch)[0])


def rasterize_font(font_path: Path, symbol_base: str, size: int, selected: set[int], output_format: str, bpp: int) -> RasterFont:
    font = ImageFont.truetype(str(font_path), size=size)
    ascent, descent = font.getmetrics()
    line_space = max(1, int(math.ceil(ascent + descent)))
    space_advance = max(1, _font_advance(font, " ") or int(round(size * 0.33)))
    exported = exported_range(selected, output_format)
    cap_bbox = _font_getbbox(font, "E")
    cap_height = max(1, -cap_bbox[1]) if cap_bbox[2] > cap_bbox[0] else max(1, ascent)
    glyphs: dict[int, FontGlyph] = {}
    levels_max = (1 << bpp) - 1

    for cp in exported:
        ch = chr(cp)
        selected_here = cp in selected
        present = selected_here
        if not selected_here:
            glyphs[cp] = FontGlyph(cp, False, False, 0, 0, 0, 0, 0, space_advance, [])
            continue

        left, top, right, bottom = _font_getbbox(font, ch)
        width = max(0, right - left)
        height = max(0, bottom - top)
        advance = max(0, _font_advance(font, ch))
        if advance == 0:
            advance = space_advance if cp == 32 else width
        if width <= 0 or height <= 0:
            glyphs[cp] = FontGlyph(cp, True, present, 0, 0, 0, 0, 0, advance, [])
            continue

        image = Image.new("L", (width, height), 0)
        draw = ImageDraw.Draw(image)
        try:
            draw.text((-left, -top), ch, fill=255, font=font, anchor="ls")
        except TypeError:
            draw.text((-left, -top - ascent), ch, fill=255, font=font)
        alpha = list(image.getdata())
        if bpp == 1:
            levels = [1 if value >= 128 else 0 for value in alpha]
        else:
            levels = [int(round(value * levels_max / 255.0)) for value in alpha]
        y_offset_ili = -bottom
        y_offset_gfx = top
        glyphs[cp] = FontGlyph(cp, True, present, width, height, left, y_offset_ili, y_offset_gfx, advance, levels)

    return RasterFont(
        source_path=font_path,
        symbol_base=symbol_base,
        size=size,
        line_space=line_space,
        cap_height=cap_height,
        glyphs=glyphs,
        selected_codepoints=set(selected),
        exported_codepoints=exported,
        bpp=bpp,
    )


class BitWriter:
    def __init__(self):
        self.data: list[int] = []
        self.current = 0
        self.count = 0

    @property
    def byte_count(self) -> int:
        return len(self.data)

    @property
    def bit_count(self) -> int:
        return len(self.data) * 8 + self.count

    def write_bit(self, value: int) -> None:
        if value:
            self.current |= 1 << (7 - self.count)
        self.count += 1
        if self.count == 8:
            self.data.append(self.current)
            self.current = 0
            self.count = 0

    def write_unsigned(self, value: int, bits: int) -> None:
        if bits <= 0:
            return
        if value < 0 or value >= (1 << bits):
            raise ValueError(f"value {value} does not fit in {bits} unsigned bits")
        for bit in range(bits - 1, -1, -1):
            self.write_bit((value >> bit) & 1)

    def write_signed(self, value: int, bits: int) -> None:
        if bits <= 0:
            return
        min_value = -(1 << (bits - 1))
        max_value = (1 << (bits - 1)) - 1
        if value < min_value or value > max_value:
            raise ValueError(f"value {value} does not fit in {bits} signed bits")
        if value < 0:
            value = (1 << bits) + value
        self.write_unsigned(value, bits)

    def pad_to_byte(self) -> None:
        while self.count:
            self.write_bit(0)

    def bytes(self) -> bytes:
        self.pad_to_byte()
        return bytes(self.data)


def bits_required_unsigned(max_value: int) -> int:
    max_value = max(0, int(max_value))
    bits = 1
    while max_value >= (1 << bits):
        bits += 1
    return bits


def bits_required_signed(min_value: int, max_value: int) -> int:
    min_value = min(0, int(min_value))
    max_value = max(0, int(max_value))
    bits = 2
    while min_value < -(1 << (bits - 1)):
        bits += 1
    while max_value >= (1 << (bits - 1)):
        bits += 1
    return bits


@dataclass
class IliEncodedFont:
    symbol: str
    data: bytes
    index: bytes
    version: int
    reserved: int
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


@dataclass
class GfxEncodedFont:
    symbol: str
    bitmap: bytes
    glyph_rows: list[tuple[int, int, int, int, int, int]]
    first: int
    last: int
    y_advance: int


def _glyph_line(glyph: FontGlyph, y: int) -> list[int]:
    if glyph.width <= 0 or glyph.height <= 0:
        return []
    start = y * glyph.width
    return glyph.levels[start : start + glyph.width]


def _lines_identical(glyph: FontGlyph, y0: int, y1: int) -> bool:
    return _glyph_line(glyph, y0) == _glyph_line(glyph, y1)


def _num_identical_lines(glyph: FontGlyph, y: int) -> int:
    count = 0
    for y2 in range(y + 1, glyph.height):
        if not _lines_identical(glyph, y, y2):
            break
        count += 1
    return count


def _encode_ili_glyph(writer: BitWriter, glyph: FontGlyph, bpp: int, bits_width: int, bits_height: int, bits_xoffset: int, bits_yoffset: int, bits_delta: int) -> None:
    writer.write_unsigned(0, 3)
    writer.write_unsigned(glyph.width, bits_width)
    writer.write_unsigned(glyph.height, bits_height)
    writer.write_signed(glyph.x_offset, bits_xoffset)
    writer.write_signed(glyph.y_offset_ili, bits_yoffset)
    writer.write_unsigned(glyph.x_advance, bits_delta)
    if bpp == 1:
        y = 0
        while y < glyph.height:
            repeat = min(6, _num_identical_lines(glyph, y))
            if repeat == 0:
                writer.write_bit(0)
                for value in _glyph_line(glyph, y):
                    writer.write_unsigned(value, 1)
            else:
                writer.write_bit(1)
                writer.write_unsigned(repeat - 1, 3)
                for value in _glyph_line(glyph, y):
                    writer.write_unsigned(value, 1)
                y += repeat
            y += 1
    else:
        writer.pad_to_byte()
        for value in glyph.levels:
            writer.write_unsigned(value, bpp)
    writer.pad_to_byte()


def encode_ili9341(font: RasterFont, symbol: str, version: int) -> IliEncodedFont:
    glyph_list = [font.glyphs[cp] for cp in font.exported_codepoints]
    max_width = max((g.width for g in glyph_list), default=0)
    max_height = max((g.height for g in glyph_list), default=0)
    min_x = min((g.x_offset for g in glyph_list), default=0)
    max_x = max((g.x_offset for g in glyph_list), default=0)
    min_y = min((g.y_offset_ili for g in glyph_list), default=0)
    max_y = max((g.y_offset_ili for g in glyph_list), default=0)
    max_delta = max((g.x_advance for g in glyph_list), default=0)

    bits_width = bits_required_unsigned(max_width)
    bits_height = bits_required_unsigned(max_height)
    bits_xoffset = bits_required_signed(min_x, max_x)
    bits_yoffset = bits_required_signed(min_y, max_y)
    bits_delta = bits_required_unsigned(max_delta)

    data_writer = BitWriter()
    offsets: list[int] = []
    for glyph in glyph_list:
        offsets.append(data_writer.byte_count)
        _encode_ili_glyph(data_writer, glyph, font.bpp, bits_width, bits_height, bits_xoffset, bits_yoffset, bits_delta)
    data = data_writer.bytes()
    bits_index = bits_required_unsigned(len(data))
    index_writer = BitWriter()
    for offset in offsets:
        index_writer.write_unsigned(offset, bits_index)
    index = index_writer.bytes()
    reserved = 0 if version == 1 else {1: 0, 2: 1, 4: 2, 8: 3}[font.bpp]
    return IliEncodedFont(
        symbol=symbol,
        data=data,
        index=index,
        version=version,
        reserved=reserved,
        first=font.exported_codepoints[0],
        last=font.exported_codepoints[-1],
        bits_index=bits_index,
        bits_width=bits_width,
        bits_height=bits_height,
        bits_xoffset=bits_xoffset,
        bits_yoffset=bits_yoffset,
        bits_delta=bits_delta,
        line_space=font.line_space,
        cap_height=font.cap_height,
    )


def encode_gfx(font: RasterFont, symbol: str) -> GfxEncodedFont:
    writer = BitWriter()
    rows: list[tuple[int, int, int, int, int, int]] = []
    for cp in font.exported_codepoints:
        glyph = font.glyphs[cp]
        if glyph.width > 255 or glyph.height > 255 or glyph.x_advance > 255:
            raise ValueError(
                f"glyph U+{cp:04X} is too large for Adafruit GFX; reduce the font size or use ILI9341_t3 format"
            )
        if glyph.x_offset < -128 or glyph.x_offset > 127 or glyph.y_offset_gfx < -128 or glyph.y_offset_gfx > 127:
            raise ValueError(
                f"glyph U+{cp:04X} offset is too large for Adafruit GFX; reduce the font size or use ILI9341_t3 format"
            )
        writer.pad_to_byte()
        offset = writer.byte_count
        for value in glyph.levels:
            writer.write_unsigned(1 if value else 0, 1)
        rows.append((offset, glyph.width, glyph.height, glyph.x_advance, glyph.x_offset, glyph.y_offset_gfx))
    bitmap = writer.bytes()
    if len(bitmap) > 65535:
        raise ValueError("Adafruit GFX bitmap is larger than 65535 bytes; split the font or use ILI9341_t3 format")
    return GfxEncodedFont(symbol, bitmap, rows, font.exported_codepoints[0], font.exported_codepoints[-1], font.line_space)


def _bytes_array(name: str, values: bytes, static: bool, progmem: bool) -> str:
    storage = "static const" if static else "const"
    pm = " PROGMEM" if progmem else ""
    lines = [f"{storage} unsigned char {name}[]{pm} = {{"]
    for i in range(0, len(values), 16):
        chunk = values[i : i + 16]
        suffix = "," if i + 16 < len(values) else ""
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + suffix)
    if not values:
        lines.append("    0x00")
    lines.append("};")
    return "\n".join(lines) + "\n\n"


def _gfx_glyph_array(name: str, rows: list[tuple[int, int, int, int, int, int]], static: bool, progmem: bool) -> str:
    storage = "static const" if static else "const"
    pm = " PROGMEM" if progmem else ""
    lines = [f"{storage} GFXglyph {name}[]{pm} = {{"]
    for row in rows:
        lines.append("    {{ {}, {}, {}, {}, {}, {} }},".format(*row))
    lines.append("};\n")
    return "\n".join(lines)


def _header_guard(path: Path) -> str:
    return "_" + sanitize_identifier(path.stem).upper() + "_H_"


def _comment(options: FontExportOptions, symbols: list[str], glyphs: int, data_bytes: int) -> str:
    return "\n".join(
        [
            "//",
            "// TGX font generated by tgx_font",
            f"// Source: {Path(options.input_path).name}",
            f"// Format: {options.output_format}",
            f"// Symbols: {', '.join(symbols)}",
            f"// Exported glyph slots: {glyphs}",
            f"// Static font data: {data_bytes} bytes ({data_bytes / 1024.0:.1f} KiB), excluding C++ struct padding",
            "//",
            "",
        ]
    )


def _font_symbol(base: str, size: int, output_format: str, bpp: int) -> str:
    if output_format == "ili9341-v23":
        return f"{base}_AA{bpp}_{size}"
    return f"{base}_{size}"


def export_font(options: FontExportOptions) -> FontExportResult:
    options.input_path = Path(options.input_path)
    options.output = Path(options.output)
    options.output_format = normalize_format(options.output_format)
    options.layout = options.layout.lower()
    if options.layout not in SUPPORTED_LAYOUTS:
        raise ValueError("layout must be 'header' or 'split'")
    if not options.input_path.exists():
        raise FileNotFoundError(str(options.input_path))

    sizes = parse_sizes(options.sizes)
    family = font_family_name(options.input_path)
    symbol_base = sanitize_identifier(options.symbol_base or f"font_{family}")
    available = available_codepoints(options.input_path)
    selected = parse_character_selection(options.chars, available)
    if not selected:
        raise ValueError("no characters selected; choose a character preset or explicit ranges")
    bpp = normalize_aa(options.antialias, options.output_format)
    if options.output_format == "ili9341-v1":
        version = 1
    elif options.output_format == "ili9341-v23":
        version = 23
    else:
        version = 0

    header_path = options.output
    if header_path.suffix.lower() not in (".h", ".hpp"):
        header_path = header_path.with_suffix(".h")
    cpp_path = None if options.layout == "header" else header_path.with_suffix(".cpp")
    header_path.parent.mkdir(parents=True, exist_ok=True)
    symbols: list[str] = []
    bodies: list[str] = []
    externs: list[str] = []
    total_data = 0
    total_index = 0
    total_bitmap = 0
    total_glyphs = 0

    preview_rasters: list[RasterFont] = []
    for size in sizes:
        symbol = _font_symbol(symbol_base, size, options.output_format, bpp)
        symbols.append(symbol)
        raster = rasterize_font(options.input_path, symbol_base, size, selected, options.output_format, bpp)
        if options.preview_png is not None:
            preview_rasters.append(raster)
        total_glyphs += len(raster.exported_codepoints)
        if options.output_format == "adafruit-gfx":
            encoded = encode_gfx(raster, symbol)
            bitmap_name = f"{symbol}Bitmaps"
            glyph_name = f"{symbol}Glyphs"
            static = options.layout == "header"
            bodies.append(_bytes_array(bitmap_name, encoded.bitmap, static, options.progmem))
            bodies.append(_gfx_glyph_array(glyph_name, encoded.glyph_rows, static, options.progmem))
            storage = "static const" if static else "const"
            pm = " PROGMEM" if options.progmem else ""
            bodies.append(
                f"{storage} GFXfont {symbol}{pm} = {{\n"
                f"    (uint8_t*){bitmap_name},\n"
                f"    (GFXglyph*){glyph_name},\n"
                f"    0x{encoded.first:02X}, 0x{encoded.last:02X}, {encoded.y_advance}\n"
                f"}};\n\n"
            )
            externs.append(f"extern const GFXfont {symbol};")
            total_bitmap += len(encoded.bitmap)
            total_data += len(encoded.bitmap) + len(encoded.glyph_rows) * 7
        else:
            encoded = encode_ili9341(raster, symbol, version)
            data_name = f"{symbol}_data"
            index_name = f"{symbol}_index"
            static = options.layout == "header"
            bodies.append(_bytes_array(data_name, encoded.data, static, options.progmem))
            bodies.append(_bytes_array(index_name, encoded.index, static, options.progmem))
            storage = "static const" if static else "const"
            pm = " PROGMEM" if options.progmem else ""
            bodies.append(
                f"{storage} ILI9341_t3_font_t {symbol}{pm} = {{\n"
                f"    {index_name},\n"
                f"    0,\n"
                f"    {data_name},\n"
                f"    {encoded.version}, {encoded.reserved},\n"
                f"    {encoded.first}, {encoded.last},\n"
                f"    0, 0,\n"
                f"    {encoded.bits_index}, {encoded.bits_width}, {encoded.bits_height},\n"
                f"    {encoded.bits_xoffset}, {encoded.bits_yoffset}, {encoded.bits_delta},\n"
                f"    {encoded.line_space}, {encoded.cap_height}\n"
                f"}};\n\n"
            )
            externs.append(f"extern const ILI9341_t3_font_t {symbol};")
            total_bitmap += len(encoded.data)
            total_index += len(encoded.index)
            total_data += len(encoded.data) + len(encoded.index)

    comment = _comment(options, symbols, total_glyphs, total_data)
    if options.layout == "header":
        header_path.write_text(comment + "#pragma once\n\n#include <tgx.h>\n\n" + "".join(bodies), encoding="utf-8", newline="\n")
    else:
        guard = _header_guard(header_path)
        header_path.write_text(
            comment
            + f"#ifndef {guard}\n#define {guard}\n\n#include <tgx.h>\n\n"
            + "\n".join(externs)
            + f"\n\n#endif // {guard}\n",
            encoding="utf-8",
            newline="\n",
        )
        assert cpp_path is not None
        cpp_path.write_text(comment + f'#include "{header_path.name}"\n\n' + "".join(bodies), encoding="utf-8", newline="\n")

    preview_path = None
    if options.preview_png is not None:
        preview_path = Path(options.preview_png)
        write_preview_png(preview_rasters, preview_path)

    return FontExportResult(header_path, cpp_path, symbols, total_glyphs, total_data, total_index, total_bitmap, preview_path)


def _blend_channel(background: int, foreground: int, alpha: int) -> int:
    return int(round(background + (foreground - background) * alpha / 255.0))


def _blend_rgb(background: tuple[int, int, int], foreground: tuple[int, int, int], alpha: int) -> tuple[int, int, int]:
    if alpha <= 0:
        return background
    if alpha >= 255:
        return foreground
    return (
        _blend_channel(background[0], foreground[0], alpha),
        _blend_channel(background[1], foreground[1], alpha),
        _blend_channel(background[2], foreground[2], alpha),
    )


def write_preview_png(fonts: RasterFont | list[RasterFont] | tuple[RasterFont, ...], output: Path, columns: int = 16) -> None:
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    font_list = list(fonts) if isinstance(fonts, (list, tuple)) else [fonts]
    if not font_list:
        raise ValueError("preview needs at least one rasterized font")

    section_margin = 12
    title_h = 24
    sections: list[tuple[RasterFont, int, int, int]] = []
    total_h = section_margin
    max_w = 0
    for font in font_list:
        cell_w = max(24, max((g.width + 8 for g in font.glyphs.values()), default=24))
        cell_h = max(24, font.line_space + 8)
        rows = math.ceil(len(font.exported_codepoints) / columns)
        section_w = columns * cell_w
        section_h = title_h + rows * cell_h
        sections.append((font, cell_w, cell_h, rows))
        total_h += section_h + section_margin
        max_w = max(max_w, section_w)

    image = Image.new("RGB", (max_w, total_h), (32, 34, 38))
    draw = ImageDraw.Draw(image)
    y_base = section_margin
    for font, cell_w, cell_h, rows in sections:
        draw.text((2, y_base + 4), f"{Path(font.source_path).name} - {font.size}px - {font.bpp} bpp", fill=(225, 225, 225))
        grid_y = y_base + title_h
        for index, cp in enumerate(font.exported_codepoints):
            col = index % columns
            row = index // columns
            x0 = col * cell_w
            y0 = grid_y + row * cell_h
            glyph = font.glyphs[cp]
            bg = (52, 76, 54) if glyph.selected else (48, 48, 52)
            draw.rectangle((x0, y0, x0 + cell_w - 1, y0 + cell_h - 1), fill=bg, outline=(88, 88, 96))
            baseline = y0 + 4 + max(1, font.cap_height)
            if glyph.width and glyph.height:
                gx = x0 + 4 + glyph.x_offset
                gy = baseline - glyph.height - glyph.y_offset_ili
                for yy in range(glyph.height):
                    for xx in range(glyph.width):
                        value = glyph.levels[yy * glyph.width + xx]
                        if value:
                            alpha = 255 if font.bpp == 1 else int(round(value * 255 / ((1 << font.bpp) - 1)))
                            px = gx + xx
                            py = gy + yy
                            if 0 <= px < image.width and 0 <= py < image.height:
                                image.putpixel((px, py), _blend_rgb(bg, (255, 255, 255), alpha))
            label = f"{cp:02X}"
            draw.text((x0 + 2, y0 + cell_h - 10), label, fill=(190, 190, 190))
        y_base += title_h + rows * cell_h + section_margin
    image.save(output)
