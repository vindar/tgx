from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - user-facing dependency error
    raise RuntimeError("Pillow is required. Install it with: python -m pip install pillow") from exc


SUPPORTED_COLOR_TYPES = ("RGB565", "RGB24", "RGB32", "RGB64", "RGBf")
SUPPORTED_LAYOUTS = ("header", "split")
SUPPORTED_RESAMPLING = ("nearest", "bilinear", "bicubic", "lanczos")


@dataclass
class ImageExportOptions:
    input_path: Path
    output_dir: Path
    color_type: str = "RGB565"
    object_name: Optional[str] = None
    output_base: Optional[str] = None
    layout: str = "header"
    resize: Optional[Tuple[int, int]] = None
    resample: str = "lanczos"
    flip_y: bool = False
    progmem: bool = True
    premultiply_alpha: bool = True


@dataclass
class ImageExportResult:
    object_name: str
    color_type: str
    width: int
    height: int
    header_path: Path
    cpp_path: Optional[Path]
    data_bytes: int


def sanitize_identifier(name: str, fallback: str = "image") -> str:
    out = re.sub(r"\W+", "_", name.strip())
    out = out.strip("_")
    if not out:
        out = fallback
    if out[0].isdigit():
        out = "_" + out
    return out


def normalize_color_type(color_type: str) -> str:
    key = color_type.strip().lower()
    aliases = {
        "rgb16": "RGB565",
        "rgb565": "RGB565",
        "565": "RGB565",
        "rgb24": "RGB24",
        "rgb888": "RGB24",
        "rgb32": "RGB32",
        "rgba": "RGB32",
        "rgba32": "RGB32",
        "rgb64": "RGB64",
        "rgba64": "RGB64",
        "rgbf": "RGBf",
        "float": "RGBf",
    }
    if key not in aliases:
        raise ValueError("Unsupported color type '{}'. Choose one of: {}".format(color_type, ", ".join(SUPPORTED_COLOR_TYPES)))
    return aliases[key]


def parse_resize(value: Optional[str]) -> Optional[Tuple[int, int]]:
    if value is None or value == "":
        return None
    match = re.match(r"^\s*(\d+)\s*[xX,]\s*(\d+)\s*$", value)
    if not match:
        raise ValueError("Resize must use WIDTHxHEIGHT, for example 128x64")
    width = int(match.group(1))
    height = int(match.group(2))
    if width <= 0 or height <= 0:
        raise ValueError("Resize dimensions must be positive")
    return width, height


def color_type_size(color_type: str) -> int:
    return {
        "RGB565": 2,
        "RGB24": 3,
        "RGB32": 4,
        "RGB64": 8,
        "RGBf": 12,
    }[normalize_color_type(color_type)]


def _resample_filter(name: str) -> int:
    name = name.lower()
    if name not in SUPPORTED_RESAMPLING:
        raise ValueError("Unsupported resampling '{}'. Choose one of: {}".format(name, ", ".join(SUPPORTED_RESAMPLING)))

    resampling = getattr(Image, "Resampling", Image)
    if name == "nearest":
        return resampling.NEAREST
    if name == "bilinear":
        return resampling.BILINEAR
    if name == "bicubic":
        return resampling.BICUBIC
    return resampling.LANCZOS


def _premultiply_pixel(pixel: Tuple[int, int, int, int]) -> Tuple[int, int, int, int]:
    r, g, b, a = pixel
    if a >= 255:
        return pixel
    return (int(round(r * a / 255.0)), int(round(g * a / 255.0)), int(round(b * a / 255.0)), a)


def _load_pixels(options: ImageExportOptions) -> Tuple[Image.Image, List[Tuple[int, int, int, int]]]:
    image = Image.open(options.input_path)
    image = image.convert("RGBA")
    if options.resize is not None:
        image = image.resize(options.resize, _resample_filter(options.resample))
    if options.flip_y:
        image = image.transpose(Image.FLIP_TOP_BOTTOM)
    pixels = list(image.getdata())
    if options.premultiply_alpha and options.color_type in ("RGB32", "RGB64"):
        pixels = [_premultiply_pixel(pixel) for pixel in pixels]
    return image, pixels


def _rgb565_word(pixel: Tuple[int, int, int, int]) -> int:
    r, g, b, _ = pixel
    return (int((r + 4) * 31.0 / 255.0) << 11) | (int((g + 2) * 63.0 / 255.0) << 5) | int((b + 4) * 31.0 / 255.0)


def _color_literal(pixel: Tuple[int, int, int, int], color_type: str) -> str:
    r, g, b, a = pixel
    if color_type == "RGB565":
        return "C(0x{:04x})".format(_rgb565_word(pixel))
    if color_type == "RGB24":
        return "C({},{},{})".format(r, g, b)
    if color_type == "RGB32":
        return "C({},{},{},{})".format(r, g, b, a)
    if color_type == "RGB64":
        return "C({},{},{},{})".format(r * 257, g * 257, b * 257, a * 257)
    if color_type == "RGBf":
        return "C({},{},{})".format(
            _float_literal(r / 255.0),
            _float_literal(g / 255.0),
            _float_literal(b / 255.0),
        )
    raise ValueError("Unsupported color type '{}'".format(color_type))


def _float_literal(value: float) -> str:
    text = "{:.8g}".format(value)
    if "." not in text and "e" not in text and "E" not in text:
        text += ".0"
    return text + "f"


