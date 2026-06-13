# Current Performance Baseline

Compact TGX performance baseline captured on 2026-06-13 after the rasterizer/shader dispatch signature work.

Source state:

```text
branch: main
commit: bbdba1048077b391a091f63fc4ea1e5521ba9084
working tree source diff: see source_diff_stat.txt/source_diff.patch (empty when source is clean)
label: baseline_20260613_bbdba104
```

## Boards And Build Options

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`, upload `usb:80000/3/0/1`, serial `COM3`.
- Pico2 / RP2350 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast`, serial/upload `COM21`; `TFT_eSPI` selected `Setup_RP2040_RP2350_ILI9341.h`.
- Core2 / ESP32: `esp32:esp32:m5stack_core2`, port `COM5`.
- CoreS3 / ESP32-S3: `esp32:esp32:m5stack_cores3`, port `COM10`.

## Benchmark Global Scores

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------- | ------- | ------- |
| teensy41 | 106.34 | 80.81 | 70.95 |
| pico2 | 18.88 | 14.85 | 13.16 |
| core2 | 26.8 | 19.51 |  |
| cores3 | 36.82 | 26.71 | 23.46 |

Comparison against the previous current baseline is stored in `comparison_previous_benchmark_global.csv`.

## Selected Real Examples

Teensy 4.1: `mars`, `test-shading`, `test-texture`, `buddha`.

Pico2: `borg_cube`, `bunny_fig`, `scream`.

Core2/CoreS3: `borg_cube`, `donkeykong`, `scream`.

Aggregate example comparison vs previous current baseline:

| Board | Example | Previous mean fps | New mean fps | Delta |
| ----- | ------- | ----------------: | -----------: | ----: |
| core2 | borg_cube | 46.4556 | 46.4246 | -0.067% |
| core2 | donkeykong | 27.0951 | 28.4694 | +5.072% |
| core2 | scream | 14.5909 | 15.1600 | +3.900% |
| cores3 | borg_cube | 49.5714 | 49.5810 | +0.019% |
| cores3 | donkeykong | 30.6921 | 32.0928 | +4.564% |
| cores3 | scream | 22.1778 | 23.2203 | +4.701% |
| pico2 | borg_cube | 31.0000 | 31.0000 | +0.000% |
| pico2 | bunny_fig | 27.0797 | 27.3563 | +1.022% |
| pico2 | scream | 25.1500 | 25.3107 | +0.639% |
| teensy41 | buddha | 29.7372 | 29.6431 | -0.316% |
| teensy41 | mars | 188.0389 | 191.7207 | +1.958% |
| teensy41 | test-shading | 81.0662 | 81.9755 | +1.122% |
| teensy41 | test-texture | 72.3896 | 71.3345 | -1.457% |

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
- `validation_results.csv`
- `source_diff_stat.txt`
- `source_diff_name_only.txt`
- `source_diff.patch`

## Reuse Policy

Reuse this baseline only with the same source state, board/display setup, build profile, benchmark/example sketch, and robust upload/capture tooling.
