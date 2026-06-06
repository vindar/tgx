# Session Start: Texture Coordinate Fixed-Point Investigation

Date: 2026-06-06

Branch: `feature/multi-directional-lights`

Commit:

```text
3b364a8743b59ac058b7c3c83bfdc83298f3226f
```

Initial source diff:

```text
git status --short: clean
git diff --stat: clean
```

## Target Boards

| Board | Port | Role |
| --- | --- | --- |
| Teensy 4.1 / Cortex-M7 | `COM3` | ARM runtime target |
| Pico2 / RP2350 / Cortex-M33 | `COM21` | ARM runtime target |

Pico W / RP2040 is intentionally ignored. Core2/CoreS3 are compile sanity
targets only if a generic production source patch survives.

## Tools

Use the reusable upload/capture tooling in:

```text
validation/performance/tools/
```

In particular:

- `upload_and_capture.ps1`
- `run_macro_probe.ps1` / `TgxMacroProbe`

No ad-hoc serial capture should be used.

## Previous Result Carried Forward

The previous raster span microbenchmark found the following exact isolated
kernel gains:

| Kernel | Teensy 4.1 mean | Pico2 mean |
| --- | ---: | ---: |
| nearest texture with fixed 8.8 coordinate increments | +53.1% | +28.0% |
| bilinear texture with fixed 8.8 coordinate increments | +15.5% | +11.9% |

No production patch was integrated because that benchmark used a controlled
8-bit fractional coordinate model. The production shader has additional
semantics:

- perspective-correct texture interpolation in `SHADER_PERSPECTIVE`;
- affine texture interpolation in `SHADER_ORTHO`;
- nearest and bilinear sampling with different integer/floor behavior;
- texture clamp and power-of-two wrap;
- arbitrary texture sizes for clamp and power-of-two assumptions for wrap;
- color modulation after sampling.

## Current Goal

Determine whether fixed-point incremental texture coordinates can be used
safely in the real TGX shader path, at least in a restricted fast path.

Pixel-perfect equality is not required in every case, but differences must be
quantified. A production patch is acceptable only if it has a clear safe subset,
bounded visual error, and a fallback for risky cases.

## Planned Tests

1. Audit the exact texture-coordinate semantics in `src/Shaders.h`.
2. Build a display-free equivalence/performance sketch that compares:
   - current production-like float coordinate sampling;
   - fixed-point candidates with 8, 10, 12, and 16 fractional bits;
   - nearest and bilinear sampling;
   - clamp and wrap;
   - interior and edge coordinates;
   - affine and perspective-like coordinate patterns.
3. Measure mismatch count, channel errors, RMSE, hashes, and speed.
4. Attempt a production patch only after the equivalence data identifies a safe
   subset.

## Visual Correctness Policy

Small rounding differences may be acceptable if:

- they do not create out-of-bounds, wrap/clamp, or edge artifacts;
- nearest sampling does not shift texels systematically;
- bilinear errors are bounded and visually smooth;
- max channel error is usually within one RGB565 quantization level;
- mean error is very small;
- representative checker, stripe, gradient, random, and edge cases are tested.
