# TGX Image Inline Investigation

Date: 2026-06-06
Branch: `feature/multi-directional-lights`
Start commit: `5fbe8cf`

## A. Executive Summary

Final recommendation: do not keep an Image inline optimization from this pass, and do not keep the broad neutral category macro layer in source.

The promising Pico2 signal was reproduced: compiling with coarse `TGX_INLINE_IMAGE=` improves the three global benchmark scores by about +4.4% on RP2350. However, the same candidate creates severe local regressions in important wire and point paths, including `Wire rib tri thick` at -19.62% and `Point cloud dots` around -15%.

I split the Image inline policy into narrower experimental categories and tested accessors, pixel read/write helpers, public pixel API wrappers, wire helpers, span dispatchers, triangle coordinate helpers, font helpers, and mesh-safe combinations. The result is clear:

- The gains come mostly from relaxing public draw pixel wrappers and/or internal `_drawPixel` paths.
- Those are also the paths that damage wire/dots and point-cloud performance.
- When the unsafe paths are protected, the global gain disappears and the candidate becomes effectively neutral.

Final source files changed: none. The previous broad neutral macro layer was saved as a rejected patch and reverted from the working tree.

Key artifacts:

- `tmp/image_inline_investigation/SESSION_START.md`
- `tmp/image_inline_investigation/hardware_ready.md`
- `tmp/image_inline_investigation/image_inline_inventory.csv`
- `tmp/image_inline_investigation/reports/image_path_map.md`
- `tmp/image_inline_investigation/pico2_candidate_summary.csv`
- `tmp/image_inline_investigation/pico2_subtest_matrix.csv`
- `tmp/image_inline_investigation/reports/codegen_image_summary.md`
- `tmp/image_inline_investigation/patches/`

## B. Starting State

The session started from the uncommitted output of the previous inline-granularity investigation. The tracked source diff added category macros in:

- `src/tgx_config.h`
- `src/Vec2.h`
- `src/Vec3.h`
- `src/Vec4.h`
- `src/Color.h`
- `src/Misc.h`
- `src/Shaders.h`
- `src/Image.h`

Those macros all fell back to `TGX_INLINE`, so the layer was intended to be behavior-neutral. It was useful as experiment scaffolding, but not justified as final source because no category override survived benchmark screening.

The full WIP source patch was saved before reverting:

- `tmp/image_inline_investigation/patches/rejected_full_source_wip.patch`

## C. Hardware and Capture Readiness

Hardware was quickly revalidated with `tmp/hardware_validation/tools/upload_and_capture.ps1` and `TgxMacroProbe`.

| Board | Port | Status |
| --- | --- | --- |
| Core2 | `COM5` | Upload/capture/probe OK |
| CoreS3 | `COM10` | Upload/capture/probe OK |
| Pico2 / RP2350 | `COM21` | Upload/capture/probe OK |
| Teensy 4.1 | `COM3` | Upload/capture/probe OK |
| Pico W / RP2040 | `COM28` | Not retried; compile-only due previous UF2/picotool upload failure |

Details are in `tmp/image_inline_investigation/hardware_ready.md`.

## D. Reproduction of Coarse `TGX_INLINE_IMAGE=`

Patch:

- `tmp/image_inline_investigation/patches/reproduce_image_empty_pico2.patch`

Run:

- Board: Pico2 / RP2350
- Port: `COM21`
- Flag: `-DTGX_INLINE_IMAGE=`
- Status: `SUCCESS`
- Parsed: 3 global scores, 162 subtests

Global scores:

| Score | Baseline | Candidate | Delta |
| --- | ---: | ---: | ---: |
| 1 | 23.04 | 24.05 | +4.38% |
| 2 | 16.545 | 17.31 | +4.62% |
| 3 | 14.01 | 14.63 | +4.43% |

Subtest summary:

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

This reproduced the previous signal. It is real, but unsafe.

## E. Image Helper and Path Inventory

The inventory is in:

- `tmp/image_inline_investigation/image_inline_inventory.csv`
- `tmp/image_inline_investigation/reports/image_path_map.md`

Main Image helper families:

