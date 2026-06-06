# Raster Span Kernel Investigation

Date: 2026-06-06

This investigation builds a display-free microbenchmark for isolated TGX
rasterizer/shader span kernels.

The target boards were:

- Teensy 4.1 / Cortex-M7 on `COM3`
- Pico2 / RP2350 / Cortex-M33 on `COM21`

Pico W / RP2040 was intentionally ignored because it was unavailable. Core2 and
CoreS3 were not runtime targets for this ARM span-kernel pass.

## Purpose

Previous optimization passes found that broad Renderer3D/layout changes were
fragile, and isolated RGB565 color kernels were already hard to improve exactly.
This pass moves one level up: realistic per-pixel span kernels that model the
inner work of `src/Shaders.h::uber_shader`.

## Contents

- `SESSION_START.md`: starting state, commit, boards, and planned kernels.
- `sketches/RasterSpanKernelBench/`: display-free Arduino benchmark sketch.
- `tools/parse_raster_span_bench.py`: parser for structured serial output.
- `results/`: parsed benchmark tables, hardware probe, source path inventory,
  and raw captures.
- `notes/span_path_map.md`: mapping between benchmark kernels and TGX shader
  paths.
- `codegen/`: `size`, `nm`, `objdump`, selected symbol tables, and codegen
  summary.
- `REPORT_RASTER_SPAN_KERNELS.md`: final report and integration decision.

## Final Decision

No production TGX source patch is retained from this pass.

The best exact microbenchmark signal is the fixed-point incremental texture
coordinate path:

- nearest texture span: strong win on both Pico2 and Teensy;
- bilinear texture span: clear win on both Pico2 and Teensy.

That result is not integrated yet because the standalone benchmark uses a
controlled fixed 8.8 coordinate model. Production `uber_shader` has additional
semantics around perspective correction, wrapping/clamping, fractional weights,
and float-derived coordinates. A source patch should wait for a dedicated
texture-coordinate semantics pass.

## How To Reuse

Use the existing robust upload/capture tool from:

```text
validation/performance/tools/upload_and_capture.ps1
```

The benchmark sketch prints:

```text
TGX_RASTER_SPAN_BENCH_BEGIN
...
TGX_RASTER_SPAN_BENCH_END
```

Parse captured telemetry with:

```text
python validation/performance/investigations/2026-06-raster-span-kernels/tools/parse_raster_span_bench.py <telemetry files> --out-dir validation/performance/investigations/2026-06-raster-span-kernels/results
```
