# TGX inline macro investigation - 2026-06

## Goal

Measure the usefulness of the new narrow inline control macros:

- `TGX_UBER_SHADER_INLINE`
- `TGX_SHADER_SELECT_INLINE`
- `TGX_RASTERIZE_TRIANGLE_INLINE`
- `TGX_RENDERER3D_SHADING_INLINE`

The first target was Teensy 4.1 + ILI9341, then Core2, CoreS3, and Pico2.

## Tested variants

The scripts build the same sketches with additional compiler flags:

| Variant | Extra flags |
|---|---|
| `default` | none |
| `uber_empty` | `-DTGX_UBER_SHADER_INLINE=` |
| `uber_noinline` | `-DTGX_UBER_SHADER_INLINE=__attribute__((noinline))` |
| `select_empty` | `-DTGX_SHADER_SELECT_INLINE=` |
| `raster_empty` | `-DTGX_RASTERIZE_TRIANGLE_INLINE=` |
| `shading_empty` | `-DTGX_RENDERER3D_SHADING_INLINE=` |
| `all_empty` | all four macros empty |

## Commands

Compile-only Teensy benchmark matrix:

```powershell
powershell -ExecutionPolicy Bypass -File validation\performance\investigations\2026-06-inline-macros\run_inline_macro_matrix.ps1 -Boards teensy41 -Benchmark -CompileOnly -BenchmarkTimeoutSeconds 60 -RetryCount 0
```

Teensy benchmark for variants that fit in RAM:

```powershell
powershell -ExecutionPolicy Bypass -Command "& { .\validation\performance\investigations\2026-06-inline-macros\run_inline_macro_matrix.ps1 -Boards @('teensy41') -Variants @('select_empty','raster_empty','shading_empty','all_empty') -Benchmark -BenchmarkTimeoutSeconds 210 -RetryCount 1 }"
```

Teensy examples:

```powershell
powershell -ExecutionPolicy Bypass -Command "& { .\validation\performance\investigations\2026-06-inline-macros\run_inline_macro_matrix.ps1 -Boards @('teensy41') -Variants @('default','uber_empty','uber_noinline','select_empty','raster_empty','shading_empty','all_empty') -Examples -ExampleTimeoutSeconds 24 -RetryCount 1 }"
```

Core2/CoreS3/Pico2 benchmarks and examples:

```powershell
powershell -ExecutionPolicy Bypass -Command "& { .\validation\performance\investigations\2026-06-inline-macros\run_inline_macro_matrix.ps1 -Boards @('core2','cores3','pico2') -Benchmark -BenchmarkTimeoutSeconds 210 -RetryCount 1 }"
powershell -ExecutionPolicy Bypass -Command "& { .\validation\performance\investigations\2026-06-inline-macros\run_inline_macro_matrix.ps1 -Boards @('core2','cores3','pico2') -Examples -ExampleTimeoutSeconds 20 -RetryCount 1 }"
```

Note: the first Core2 benchmark attempt used the wrong baud rate. The script was fixed to capture `examples/benchmark` at 9600 bauds, matching `Serial.begin(9600)`, and Core2 was rerun successfully.

## Main benchmark results

Global benchmark averages relative to `default`, when `default` is available:

| Board | Variant | Delta |
|---|---|---:|
| Core2 | `uber_empty` | -0.55% |
| Core2 | `uber_noinline` | -0.55% |
| Core2 | `select_empty` | -0.65% |
| Core2 | `raster_empty` | +1.67% |
| Core2 | `shading_empty` | -0.88% |
| Core2 | `all_empty` | +1.19% |
| CoreS3 | `uber_empty` | -0.06% |
| CoreS3 | `uber_noinline` | -0.06% |
| CoreS3 | `select_empty` | -1.82% |
| CoreS3 | `raster_empty` | +1.28% |
| CoreS3 | `shading_empty` | -2.01% |
| CoreS3 | `all_empty` | +0.33% |
| Pico2 | `uber_empty` | -0.25% |
| Pico2 | `uber_noinline` | -0.09% |
| Pico2 | `select_empty` | -6.81% |
| Pico2 | `raster_empty` | +1.74% |
| Pico2 | `shading_empty` | -1.15% |
| Pico2 | `all_empty` | -13.06% |

Teensy full benchmark with current source does not fit in RAM1 for `default`, `uber_empty`, or `uber_noinline`, so there is no valid default runtime comparison on this full benchmark. The variants that reduce code size enough to fit gave these averages:

| Teensy variant | Mean global benchmark score |
|---|---:|
| `select_empty` | 96.14 |
| `raster_empty` | 97.41 |
| `shading_empty` | 96.56 |
| `all_empty` | 97.01 |

## Teensy size results

For `examples/benchmark`, the RAM1 code pressure is the important number:

| Variant | Flash code | RAM1 code | RAM1 free |
|---|---:|---:|---:|
| `default` | 276128 | 262496 | -6720 |
| `uber_empty` | 293856 | 280224 | -6720 |
| `uber_noinline` | 276192 | 262560 | -6720 |
| `select_empty` | 270880 | 257248 | 26048 |
| `raster_empty` | 261856 | 248224 | 26048 |
| `shading_empty` | 275424 | 261792 | 26048 |
| `all_empty` | 261344 | 247712 | 26048 |

`uber_empty` is clearly harmful for Teensy code size. It increases the full benchmark code by about 17.7 KB compared with `default`.

## Codegen observation for `TGX_UBER_SHADER_INLINE`

The active shader path is:

```text
shader_select() -> uber_shader<...>() -> uber_shader_inline<...>()
```

`uber_shader_inline()` contains the real pixel shader body. `uber_shader()` is a wrapper.

The `nm` symbol dumps show that the default `TGX_UBER_SHADER_INLINE` changes emitted code structure:

- With default settings, `uber_shader_inline()` usually disappears as an emitted standalone symbol; its body is folded into `uber_shader<...>()`.
- With `TGX_UBER_SHADER_INLINE=` or `noinline`, separate `uber_shader_inline<...>()` symbols appear.
- On Teensy and Pico2, `uber_empty` can increase code size substantially, even though the call cost is only one call per triangle.
- On ESP32/ESP32-S3, the runtime effect is nearly neutral.

Conclusion: `TGX_UBER_SHADER_INLINE` is useful, but mainly as a codegen/size guard rather than as a major runtime speed knob. It should stay `TGX_INLINE` by default.

## Per-macro conclusions

### `TGX_UBER_SHADER_INLINE`

Keep default as `TGX_INLINE`.

Removing it is not safe globally:

- Teensy benchmark size gets much worse.
- Pico2 code size also grows on several examples.
- ESP32/Core2 and CoreS3 are almost neutral, but there is no clear benefit.

`uber_noinline` is surprisingly close to default in runtime, which confirms that this boundary is not a per-pixel cost. Still, it does not improve the situation and should not be the default.

### `TGX_SHADER_SELECT_INLINE`

Keep default as `TGX_INLINE`.

Making it empty is bad on Pico2 and not convincing elsewhere:

- Pico2 global benchmark: about -6.8%.
- CoreS3 global benchmark: about -1.8%.
- Core2: slightly negative globally.

This macro is useful for diagnostics, but it should not be disabled by default.

### `TGX_RASTERIZE_TRIANGLE_INLINE`

This is the most interesting candidate for future tuning.

Making `rasterizeTriangle()` non-forced-inline:

- Reduces code size.
- Improves global benchmark on Core2, CoreS3, and Pico2 by about +1.3% to +1.7%.
- Makes the Teensy full benchmark fit in RAM1 again.

However, real examples show some noisy or scene-dependent behavior, especially on Pico2. This deserves a dedicated follow-up before changing board defaults.

### `TGX_RENDERER3D_SHADING_INLINE`

Keep default as `TGX_INLINE` for now.

Making it empty helps Teensy code size a little, but the runtime result is mixed:

- CoreS3 global benchmark: about -2.0%.
- Pico2 global benchmark: about -1.2%.
- Core2: slightly negative globally.

The shading functions are called per vertex/face, so this is a more sensitive boundary than `uber_shader_inline()`.

### `all_empty`

Do not use globally.

It can reduce code size, but Pico2 benchmark loses about 13%. It mixes several effects and is not a clean optimization direction.

## Recommended defaults

For now:

```cpp
#ifndef TGX_RASTERIZE_TRIANGLE_INLINE
#define TGX_RASTERIZE_TRIANGLE_INLINE TGX_INLINE
#endif

#ifndef TGX_UBER_SHADER_INLINE
#define TGX_UBER_SHADER_INLINE TGX_INLINE
#endif

#ifndef TGX_SHADER_SELECT_INLINE
#define TGX_SHADER_SELECT_INLINE TGX_INLINE
#endif

#ifndef TGX_RENDERER3D_SHADING_INLINE
#define TGX_RENDERER3D_SHADING_INLINE TGX_INLINE
#endif
```

Recommended next experiment:

- Study `TGX_RASTERIZE_TRIANGLE_INLINE=` as a possible board-specific setting.
- Use targeted real examples and not only `benchmark3D`, because this macro changes code size/layout and can interact with cache/flash placement.

## Generated artifacts

Useful compact summaries:

- `hardware/benchmark_global_summary.csv`
- `hardware/benchmark_subtests.csv`
- `hardware/benchmark_subtest_summary.csv`
- `hardware/example_fps_summary.csv`
- `hardware/example_variant_summary.csv`
- `hardware/compile_size_summary.csv`
- `codegen/hot_symbol_rows.csv`
- `codegen/hot_symbol_summary.csv`

Reusable scripts:

- `run_inline_macro_matrix.ps1`
- `aggregate_inline_macro_results.py`

