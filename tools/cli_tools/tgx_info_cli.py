#!/usr/bin/env python
"""Command-line entry point for the TGX asset inspector."""

from pathlib import Path
import sys

_tools_root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(_tools_root))
sys.path.insert(0, str(_tools_root.parent))

from _internal.modules.setup.checks import ensure_tool_environment

ensure_tool_environment("info", gui=False)

from _internal.modules.info.cli import main


if __name__ == "__main__":
    raise SystemExit(main())
