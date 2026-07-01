# Spotlight pre-change performance baseline

Date: 2026-06-25

Branch: `feature/point-spot-lights`

Commit: `c3a149f1` (`new spotlight API`)

Purpose: establish a hardware performance baseline before adding the private
spot-light data structures and renderer implementation.

## Boards

All expected boards were detected before the run:

| Board | Port | FQBN |
| --- | --- | --- |
| Teensy 4.1 + ILI9341 | `COM3`, upload `usb:80000/3/0/1` | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` |
| Core2 | `COM5` | `esp32:esp32:m5stack_core2` |
| CoreS3 | `COM10` | `esp32:esp32:m5stack_cores3` |
| Pico2 + ILI9341 | `COM21` | `rp2040:rp2040:rpipico2:opt=Fast` |

`TFT_eSPI/User_Setup_Select.h` was checked before the Pico2 example runs and
selected `Setup_RP2040_RP2350_ILI9341.h`.

## Capture Status

All 16 uploads/captures completed successfully on the first attempt.

| Category | Runs | Status |
| --- | ---: | --- |
| `benchmark3D` | 4 | success |
| real examples | 12 | success |

The robust `validation/performance/tools/upload_and_capture.ps1` workflow was
used for all runs. Exact commands, ports, FQBNs and sketches are recorded in
`run_manifest.csv`.

## Benchmark Global Scores

| Board | Score 1 | Score 2 | Score 3 |
| --- | ---: | ---: | ---: |
| Teensy 4.1 | 118.53 | 90.38 | 80.65 |
| Core2 | 32.41 | 23.88 | n/a |
| CoreS3 | 43.52 | 31.87 | 28.31 |
| Pico2 | 21.90 | 17.21 | 15.35 |

Full benchmark subtest data is in `benchmark_subtests.csv`.

## Example Coverage

| Board | Examples |
| --- | --- |
| Teensy 4.1 | `test-shading`, `test-texture`, `scream` |
| Core2 | `borg_cube`, `donkeykong`, `scream` |
| CoreS3 | `borg_cube`, `donkeykong`, `scream` |
| Pico2 | `borg_cube`, `bunny_fig`, `scream` |

Example FPS rows and grouped per-scene averages are in:

- `example_telemetry.csv`
- `example_telemetry_summary.csv`

## Notes

Pico2 builds emitted the usual `TFT_eSPI` touch warning and RP2040 core/lwIP
warnings. `borg_cube` and `bunny_fig` also reported low dynamic memory, as in
previous Pico2 runs. These warnings did not prevent successful upload, serial
capture, or telemetry parsing.

This directory is the compact retained baseline. Raw metadata, serial logs and
build logs were intentionally pruned after the parsed CSV summaries were copied
here.
