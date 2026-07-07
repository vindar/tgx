# TGX Validation Suites

This directory contains developer-oriented validation suites for TGX rendering
code. The active 3D benchmark suite lives here rather than under `examples/`,
because it has CPU, Arduino and ESP-IDF backends plus shared tooling.

- `2d/`: CPU and Teensy 4.1 validation for the 2D API, colors, fonts, blits,
  primitives and optional decoder/font wrappers.
- `3d/`: CPU validation for the 3D renderer, including legacy `Mesh3D`,
  `Mesh3Dv2`, texturing, lighting, clipping, z-buffer, direct primitives,
  wireframe and point paths.
- `performance/`: reusable scripts and documentation for timing, code-size and
  board-specific performance work. Generated performance results, release
  baselines and development baselines are kept locally under
  `local_results/performance/`, which is ignored by Git.
- `benchmark3d/`: modular 3D benchmark suite under active development. It
  uses board-independent test modules, CPU/MCU backends and parseable telemetry.
- `local_results/`: ignored local workspace for generated validation and
  performance outputs that are useful locally but too large or too specific for
  the public repository.

Run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File validation\2d\run_cpu.ps1
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Compare strict
```

The 2D Teensy 4.1 suite can be built and run with:

```powershell
powershell -ExecutionPolicy Bypass -File validation\2d\build_teensy.ps1
powershell -ExecutionPolicy Bypass -File validation\2d\upload_teensy.ps1
powershell -ExecutionPolicy Bypass -File validation\2d\read_teensy_serial.ps1
```

Generated runtime files go under the repository-level `tmp/`. Small 2D/3D CSV
hash baselines remain checked in under `validation/2d/baselines/` and
`validation/3d/baselines/`. Large image golden files and performance telemetry
must stay under ignored local directories, primarily
`validation/local_results/golden/` and
`validation/local_results/performance/`.
