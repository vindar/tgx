# ESP32-S3 / ESP32 Image Span Copy/Fill Report

## A. Executive Summary

Recommendation: keep the small `src/Image.inl` source patch.

The retained optimization is a conservative same-format `Image::blit()` improvement:

- use direct assignment for `1x1` copies;
- use `memcpy()` for rows/regions whose source and destination byte ranges are provably non-overlapping;
- keep `memmove()` for overlap and tiny copies;
- leave RGB565 fill helpers unchanged.

No `dsps_memcpy()` production dependency is kept.

Boards tested:

- Core2 / classic ESP32 / `COM5`
- CoreS3 / ESP32-S3 / `COM10`

Best measured Image-wrapper gains:

| Board | Workload | Baseline us | Candidate us | Delta |
| ----- | -------- | ----------- | ------------ | ----- |
| Core2 | contiguous `Image_blit` 64x32 | 26,105 | 2,947 | +785.82% |
| Core2 | rect copy 320x20 | 28,511 | 3,727 | +664.99% |
| Core2 | mixed workload | 1,313,269 | 1,107,937 | +18.53% |
| CoreS3 | contiguous `Image_blit` 64x32 | 22,155 | 1,508 | +1369.16% |
| CoreS3 | rect copy 320x20 | 22,184 | 2,099 | +956.88% |
| CoreS3 | mixed workload | 632,580 | 535,667 | +18.09% |

TGX benchmark global scores were unchanged on both boards. Real examples were neutral within capture limitations.

## B. Previous ESP Carry-over

This session reused the previous ESP kernel investigation under:

```text
validation/performance/investigations/2026-06-esp32-esp32s3-kernels/
```

Key carry-over:

- `fast_inv()` was already the right safe `recip0.s` pattern.
- IRAM placement was neutral or negative.
- Fixed16 texture coordinates are a separate shader problem.
- Raw RGB565 copy spans showed very large long-copy gains, especially CoreS3 `dsps_memcpy`, but short spans regressed.

The new question was whether those copy gains exist in real TGX `Image` wrappers.

## C. Source Call-site Audit

Audit files:

- `src/Image.h`
- `src/Image.inl`
- `src/Color.h`
- `src/Renderer3D.inl`
- `src/Shaders.h`
- `src/Misc.h`
- `src/tgx_config.h`

Important findings are archived in:

```text
results/image_span_callsite_inventory.csv
notes/image_span_callsite_map.md
```

The main result: RGB565 fills are already optimized. Public fill paths such as `fillScreen()`, `fillRect()`, and opaque `drawFastHLine()` route into `_fast_memset()`, which already uses RGB565 32-bit/two-pixel and unrolled stores.

The useful production target was same-format opaque `Image::blit()`. Its `_blitRegionUp()` and `_blitRegionDown()` helpers used `memmove()` for all copies, even when the regions are known not to overlap.

Rejected during audit:

- blended/masked/custom blits, because they are per-pixel semantic operations;
- vertical lines, because they are not contiguous spans;
- generic renderer/shader code, outside the scope of this session;
- fill paths, because current TGX RGB565 fill code is already specialized.

## D. Microbenchmark Correctness

Benchmark sketch:

```text
sketches/ImageSpanCopyFillBench/
```

Parser:

```text
tools/parse_image_span_bench.py
```

Candidate correctness summary:

| Correct | Rows |
| ------- | ---- |
| yes | 972 |
| no | 4 |

The four failures were intentional raw-copy diagnostics:

| Board | Kernel | Variant | Case |
| ----- | ------ | ------- | ---- |
| Core2 | `copy_overlap` | `memcpy` | forward overlap |
| Core2 | `copy_overlap` | `dsps_memcpy` | forward overlap |
| CoreS3 | `copy_overlap` | `memcpy` | forward overlap |
| CoreS3 | `copy_overlap` | `dsps_memcpy` | forward overlap |

These raw primitive failures are why the production patch keeps `memmove()` whenever ranges may overlap. Production `Image_blit` rows were correct.

## E. Microbenchmark Performance

Main result files:

```text
results/image_span_results.csv
results/summary.csv
results/candidate_image_span_results.csv
results/candidate_summary.csv
results/production_candidate_delta.csv
results/threshold_matrix.csv
```

The retained Image wrapper rows were all positive:

| Board | Image wrapper | Smallest retained gain | Best retained gain |
| ----- | ------------- | ---------------------- | ------------------ |
| Core2 | `Image_blit` / rect copy | +14.04% for `1x1` | +785.82% contiguous 64x32 |
| CoreS3 | `Image_blit` / rect copy | +16.10% for `1x1` | +1369.16% contiguous 64x32 |

Mixed workloads also improved:

| Board | Baseline us | Candidate us | Delta |
| ----- | ----------- | ------------ | ----- |
| Core2 | 1,313,269 | 1,107,937 | +18.53% |
| CoreS3 | 632,580 | 535,667 | +18.09% |

Some unchanged helper rows moved slightly negative, mostly JPEG-like import and fill diagnostic rows around 1-2%. Those are not affected by the source patch and are treated as measurement/layout noise, not evidence against the Image blit change.

Threshold decision:

- `1x1`: direct assignment.
- `< 8` bytes: keep `memmove()`.
- `>= 8` bytes: use `memcpy()` only if byte ranges are provably non-overlapping.

## F. Production Integration Design

Patch snapshot:

```text
patches/integrate_image_span_copyfill_candidate.patch
```

Source changed:

```text
src/Image.inl
```

Functions changed:

- `_blitRegionUp(color_t*, int, color_t*, int, int, int)`
- `_blitRegionDown(color_t*, int, color_t*, int, int, int)`

