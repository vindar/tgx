# TGX_RASTERIZE_TRIANGLE_INLINE investigation - 2026-06

## Goal

Decide whether the main triangle rasterizer entry point should be forced inline
per board.

The tested function is:

```cpp
TGX_RASTERIZE_TRIANGLE_INLINE inline void rasterizeTriangle(...)
```

So `TGX_RASTERIZE_TRIANGLE_INLINE=` does not make the function non-inline. It
only removes the forced `always_inline` attribute and leaves a normal C++
`inline` function for the compiler to decide.

## Variants

| Variant | Meaning |
|---|---|
| `default` | Current board default before this investigation, generally `TGX_INLINE` |
| `raster_empty` | `-DTGX_RASTERIZE_TRIANGLE_INLINE=` |
| `raster_noinline` | `-DTGX_RASTERIZE_TRIANGLE_INLINE=__attribute__((noinline))` |
| `raster_noinline_noclone` | `-DTGX_RASTERIZE_TRIANGLE_INLINE=__attribute__((noinline,noclone))` |

## Test Coverage

Boards:

- Teensy 4.1 + ILI9341
- Core2 / ESP32
- CoreS3 / ESP32-S3
- Pico2 / RP2350 + ILI9341

Tests:

- `examples/benchmark`
- Standard real examples:
  - Teensy4: `mars`, `test-shading`, `test-texture`, `buddha`
  - Core2/CoreS3: `borg_cube`, `donkeykong`, `scream`
  - Pico2: `borg_cube`, `bunny_fig`, `scream`
- New light examples:
  - `pointlight_textured_meshes`
  - `spotlight_checkerboard`
  - `spotlights_room`

Problematic/outlier examples were repeated:

- Pico2 `spotlight_checkerboard`
- Pico2 `spotlights_room`
- Core2 `pointlight_textured_meshes`
- Core2 `spotlight_checkerboard`

## Main Benchmark Results

Global benchmark score delta versus forced-inline default:

| Board | `raster_empty` | `raster_noinline` | `raster_noinline_noclone` |
|---|---:|---:|---:|
| Core2 | +1.67% | +1.67% | +1.67% |
| CoreS3 | +1.28% | +1.28% | +1.28% |
| Pico2 | +1.74% | +1.74% | +1.74% |
| Teensy4 | default benchmark does not fit RAM1 | default benchmark does not fit RAM1 | default benchmark does not fit RAM1 |

Teensy4 full benchmark with forced inline overflows RAM1:

| Teensy4 benchmark variant | FLASH code | RAM1 code | RAM1 free |
|---|---:|---:|---:|
| forced inline default | 276128 | 262496 | -6720 |
| `raster_empty` | 261856 | 248224 | 26048 |
| `raster_noinline` | 261856 | 248224 | 26048 |
| `raster_noinline_noclone` | 261792 | 248160 | 26048 |

Teensy4 runtime score with the variants that fit:

| Variant | Mean global benchmark score |
|---|---:|
| `raster_empty` | 97.42 |
| `raster_noinline` | 97.43 |
| `raster_noinline_noclone` | 97.89 |

## Real Examples

Summary of real example FPS deltas:

| Board | `raster_empty` mean | Notes |
|---|---:|---|
| Teensy4 | +0.41% | No meaningful regression; large RAM1/code saving |
| Core2 | +8.33% | Dominated by repeated stable gain on `pointlight_textured_meshes`; `spotlight_checkerboard` is 25 -> 24 fps |
| CoreS3 | +1.80% | Cleanest result; all tested examples improve or stay neutral |
| Pico2 | -10.72% | Bad for spotlight/sphere-heavy real examples despite benchmark improvement |

Pico2 repeat confirmed the regression:

| Pico2 example | forced inline | `raster_empty` | Delta |
|---|---:|---:|---:|
| `spotlight_checkerboard` | 31.0 fps | 23.04 fps | -25.7% |
| `spotlights_room` | 28.02 fps | 16.46 fps | -41.2% |

The benchmark subtests explain part of the Pico2 behavior. The worst
`raster_empty` regressions include sphere direct drawing:

| Pico2 benchmark subtest | Delta |
|---|---:|
| `Sphere direct TEX` 100x100 | -22.8% |
| `Sphere direct TEX` 200x200 | -9.7% |
| `Sphere direct GOUR` 100x100 | -9.3% |
| `Sphere direct TEX` 320x240 | -7.4% |
| `Sphere direct GOUR` 200x200 | -6.2% |

This matters because the new light examples use sphere-like primitives and many
small/medium triangles where RP2350 benefits from keeping the rasterizer body
forced inline.

Core2 repeat confirmed the large pointlight gain:

| Core2 example | forced inline | `raster_empty` | Delta |
|---|---:|---:|---:|
| `pointlight_textured_meshes` | 9.34 fps | 14.15 fps | +51.6% |
| `spotlight_checkerboard` | 25.0 fps | 24.0 fps | -4.0% |

The `spotlight_checkerboard` result is integer-FPS limited and small in absolute
terms. The global benchmark, code size, and other examples still favor removing
forced inline on Core2.

## Codegen Observation

With forced inline, `rasterizeTriangle()` disappears as a separate symbol in
`nm` output. With `raster_empty`, one emitted specialization appears.

For Core2, CoreS3, and Pico2, `raster_empty`, `raster_noinline`, and
`raster_noinline_noclone` produce the same symbol sizes for the inspected
benchmark/pointlight/checkerboard builds. This means the compiler already makes
the same effective decision once `always_inline` is removed.

For Teensy4, `noinline_noclone` emits a smaller symbol than `raster_empty`, but
the runtime gain is small and the macro is less clean as a public default because
the declaration still contains the C++ `inline` keyword. The accepted default is
therefore the simpler empty macro.

## Decision

Use a board-specific config macro:

```cpp
TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
```

Recommended defaults:

| Target | Setting | Reason |
|---|---|---|
| Teensy4 | empty | Saves about 14 KB RAM1 code on benchmark; examples neutral/slightly positive |
| ESP32 / Core2 | empty | Benchmark and code size improve; real examples mostly positive |
| ESP32-S3 / CoreS3 | empty | Clean benchmark and example improvement |
| RP2350 / Pico2 | not defined, fallback to `TGX_INLINE` | Keeps forced inline; avoids large regressions in spotlight/sphere-heavy examples |
| Other targets | not defined, fallback to `TGX_INLINE` | Not measured in this investigation |

## Post-Config Validation

After applying the board-specific defaults, `examples/benchmark` was uploaded and
captured on all four boards with no extra flags.

| Board | Post-config benchmark scores | Mean |
|---|---|---:|
| Teensy4 | 119.78 / 91.17 / 81.28 | 97.41 |
| Core2 | 33.34 / 24.49 | 28.915 |
| CoreS3 | 43.80 / 32.25 / 28.66 | 34.903 |
| Pico2 | 22.71 / 17.76 / 15.80 | 18.757 |

Post-config Pico2 critical examples stayed on the forced-inline performance path:

| Pico2 example | FPS |
|---|---:|
| `spotlight_checkerboard` | 31.0 |
| `spotlights_room` | 28.0 |

## Artifacts

Reusable scripts:

- `run_rasterize_inline_matrix.ps1`
- `aggregate_rasterize_inline_results.py`

Compact summaries:

- `hardware/benchmark_global_summary.csv`
- `hardware/benchmark_subtest_summary.csv`
- `hardware/example_variant_summary.csv`
- `hardware/compile_size_summary.csv`
- `codegen/rasterize_symbol_summary.csv`
