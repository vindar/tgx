# ARM DSP / RGB565 Hot 3D Report

## A. Executive Summary

Final recommendation: keep no new production source patch from this session.

The previous committed `meanColor(RGB565, RGB565)` packed average remains the
only retained RGB565 optimization. This session built and ran a display-free
hot-3D RGB565 microbenchmark on Teensy 4.1 and Pico2, audited real 3D call
sites, and tested candidate kernels for bilinear texture interpolation,
Gouraud-style triangle interpolation, RGB565 modulation, RGBf conversion, and
span forms.

No candidate satisfied all integration requirements:

- exact output;
- meaningful speedup on a kernel used in 3D;
- no slowdown on the other ARM board or a clean guarded path;
- small maintainable source diff.

Best useful-looking result:

- Pico2 `interpolateColorsBilinear` scalar-channel microbench variant measured
  +3.456%, exact.
- However, moving the same idea into `src/Color.h` did not transfer:
  +0.055% direct kernel and -0.134% long span, effectively noise.

Fastest new rejected result:

- `bilinearPackedMasks` measured +12.523% on Pico2 direct bilinear and about
  +7.6% on long spans, but it was not exact, with 492 mismatches in the direct
  random test.

Boards tested:

- Teensy 4.1 / Cortex-M7 on `COM3`.
- Pico2 / RP2350 / Cortex-M33 on `COM21`.

## B. Previous Session Carry-Over

Accepted before this session:

- `src/Color.h`
- `meanColor(RGB565, RGB565)` now uses the exact packed average:
  `(a & b) + (((a ^ b) & 0xF7DEu) >> 1)`

Rejected before this session and not repeated:

- broad renderer/layout experiments;
- broad inline category macros;
- coarse `TGX_INLINE_IMAGE=`;
- non-exact naive ARM `UHADD16` RGB565 average variants.

This session adds a 3D call-site audit and hot-kernel-specific microbenchmark
data for shader-relevant RGB565 operations.

## C. Call-Site Audit

See:

- `results/color_callsite_inventory.csv`
- `notes/color_callsite_map.md`

Priority ranking:

| Priority | Kernel | 3D relevance |
| ---: | --- | --- |
| 1 | `interpolateColorsBilinear(RGB565, ...)` | Per-pixel bilinear texture path in `src/Shaders.h`. |
| 2 | `RGB565::mult256(...)` | Per-pixel texture x lighting / color modulation in `src/Shaders.h`. |
| 3 | `interpolateColorsTriangle(RGB565, ...)` | Per-pixel untextured Gouraud color interpolation in `src/Shaders.h`. |
| 4 | `RGBf -> RGB565` | Mostly setup/vertex/material conversion, not usually RGB565 per-pixel shader output. |
| 5 | `meanColor4` | Mostly image/downsample support, not a main 3D path. |

The audit confirmed that `meanColor2`, although worth keeping, is not the main
3D renderer bottleneck.

## D. Correctness Results

Exact variants:

- `interpolateBilinear scalar_channels`
- `interpolateTriangle scalar_channels`
- `interpolateTriangle packed_copy`
- `mult256_rgb shift_pack`
- `mult256_rgba shift_pack`
- `RGBf -> RGB565 scalar_pack`
- `meanColor4 packed_lane_sum`

Invalid or approximate-only variants:

- Previous naive `arm_uhadd16` mean variants: channel-boundary errors.
- Previous bilinear `two_stage_approx`: changed rounding.
- New `bilinearPackedMasks`: faster but incorrect.

Key invalid rows are listed in:

- `results/correctness_summary_all_runs.csv`

The new `bilinearPackedMasks` candidate failed exactness:

| Kernel | Variant | Board | Mismatches | Speed vs ref |
| --- | --- | --- | ---: | ---: |
| `interpolateBilinear` | `packed_masks` | Pico2 | 492 | +12.523% |
| `span_bilinear_n320_o0` | `packed_masks_loop` | Pico2 | 309 | +7.637% |
| `span_bilinear_n320_o1` | `packed_masks_loop` | Pico2 | 299 | +7.853% |

Reason: multiplying already-shifted RGB565 fields by 16-bit bilinear weights
does not preserve the same channel rounding as multiplying each channel and
then packing. This is not merely an R/B ordering issue.

## E. Performance Results

Baseline candidate deltas against current TGX reference:

| Board | Kernel | Variant | Correct | Delta vs ref | ns/op |
| --- | --- | --- | ---: | ---: | ---: |
| Teensy 4.1 | `interpolateBilinear` | `scalar_channels` | yes | -2.196% | 75.058 |
| Teensy 4.1 | `interpolateTriangle` | `scalar_channels` | yes | -14.172% | 70.686 |
| Teensy 4.1 | `interpolateTriangle` | `packed_copy` | yes | 0.000% | 60.669 |
| Teensy 4.1 | `mult256_rgb` | `shift_pack` | yes | 0.000% | 31.700 |
| Teensy 4.1 | `mult256_rgba` | `shift_pack` | yes | -0.012% | 31.700 |
| Teensy 4.1 | `rgbf_to_rgb565` | `scalar_pack` | yes | -15.736% | 31.708 |
| Teensy 4.1 | `meanColor4` | `packed_lane_sum` | yes | -6.522% | 38.372 |
| Pico2 | `interpolateBilinear` | `scalar_channels` | yes | +3.456% | 588.989 |
| Pico2 | `interpolateTriangle` | `scalar_channels` | yes | -12.859% | 519.646 |
| Pico2 | `interpolateTriangle` | `packed_copy` | yes | +1.526% | 446.018 |
| Pico2 | `mult256_rgb` | `shift_pack` | yes | -32.387% | 247.688 |
| Pico2 | `mult256_rgba` | `shift_pack` | yes | -24.264% | 247.696 |
| Pico2 | `rgbf_to_rgb565` | `scalar_pack` | yes | +5.495% | 247.696 |
| Pico2 | `meanColor4` | `packed_lane_sum` | yes | +99.976% | 194.126 |

