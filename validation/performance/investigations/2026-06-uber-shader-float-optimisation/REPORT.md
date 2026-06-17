# TGX uber_shader_inline float optimisation report

Date: 2026-06-17  
Branch: `affine-texture-fixedpoint`  
Target code area: `src/Shaders.h`, `src/Misc.h`, `src/tgx_config.h`

## Scope

This investigation focused on `uber_shader_inline()` after the affine-texturing work. The fixed-point texture-coordinate path was rejected before this phase because it was visually unstable. The strategy here was therefore to keep float interpolation and look for small, targeted optimisations that do not perturb shader selection or unrelated render paths.

Priority paths:

- perspective projection;
- z-buffer enabled;
- textured rendering, especially perspective-correct texture;
- current target MCUs: ESP32, ESP32-S3, RP2350, Teensy 4.x.

## Hardware and build matrix

All hardware runs used `validation/performance/tools/upload_and_capture.ps1`, not ad-hoc serial capture.

| Board | Port | FQBN | Baud |
| --- | --- | --- | --- |
| Teensy 4.1 + ILI9341 | `COM3`, upload `usb:80000/3/0/1` | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` | 9600 |
| Core2 / ESP32 | `COM5` | `esp32:esp32:m5stack_core2` | 115200 |
| CoreS3 / ESP32-S3 | `COM10` | `esp32:esp32:m5stack_cores3` | 115200 |
| Pico2 / RP2350 + ILI9341 | `COM21` | `rp2040:rp2040:rpipico2:opt=Fast` | 115200 |

Pico2 `TFT_eSPI/User_Setup_Select.h` was verified before Pico builds; the active setup was `Setup_RP2040_RP2350_ILI9341.h`.

## Architecture notes

- Teensy 4.1 uses an Arm Cortex-M7 at 600 MHz and PJRC documents both 32-bit and 64-bit floating-point support: https://www.pjrc.com/store/teensy41.html
- RP2350 / Pico2 uses dual Cortex-M33 cores. Raspberry Pi documents the standard Arm single-precision FPU and automatic SDK use for `float`: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
- ESP32 uses Xtensa LX6 and Espressif documents FPU support: https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf
- ESP32-S3 uses Xtensa LX7; Espressif documents a single-precision FPU and 240 MHz clock: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf

The important practical conclusion is that float arithmetic itself is not the enemy on these targets. The costly operation inside the perspective-texture path is the per-pixel reciprocal. Reducing that cost is more promising than converting the whole affine path to fixed point.

## Accepted source changes

1. `src/Misc.h`

   `fast_inv_approx(float)` now uses one correction step on non-Xtensa targets when `TGX_USE_FAST_INV_TRICK` is enabled:

   - old non-Xtensa path: same initial bit estimate as `fast_inv()`, then two correction steps;
   - new non-Xtensa approximate path: one correction step using `fmaf(-x, y, 2.00128722f)`;
   - Xtensa path unchanged: it already used `recip0.s` plus one Newton-style correction.

   Local numeric sampling showed max relative error around `1.288e-3` for the one-step approximation, versus about `1e-6` for the old two-step path. This is intentional for `fast_inv_approx()`, but it is the main visual-quality risk to inspect on perspective-textured scenes.

2. `src/Shaders.h`

   Removed an unused per-scanline `C1` calculation from the active `uber_shader_inline()` implementation. This is mainly pruning/cleanup; it did not move code size in the tested examples.

   Also avoided computing `flat_color = (color_t)data.facecolor` in textured flat/unlit template variants where the value is never used. This is compile-time-pruned by `if constexpr`.

3. `src/tgx_config.h`

   Enabled `TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` only for Teensy 4.x. This remained rejected for ESP32/ESP32-S3/ESP32-P4 after measurements showed slowdowns there.

4. `examples/Teensy4/3D/test-texture/test-texture.ino`

   This file remains in the earlier user-approved affine-texture visual-test state. It was not part of the float optimisation itself. Consequently, the `test-texture` baseline and final data in this investigation compare affine-vs-affine, not perspective-vs-affine.

## Rejected candidates

| Candidate | Decision | Reason |
| --- | --- | --- |
| Fixed-point affine texture coordinates | Rejected before this phase | Visually unstable texture mapping on hardware. |
| Z-buffer early-exit with `goto` | Rejected | Added warnings and slowed important scenes, including Pico2 textured Gouraud and Core2/CoreS3 paths. |
| RGB565 bilinear helper arguments by value | Rejected | No useful size/performance improvement. |
| Incremental pixel pointers on ESP32/ESP32-S3/ESP32-P4 | Rejected | Core2/CoreS3 donkeykong regressed; only Teensy4 kept. |
| Non-FMA reciprocal correction: `y *= 2.001... - x*y` | Rejected | Teensy `mars` failed compile due RAM1/code growth; Pico2 sketch grew by 912 bytes. |
| `__builtin_expect(W < current_z, 1)` z-pass hint | Rejected | Helped some no-texture z paths but slowed priority textured scenes: Teensy `mars/movie` +7.80% frame time, Pico2 `bunny_fig/gouraud_texture` +3.30%. |

## Measurement semantics

Primary metric when available:

- `frame_avg_us_weighted`: microseconds per TGX frame, weighted by the number of frames reported by telemetry.
- Delta column: `frame_delta_pct_negative_is_faster`; negative means faster.

Fallback metric for examples that only report FPS:

- `fps_avg_reported`: arithmetic mean of reported FPS lines.
- Delta column: `fps_delta_pct_positive_is_faster`; positive means faster.

The full CSV files are:

- `example_summary_floatopt_final_with_base.csv`
- `delta_floatopt_final_vs_base.csv`
- `build_size_floatopt_final_vs_base.csv`
- `command_manifest_floatopt_final_vs_base.csv`
- `delta_floatopt_zlikely_vs_final.csv`

## Full-suite results

All 13 standard real-example uploads/captures succeeded for the accepted final candidate:

- Teensy 4.1: `mars`, `test-shading`, `test-texture`, `buddha`.
- Core2: `borg_cube`, `donkeykong`, `scream`.
- CoreS3: `borg_cube`, `donkeykong`, `scream`.
- Pico2: `borg_cube`, `bunny_fig`, `scream`.

Important frame-time results:

| Board | Example / scene | Baseline us/frame | Final us/frame | Delta |
| --- | --- | ---: | ---: | ---: |
| Teensy 4.1 | `mars/movie` | 10643.54 | 9048.91 | -14.98% |
| Teensy 4.1 | `mars/falcon` | 18136.52 | 16895.33 | -6.84% |
| Pico2 | `bunny_fig/gouraud_texture` | 38890.78 | 37591.99 | -3.34% |
| Teensy 4.1 | `buddha/buddha_rotation` | 19167.72 | 19068.84 | -0.52% |
| Teensy 4.1 | `test-shading` mean over 15 scenes | n/a | n/a | -0.17% mean |
| Teensy 4.1 | `test-texture` affine mean over 9 scenes | n/a | n/a | -0.22% mean |
| Core2 | `donkeykong` mean over 4 scenes | n/a | n/a | +0.33% mean |
| CoreS3 | `donkeykong` mean over 4 scenes | n/a | n/a | +0.30% mean |

The small Core2/CoreS3 `donkeykong` slowdowns are not tied to a changed Xtensa reciprocal path: the Xtensa `fast_inv_approx()` implementation was already one-step assembly and was not changed. Code size also stayed identical on ESP32/ESP32-S3. Treat these as run noise unless repeated runs show the same sign.

FPS-only examples:

| Board | Example | Baseline FPS | Final FPS | Delta |
| --- | --- | ---: | ---: | ---: |
| Core2 | `borg_cube` | 65.73 | 65.70 | -0.04% |
| Core2 | `scream` | 18.80 | 18.83 | +0.16% |
| CoreS3 | `borg_cube` | 184.99 | 184.78 | -0.11% |
| CoreS3 | `scream` | 39.58 | 39.35 | -0.58% |
| Pico2 | `borg_cube` | 101.85 | 106.76 | +4.83% |
| Pico2 | `scream` | 30.43 | 31.49 | +3.49% |

Build size:

| Board | Example | Size field | Baseline | Final | Delta |
| --- | --- | --- | ---: | ---: | ---: |
| Teensy 4.1 | `mars` | FLASH code bytes | 238064 | 237936 | -128 |
| Teensy 4.1 | `test-shading` | FLASH code bytes | 192292 | 192292 | 0 |
| Teensy 4.1 | `test-texture` | FLASH code bytes | 177316 | 177316 | 0 |
| Teensy 4.1 | `buddha` | FLASH code bytes | 142340 | 142340 | 0 |
| Core2 | all 3 examples | sketch bytes | unchanged | unchanged | 0 |
| CoreS3 | all 3 examples | sketch bytes | unchanged | unchanged | 0 |
| Pico2 | `borg_cube` | sketch bytes | 173236 | 173220 | -16 |
| Pico2 | `bunny_fig` | sketch bytes | 345348 | 345308 | -40 |
| Pico2 | `scream` | sketch bytes | 205076 | 205044 | -32 |

## Final board state

After rejecting the temporary candidates, the accepted source version was uploaded again and captured successfully:

- Teensy 4.1: `examples/Teensy4/3D/mars`
- Core2: `examples/M5Stack/donkeykong`
- CoreS3: `examples/M5Stack/donkeykong`
- Pico2: `examples/Pico_RP2040_RP2350/bunny_fig`

These are the sketches currently left running on the boards.

## Quality risk

The accepted performance win comes from reducing the accuracy of `fast_inv_approx()` on non-Xtensa targets. The function name and documentation already make it an approximate reciprocal, and the visual paths tested by telemetry ran without crashes. However, this should still be visually inspected on high-perspective, textured scenes (`mars`, `bunny_fig`, and future texture stress scenes) before committing as a permanent default.

If visual artifacts appear, the safest rollback is to keep the pruning cleanups but restore the non-Xtensa `fast_inv_approx()` branch to call `fast_inv(x)` or to use the previous two-correction sequence.

## Recommendation

Keep the accepted candidate if visual inspection of `mars` on Teensy 4.1 and `bunny_fig` on Pico2 is satisfactory. It gives a real gain where TGX currently spends time in perspective texturing:

- large win on Teensy `mars/movie` and `mars/falcon`;
- clear win on Pico2 `bunny_fig/gouraud_texture`;
- no meaningful code-size cost;
- no meaningful ESP code-size change;
- no regression in affine texture rendering, because affine texture skips the per-pixel perspective reciprocal.

Do not revive the fixed-point texture-coordinate path for these priority boards unless a new, visually stable formulation is found. The FPU-equipped targets benefit more from reducing divisions/reciprocals than from replacing all float interpolation.
