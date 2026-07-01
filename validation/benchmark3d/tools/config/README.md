# Runner Configuration

This directory contains data files consumed by `../run_bench3d.py`.

- `modules.json` maps a module id to its compile define, Arduino sketch and CPU
  executable name.
- `boards/` contains one JSON profile per target board.

Prefer extending these JSON files over hard-coding board or module decisions in
the Python runner.