Interpretation:

- `mult256` is already strong in TGX. The shift/pack rewrite is much slower on
  Pico2 and neutral on Teensy.
- `interpolateTriangle` current packed implementation is already close to best.
  The apparent `packed_copy` Pico2 gain is a microbench wrapper/codegen effect,
  not a source change.
- `RGBf -> RGB565` and `meanColor4` have Pico2 gains, but they either regress
  Teensy or are not hot enough in the 3D renderer to justify generic changes.
- `interpolateBilinear scalar_channels` was the only relevant exact Pico2 gain,
  so it was tested as a guarded source candidate.

## F. Codegen Analysis

See:

- `codegen/CODEGEN_SUMMARY.md`
- `codegen/selected_symbol_summary.csv`
- `codegen/pico2_baseline_objdump.txt`
- `codegen/pico2_packed_masks_objdump.txt`
- `codegen/teensy41_baseline_objdump.txt`

Selected observations:

- Pico2 baseline `refBilinear` is `0xb0` bytes.
- Pico2 separate `bilinearScalar` helper is `0xb2` bytes.
- Pico2 experimental `bilinearPackedMasks` is only `0x8c` bytes and faster, but
  not exact.
- Pico2 `multShiftPack` is compact (`0x28` bytes), but the measured runtime is
  much worse than TGX `mult256`; smaller code alone did not produce better
  scheduling or lower arithmetic cost.
- The current TGX triangle interpolation compiles into a compact packed path
  with integer division and multiply-accumulate style code. The scalar-channel
  rewrite increases cost substantially.

No CMSIS/DSP intrinsic variant produced a maintainable exact speedup for these
hot 3D RGB565 kernels in this session. The compiler-generated scalar/packed
code is already difficult to beat.

## G. Integration Decision

Production patch attempted experimentally:

- `patches/integrate_hot3d_color_candidate.patch`
- `patches/rp2350_bilinear_plain_inline_candidate.patch`
- `patches/rp2350_bilinear_noinline_candidate.patch`

These explored an RP2350-only source rewrite of `interpolateColorsBilinear`.

Pico2 source-control results against baseline `current_tgx`:

| Candidate | Kernel | Delta vs baseline | Result |
| --- | --- | ---: | --- |
| shift/mask body, still inline | direct bilinear | +0.055% | noise, rejected |
| shift/mask body, still inline | long span n320 | -0.134% | noise/regression, rejected |
| plain `inline` without forced `TGX_INLINE` | direct bilinear | -0.048% | noise, rejected |
| plain `inline` without forced `TGX_INLINE` | long span n320 | -0.311% | noise/regression, rejected |
| RP2350 `noinline` | direct bilinear | -17.295% | rejected |
| RP2350 `noinline` | long span n320 | -16.237% | rejected |

The microbench-only `scalar_channels` gain did not transfer into an actual
`Color.h` source change. The likely cause is different inlining/codegen around
the benchmark helper versus the production `TGX_INLINE` function body.

Final source decision:

- no production source change retained;
- keep only investigation artifacts.

## H. TGX Benchmark Sanity

No full TGX renderer benchmark sanity was run for a retained source patch,
because no source patch survived the microbenchmark and correctness gates.

This follows the session rule: do not spend full benchmark cycles on failed
microbench variants.

## I. Final Source Diff

Final checks:

```text
git status --short
?? validation/performance/investigations/2026-06-arm-dsp-rgb565-hot3d/

git diff --stat
<empty>

git diff --name-only
<empty>

git diff --check
<empty>
```

Only new validation/performance investigation artifacts should remain.

## J. Rejected Candidates

| Patch / candidate | Reason rejected |
| --- | --- |
| `integrate_hot3d_color_candidate.patch` | RP2350 guarded bilinear body rewrite did not move production `current_tgx` timing beyond noise. |
| `rp2350_bilinear_plain_inline_candidate.patch` | Removing forced inline did not improve the direct or long-span kernel. |
| `rp2350_bilinear_noinline_candidate.patch` | Exact but about -17% on Pico2 due call overhead / lost inlining. |
| `microbench_bilinear_packed_masks_candidate.patch` | Fast on Pico2 but not exact; hundreds of mismatches. |
| `mult256 shift_pack` | Exact but -24% to -32% on Pico2. |
| `interpolateTriangle scalar_channels` | Exact but -13% Pico2 and -14% Teensy. |
| `RGBf scalar_pack` | Exact but Teensy regressed heavily and it is not a main per-pixel RGB565 shader path. |
| `meanColor4 packed_lane_sum` | Huge Pico2 gain but Teensy regression and low 3D relevance. |

## K. Next Steps

Recommended next targets:

1. Rasterizer span microbench for real shader inner-loop structure, not isolated
   color helpers.
2. Texture sampling address/weight generation microbench on RP2350.
3. ESP32-S3 SIMD/RGB565 investigation as a separate architecture-specific pass.
4. RP2350 Vec3/Mat4/FMA microbench if lighting or transform paths remain the
   dominant bottleneck.
5. Image/span copy/fill kernels only if a call-site audit shows they dominate a
   real workload.
