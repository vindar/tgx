# TGX affine texture fixed-point investigation

Date: 2026-06-17

Branch: `affine-texture-fixedpoint`

Status: rejected and removed from source after measurements.

## Scope

This investigation tests the idea of using fixed-point arithmetic only for texture coordinates `tx` and `ty` on non perspective-correct texture paths:

- perspective projection with `SHADER_TEXTURE_AFFINE`;
- orthographic texturing, which already uses linear texture interpolation.

The perspective-correct texture path is intentionally unchanged.

## Implementation Tested

The tested implementation keeps the existing floating-point setup for texture gradients and scanline start coordinates. This is important for correctness: an earlier attempt quantized barycentric texture coefficients too early, then multiplied them by large edge-function values. That produced visible texture instability even with Q16 precision.

The current tested fixed path is therefore:

- setup: compute `T21x`, `T31x`, `T21y`, `T31y`, `dtx`, `dty` as before;
- setup: convert only `dtx` and `dty` to fixed point when the fixed path is enabled;
- per scanline: compute the existing float start `tx` and `ty`, then convert that start to fixed point;
- per pixel: increment `tx_fixed` and `ty_fixed` with integer adds;
- nearest: use fixed-point integer texel coordinates;
- RGB565 bilinear fast paths: use integer 8-bit bilinear fractions.

No 64-bit arithmetic is used.

The feature is controlled by:

- `TGX_SHADER_AFFINE_TEXTURE_FIXEDPOINT`, default `0`;
- `TGX_SHADER_AFFINE_TEXTURE_FIXEDPOINT_BITS`, default `16`.

`0` meant the existing float path was used. `2` was used in measurements as an enabled fixed-point value. After review, these fixed-point macros and shader paths were removed from the source tree.

## Optimization Attempts

Accepted for testing:

- split fixed/float texture sampling branches in `uber_shader_inline` to avoid mixed live variables in the pixel loop;
- keep Q16 start conversion per scanline for correctness;
- use RGB565 integer bilinear helpers for fixed bilinear sampling.

Rejected during testing:

- row-base incremental scanline start for fixed texturing.

The row-base attempt compiled and ran, but worsened Teensy `test-texture` fixed timings. Example: `bob_tex_nearest` went from 13556.6 us/frame to 13749.8 us/frame, so it was removed.

## Validation Matrix

Benchmarks were run with the robust `validation/performance/tools/upload_and_capture.ps1` tool.

Boards:

- Teensy 4.1 + ILI9341: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`
- Pico2 + ILI9341: `rp2040:rp2040:rpipico2:opt=Fast`
- Core2: `esp32:esp32:m5stack_core2`
- CoreS3: `esp32:esp32:m5stack_cores3`

Runs:

- standard `examples/benchmark`, float vs fixed, all four boards;
- Teensy `test-texture` affine, float vs fixed;
- Core2/CoreS3 temporary `donkeykong` affine copy, float vs fixed;
- Pico2 temporary `bunny_fig` affine copy, float vs fixed.

All benchmark and example runs completed successfully.

## Result Files

Compact result files kept here:

- `benchmark_global_scores_fixed_vs_float.csv`
- `benchmark_texture_subtests_fixed_vs_float.csv`
- `real_examples_affine_fixed_vs_float.csv`
- `build_size_summary.csv`
- `run_status_summary.csv`
- `command_manifest.csv`

Column convention:

- `*_us`: microseconds per frame; lower is better.
- `*_fps`: frames per second; higher is better.
- `delta_us = fixed - float`; negative means fixed is faster.
- `delta_pct` on time columns: negative means fixed is faster.
- `delta_score` on benchmark global scores: positive means fixed is better.

## Main Results

Standard benchmark global scores are essentially unchanged. The largest global score movement is within about 0.25%, which is noise-level for this suite.

Texture-specific benchmark result:

- perspective texture subtests are unchanged as expected, generally within about +/-0.6%;
- `Bunny ORTHO TEX` is consistently slower with fixed-point coordinates:
  - Teensy 4.1: +1.44% to +2.33% frame time;
  - Pico2: +1.24% to +2.64% frame time;
  - Core2: +2.30% to +4.51% frame time;
  - CoreS3: +2.51% to +4.84% frame time.

Real affine examples:

- Teensy `test-texture`, textured scenes: +0.34% to +0.91% frame time.
- Core2 `donkeykong` affine `gouraud_texture`: +4.64% frame time.
- CoreS3 `donkeykong` affine `gouraud_texture`: +6.14% frame time.
- Pico2 `bunny_fig` affine `gouraud_texture`: +0.67% frame time.

Build size impact when fixed is enabled:

- benchmark Teensy: +1216 bytes flash code;
- benchmark Pico2: +1000 bytes program;
- benchmark Core2: +388 bytes program;
- benchmark CoreS3: +396 bytes program;
- real examples: +108 to +384 bytes on ESP/Teensy, +144 bytes on Pico2.

## Analysis

The fixed-point path is visually correct after avoiding early coefficient quantization, but it is not faster on the tested hardware.

The likely reason is that the correct implementation cannot remove the float scanline setup: it still needs accurate float `tx` and `ty` starts per scanline before converting to Q16. On meshes made of many short spans, that conversion overhead and extra integer sampling work outweigh the saved per-pixel float increment/cast work.

On Teensy 4.1, the FPU is fast enough that replacing float increments with integer increments does not help. On Core2/CoreS3, the extra fixed-point path is clearly slower in real affine texture scenes. Pico2 is closest to neutral, but still slightly slower.

The rejected row-base attempt tried to reduce per-scanline setup, but added enough arithmetic/register pressure to make the result worse on Teensy.

## Recommendation

Do not enable the fixed-point affine texture coordinate path as a TGX optimization. The measured path was removed from the code after this investigation.

For TGX optimization, the better accepted path remains the affine texturing mode itself (`SHADER_TEXTURE_AFFINE`) using the float coordinate interpolation already implemented. It gives the desired visual quality/performance tradeoff without this fixed-point subpath.

If this idea is revisited later, it needs a new hypothesis, for example a specialized large-span textured quad/sprite workload where per-pixel coordinate increments dominate and triangle setup is not the bottleneck.
