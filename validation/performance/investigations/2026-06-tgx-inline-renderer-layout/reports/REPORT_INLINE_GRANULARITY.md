# TGX Inline Granularity Investigation

Date: 2026-06-05  
Branch: `feature/multi-directional-lights`  
Baseline commit: `5fbe8cf`

## A. Executive Summary

The hardware/capture tooling was repaired first, then the inline-configuration campaign was run on the reliable connected boards.

Final recommendation:

- Keep the new category-level inline macro layer as a neutral, reviewable tuning hook.
- Do not enable any board-specific category override yet.
- Keep the previously committed `TGX_INLINE_ZDIVIDE` and RP2350 shader incremental pointer policy as the baseline.

Final source files changed:

- `src/tgx_config.h`
- `src/Vec2.h`
- `src/Vec3.h`
- `src/Vec4.h`
- `src/Color.h`
- `src/Misc.h`
- `src/Shaders.h`
- `src/Image.h`

The final source diff introduces these fallback macros:

- `TGX_INLINE_VEC2`
- `TGX_INLINE_VEC3`
- `TGX_INLINE_VEC4`
- `TGX_INLINE_COLOR`
- `TGX_INLINE_RGBF`
- `TGX_INLINE_MATH`
- `TGX_INLINE_SHADER`
- `TGX_INLINE_IMAGE`

All default to `TGX_INLINE`, so the final codegen is intentionally equivalent to the current board-wide policy unless a board or build flag overrides one category. `Vec4::zdivide()` remains controlled only by `TGX_INLINE_ZDIVIDE`.

No tested category override was robust enough to keep globally. On Pico2/RP2350, removing forced inline from `Vec3`, `Vec4`, `RGBf`, color interpolation, or math helpers causes large regressions. `TGX_INLINE_IMAGE` removal improved Pico2 global benchmark scores but caused severe local regressions in wire/dots paths, so it was rejected.

## B. Hardware/Capture Reliability

Reusable tools:

- `tmp/hardware_validation/tools/upload_and_capture.ps1`
- `tmp/inline_granularity/tools/run_benchmark_candidate.ps1`
- `tmp/inline_granularity/tools/aggregate_capture_runs.py`
- `tmp/inline_granularity/tools/aggregate_candidates.py`
- `tmp/inline_granularity/tools/aggregate_examples.py`
- `tmp/inline_granularity/tools/extract_codegen.py`

Reliable boards:

| Board | Port | Upload | Serial | Macro probe | Telemetry probe | Campaign status |
| --- | --- | --- | --- | --- | --- | --- |
| Core2 | COM5 | OK | OK | OK | OK | Trusted, except one benchmark candidate serial timeout |
| CoreS3 | COM10 | OK | OK | OK | OK | Trusted |
| Pico W / RP2040 | COM28 | Failed | Not trusted | Failed | Failed | Excluded from hardware measurements |
| Pico2 / RP2350 | COM21 | OK | OK | OK | OK | Trusted |
| Teensy 4.1 | COM3 | OK | OK | OK | OK | Trusted |

Pico W was not used for performance numbers because UF2 upload reported `No drive to deploy`, and `picotool` reported an inaccessible RP2040 BOOTSEL device consistent with a local driver/bootloader access problem.

The capture script now waits for port return, retries serial opening, supports kick text for sketches that wait on serial input, rejects missing markers and partial telemetry, and writes raw telemetry plus metadata. Benchmark runs count only when upload, capture, parsing, and expected score/subtest extraction all succeed.

## C. Inline Usage Inventory

Generated inventory artifacts:

- `tmp/inline_granularity/inline_usage_inventory.csv`
- `tmp/inline_granularity/config_macro_inventory.csv`
- `tmp/inline_granularity/macro_matrix_baseline.csv`

Baseline macro highlights:

