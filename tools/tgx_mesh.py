#!/usr/bin/env python
"""Graphical entry point for the TGX mesh converter."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

from _internal.modules.setup.checks import ensure_tool_environment

ensure_tool_environment("mesh", gui=True)

from _internal.modules.mesh.gui import main


if __name__ == "__main__":
    raise SystemExit(main())
