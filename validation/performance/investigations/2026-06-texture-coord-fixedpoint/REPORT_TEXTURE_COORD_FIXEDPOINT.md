# Texture Coordinate Fixed-Point Report

## A. Executive Summary

Final recommendation: do not keep a production source patch yet, but the fixed
texture-coordinate idea is real and worth integrating in a dedicated follow-up.

The safe candidate is a restricted affine/interior path:

- only `USE_TEXTURE && USE_ORTHO`;
- not perspective-correct paths;
- whole span safely inside the texture;
- fixed 16 fractional-bit coordinate accumulators;
- avoid very short spans, with `len >= 4` as minimum and `len >= 8` as a safer threshold.

No `src/` files were modified. The final working tree contains only validation
artifacts under this investigation folder.

## B. Previous Span Microbench Carry-Over

The earlier raster span kernel benchmark found large speedups from fixed-point
incremental coordinates:

| Kernel | Teensy 4.1 | Pico2 |
| --- | ---: | ---: |
| nearest texture, fixed 8.8 | +53.1% | +28.0% |
| bilinear texture, fixed 8.8 | +15.5% | +11.9% |

That was not integrated because the model did not prove compatibility with real
TGX texture semantics: perspective correction, nearest truncation, bilinear
flooring, clamp/wrap, arbitrary texture sizes, and edge behavior.

## C. Texture-Coordinate Semantics Audit

The production 3D path is `uber_shader()` in `src/Shaders.h`.

Important findings:

- Texture coordinates are stored as `fVec2`.
- For `USE_ORTHO`, texture coordinates are affine in screen space. The shader
  computes `tx`/`ty` per scanline and increments them by `dtx`/`dty` per pixel.
- For perspective paths, the shader computes `xx = tx * fast_inv(cw_p)` and
  `yy = ty * fast_inv(cw_p)` per pixel. This is rational, not affine; a simple
  fixed incremental coordinate is not exact.
- Nearest sampling uses `(int)(xx)` and `(int)(yy)`, so positive values truncate
  like floor but negative fractional values truncate toward zero.
- Bilinear sampling uses `lfloorf(xx)`, `lfloorf(yy)`, then `ax = xx - floor`.
- Clamp uses `shaderclip`.
- Wrap is power-of-two masking with `& (size - 1)`, not generic modulo.
- Coordinates are scaled by texture width/height, not `width - 1` / `height - 1`.

The safe subset is therefore affine, non-perspective texture spans whose whole
sample footprint remains inside the texture. Perspective fixed approximation is
not safe.

## D. Equivalence/Error Benchmark

Created display-free sketch:

```text
validation/performance/investigations/2026-06-texture-coord-fixedpoint/sketches/TextureCoordEquivalenceBench/
```

It compares production-like float sampling against fixed candidates for nearest
and bilinear sampling. It reports hashes, mismatch counts, maximum RGB channel
error, mean error, RMSE, and timings.

Validated captures:

| Board | Label | Lines | Status |
| --- | --- | ---: | --- |
| Teensy 4.1 | `texcoord_eq_teensy41_run5_full_lengths` | 4312 | SUCCESS |
| Pico2 | `texcoord_eq_pico2_run5_full_lengths` | 4312 | SUCCESS |

Tested span lengths:

```text
1, 2, 3, 4, 5, 8, 16, 32, 64, 128, 320
```

Textures included checkerboard, gradient, random, stripes, power-of-two wrap,
and non-power-of-two clamp cases.

## E. Fixed-Point Precision Comparison

The general `fixed_round` path was tested with 8, 10, 12, and 16 fractional bits.
It is not suitable as a direct replacement:

- affine bilinear F16 remained slower overall on Pico2 because edge/clamp/floor
  handling dominates;
- perspective approximation produced large errors;
- nearest F16 had a few boundary-sensitive one-texel errors outside the
  restricted safe subset.

The useful design is not the general path; it is the restricted interior F16 path.

