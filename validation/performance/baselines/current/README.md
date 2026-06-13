# Current Performance Baseline

This folder contains the compact TGX validation/performance baseline captured on
2026-06-13 for the four primary boards connected during the rasterizer/shader
signature work.

Source state:

```text
branch: main
commit: 366112ea567651cecf58e51f096d62fa217fcace
working tree: dirty source state captured by source_diff_stat.txt/source_diff.patch
label: baseline_20260613_rastertemplate
```

## Files

- `benchmark_global_scores.csv`: 3D benchmark final scores.
- `benchmark_subtests.csv`: detailed 3D benchmark subtest rows.
- `binary_size.csv`: compiler memory/size lines captured with the benchmark.
- `example_telemetry.csv`: parsed telemetry rows from the selected real examples.
- `example_telemetry_summary.csv`: compact averages/medians by board, example,
  and scene.
- `run_manifest.csv`: sketch path, FQBN, upload port, capture status, and exact
  upload command for each run.
- `validation_results.csv`: records that CPU/Teensy validation was not rerun in
  this benchmark refresh.
- `source_diff_stat.txt`, `source_diff_name_only.txt`, `source_diff.patch`: the
  uncommitted source state used for this baseline.

The previous compact baseline was archived under:

```text
validation/performance/baselines/archive/2026-06-12-watertight-plus-extended/
```

## Boards And Build Options

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`,
  upload `usb:80000/3/0/1`, serial `COM3`.
- Pico2 / RP2350 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast`, serial `COM21`.
- Core2 / ESP32: `esp32:esp32:m5stack_core2`, port `COM5`.
- CoreS3 / ESP32-S3: `esp32:esp32:m5stack_cores3`, port `COM10`.

For Pico2 examples, `TFT_eSPI/User_Setup_Select.h` was configured for the
RP2040/RP2350 ILI9341 setup. The benchmark itself does not use the display.

## Benchmark Suite

Recorded global scores:

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------- | ------- | ------- |
| teensy41 | 105.37 | 80.24 | 70.66 |
| pico2 | 19.78 | 15.5 | 13.69 |
| core2 | 23.31 | 17.49 |  |
| cores3 | 32.72 | 24.59 | 21.86 |

Use `benchmark_subtests.csv` for local regressions; global scores alone are not
enough to accept an optimization.

Pico2 note: the benchmark upload succeeded after the board entered UF2 mode, but
the benchmark waits for serial input. The valid performance data comes from a
second capture-only pass using `SkipCompileUpload` and `KickText=x`; see
`run_manifest.csv`.

## Real-Example Telemetry Suite

Teensy 4.1:

- `examples/Teensy4/3D/mars`
- `examples/Teensy4/3D/test-shading`
- `examples/Teensy4/3D/test-texture`
- `examples/Teensy4/3D/buddha`

Pico2:

- `examples/Pico_RP2040_RP2350/borg_cube`
- `examples/Pico_RP2040_RP2350/bunny_fig`
- `examples/Pico_RP2040_RP2350/scream`

Core2 and CoreS3:

- `examples/M5Stack/borg_cube`
- `examples/M5Stack/donkeykong`
- `examples/M5Stack/scream`

The exact upload/capture commands are in `run_manifest.csv`.

## Validation Status

This refresh intentionally captured benchmark and real-example telemetry only.
CPU/golden validation was not rerun in this pass. If the current source changes
are accepted as production code, rerun the CPU validation suite before release.

## Reuse Policy

Reuse this baseline only when the source state, board/display setup, build
profile, benchmark/example sketch, and capture method are equivalent.

Rerun the relevant subset when:

- renderer, shader, rasterizer, mesh, Image, or display-upload behavior changed;
- Arduino board packages or optimization options changed;
- `TFT_eSPI` setup changed for Pico2/Feather-style boards;
- an optimization result is near the noise threshold or suspicious.
