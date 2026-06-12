# Current Performance Baseline

This folder contains the compact TGX validation/performance baseline captured on
2026-06-12 and extended on 2026-06-13 after regenerating the Mesh3Dv2 example
meshes with the watertight mesh-tool patch.

Source state:

```text
commit: fe98d984feda942c88de6c036ec0439d0cd70940
labels: baseline_20260612_watertight
        baseline_20260613_extended
branch: main
```

## Files

- `benchmark_global_scores.csv`: 3D benchmark final scores for the boards
  listed below.
- `benchmark_subtests.csv`: detailed 3D benchmark subtest rows.
- `binary_size.csv`: compiler memory/size lines captured with the benchmark.
- `example_telemetry.csv`: raw parsed telemetry rows from the selected real
  examples.
- `example_telemetry_summary.csv`: compact averages/medians by board, example,
  and scene.
- `run_manifest.csv`: sketch path, FQBN, upload port, capture status, and exact
  upload command for each real-example run.
- `validation_results.csv`: CPU and Teensy validation suites run for this
  baseline.

## Boards And Build Options

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`
  on Teensy upload port `usb:80000/3/0/1`, serial `COM3`.
- Core2 / ESP32: `esp32:esp32:m5stack_core2` on `COM5`.
- CoreS3 / ESP32-S3: `esp32:esp32:m5stack_cores3` on `COM10`.
- Pico2 / RP2350 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast` on `COM21`.
- Teensy 3.6 + ST7735: `teensy:avr:teensy36:usb=serial,speed=180,opt=o3std`
  on Teensy upload port `usb:80000/3/0/1`, serial `COM23`.
- Pico W / RP2040 + ILI9341: `rp2040:rp2040:rpipicow` on `COM19`.
- Feather S3 + ILI9341: `esp32:esp32:adafruit_feather_esp32s3_tft`, upload
  via bootloader `COM13` when needed, serial/display run on `COM14`.
- Feather S2 + ILI9341: `esp32:esp32:adafruit_feather_esp32s2_tft` on `COM11`;
  examples compile, but runtime baseline is missing because Windows refused to
  open/upload `COM11` during the 2026-06-13 pass.

For Pico2/Pico W, `TFT_eSPI/User_Setup_Select.h` was configured for
`Setup_RP2040_RP2350_ILI9341.h`. For Feather S3/S2 compile and runtime tests it
was configured for `Setup_feather_TFT_S2_S3_ILI9341.h`.

## Benchmark Suite

The 3D benchmark was run on all four boards with:

```powershell
powershell -ExecutionPolicy Bypass -File validation\performance\tools\run_benchmark.ps1 -Board <board> -Label baseline_20260612_watertight
```

Recorded global scores:

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------- | ------- | ------- |
| Teensy 4.1 | 105.05 | 79.99 | 70.45 |
| Teensy 3.6 | 101.05 | 90.89 | 68.07 |
| Core2 | 29.59 | 21.04 | |
| CoreS3 | 38.24 | 27.58 | 24.18 |
| Pico2 | 19.75 | 15.47 | 13.67 |
| Pico W | 13.97 | 9.66 | |
| Feather S3 | 38.03 | 27.46 | 24.09 |
| Feather S2 | unavailable | unavailable | unavailable |

Use `benchmark_subtests.csv` for local regressions; global scores alone are not
enough to accept an optimization.

## Real-Example Telemetry Suite

Teensy 4.1:

- `examples/Teensy4/3D/mars`
- `examples/Teensy4/3D/test-shading`
- `examples/Teensy4/3D/test-texture`
- `examples/Teensy4/3D/buddha`

Core2 and CoreS3:

- `examples/M5Stack/borg_cube`
- `examples/M5Stack/donkeykong`
- `examples/M5Stack/scream`

Pico2:

- `examples/Pico_RP2040_RP2350/borg_cube`
- `examples/Pico_RP2040_RP2350/bunny_fig`
- `examples/Pico_RP2040_RP2350/scream`

Teensy 3.6:

- `examples/Teensy3/3D/scream`
- `examples/Teensy3/3D/characters`

Pico W:

- `examples/Pico_RP2040_RP2350/scream`
- `examples/Pico_RP2040_RP2350/borg_cube`
- `examples/Pico_RP2040_RP2350/bunny_fig`

Feather S3:

- `examples/ESP32/donkeykong`
- `examples/ESP32/scream`
- `examples/ESP32/borg_cube`

Feather S2:

- The same three ESP32 examples compile, but were not uploaded/captured because
  `COM11` was visible yet not openable by Windows/esptool.

The exact upload/capture commands are in `run_manifest.csv`.

## Validation Suites

The baseline includes:

- CPU 2D golden validation.
- CPU 3D strict validation after updating the Mesh3Dv2 CPU baselines/goldens.
- CPU 2D large validation.
- Teensy 4.1 2D main validation.
- Teensy 4.1 2D optional validation (`pngdec`, `jpegdec`, `animatedgif`,
  `openfontrender`).

See `validation_results.csv` for the exact commands and statuses.

## Reuse Policy

Reuse this baseline only when the source state, board/display setup, build
profile, benchmark/example sketch, and capture method are equivalent.

Rerun the relevant subset when:

- renderer, shader, rasterizer, mesh, Image, or display-upload behavior changed;
- Arduino board packages or optimization options changed;
- `TFT_eSPI` setup changed for Pico2/Feather-style boards;
- an optimization result is near the noise threshold or suspicious.

During future investigations, keep new raw logs under the dated investigation
folder or temporary validation logs, then update this folder only when a new
baseline is intentionally accepted.
