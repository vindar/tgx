# TGX Validation Suites

This directory contains developer-oriented validation suites for TGX rendering
code. They are separate from `examples/benchmark`, which is focused on portable
MCU/CPU performance comparisons.

- `2d/`: CPU and Teensy 4.1 validation for the 2D API, colors, fonts, blits,
  primitives and optional decoder/font wrappers.
- `3d/`: CPU validation for the 3D renderer, including legacy `Mesh3D`,
  `Mesh3Dv2`, texturing, lighting, clipping, z-buffer, direct primitives,
  wireframe and point paths.

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

Generated runtime files go under `tmp/`. Baselines and golden images are stored
inside each suite directory and should only be updated after reviewing an
intentional rendering change.
