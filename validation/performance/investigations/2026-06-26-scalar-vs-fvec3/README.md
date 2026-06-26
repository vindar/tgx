# Scalar vs fVec3 spotlight shading probe - 2026-06-26

Goal: compare the existing scalar `Lx/Ly/Lz` spotlight runtime code against an
`fVec3` version using `dotProduct()` in `_shadeVertex()` and `_shadeFace()`.

Sketches:

- Pico2: `examples/Pico_RP2040_RP2350/spotlights_room`
- Core2/CoreS3: `examples/M5Stack/spotlights_room`
- Teensy 4.1: `examples/Teensy4/3D/spotlights_room`

Build profiles:

- Pico2: `rp2040:rp2040:rpipico2:opt=Fast`
- Core2: `esp32:esp32:m5stack_core2`
- CoreS3: `esp32:esp32:m5stack_cores3`
- Teensy 4.1: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`

Each run uploaded the sketch and captured about 24 seconds of serial FPS
telemetry. The result is the average of all parsed FPS lines in that capture.

| Board | Scalar avg FPS | fVec3 avg FPS | Delta | Scalar median | fVec3 median | Flash delta |
|---|---:|---:|---:|---:|---:|---:|
| Pico2 | 16.70 | 16.70 | +0.00% | 17 | 17 | 0 |
| Core2 | 20.42 | 20.42 | +0.00% | 20 | 20 | 0 |
| CoreS3 | 38.04 | 38.12 | +0.21% | 38 | 38 | -16 |
| Teensy 4.1 | 100.92 | 101.77 | +0.84% | 101 | 102 | -13312 |

Conclusion: the `fVec3` version has no measurable penalty on this spotlight
example across the four boards. It is neutral on Pico2/Core2, within noise but
slightly positive on CoreS3, and slightly positive on Teensy 4.1 with a notable
code-size reduction.

Compact CSVs:

- `summary_scalar_vs_fvec3.csv`
- `comparison_scalar_vs_fvec3.csv`

Raw parsed telemetry/logs were pruned after the compact CSV summaries were
written.
