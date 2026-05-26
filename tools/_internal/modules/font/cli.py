#!/usr/bin/env python
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .core import FontExportOptions, SUPPORTED_FONT_FORMATS, SUPPORTED_LAYOUTS, export_font, parse_sizes


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert TrueType/OpenType fonts to TGX-compatible font headers.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("input", help="Input font file (.ttf, .otf, .ttc if supported by Pillow)")
    parser.add_argument("-o", "--output", required=True, help="Output .h path")
    parser.add_argument(
        "--name",
        help="C++ font symbol base. The pixel size is always appended; ili9341-v23 also appends AA depth, for example _AA4_16",
    )
    parser.add_argument("--format", choices=SUPPORTED_FONT_FORMATS, default="ili9341-v23", help="Output font format")
    parser.add_argument("--sizes", default="12,16,24", help="Comma-separated pixel sizes, for example 10,12,16")
    parser.add_argument(
        "--chars",
        default="ascii",
        help="Character preset or ranges: none, all, ascii, printable, latin1, letters-digits, letters, digits, 32-126, 0xA0-0xFF",
    )
    parser.add_argument("--aa", default="4", help="Antialiasing levels: none, 2, 4, 8. Forced to none for ili9341-v1 and Adafruit GFX")
    parser.add_argument("--layout", choices=SUPPORTED_LAYOUTS, default="header", help="'header' creates one .h; 'split' creates .h + .cpp")
    parser.add_argument("--no-progmem", action="store_true", help="Do not add PROGMEM to generated arrays/objects")
    parser.add_argument("--preview-png", help="Also render a PNG preview of the exported character grid")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        result = export_font(
            FontExportOptions(
                input_path=Path(args.input),
                output=Path(args.output),
                symbol_base=args.name,
                sizes=parse_sizes(args.sizes),
                output_format=args.format,
                antialias=args.aa,
                chars=args.chars,
                layout=args.layout,
                progmem=not args.no_progmem,
                preview_png=Path(args.preview_png) if args.preview_png else None,
            )
        )
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print("Generated TGX font:")
    print(f"  symbols : {', '.join(result.symbols)}")
    print(f"  glyphs  : {result.glyph_count}")
    print(f"  memory  : {result.data_bytes} bytes ({result.data_bytes / 1024.0:.1f} KiB)")
    print(f"  header  : {result.header_path}")
    if result.cpp_path:
        print(f"  source  : {result.cpp_path}")
    if result.preview_path:
        print(f"  preview : {result.preview_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
