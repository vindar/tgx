# Current Performance Baseline

Compact TGX performance baseline promoted on 2026-06-15 after accepting the `src/Shaders.h` Gouraud incremental shader candidate.

Source state:

```text
branch: main
commit: 384d12ffa0aeaa01897cf45bcd695fdf4d776084
working tree source diff: none; the accepted shader changes are committed
label: baseline_20260615_shader_incremental
```

The previous current baseline was archived to:

```text
validation/performance/baselines/previous/2026-06-14-before-shader-incremental/
```

## Boards And Build Options

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`, upload `usb:80000/3/0/1`, serial `COM3`.
- Pico2 / RP2350 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast`, serial/upload `COM21`; `TFT_eSPI` selected `Setup_RP2040_RP2350_ILI9341.h`.
- Core2 / ESP32: `esp32:esp32:m5stack_core2`, port `COM5`.
- CoreS3 / ESP32-S3: `esp32:esp32:m5stack_cores3`, port `COM10`.

All runs used `validation/performance/tools/upload_and_capture.ps1` with parsed benchmark/example telemetry.

## Shader Flags

This baseline corresponds to the accepted candidate default on the four primary boards:

```cpp
TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=1
TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=1
```

Extra Pico W / RP2040 checks showed that RP2040 without FPU should not use those incremental-float paths:

```cpp
TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=0
TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=0
```

The Pico W matrix is retained in:

- `picow_rp2040_flag_matrix_delta_vs_head.csv`
- `picow_rp2040_flag_matrix_best_variant.csv`
- `picow_rp2040_flag_matrix_binary_size.csv`

## Benchmark Global Scores

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------: | ------: | ------: |
| Teensy 4.1 | 115.90 | 87.32 | 77.18 |
| Pico2 | 21.50 | 16.66 | 14.76 |
| Core2 | 31.69 | 22.51 | |
| CoreS3 | 42.44 | 30.07 | 26.34 |

Comparison against the same-session clean `HEAD` baseline is stored in `comparison_previous_benchmark_global.csv`.

## Selected Real Examples

Teensy 4.1:

- `mars`
- `test-shading`
- `test-texture`
- `buddha`

Pico2:

- `borg_cube`
- `bunny_fig`
- `scream`

Core2/CoreS3:

- `borg_cube`
- `donkeykong`
- `scream`

Important candidate-vs-HEAD examples:

| Board | Example / scene | Delta |
| ----- | --------------- | ----: |
| Teensy 4.1 | `buddha / buddha_rotation` | -5.18% frame time |
| Teensy 4.1 | `test-texture / spot_tex_nearest` | -7.35% frame time |
| Teensy 4.1 | `mars / movie` | +1.31% frame time |
| Pico2 | `bunny_fig / gouraud` | -10.30% frame time |
| Pico2 | `bunny_fig / gouraud_texture` | -18.84% frame time |
| Pico2 | `scream` | +20.47% FPS |
| Core2 | `donkeykong / gouraud` | -8.24% frame time |
| Core2 | `donkeykong / gouraud_texture` | -9.29% frame time |
| Core2 | `scream` | +11.37% FPS |
| CoreS3 | `donkeykong / gouraud` | -7.55% frame time |
| CoreS3 | `donkeykong / gouraud_texture` | -9.21% frame time |
| CoreS3 | `scream` | +14.35% FPS |

Use `example_telemetry_summary.csv` for per-scene values and `comparison_previous_example_summary.csv` for per-scene deltas.

## Files

- `benchmark_global_scores.csv`
- `benchmark_subtests.csv`
- `binary_size.csv`
- `example_telemetry.csv`
- `example_telemetry_summary.csv`
- `run_manifest.csv`
- `comparison_previous_benchmark_global.csv`
- `comparison_previous_example_summary.csv`
- `comparison_previous_example_aggregate.csv`
- `validation_results.csv`
- `source_diff_stat.txt`
- `source_diff_name_only.txt`
- `source_diff.patch`

The `source_diff.*` files are intentionally empty for this baseline because it
is tied to the committed `main` source state above, not to a local candidate
diff.

## Reuse Policy

Reuse this baseline only with the same source state, board/display setup, build profile, benchmark/example sketch, shader flag defaults, and robust upload/capture tooling.

The detailed investigation report and flag matrix remain in:

```text
validation/performance/investigations/2026-06-shader-incremental-flag-matrix/
```
