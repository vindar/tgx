from __future__ import annotations

import argparse
from pathlib import Path

from .core import InfoError, inspect_path


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Inspect TGX generated mesh, image and font files.")
    parser.add_argument("input", help="Generated TGX .h/.hpp/.cpp file")
    parser.add_argument("--preview", help="Write a PNG preview for images and fonts")
    parser.add_argument("--view", action="store_true", help="Open an interactive PyVista viewer for meshes")
    args = parser.parse_args(argv)

    try:
        result = inspect_path(args.input)
        print(result.report)
        if args.preview:
            if result.preview.image is None:
                raise InfoError("--preview is only available for image and font files")
            out = Path(args.preview)
            out.parent.mkdir(parents=True, exist_ok=True)
            result.preview.image.save(out)
            print(f"\nPreview written: {out}")
        if args.view:
            if result.preview.view_callback is None:
                raise InfoError("--view is only available for mesh files")
            result.preview.view_callback()
    except InfoError as exc:
        print(f"tgx_info: {exc}")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