Fallback behavior:

- zero-size regions return immediately;
- `1x1` copies use direct assignment;
- tiny copies keep `memmove()`;
- possible overlap keeps `memmove()`;
- non-overlap uses `memcpy()`.

No board macro was added. The helper already used byte-wise move operations, and the optimized path uses standard `memcpy()` only when it is semantically safe.

No `dsps_memcpy()` was integrated because:

- it is not overlap-safe;
- it needs a second threshold policy;
- it adds ESP-DSP dependency surface;
- standard `memcpy()` already delivered robust Image-wrapper gains.

## G. Production Validation

CPU validation:

```text
powershell -ExecutionPolicy Bypass -File validation\2d\run_cpu.ps1
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Compare strict
```

Results:

- CPU 2D: pass.
- CPU 3D strict: pass, 82 PASS / 0 FAIL.

Logs:

```text
results/cpu_2d_validation.log
results/cpu_3d_strict_validation.log
```

Hardware microbench after patch:

- Core2: success.
- CoreS3: success.
- Complete telemetry captured with `validation/performance/tools/upload_and_capture.ps1`.

TGX benchmark sanity:

| Board | Global score index | Baseline | Candidate | Delta |
| ----- | ------------------ | -------- | --------- | ----- |
| Core2 | 1 | 32.65 | 32.65 | 0.000% |
| Core2 | 2 | 22.81 | 22.81 | 0.000% |
| CoreS3 | 1 | 46.66 | 46.66 | 0.000% |
| CoreS3 | 2 | 32.21 | 32.21 | 0.000% |
| CoreS3 | 3 | 27.83 | 27.83 | 0.000% |

Worst benchmark subtest movement:

| Board | Subtest | Delta |
| ----- | ------- | ----- |
| CoreS3 | Wire cube fast, small | -1.389% |
| CoreS3 | Wire cube fast, large | -0.933% |
| Core2 | Wire cube fast, small | -0.814% |

These are renderer/wire paths unrelated to the changed Image blit helper. Global scores and texture/mesh rows stayed neutral.

One invalid benchmark attempt occurred on Core2 with the wrong serial baud rate and produced no valid telemetry. It was discarded and rerun at 9600 baud.

Real examples:

| Board | Examples |
| ----- | -------- |
| Core2 | `donkeykong`, `borg_cube`, `scream` |
| CoreS3 | `donkeykong`, `borg_cube`, `scream` |

Summary files:

```text
results/example_telemetry.csv
results/example_telemetry_summary.csv
results/example_telemetry_delta.csv
```

Stable example rows were neutral:

- Core2 `borg_cube`: 46.432 fps baseline and candidate.
- Core2 `scream`: -0.279%.
- CoreS3 `borg_cube`: -0.041%.
- CoreS3 `scream`: -0.295%.

`donkeykong` Gouraud averages show apparent negative deltas because the archived baseline captured only the first few rows of that scene, while the candidate capture included a fuller scene cycle. Matching early rows are consistent, so this is recorded as a baseline-window limitation rather than a real regression.

## H. Codegen Analysis

Detailed notes:

```text
codegen/CODEGEN_SUMMARY.md
```

Highlights:

- Microbenchmark program size increased only about +396 bytes on Core2 and +456 bytes on CoreS3.
- Candidate emits small weak `_blitRegionUp/_blitRegionDown` template symbols:
  - Core2: `0x00f2` and `0x00fa`.
  - CoreS3: `0x010d` and `0x011d`.
- Existing `_fast_memset()` RGB565 symbol size stayed unchanged.
- Core2 candidate uses normal `memcpy` / `memmove`.
- CoreS3 resolves `memcpy` / `memmove` through ESP32-S3 linker aliases.
- `dsps_memcpy_aes3` appears only because the microbenchmark tested raw variants; the production patch does not call it.

The gains are direct copy-path gains, not a renderer layout side effect.

## I. Final Source Diff

Production diff:

```text
src/Image.inl | 82 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++---
1 file changed, 78 insertions(+), 4 deletions(-)
```

Production diff name-only:

```text
src/Image.inl
```

`git diff --check` reported only the Windows working-copy line-ending notice:

```text
warning: in the working copy of 'src/Image.inl', LF will be replaced by CRLF the next time Git touches it
```

## J. Rejected Candidates

`dsps_memcpy()` production path:

- Rejected for this patch.
- Strong CoreS3 long-copy microbench result, but overlap semantics and thresholds make it riskier than standard `memcpy()`.
- Useful future work if TGX adds a separate non-overlap long-row ESP32-S3 helper.

RGB565 fill changes:

- Rejected.
- Existing `_fast_memset()` is already RGB565-specialized and remained unchanged.

Manual 32-bit copy/unroll helpers:

- Rejected.
- Standard `memcpy()` was simpler, safer, and fast enough in real Image-wrapper rows.

Raw `memcpy()` for overlap:

- Rejected by correctness diagnostics.
- Forward overlap mismatched on both Core2 and CoreS3.

JPEG RGB565 row import:

- Deferred.
- It remains a plausible future row-copy target, but this session kept the retained patch to same-format `Image::blit()`.

Broad IRAM placement:

- Not pursued.
- Previous ESP investigation found it neutral or negative.

## K. Next Steps

Good follow-up targets:

- JPEG RGB565 row import copy helper.
- ESP32-S3-only long non-overlap row helper using `dsps_memcpy()` with a conservative threshold.
- Image copy/fill/blit validation on ARM boards if the generic source patch is reviewed for all targets.
- Separate ESP texture-coordinate guarded shader integration, outside this Image/blit session.
