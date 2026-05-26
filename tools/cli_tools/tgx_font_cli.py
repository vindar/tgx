#!/usr/bin/env python
"""Command-line entry point for the TGX font converter."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from _internal.modules.setup.checks import ensure_tool_environment

ensure_tool_environment("font", gui=False)

from _internal.modules.font.cli import main


if __name__ == "__main__":
    raise SystemExit(main())