def _color_macro(color_type: str) -> str:
    if color_type == "RGB565":
        return "#define C(X) tgx::RGB565((uint16_t)(X))\n\n"
    if color_type == "RGB24":
        return "#define C(R,G,B) tgx::RGB24((R),(G),(B))\n\n"
    if color_type == "RGB32":
        return "#define C(R,G,B,A) tgx::RGB32((R),(G),(B),(A))\n\n"
    if color_type == "RGB64":
        return "#define C(R,G,B,A) tgx::RGB64((R),(G),(B),(A))\n\n"
    if color_type == "RGBf":
        return "#define C(R,G,B) tgx::RGBf((R),(G),(B))\n\n"
    raise ValueError("Unsupported color type '{}'".format(color_type))


def _write_array(f, pixels: Iterable[Tuple[int, int, int, int]], color_type: str, items_per_line: int = 8) -> None:
    pixels = list(pixels)
    total = len(pixels)
    col = 0
    for i, pixel in enumerate(pixels):
        if col == 0:
            f.write("    ")
        f.write(_color_literal(pixel, color_type))
        col += 1
        if i != total - 1:
            f.write(",")
        if col >= items_per_line:
            f.write("\n")
            col = 0
        else:
            f.write(" ")
    if col:
        f.write("\n")


def _header_comment(result_base: str, options: ImageExportOptions, width: int, height: int, data_bytes: int) -> str:
    kib = data_bytes / 1024.0
    rel = options.input_path.name
    lines = [
        "//",
        "// TGX image generated from: {}".format(rel),
        "// Object: {}".format(result_base),
        "// Format: tgx::Image<tgx::{}>".format(options.color_type),
        "// Dimensions: {} x {}".format(width, height),
        "// Static image data: {} bytes ({:.1f} KiB)".format(data_bytes, kib),
    ]
    if options.resize is not None:
        lines.append("// Resize: {} x {} ({})".format(options.resize[0], options.resize[1], options.resample))
    if options.flip_y:
        lines.append("// Rows are vertically flipped for texture-coordinate workflows.")
    if options.premultiply_alpha and options.color_type in ("RGB32", "RGB64"):
        lines.append("// Alpha format: RGB components are pre-multiplied by A, as expected by TGX blending.")
    lines.append("//")
    return "\n".join(lines) + "\n\n"


def export_image(options: ImageExportOptions) -> ImageExportResult:
    options.input_path = Path(options.input_path)
    options.output_dir = Path(options.output_dir)
    options.color_type = normalize_color_type(options.color_type)
    options.layout = options.layout.lower()
    options.resample = options.resample.lower()

    if options.layout not in SUPPORTED_LAYOUTS:
        raise ValueError("Unsupported layout '{}'. Choose header or split".format(options.layout))
    if not options.input_path.exists():
        raise FileNotFoundError(str(options.input_path))

    object_name = sanitize_identifier(options.object_name or options.input_path.stem)
    output_base = sanitize_identifier(options.output_base or object_name)
    image, pixels = _load_pixels(options)
    width, height = image.size
    data_bytes = width * height * color_type_size(options.color_type)
    options.output_dir.mkdir(parents=True, exist_ok=True)

    header_path = options.output_dir / "{}.h".format(output_base)
    cpp_path = None if options.layout == "header" else options.output_dir / "{}.cpp".format(output_base)
    data_symbol = "{}_data".format(object_name)
    progmem = " PROGMEM" if options.progmem else ""
    comment = _header_comment(object_name, options, width, height, data_bytes)

    if options.layout == "header":
        with header_path.open("w", encoding="utf-8", newline="\n") as f:
            f.write(comment)
            f.write("#pragma once\n\n")
            f.write("#include <tgx.h>\n\n")
            f.write(_color_macro(options.color_type))
            f.write("static const tgx::{} {}[{} * {}]{} = {{\n".format(options.color_type, data_symbol, width, height, progmem))
            _write_array(f, pixels, options.color_type)
            f.write("};\n\n")
            f.write("#undef C\n\n")
            f.write("static const tgx::Image<tgx::{}> {}({}, {}, {});\n".format(options.color_type, object_name, data_symbol, width, height))
    else:
        with header_path.open("w", encoding="utf-8", newline="\n") as f:
            f.write(comment)
            f.write("#pragma once\n\n")
            f.write("#include <tgx.h>\n\n")
            f.write("extern const tgx::Image<tgx::{}> {};\n".format(options.color_type, object_name))
        with cpp_path.open("w", encoding="utf-8", newline="\n") as f:
            f.write(comment)
            f.write('#include "{}.h"\n\n'.format(output_base))
            f.write(_color_macro(options.color_type))
            f.write("static const tgx::{} {}[{} * {}]{} = {{\n".format(options.color_type, data_symbol, width, height, progmem))
            _write_array(f, pixels, options.color_type)
            f.write("};\n\n")
            f.write("#undef C\n\n")
            f.write("const tgx::Image<tgx::{}> {}({}, {}, {});\n".format(options.color_type, object_name, data_symbol, width, height))

    return ImageExportResult(
        object_name=object_name,
        color_type=options.color_type,
        width=width,
        height=height,
        header_path=header_path,
        cpp_path=cpp_path,
        data_bytes=data_bytes,
    )
