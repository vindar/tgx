from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image


@dataclass
class TextureExportResult:
    symbol: str
    header: Path
    width: int
    height: int


def identifier(name: str) -> str:
    out = re.sub(r"\W+", "_", name.strip())
    if not out:
        out = "texture"
    if out[0].isdigit():
        out = "_" + out
    return out


def _rgb565(rgb: np.ndarray) -> int:
    r = int(rgb[0])
    g = int(rgb[1])
    b = int(rgb[2])
    return (int((r + 4) * 31.0 / 255.0) << 11) | (int((g + 2) * 63.0 / 255.0) << 5) | int((b + 4) * 31.0 / 255.0)


def _is_pow2(v: int) -> bool:
    return v > 0 and (v & (v - 1)) == 0


def export_texture_rgb565_header(
    image_path: str | Path,
    output_dir: str | Path,
    *,
    symbol: str | None = None,
    resize: tuple[int, int] | None = None,
    require_pow2: bool = False,
) -> TextureExportResult:
    image_path = Path(image_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    symbol = identifier(symbol or image_path.stem + "_texture")
    image = Image.open(image_path).convert("RGB")
    if resize is not None:
        image = image.resize(resize, Image.Resampling.BICUBIC)
    if require_pow2 and (not _is_pow2(image.width) or not _is_pow2(image.height)):
        raise ValueError(f"{image_path} is {image.width}x{image.height}; texture wrap mode expects power-of-two dimensions")

    # TGX texture coordinates follow the historical converter convention: rows are written from
    # bottom to top so OBJ UVs map like the legacy Mesh3D texture headers.
    pixels = np.asarray(image, dtype=np.uint8)
    header = output_dir / f"{symbol}.h"
    with header.open("w", encoding="utf-8", newline="\n") as f:
        f.write("#pragma once\n\n")
        f.write("#include <tgx.h>\n\n")
        f.write(f"// Texture generated from {image_path.name}.\n")
        f.write(f"const uint16_t {symbol}_data[{image.width}*{image.height}] PROGMEM = {{\n")
        col = 0
        for y in range(image.height):
            src_y = image.height - 1 - y
            for x in range(image.width):
                f.write(f"0x{_rgb565(pixels[src_y, x]):04x}")
                if x != image.width - 1 or y != image.height - 1:
                    f.write(", ")
                col += 1
                if col == 16:
                    f.write("\n")
                    col = 0
        if col:
            f.write("\n")
        f.write("};\n\n")
        f.write(f"const tgx::Image<tgx::RGB565> {symbol}((void*){symbol}_data, {image.width}, {image.height});\n")
    return TextureExportResult(symbol, header, image.width, image.height)