For `len >= 4`:

| Board | Mode | Exact | Mean Speedup | Worst Speedup | Max Channel Error |
| --- | --- | ---: | ---: | ---: | ---: |
| Teensy 4.1 | nearest | 100.0% | +99.42% | +8.84% | 0 |
| Teensy 4.1 | bilinear | 93.64% | +47.19% | +12.24% | 1 |
| Pico2 | nearest | 100.0% | +78.67% | +7.04% | 0 |
| Pico2 | bilinear | 93.64% | +31.15% | +5.05% | 1 |

For `len >= 8`:

| Board | Mode | Exact | Mean Speedup | Worst Speedup | Max Channel Error |
| --- | --- | ---: | ---: | ---: | ---: |
| Teensy 4.1 | nearest | 100.0% | +138.62% | +48.94% | 0 |
| Teensy 4.1 | bilinear | 91.25% | +56.04% | +28.63% | 1 |
| Pico2 | nearest | 100.0% | +108.48% | +41.98% | 0 |
| Pico2 | bilinear | 91.25% | +38.50% | +20.08% | 1 |

The non-exact bilinear rows were long subtexel cases. They had:

- max absolute channel error: 1;
- no pixels above one RGB565 quantization step;
- mismatch percentage generally around 1.25% to 2.5% in the hardest synthetic rows.

## F. Candidate Matrix

| Candidate | Decision | Reason |
| --- | --- | --- |
| General `fixed_round` | rejected | Not consistently faster; perspective and boundary behavior not safe. |
| Perspective fixed approximation | rejected | Large mismatch percentages and max channel errors up to 63. |
| `fixed16_interior_nearest` | candidate, not integrated | Exact and fast for affine interior spans once setup is amortized. |
| `fixed16_interior_bilinear` | candidate, not integrated | Fast with bounded RGB565 error, but approximate output needs real render validation. |

## G. Production Patch

No production patch was applied.

Reason: the useful candidate is not a drop-in replacement. A correct integration
requires restructuring the texture part of `uber_shader()` around a scanline-level
safety test:

- compute actual span end before the pixel loop;
- reject `len < 4` or possibly `len < 8`;
- reject perspective paths;
- reject edge-risk spans or fallback when the bilinear footprint leaves the texture;
- keep current float path as fallback.

This is feasible but should be done as a focused `src/Shaders.h` patch with real
render-hash and benchmark validation. Leaving an unvalidated central shader patch
would be riskier than useful.

## H. Codegen Analysis

See `codegen/CODEGEN_SUMMARY.md`.

Main points:

- `renderCandidateInterior(...)` is substantially smaller than the float
  reference path.
- The fixed interior loop replaces per-pixel float coordinate conversion/floor
  work with integer shifts/index arithmetic.
- The setup still uses float-to-fixed conversion, explaining why `LEN=1` and
  other tiny spans regress.
- The speedup is from real loop work reduction, not just layout noise.

## I. Final Source Diff

No production source files were changed.

Final checks are recorded in the session final output. The only intended
changes are investigation artifacts under:

```text
validation/performance/investigations/2026-06-texture-coord-fixedpoint/
```

## J. Rejected Candidates

- General fixed path: rejected due speed and semantics.
- Fixed perspective approximation: rejected due severe visual/error metrics.
- Edge/wrap unrestricted fixed path: rejected because negative/wrap and edge
  behavior is too easy to change.

## K. Next Steps

Recommended next session:

1. Implement a guarded `src/Shaders.h` prototype for `USE_TEXTURE && USE_ORTHO`
   interior spans only.
2. Start with exact nearest.
3. Add bilinear only after render-hash image-difference tests accept the
   observed one-level RGB565 errors.
4. Run TGX benchmark rows for texture nearest/bilinear on Teensy and Pico2.
5. Run real textured examples: `test-texture`, `test-shading`, `borg_cube`,
   `scream`, and Pico2 `bunny_fig`.