| Family | Representative helpers | Main paths |
| --- | --- | --- |
| Accessors | `isValid`, `width`, `height`, `stride`, `data`, `operator()` | Setup, clipping, direct image access |
| Public pixel wrappers | `drawPixel`, `drawPixelf`, `readPixel`, `readPixelf` | Direct pixel API, examples, point/dot-like paths |
| Internal pixel helpers | `_drawPixel`, `_readPixel` | Primitive internals, line endpoints, text/shape helpers |
| Wire helpers | `_bseg_update_pixel`, `_drawSeg` | Wire cube, wire rib, thick/AA wire |
| Span wrappers | `_drawFastVLine`, `_drawFastHLine` | Fill, rect, circle, span paths |
| Triangle coordinate helpers | `_coord_texture`, `_coord_viewport` | 2D textured/gradient triangles |
| Font helper | `_drawFontPixel` | Text rendering |

Important attribution: the best mesh-heavy gains do not come from direct textured 3D Image calls. Textured mesh rendering is primarily Renderer3D/shader code. Image inline changes affect those rows indirectly through template code size, function placement, and layout.

## F. Candidate Matrix

All candidate patches are saved under `tmp/image_inline_investigation/patches/`.

| Candidate | Pico2 result | Verdict |
| --- | --- | --- |
| `image_access_policy.patch` | Mean -0.044%, worst -3.70% `Wire cube AA` | Rejected |
| `image_pixel_policy.patch` | Mean +1.94%, worst -25.65% `Wire rib tri thick` | Rejected |
| `image_wire_dots_policy.patch` | Mean +0.435%, worst -4.99% `Wire rib tri thick` | Rejected |
| `image_span_blit_policy.patch` | Mean +0.009%, worst -4.72% `Point cloud pixels` | Rejected |
| Triangle-only control | Mean +0.016%, no useful gain | Rejected |
| Font-only control | Mean +0.021%, no useful gain | Rejected |
| `PIXEL_API_DRAW=` | Mean +3.426%, global +4.8 to +5.1%, worst -22.85% `Point cloud dots` | Rejected |
| `PIXEL_API_READ=` | Mean +0.017%, no useful gain | Rejected |
| `_drawPixel=` | Mean +1.999%, worst -25.65% `Wire rib tri thick` | Rejected |
| Mesh-safe keep draw/wire/span | Mean +0.012%, no <= -3% regressions, global baseline | Rejected as neutral |

The only candidate passing the local regression gates was the mesh-safe keep-draw/wire/span variant, but it lost the performance improvement. It does not justify source churn.

## G. Detailed Subtest Analysis

The complete matrix is:

- `tmp/image_inline_investigation/pico2_subtest_matrix.csv`
- `tmp/image_inline_investigation/pico2_candidate_summary.csv`

The harmful pattern is consistent:

- Relaxing public `drawPixel/drawPixelf` produces large global gains and large point-cloud/wire losses.
- Relaxing internal `_drawPixel` produces strong mesh-heavy gains but heavily regresses thick wire.
- Relaxing wire-specific helpers alone still regresses wire rows.
- Access, read, triangle, font, and span categories are mostly neutral.
- Protecting draw/wire/span helpers removes the severe regressions, but also removes the useful gain.

The important positive rows were mostly mesh-heavy or far/farclip rows, for example `R2D2 (far)`, `R2D2 (farclip)`, `Bunny TEX_BILINEAR`, and `Sphere direct GOUR`.

The important negative rows were concentrated in `Wire rib tri thick`, `Point cloud dots`, `Point cloud pixels`, `Wire sphere thick`, `Wire cube fast`, and `Wire cube AA`.

## H. Real Example Telemetry

No narrowed Image inline candidate survived Pico2 benchmark screening with a meaningful gain, so no new cross-board real-example matrix was run for rejected candidates.

The safe candidate was effectively neutral:

- Mean subtest delta: +0.012%
- Median subtest delta: 0%
- Regressions <= -3%: 0
- Worst regression: -0.84%
- Global score: baseline

That candidate does not justify changing source or spending a full real-example matrix. This is documented in:

- `tmp/image_inline_investigation/reports/example_validation.md`

Previous inline-granularity real-example data remains relevant for the neutral category macro layer and showed no meaningful benefit requiring the macro layer to be kept.

## I. Codegen and Layout Analysis

Codegen artifacts:

- `tmp/image_inline_investigation/codegen/pico2_image_binary_sizes.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbols.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbol_deltas.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_top_size_deltas.csv`
- `tmp/image_inline_investigation/codegen/*.objdump.txt`
- `tmp/image_inline_investigation/reports/codegen_image_summary.md`

Binary size summary on Pico2:

| Candidate | ELF bytes | BIN bytes | UF2 bytes |
| --- | ---: | ---: | ---: |
| Baseline | 3,780,216 | 1,380,968 | 2,762,752 |
| Category default | 3,780,224 | 1,380,968 | 2,762,752 |
| Coarse `TGX_INLINE_IMAGE=` | 3,764,140 | 1,378,824 | 2,758,656 |
| `PIXEL_API_DRAW=` | 3,769,444 | 1,380,112 | 2,761,216 |
| `_drawPixel=` | 3,767,984 | 1,379,320 | 2,759,168 |
| Mesh-safe keep draw/wire/span | 3,780,264 | 1,380,968 | 2,762,752 |

Important observations:

- The default category split was codegen-neutral at `.bin` and `.uf2` level.
- Coarse `TGX_INLINE_IMAGE=` reduced `.bin` by about 2.1 KB.
- `_drawPixel=` shrank `Renderer3D::_drawDots<...>` by about 742 bytes.
- Coarse `TGX_INLINE_IMAGE=` also shrank several `Image<RGB565>::_bseg_*` helpers by 80 to 404 bytes.
- `PIXEL_API_DRAW=` changed layout and hot symbol placement more than it changed individual symbol sizes.
- Mesh-heavy gains were associated with layout and code-size movement, not a direct faster mesh Image helper.
- Wire/dots regressions correlate with exactly the pixel and Bresenham helpers that were no longer forced inline.

Conclusion: the coarse Image signal is a layout/codegen trade plus real helper call costs. It is not a robust optimization policy.

## J. Correctness Validation

Final source state has no tracked source diff.

CPU validations run after reverting the experimental source changes:

| Validation | Result | Log |
| --- | --- | --- |
| 3D CPU strict | 82 PASS, 0 FAIL | `tmp/image_inline_investigation/validation_3d_strict.log` |
| 2D CPU | All rows OK | `tmp/image_inline_investigation/validation_2d.log` |

The 2D validation emitted expected deprecation warnings from the validation harness for older text/line APIs, but all validation rows were OK.

## K. Final Source Diff

Final selected source changes: none.

The previous broad macro layer and Image split were reverted because they were either neutral infrastructure or rejected experimental scaffolding.

Final expected Git diff:

```text
git diff --stat
<empty>

git diff --name-only
<empty>

git diff --check
<empty>
```

Untracked report artifacts are expected:

- `REPORT_INLINE_GRANULARITY.md`
- `REPORT_IMAGE_INLINE_INVESTIGATION.md`
- files under `tmp/image_inline_investigation/`

## L. Rejected Candidates

Rejected patches saved in:

- `tmp/image_inline_investigation/patches/image_access_policy.patch`
- `tmp/image_inline_investigation/patches/image_pixel_policy.patch`
- `tmp/image_inline_investigation/patches/image_wire_dots_policy.patch`
- `tmp/image_inline_investigation/patches/image_span_blit_policy.patch`
- `tmp/image_inline_investigation/patches/image_mesh_safe_policy.patch`
- `tmp/image_inline_investigation/patches/image_combined_best_candidate.patch`
- `tmp/image_inline_investigation/patches/rejected_full_source_wip.patch`

Reasons:

- Unsafe global-gain candidates regressed wire/dots or point-cloud paths too much.
- Safe candidates were neutral and did not justify source churn.
- Broad neutral macros make further experiments easy, but their review cost is not justified without a real retained policy.

## M. Remaining Risks and Next Steps

Remaining risks:

- Pico2 layout sensitivity is real and can produce tempting global gains.
- The benchmark global score can hide severe local regressions.
- Pico W remained compile-only in this session because the previous UF2/picotool upload issue was not fixed here.
- No new real-example matrix was run because no non-neutral candidate survived Pico2 screening.

Recommended next steps:

- Do not introduce `TGX_INLINE_IMAGE` subcategories in main source at this time.
- If pursuing Pico2 layout wins later, investigate Renderer3D/shader code placement directly rather than using Image inline policy as an indirect lever.
- Keep using detailed subtest gates before accepting any global-score improvement.
