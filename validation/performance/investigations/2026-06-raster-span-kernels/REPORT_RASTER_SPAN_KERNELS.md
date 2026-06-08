# Raster Span Kernel Investigation Report

Date: 2026-06-06

## A. Executive Summary

No production TGX source patch is kept from this session.

The session produced a reusable display-free raster/span microbenchmark and ran
it on both target ARM boards:

- Teensy 4.1 / Cortex-M7 on `COM3`
- Pico2 / RP2350 / Cortex-M33 on `COM21`

The strongest exact result was a fixed-point incremental texture-coordinate
microbenchmark:

| Kernel | Teensy 4.1 mean | Pico2 mean | Verdict |
| --- | ---: | ---: | --- |
| nearest texture, fixed 8.8 increments | +53.1% | +28.0% | promising, not integrated |
| bilinear texture, fixed 8.8 increments | +15.5% | +11.9% | promising, not integrated |

This is a serious lead, but it is not yet safe as a production `Shaders.h`
patch. The microbenchmark uses controlled fixed-point texture coordinates,
while the real TGX shader path also handles float-derived coordinates,
perspective correction, clamp/wrap behavior, and exact bilinear fractional
semantics. Integrating it now would risk a hidden rendering behavior change.

The rest of the variants were either board-specific, weak, invalid diagnostics,
or already represented by existing production policy.

## B. Previous Context

This pass intentionally did not repeat earlier work:

- `TGX_INLINE_ZDIVIDE` is already committed and treated as baseline.
- Broad inline category macros were tested, rejected, and reverted.
- Coarse `TGX_INLINE_IMAGE=` produced a real Pico2 global gain, but was rejected
  because it hid severe wire/dots regressions.
- Broad Renderer3D/shader/layout perturbations were fragile.
- RGB565-only color kernels already yielded one accepted patch, `meanColor2`,
  but the hot 3D color kernels did not produce a robust source patch.

The new target was isolated span-level shader/rasterizer work, close enough to
real `uber_shader` loops to be informative, but separated from the full triangle
pipeline.

## C. Source / Path Audit

The main mapped production path is `src/Shaders.h::uber_shader(...)`.

The audit artifacts are:

- `results/span_callsite_inventory.csv`
- `notes/span_path_map.md`

The benchmark maps these source paths:

| Benchmark kernel | TGX path represented |
| --- | --- |
| `z_only` | z-buffer compare/write branch |
| `flat_noz` | no-z flat RGB565 output |
| `flat_z` | flat RGB565 output guarded by z-test |
| `gouraud_noz` / `gouraud_z` | untextured Gouraud color interpolation |
| `tex_nearest_noz` / `tex_nearest_z` | nearest texture sampling |
| `tex_bilinear_noz` | bilinear texture sampling with exact TGX bilinear color |
| `tex_mod_noz` / `tex_mod_z` | texture x color/light modulation with `RGB565::mult256` |
| `tex_nearest_inc` / `tex_bilinear_inc` | texture-coordinate increment strategy diagnostics |

The RP2350 production macro `TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` is
represented by pointer-increment variants for framebuffer and z-buffer access.

## D. Correctness Results

The final run produced 3564 benchmark rows.

All exact variants passed output comparison against their in-sketch reference
except the deliberately invalid diagnostic:

- `flat_z/allpass_fast` is exact for `allpass` spans only.
- It is intentionally incorrect for `mixed` and `allfail` spans.
- The parser recorded 88 invalid rows, all from that diagnostic variant.

No exact candidate had framebuffer or z-buffer mismatches.

Correctness artifacts:

- `results/correctness_summary.csv`
- `results/raster_span_results.csv`

## E. Performance Results

The full tables are in:

- `results/summary.csv`
- `results/short_long_summary.csv`
- `results/teensy41_results.csv`
- `results/pico2_results.csv`

### Pico2 / RP2350 Highlights

| Kernel / variant | Mean delta | Notes |
| --- | ---: | --- |
| `tex_nearest_inc/fixed8` | +28.0% | strong on all lengths |
| `tex_bilinear_inc/fixed8` | +11.9% | strong, especially long spans |
| `flat_noz/unroll4` | +3.9% | useful on long spans, regresses short spans to -4.1% |
| `flat_noz/store32` | +3.3% | useful mostly on longer spans |
| `flat_z/ptr` | +1.2% | weak but generally positive |
| `tex_bilinear_noz/scalar_channels` | +4.0% | not portable; Teensy regresses |
| `gouraud_noz/scalar_channels` | -2.1% | reject |
| `z_only/ptr` | -0.9% | reject |

Pico2 short/long behavior:

