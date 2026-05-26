#!/usr/bin/env python
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .core import ImageExportOptions, SUPPORTED_COLOR_TYPES, SUPPORTED_RESAMPLING, export_image, parse_resize


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert PNG/JPEG/BMP images to TGX tgx::Image headers.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("input", help="Input image file (.png, .jpg, .bmp, ...)")
    parser.add_argument("-o", "--output-dir", default=".", help="Directory for generated files")
    parser.add_argument("--output", help="Full output path. For split output, the .h/.cpp pair uses this path stem")
    parser.add_argument("--format", choices=SUPPORTED_COLOR_TYPES, default="RGB565", help="TGX output color type")
    parser.add_argument("--name", help="C++ object name. Defaults to the input file stem")
    parser.add_argument("--layout", choices=("header", "split"), default="header", help="'header' creates one .h; 'split' creates .h + .cpp")
    parser.add_argument("--resize", type=parse_resize, help="Resize to WIDTHxHEIGHT before exporting")
    parser.add_argument("--resample", choices=SUPPORTED_RESAMPLING, default="lanczos", help="Resize filter")
    parser.add_argument("--flip-y", action="store_true", help="Write rows bottom-to-top, useful for OBJ texture workflows")
    parser.add_argument("--no-progmem", action="store_true", help="Do not add PROGMEM to the generated pixel array")
    parser.add_argument("--straight-alpha", action="store_true", help="Keep RGB32/RGB64 alpha as stored in the source instead of TGX pre-multiplied alpha")
    return parser


def _apply_output_path(args) -> None:
    args.output_base = None
    if not args.output:
        return
    output_path = Path(args.output)
    args.output_dir = str(output_path.parent if str(output_path.parent) else Path("."))
    if output_path.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
        args.output_base = output_path.stem
    else:
        args.output_base = output_path.name


def main(argv=None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    _apply_output_path(args)
    try:
        result = export_image(
            ImageExportOptions(
                input_path=Path(args.input),
                output_dir=Path(args.output_dir),
                color_type=args.format,
                object_name=args.name,
                output_base=args.output_base,
                layout=args.layout,
                resize=args.resize,
                resample=args.resample,
                flip_y=args.flip_y,
                progmem=not args.no_progmem,
                premultiply_alpha=not args.straight_alpha,
            )
        )
    except Exception as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 1

    print("Generated TGX image:")
    print("  object : {}".format(result.object_name))
    print("  format : tgx::{}".format(result.color_type))
    print("  size   : {}x{}".format(result.width, result.height))
    print("  memory : {} bytes ({:.1f} KiB)".format(result.data_bytes, result.data_bytes / 1024.0))
    print("  header : {}".format(result.header_path))
    if result.cpp_path:
        print("  source : {}".format(result.cpp_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
