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

The validation meshes are stored locally under `validation/3d/models` so the
suite does not depend on example sketches. It currently covers both TGX mesh
formats:

- legacy `Mesh3D`: `bunny_fig_small`, `buddha` and `R2D2`;
- compact `Mesh3Dv2`: `validation_bunny_v2`, `validation_buddha_v2` and
  `validation_R2D2_v2`, plus the multi-material textured
  `validation_falcon_v2`.

The fixed scene set contains 23 scenes per color type. It includes legacy and
Mesh3Dv2 real-mesh rendering, flat and Gouraud shading, nearest and bilinear
texturing, orthographic projection, near/far clipping, z-buffer overlap, direct
triangle/cube/sphere helpers, small-triangle grids, point/dot drawing and
wireframe paths.

Build and run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1
```

Comparison modes:

- `strict`: exact image hash and exact coarse block hash.
- `tolerant`: default. Allows tiny global numeric changes while checking object
  coverage, bounding box and color sums. It also reports if the coarse block
  hash changes, without failing only for that.
- `smoke`: checks only that each scene produces plausible non-empty output.

Examples:

```powershell
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Compare strict
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Compare smoke
```

Use this only after reviewing an intentional rendering change:

```powershell
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -UpdateBaseline -UpdateGolden
```

The suite can also run a CImg desktop animation:

```powershell
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Demo
```

Use `-DemoOnly` when you only want to preview the animation without running the
validation scenes afterwards.

Generated runtime files are written under `tmp\tgx_3d_cpu_suite` by default.
Golden images are stored under `validation\3d\golden\cpu`.