| Board | `TGX_INLINE` | `TGX_INLINE_ZDIVIDE` | Incremental pixel pointers | Fast inv/sqrt | FMA |
| --- | --- | --- | --- | --- | --- |
| Core2 | `always_inline` | empty | 0 | 0 / 0 / 0 | 0 |
| CoreS3 | `always_inline` | `always_inline` | 0 | 0 / 0 / 0 | 0 |
| Pico2 | `always_inline` | `always_inline` | 1 | 1 / 1 / 1 | 0 |
| Teensy 4.1 | `always_inline` | `always_inline` | 0 | 1 / 1 / 1 | 1 |
| CPU | empty | empty | 0 | default CPU profile | default CPU profile |

`TGX_INLINE` usage was split into categories for vector, color, math, shader, and image helpers. The split is intentionally conservative: the default build does not change behavior, but future tests can override one category without editing hot headers again.

## D. Candidate Matrix

Patch artifacts:

- `tmp/inline_granularity/patches/inline_vec_policy.patch`
- `tmp/inline_granularity/patches/inline_color_policy.patch`
- `tmp/inline_granularity/patches/inline_math_policy.patch`
- `tmp/inline_granularity/patches/inline_shader_policy.patch`
- `tmp/inline_granularity/patches/inline_image_policy.patch`
- `tmp/inline_granularity/patches/inline_vec_color_combined.patch`
- `tmp/inline_granularity/patches/inline_all_categories_board_specific.patch`

Candidate summary:

| Candidate | Boards tested | Result |
| --- | --- | --- |
| Vector macro layer, default fallback | Pico2, CoreS3, Teensy | Neutral; kept as part of final macro layer |
| `TGX_INLINE_VEC2=` on Pico2 | Pico2 | Weak mixed signal, no global gain; rejected |
| `TGX_INLINE_VEC3=` on Pico2 | Pico2 | Large global and local regressions; rejected |
| `TGX_INLINE_VEC4=` on Pico2 | Pico2 | Very large regressions; rejected |
| `TGX_INLINE_VEC2/3/4=` on Pico2 | Pico2 | Very large regressions; rejected |
| `TGX_INLINE_COLOR=` on Pico2 | Pico2 | Large regressions; rejected |
| `TGX_INLINE_RGBF=` on Pico2 | Pico2 | Large regressions; rejected |
| `TGX_INLINE_COLOR/RGBF=` on Pico2 | Pico2 | Large regressions; rejected |
| `TGX_INLINE_MATH=` on Pico2 | Pico2 | Large regressions; rejected |
| `TGX_INLINE_SHADER=` on Pico2 | Pico2 | Mostly neutral, no useful robust gain; rejected as override |
| `TGX_INLINE_IMAGE=` on Pico2 | Pico2 | Global gain but severe local regressions; rejected |
| `TGX_INLINE_VEC2=` + `TGX_INLINE_SHADER=` on Pico2 | Pico2 | Slight subtest mean gain, no global gain; rejected |
| Final all-category default fallback | Pico2, CoreS3, Teensy; examples also Core2 | Neutral and correctness-safe; kept |

## E. Detailed Subtest Analysis

Artifacts:

- `tmp/inline_granularity/global_scores.csv`
- `tmp/inline_granularity/subtest_matrix.csv`
- `tmp/inline_granularity/candidate_subtest_summary.csv`
- `tmp/inline_granularity/reports/candidate_subtest_summaries.md`

Baseline repeated-run noise:

- CoreS3: repeated global scores identical at displayed precision.
- Pico2: repeated scores differed by at most one displayed hundredth on score 2.
- Teensy 4.1: repeated scores varied by less than about 0.04%.

Final category macro fallback vs baseline:

| Board | Global score delta | Mean subtest delta | Median | Regressions <= -3% | Worst subtest |
| --- | --- | --- | --- | --- | --- |
| CoreS3 | 0 / 0 / 0 | -0.025% | 0% | 0 | -2.06% `Wire cube fast` |
| Pico2 | 0 / +0.03% / 0 | -0.023% | 0% | 0 | -2.85% `Wire cube AA` |
| Teensy 4.1 | +0.016% / +0.011% / -0.019% | +0.003% | 0% | 0 | -1.94% `Wire cube fast` |

Rejected Pico2 overrides:

