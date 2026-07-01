# Current TGX 3D performance baseline

Date: 2026-06-30

Branch: `feature/point-spot-lights`

Baseline label: `baseline_20260630_dev`

This baseline was recorded after the point/spot light work, the new textured
point-light examples, and the recent primitive / inline cleanup work. It covers
the five currently connected boards and includes the new point/spot-light
examples as first-class telemetry rows.

The previous current baseline was archived under:

`validation/performance/baselines/previous/2026-06-26-spotlight-specraw/`

## Boards

| Board | FQBN / profile | Runtime port |
|---|---|---|
| Teensy 4.1 + ILI9341 | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` | `COM3` |
| Core2 | `esp32:esp32:m5stack_core2` | `COM5` |
| CoreS3 | `esp32:esp32:m5stack_cores3` | `COM10` |
| Pico2 + ILI9341 | `rp2040:rp2040:rpipico2:opt=Fast` | `COM21` |
| FeatherS3 + ILI9341 | `esp32:esp32:adafruit_feather_esp32s3_tft` | upload `COM13`, runtime `COM14` |

`TFT_eSPI/User_Setup_Select.h` was switched to FeatherS3 for Feather runs and
back to Pico2 for Pico runs.

## Benchmark Scores

These are the global `examples/benchmark` scores for the dev branch.

| Board | Score 1 | Score 2 | Score 3 |
|---|---:|---:|---:|
| Core2 | 31.86 | 23.59 | |
| CoreS3 | 43.08 | 31.88 | 28.37 |
| FeatherS3 | 43.13 | 31.92 | 28.41 |
| Pico2 | 22.78 | 17.81 | 15.85 |

The Teensy 4.1 benchmark does not fit in RAM1 with Teensyduino 1.62.0 under the
current `o3std` profile. `teensy_size` reports:

```text
main: RAM1 free for local variables: -6720
dev:  RAM1 free for local variables: -39488
```

So the Teensy benchmark is marked as failed in `run_manifest.csv`; the real
Teensy examples below compile, upload, and run successfully.

## Example Telemetry

The real-example telemetry suite was run successfully on all five boards.

| Board | Examples |
|---|---|
| Teensy 4.1 | `mars`, `test-shading`, `test-texture`, `buddha`, `scream`, `pointlight_room`, `pointlight_textured_meshes`, `spotlight_checkerboard` |
| Core2 | `borg_cube`, `donkeykong`, `scream`, `pointlight_room`, `pointlight_textured_meshes`, `spotlight_checkerboard` |
| CoreS3 | `borg_cube`, `donkeykong`, `scream`, `pointlight_room`, `pointlight_textured_meshes`, `spotlight_checkerboard` |
| Pico2 | `borg_cube`, `bunny_fig`, `scream`, `pointlight_room`, `pointlight_textured_meshes`, `spotlight_checkerboard` |
| FeatherS3 | `borg_cube`, `donkeykong`, `scream`, `pointlight_room`, `pointlight_textured_meshes`, `spotlight_checkerboard` |

Representative new-light example averages:

| Board | `pointlight_room` | `pointlight_textured_meshes` | `spotlight_checkerboard` |
|---|---:|---:|---:|
| Teensy 4.1 | 107.56 | 39.93-47.27 by mesh | 193.89 |
| Core2 | 21.33 | 17.27-18.00 by mesh | 24.08 |
| CoreS3 | 40.81 | 27.00-27.70 by mesh | 42.00 |
| Pico2 | 22.12 | 12.67-13.55 by mesh | 31.00 |
| FeatherS3 | 31.54 | 27.00-27.64 by mesh | 42.00 |

## Main Comparison

Comparable tests were also run from a separate `main` worktree at commit
`d46bdb8f89e79ac2e571de1cf97525c9d195510f`.

Benchmark deltas versus `main`:

| Board | Largest global-score delta |
|---|---:|
| Core2 | -1.70% |
| CoreS3 | -1.01% |
| FeatherS3 | -0.71% |
| Pico2 | +3.64% |

The real examples are mostly stable. The largest negative example deltas are
`teensy41/mars/intro` at -5.49% and `pico2/scream` at -3.20%; both are short
scene windows and should be watched in future refreshes. Common mesh/texturing
examples are otherwise near noise or improved, especially wireframe paths.

## Files

Current baseline files copied to `validation/performance/baselines/current/`:

- `run_manifest.csv`: dev branch run status, commands, ports, and sketches.
- `benchmark_global_scores.csv`: dev global benchmark scores.
- `benchmark_subtests.csv`: dev benchmark subtest matrix.
- `example_telemetry.csv`: dev per-line example FPS telemetry.
- `example_telemetry_summary.csv`: dev summarized FPS by board/example/scene.
- `binary_size.csv`: compact build size lines.

Investigation-only summaries kept under `summaries/`:

- `main_run_manifest.csv`: comparable `main` run status and commands.
- `main_benchmark_global_scores.csv`: comparable `main` benchmark scores.
- `main_example_telemetry_summary.csv`: comparable `main` example summaries.
- `comparison_main_benchmark_global.csv`: dev-vs-main benchmark deltas.
- `comparison_main_example_summary.csv`: dev-vs-main example deltas.

The full source diff is intentionally not stored because the new textured
example assets make it large and hard to review. Raw build folders, serial logs,
and parsed JSON files were pruned after the compact CSVs were generated.