- `tex_nearest_inc/fixed8`: +24.4% short, +32.7% long.
- `tex_bilinear_inc/fixed8`: +9.4% short, +15.2% long.
- `flat_noz/unroll4`: +0.8% short, +7.9% long, but bad short outliers.

### Teensy 4.1 Highlights

| Kernel / variant | Mean delta | Notes |
| --- | ---: | --- |
| `tex_nearest_inc/fixed8` | +53.1% | very strong, especially long spans |
| `tex_bilinear_inc/fixed8` | +15.5% | strong on all lengths |
| `tex_mod_z/ptr` | +4.0% | positive on Teensy, not on Pico2 |
| `gouraud_noz/scalar_channels` | +2.8% | Pico2 regresses, reject globally |
| `flat_noz/unroll4` | +0.8% | weak and noisy |
| `tex_bilinear_noz/scalar_channels` | -6.9% | reject |
| `z_only/ptr` | -2.5% | reject |

Teensy short/long behavior:

- `tex_nearest_inc/fixed8`: +34.6% short, +78.3% long.
- `tex_bilinear_inc/fixed8`: +13.8% short, +17.7% long.
- `tex_mod_z/ptr`: +5.4% short, +2.2% long.

### Diagnostic Variants

`precomputed_texel` variants are intentionally not integration candidates.
They remove texture address/sample work and only show where time is being spent.

`flat_z/allpass_fast` is also diagnostic. It is fast when all pixels pass z, but
wrong for mixed or failing spans unless a cheap and exact all-pass test is added.

## F. Codegen Analysis

Codegen artifacts are in `codegen/`.

Important observations:

- `texNearestIncFixed8` is smaller than the float reference on both boards:
  0x94 vs 0xb8 on Pico2, and 0x98 vs 0xcc on Teensy.
- `texBilinearIncFixed8` is larger than the float reference on Pico2
  (0x1b8 vs 0xb8), but still much faster. The gain is from avoiding per-pixel
  float coordinate/floor work, not from code-size shrink.
- Pointer increments are not automatically better. They help in some combined
  color/z paths and hurt z-only loops.
- 32-bit and unrolled RGB565 stores help Pico2 long spans but are not portable
  enough to justify a generic source patch.

See `codegen/CODEGEN_SUMMARY.md` for symbol sizes and build-size details.

## G. Integration Decision

No production patch was attempted.

The fixed-point texture-coordinate result is too promising to ignore, but too
semantic-sensitive to integrate from this microbench alone. The next step should
be a dedicated shader texture-coordinate pass that proves equivalence for:

- perspective-correct and non-perspective paths;
- texture clamp/wrap behavior;
- nearest and bilinear rounding;
- edge coordinates;
- real mesh textured benchmark rows.

The pointer and store variants are either already reflected in the RP2350
incremental pixel pointer policy, too weak, or board-specific.

## H. TGX Benchmark Sanity

No full TGX benchmark sanity run was performed because no production TGX source
patch survived the microbenchmark phase.

The hardware upload/capture path was validated with macro probes and the
microbenchmark was run on both target boards.

## I. Final Source Diff

At report creation, production TGX source files are unchanged. The working tree
contains only new investigation artifacts under:

```text
validation/performance/investigations/2026-06-raster-span-kernels/
```

Final checks were run after writing this report; see the final assistant
message for the exact command outputs.

## J. Rejected Candidates

| Candidate | Reason |
| --- | --- |
| z-only pointer loop | Regressed on both boards. |
| branch-reduced z select/write | Regressed on both boards. |
| flat 32-bit store | Good on Pico2 long spans but not portable and not useful on Teensy. |
| flat unroll4 | Good on Pico2 long spans but weak/noisy on Teensy and bad short-span outliers. |
| `flat_z/allpass_fast` | Incorrect for mixed/failing spans; diagnostic only. |
| Gouraud scalar channel rewrite | Helps Teensy but regresses Pico2. |
| bilinear scalar channel rewrite | Helps Pico2 but regresses Teensy. |
| texture modulation pointer z | Helps Teensy but slightly regresses Pico2. |
| precomputed texel variants | Diagnostic only; remove real texture address/sample work. |

## K. Next Steps

Recommended next session:

1. Build a dedicated texture-coordinate microbenchmark mirroring
   `uber_shader` semantics more closely.
2. Test fixed-point coordinate increments against exact production shader output
   for both nearest and bilinear paths.
3. If exactness holds for a subset, integrate only that subset behind a small,
   reviewable shader helper or architecture guard.
4. Keep all wire/dots and short-span guard tests in the validation set to avoid
   repeating the previous layout/regression trap.