| Candidate | Global delta | Mean subtest delta | Regressions <= -3% | Worst regression |
| --- | --- | --- | --- | --- |
| `Vec2` empty | -0.26 / -0.21 / -0.21% | +0.105% | 0 | -2.34% `Wire cube fast` |
| `Vec3` empty | -6.12 / -3.78 / -2.86% | +0.061% but median -2.46% | 74 | -20.06% `Buddha ORTHO` |
| `Vec4` empty | -22.31 / -18.46 / -16.20% | -7.40% | 100 | -47.18% `R2D2 TEX_BILINEAR` |
| all Vec empty | -29.95 / -25.66 / -22.84% | -10.27% | 105 | -44.82% `Buddha FLAT` |
| `Color` empty | -11.07 / -8.73 / -7.35% | -1.73% | 52 | -31.16% `Ribbon quads` |
| `RGBf` empty | -18.01 / -14.78 / -12.78% | -0.23% | 49 | -49.81% `R2D2 FLAT` |
| `Math` empty | -14.67 / -12.96 / -11.78% | -5.46% | 92 | -31.15% `Ribbon strip` |
| `Shader` empty | 0 / +0.03 / 0% | +0.016% | 1 | -3.15% `Wire cube fast` |
| `Image` empty | +4.38 / +4.62 / +4.43% | +2.55% | 25 | -19.62% `Wire rib tri thick` |

The `Image` result is the clearest warning against using the global benchmark alone: it looks attractive globally on Pico2, but it badly degrades important wire/dots paths.

## F. Real-Example Telemetry

Artifacts:

- `tmp/inline_granularity/baseline_example_telemetry.csv`
- `tmp/inline_granularity/baseline_example_telemetry_summary.csv`
- `tmp/inline_granularity/example_telemetry.csv`
- `tmp/inline_granularity/example_telemetry_summary.csv`
- `tmp/inline_granularity/example_telemetry_delta.csv`

Final candidate examples completed successfully on reliable boards:

- Teensy 4.1: `characters`, `test-texture`, `test-shading`, `borg_cube`, `scream`
- Core2: `donkeykong`, `borg_cube`, `scream`
- CoreS3: `donkeykong`, `borg_cube`, `scream`
- Pico2: `bunny_fig`, `borg_cube`, `scream`

Pico2 final example deltas were effectively zero:

| Example / scene | Delta |
| --- | --- |
| `borg_cube` | 0% |
| `bunny_fig` flat | +0.034% |
| `bunny_fig` gouraud | +0.013% |
| `bunny_fig` gouraud texture | -0.031% |
| `bunny_fig` wireframe | +0.034% |
| `scream` | 0% |

Teensy 4.1 examples were also within noise, generally below 0.2%, with one small +0.57% `teapot_flat` sample.

Core2/CoreS3 `donkeykong` Gouraud scene deltas looked negative in the summary, but the baseline and final captures used different row counts for that scene while other scenes remained stable. This is treated as telemetry sampling variation rather than a source regression because the final source is behavior-neutral and the other Core2/CoreS3 examples are stable.

## G. Codegen/Layout Analysis

Artifacts:

- `tmp/inline_granularity/codegen/pico2_symbols.csv`
- `tmp/inline_granularity/codegen/pico2_hot_symbols.csv`
- `tmp/inline_granularity/codegen/pico2_hot_symbol_deltas.csv`
- `tmp/inline_granularity/codegen/*.objdump.txt`
- `tmp/inline_granularity/reports/codegen_summary.md`

Important observations:

- The final default category macro build has the same effective Pico2 program size as baseline. The ELF differs only by tiny metadata/build-output noise.
- `Vec4` no-inline changes hot wireframe/draw function sizes and alignments. For example, `_drawWireFrameQuad<4>` grew by 148 bytes and shifted alignment, matching the large wireframe and textured regressions.
- `RGBf` no-inline shrank several large renderer functions but made performance much worse. `_drawMesh(Mesh3Dv2)` shrank by about 512 bytes and `_drawTriangle*`/`_drawQuad` shrank by hundreds of bytes, but hot calls/less inlining dominated runtime cost.
- `Image` no-inline changed `_drawDots` and pixel helper layout substantially. Some mesh-heavy global scores improved, but wire/dots paths regressed sharply, indicating a fragile layout/codegen trade rather than a robust improvement.

