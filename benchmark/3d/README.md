# TGX 3D CPU Validation Suite

This directory contains the CPU-side 3D validation suite. It is separate from
`examples/benchmark`, which is focused on timing and frame-rate measurements.

The CPU suite renders fixed 800x600 scenes with both:

- `RGB565`
- `RGB32`

It writes per-scene BMP images, overview BMP sheets, and a CSV with rendering
signatures. The signatures are meant to catch broken projection, clipping,
z-buffering, texturing, lighting, direct primitives, point drawing and wireframe
paths without requiring every single pixel to be forever identical.

Build and run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\3d\run_cpu.ps1
```

Comparison modes:

- `strict`: exact image hash and exact coarse block hash.
- `tolerant`: default. Allows tiny global numeric changes while checking object
  coverage, bounding box and color sums. It also reports if the coarse block
  hash changes, without failing only for that.
- `smoke`: checks only that each scene produces plausible non-empty output.

Examples:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\3d\run_cpu.ps1 -Compare strict
powershell -ExecutionPolicy Bypass -File benchmark\3d\run_cpu.ps1 -Compare smoke
```

Use this only after reviewing an intentional rendering change:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\3d\run_cpu.ps1 -UpdateBaseline -UpdateGolden
```

The suite can also run a CImg desktop animation:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\3d\run_cpu.ps1 -Demo
```

Use `-DemoOnly` when you only want to preview the animation without running the
validation scenes afterwards.

Generated runtime files are written under `tmp\tgx_3d_cpu_suite` by default.
Golden images are stored under `benchmark\3d\golden\cpu`.
