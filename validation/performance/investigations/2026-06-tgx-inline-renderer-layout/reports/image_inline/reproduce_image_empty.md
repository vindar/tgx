# Reproduction: Coarse `TGX_INLINE_IMAGE=`

Run: `pico2_image_repro_image_empty_pico2_bench_pico2`  
Board: Pico2 / RP2350 on `COM21`  
Build flag: `-DTGX_INLINE_IMAGE=`  
Status: `SUCCESS`, 3 global scores, 162 subtests parsed.

## Global Scores

| Score | Baseline | Reproduction | Delta |
| --- | ---: | ---: | ---: |
| 1 | 23.04 | 24.05 | +4.38% |
| 2 | 16.545 | 17.31 | +4.62% |
| 3 | 14.01 | 14.63 | +4.43% |

## Subtest Summary

| Metric | Value |
| --- | ---: |
| Mean delta | +2.636% |
| Median delta | +1.338% |
| Improved > +1% | 88 |
| Neutral within +/-0.5% | 26 |
| Regressions <= -1% | 29 |
| Regressions <= -3% | 24 |
| Worst regression | -19.62% `Wire rib tri thick` |
| Best improvement | +63.92% `R2D2 (far)` |

## Worst Regressions

| Delta | Renderer size | Subtest |
| ---: | --- | --- |
| -19.62% | 100 x 100 | `Wire rib tri thick` |
| -17.31% | 200 x 200 | `Wire rib tri thick` |
| -16.28% | 320 x 240 | `Wire rib tri thick` |
| -15.11% | 320 x 240 | `Point cloud dots` |
| -14.97% | 100 x 100 | `Wire sphere thick` |
| -14.86% | 200 x 200 | `Point cloud dots` |
| -13.99% | 200 x 200 | `Wire sphere thick` |
| -13.98% | 100 x 100 | `Point cloud dots` |
| -13.54% | 320 x 240 | `Wire sphere thick` |
| -12.61% | 100 x 100 | `Wire rib strip thick` |

## Best Improvements

| Delta | Renderer size | Subtest |
| ---: | --- | --- |
| +63.92% | 320 x 240 | `R2D2 (far)` |
| +60.92% | 320 x 240 | `R2D2 (farclip)` |
| +56.22% | 200 x 200 | `R2D2 (far)` |
| +54.68% | 200 x 200 | `R2D2 (farclip)` |
| +35.35% | 100 x 100 | `R2D2 (farclip)` |
| +27.45% | 100 x 100 | `R2D2 (far)` |
| +21.71% | 100 x 100 | `Sphere direct GOUR` |
| +17.53% | 100 x 100 | `Bunny (farclip)` |
| +16.88% | 200 x 200 | `Bunny (farclip)` |
| +16.87% | 100 x 100 | `Bunny TEX_BILINEAR` |

## Conclusion

The previous signal was reproduced. It is not a measurement artifact: the global gain is stable and large. It is also unsafe: the same run reproduces severe wire/dots regressions. The pattern strongly suggests a codegen/layout trade, not a universally faster Image helper policy.