The technical explanation for the previous anomalies is therefore mixed:

- Some effects are real call/inlining costs in hot path helpers (`Vec3`, `Vec4`, `RGBf`, `Misc`).
- Some effects are layout-sensitive shifts in large template-instantiated hot functions, especially on RP2350.
- Smaller code is not automatically faster. Several rejected variants reduced ELF size but slowed hot rendering paths.

## H. Correctness Validation

CPU validation with the final source:

- 3D strict: `82 PASS, 0 FAIL`
- 2D CPU: all validation rows reported `OK`

Validation logs:

- `tmp/inline_granularity/validation_3d_strict.log`
- `tmp/inline_granularity/validation_2d.log`

No forced RP2350 CPU-profile validation was run for this final source because the final accepted diff does not change any RP2350-only behavior. RP2350-specific policy remains the previously committed baseline: `TGX_INLINE_ZDIVIDE` forced and shader incremental pixel pointers enabled.

## I. Final Source Diff

Source diff summary before adding this report:

```text
 src/Color.h      | 54 +++++++++++++++++++++----------------------
 src/Image.h      | 70 ++++++++++++++++++++++++++++----------------------------
 src/Misc.h       | 42 +++++++++++++++++-----------------
 src/Shaders.h    |  2 +-
 src/Vec2.h       | 58 +++++++++++++++++++++++-----------------------
 src/Vec3.h       | 54 +++++++++++++++++++++----------------------
 src/Vec4.h       | 54 +++++++++++++++++++++----------------------
 src/tgx_config.h | 32 ++++++++++++++++++++++++++
 8 files changed, 199 insertions(+), 167 deletions(-)
```

`git diff --name-only`:

```text
src/Color.h
src/Image.h
src/Misc.h
src/Shaders.h
src/Vec2.h
src/Vec3.h
src/Vec4.h
src/tgx_config.h
```

`git diff --check`: clean, no output.

Final source macro definitions by board are unchanged unless a new category macro is overridden externally. By default:

```cpp
TGX_INLINE_VEC2   == TGX_INLINE
TGX_INLINE_VEC3   == TGX_INLINE
TGX_INLINE_VEC4   == TGX_INLINE
TGX_INLINE_COLOR  == TGX_INLINE
TGX_INLINE_RGBF   == TGX_INLINE
TGX_INLINE_MATH   == TGX_INLINE
TGX_INLINE_SHADER == TGX_INLINE
TGX_INLINE_IMAGE  == TGX_INLINE
```

## J. Rejected Candidates

Rejected-candidate details are saved in:

- `tmp/inline_granularity/rejected_candidates.md`
- `tmp/inline_granularity/patches/`
- `tmp/inline_granularity/candidate_subtest_summary.csv`
- `tmp/inline_granularity/subtest_matrix.csv`

The most useful rejected candidate for future investigation is `TGX_INLINE_IMAGE=` on Pico2. It shows there may be a stable optimization opportunity in image/pixel helper layout, but the current coarse override is not acceptable because it regresses wire/dots paths too strongly.

## K. Remaining Risks and Next Steps

Remaining risks:

- Pico W/RP2040 could not be measured due upload/driver access failure on COM28.
- Core2 final benchmark capture timed out in one candidate run, although baseline benchmark and final real examples completed.
- Some real-example scene summaries depend on how many rows were captured per scene. The capture system rejects empty/partial telemetry, but scene-cycle row counts can still differ.
- RP2350 remains layout-sensitive; future optimizations must continue using detailed subtest deltas, not only global scores.

Recommended next steps:

- Keep the category macro layer only if this extra tuning surface is considered worth the small review cost.
- Do not ship a board-specific category override yet.
- Investigate a narrower `Image`/pixel helper optimization later, using separate wire/dots benchmarks as hard acceptance gates.
- Fix Pico W upload/BOOTSEL driver access before making RP2040 inline-policy decisions.
