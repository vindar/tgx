# Current Performance Baseline

Compact TGX performance baseline captured on 2026-06-14 after the current `Rasterizer.h` signature/update work.

Source state:

```text
branch: main
commit: 0f42085f76cf9cdf5a2a87289bce31c1ceb7e9fd
working tree source diff: see source_diff_stat.txt/source_diff.patch
label: baseline_20260614_current
```

## Boards And Build Options

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`, upload `usb:80000/3/0/1`, serial `COM3`.
- Pico2 / RP2350 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast`, serial/upload `COM21`; `TFT_eSPI` selected `Setup_RP2040_RP2350_ILI9341.h`.
- Core2 / ESP32: `esp32:esp32:m5stack_core2`, port `COM5`.
- CoreS3 / ESP32-S3: `esp32:esp32:m5stack_cores3`, port `COM10`.

All runs used `validation/performance/tools/upload_and_capture.ps1` with parsed benchmark/example telemetry.

## Benchmark Global Scores

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------- | ------- | ------- |
| teensy41 | 116.42 | 85.47 | 74.33 |
| pico2 | 20.97 | 15.89 | 13.92 |
| core2 | 30.97 | 21.29 |  |
| cores3 | 41.59 | 28.71 | 24.89 |

Comparison against the previous current baseline is stored in `comparison_previous_benchmark_global.csv`.

## Selected Real Examples

Teensy 4.1: `mars`, `test-shading`, `test-texture`, `buddha`.

Pico2: `borg_cube`, `bunny_fig`, `scream`.

Core2/CoreS3: `borg_cube`, `donkeykong`, `scream`.

Aggregate example comparison vs previous current baseline:

| Board | Example | Previous mean fps | New mean fps | Delta |
| ----- | ------- | ----------------: | -----------: | ----: |
| core2 | borg_cube | 46.4246 | 46.4144 | -0.022% |
| core2 | donkeykong | 28.451 | 27.832 | -2.176% |
| core2 | scream | 15.16 | 15.3636 | +1.343% |
| cores3 | borg_cube | 49.581 | 49.6278 | +0.094% |
| cores3 | donkeykong | 32.0311 | 32.4788 | +1.398% |
| cores3 | scream | 23.2203 | 23.5337 | +1.350% |
| pico2 | borg_cube | 31 | 30.9944 | -0.018% |
| pico2 | bunny_fig | 27.6614 | 27.773 | +0.403% |
| pico2 | scream | 25.3107 | 25.7416 | +1.702% |
| teensy41 | buddha | 29.6431 | 29.681 | +0.128% |
| teensy41 | mars | 62.3685 | 61.9863 | -0.613% |
| teensy41 | test-shading | 80.9711 | 83.4098 | +3.012% |
| teensy41 | test-texture | 73.5801 | 77.4054 | +5.199% |

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

## Reuse Policy

Reuse this baseline only with the same source state, board/display setup, build profile, benchmark/example sketch, and robust upload/capture tooling.
