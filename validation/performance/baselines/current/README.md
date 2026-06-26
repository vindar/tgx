# Current TGX 3D performance baseline

Date: 2026-06-26

Branch: `feature/point-spot-lights`

Baseline label: `baseline_spotlight_specraw_20260626`

This baseline was recorded after the first spotlight runtime optimization pass. It
uses the point/spot light implementation plus the accepted local-specular guard:
skip the half-vector inverse square root when the raw specular numerator is not
positive. The source delta is captured in `source_diff.patch`.

The previous `current` baseline was archived under:

`validation/performance/baselines/previous/2026-06-15-shader-incremental/`

## Boards

| Board | FQBN / profile | Runtime port |
|---|---|---|
| Teensy 4.1 + ILI9341 | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` | `COM3` |
| Core2 | `esp32:esp32:m5stack_core2` | `COM5` |
| CoreS3 | `esp32:esp32:m5stack_cores3` | `COM10` |
| Pico2 + ILI9341 | `rp2040:rp2040:rpipico2:opt=Fast` | `COM21` |

## Benchmark scores

These are the global `examples/benchmark` scores. Benchmark capture was run with
the serial kick enabled because the sketch waits for input before starting.

| Board | Score 1 | Score 2 | Score 3 |
|---|---:|---:|---:|
| Teensy 4.1 | 118.55 | 90.37 | 80.65 |
| Core2 | 32.41 | 23.88 | |
| CoreS3 | 43.52 | 31.87 | 28.31 |
| Pico2 | 21.91 | 17.21 | 15.35 |

Comparison against the pre-spotlight baseline
`validation/performance/baselines/spotlights_prechange_2026-06-25` shows no
meaningful historical-rendering regression:

| Board | Largest benchmark delta |
|---|---:|
| Teensy 4.1 | +0.017% / -0.011% |
| Core2 | 0% |
| CoreS3 | 0% |
| Pico2 | +0.046% |

Full data:

- `benchmark_global_scores.csv`
- `benchmark_subtests.csv`
- `comparison_pre_spotlight_benchmark_global.csv`

## Example telemetry

The real-example telemetry suite was run successfully on all four boards.

| Board | Examples |
|---|---|
| Teensy 4.1 | `mars`, `test-shading`, `test-texture`, `buddha`, `scream` |
| Core2 | `borg_cube`, `donkeykong`, `scream` |
| CoreS3 | `borg_cube`, `donkeykong`, `scream` |
| Pico2 | `borg_cube`, `bunny_fig`, `scream` |

Full data:

- `example_telemetry.csv`
- `example_telemetry_summary.csv`
- `comparison_pre_spotlight_example_summary.csv`

Most example deltas are within normal telemetry noise. The Pico2
`bunny_fig/gouraud_texture` capture is lower than the pre-spotlight comparison
row by about 6.3%; keep an eye on it in the next refresh, but the benchmark
suite and the other Pico2 examples are stable.

## CPU render validation

The accepted source optimization was also checked with a CPU spotlight-render
comparator against commit `ff8cc0b3`. Three representative frames produced
identical image hashes:

| Frame | Hash |
|---:|---:|
| 0 | 14661566424370029289 |
| 1 | 5422464622175942429 |
| 2 | 9832561839460195462 |

The hash manifests are saved as:

- `spotlight_cpu_current_hashes.csv`
- `spotlight_cpu_ref_hashes.csv`

## Capture command pattern

Benchmarks used:

```powershell
validation\performance\tools\upload_and_capture.ps1 `
  -Board <board> `
  -Sketch examples\benchmark `
  -Label baseline_spotlight_specraw_20260626_benchmark `
  -ParseMode benchmark `
  -Baud 9600 `
  -UseBoardBenchmarkDefine `
  -KickText x `
  -KickDelayMs 2500
```

Examples used the same helper with the board-specific sketch path and
`-ParseMode telemetry`.

## Files

- `run_manifest.csv`: exact board/sketch capture manifest.
- `binary_size.csv`: compact memory and binary-size lines from each build log.
- `source_diff.patch`: source delta for the baseline.
- `source_diff_stat.txt` / `source_diff_name_only.txt`: compact diff summary.

Raw build directories, serial logs, parsed JSON files, and temporary telemetry
captures were pruned after the compact CSVs were generated.
