# ESP32 / ESP32-S3 Kernel Investigation

## A. Executive summary

Final recommendation: keep no production TGX source patch from this session.

Boards tested:

- Core2 / classic ESP32 / Xtensa LX6: `COM5`
- CoreS3 / ESP32-S3 / Xtensa LX7: `COM10`

Main findings:

- Current `tgx::fast_inv()` is already the right exact-ish two-Newton-step `recip0.s` helper: moving the asm locally did not improve timing.
- A one-step `recip0.s` reciprocal is much faster, but approximate. It is not safe to integrate without visual/path-specific validation.
- RGB565 color-call kernels did not produce a compelling exact production patch.
- RGB565 copy/fill spans have large gains with `memcpy`, `dsps_memcpy`, 32-bit stores, and unrolling, especially on CoreS3; this is promising for future Image/blit/fill work, not a direct shader patch.
- Fixed16 affine texture-coordinate spans are fast on both ESP boards, but large-step and perspective cases show correctness limits. This remains a future shader-integration task with strict guards/fallbacks.
- `IRAM_ATTR` placement was neutral on Core2 and negative on CoreS3 for the kernels tested.

No TGX benchmark or real-example finalist was run because no production source patch survived microbenchmark screening.

## B. Hardware/tool status

Upload and serial capture used `validation/performance/tools/upload_and_capture.ps1`.

Both boards uploaded and captured successfully for:

- `EspCapabilityProbe`
- `EspMathKernelBench`
- `EspRgb565KernelBench`
- `EspSpanKernelBench`
- `EspShaderCoordKernelBench`
- `EspIramKernelBench`

Capability probe highlights:

- Core2: `CONFIG_IDF_TARGET_ESP32=1`, `CONFIG_SOC_SIMD_INSTRUCTION_SUPPORTED=0`, ESP-DSP headers present.
- CoreS3: `CONFIG_IDF_TARGET_ESP32S3=1`, `CONFIG_SOC_SIMD_INSTRUCTION_SUPPORTED=1`, ESP-DSP headers present.
- Arduino build did not define `ESP32S3`; use `CONFIG_IDF_TARGET_ESP32S3` for S3 guards.
- `recip0.s`, `rsqrt0.s`, `floor.s`, and `rsr.ccount` compiled and ran on both boards.
- CoreS3 accepted a small S3-only `ee.get_gpio_in` asm probe.

Raw telemetry is under `validation/performance/telemetry/`; parsed rows are under this investigation's `results/`.

## C. Source/hotpath audit

Audited files:

- `src/tgx_config.h`
- `src/Misc.h`
- `src/Vec3.h`
- `src/Vec4.h`
- `src/Mat4.h`
- `src/Shaders.h`
- `src/Renderer3D.inl`
- `src/Color.h`

Important candidates:

- `Misc.h`: `fast_inv`, `precise_invsqrt`, `fast_invsqrt`, `fast_sqrt`, `lfloorf`.
- `Vec3.h`: normalization and inverse norm.
- `Vec4.h`: `zdivide()`.
- `Shaders.h`: per-pixel perspective `fast_inv(cw_p)`, bilinear `lfloorf(xx/yy)`, z-buffer path, texture/color modulation.
- `Color.h`: RGB565 blend, modulation, bilinear/triangle interpolation, RGBf conversion, meanColor4.
- Image/framebuffer-style span fills and copies.

Detailed audit files:

- `results/esp_hotpath_inventory.csv`
- `notes/esp_hotpath_map.md`

## D. Math kernels

Key speedups versus direct scalar reference:

| Board | Kernel | Variant | Speedup | Accuracy |
| --- | --- | --- | ---: | --- |
| Core2 | reciprocal | `tgx_fast_inv` | +138.37% | exact for tested inputs |
| Core2 | reciprocal | one-step `recip0.s` | +226.23% | max rel err `2.99e-5` |
| Core2 | zdivide3 | `tgx_fast_inv` | +74.98% | max rel err `1.22e-7` |
| Core2 | zdivide3 | one-step `recip0.s` | +104.14% | max rel err `6.11e-5` |
| CoreS3 | reciprocal | `tgx_fast_inv` | +146.04% | exact for tested inputs |
| CoreS3 | reciprocal | one-step `recip0.s` | +236.72% | max rel err `2.99e-5` |
| CoreS3 | zdivide3 | `tgx_fast_inv` | +106.20% | max rel err `1.22e-7` |
| CoreS3 | zdivide3 | one-step `recip0.s` | +141.40% | max rel err `6.11e-5` |

Decision:

- Do not change `Misc.h`: the existing helper already compiles to the expected `recip0.s` pattern and matches the local two-step sequence.
- Do not integrate one-step reciprocal yet: it is attractive for speed but changes numerical behavior.
- `fast_sqrt()` is not an ESP-specific win in this toolchain; it follows the normal `sqrtf` route.

Data:

- `results/math_kernel_results.csv`
- `results/math_accuracy_summary.csv`

## E. RGB565 color kernels

Results:

- `RGB565::blend256`: formula copy matched current timing; alpha shortcut was exact but roughly 32-33% slower.
- `RGB565::mult256`: extract-scalar was +4.28% on Core2 and neutral on CoreS3.
- `RGBf -> RGB565`: manual scalar was exact and +5.81% Core2 / +7.69% CoreS3 in isolation, but no hot 3D call-site case justified a production rewrite.
- Triangle direct/component variant was not exact.
- Pairwise `meanColor4` was faster but not exact.

Decision:

- No RGB565 production patch. The exact wins are too small or not clearly relevant to the hot shader paths; the larger variants changed output.

Data:

- `results/rgb565_kernel_results.csv`
- `results/rgb565_summary.csv`

## F. Span kernels

Representative exact span results:

| Board | Kernel | Len | Variant | Speedup |
| --- | --- | ---: | --- | ---: |
| Core2 | fill RGB565 | 320 | 32-bit store | +186.45% |
| Core2 | fill RGB565 | 320 | unroll4 | +215.44% |
| Core2 | copy RGB565 | 320 | `memcpy` | +470.48% |
| CoreS3 | fill RGB565 | 320 | 32-bit store | +77.87% |
| CoreS3 | fill RGB565 | 320 | unroll4 | +171.59% |
| CoreS3 | copy RGB565 | 320 | `memcpy` | +546.44% |
| CoreS3 | copy RGB565 | 320 | `dsps_memcpy` | +1568.38% |

Important caveat:

- CoreS3 `dsps_memcpy` was excellent for long spans but regressed `len=8` versus scalar by about 11.5%.
- Any future production path needs a length threshold and should target Image/blit/copy/fill paths, not arbitrary shader pixels.

Decision:

- No production patch in this session. This is the strongest future ESP32-S3 Image/span-copy lead.

Data:

- `results/span_kernel_results.csv`
- `results/span_summary.csv`

## G. Texture-coordinate ESP results

Fixed16 affine texture-coordinate benchmark versus float reference:

| Board | Kernel | Case | Len | Speedup | Correct |
| --- | --- | --- | ---: | ---: | --- |
| Core2 | nearest | interior | 8 | +19.17% | yes |
| Core2 | nearest | interior | 320 | +26.21% | yes |
| Core2 | bilinear | interior | 8 | +102.32% | yes |
| Core2 | bilinear | interior | 320 | +108.51% | yes |
| CoreS3 | nearest | interior | 8 | +20.79% | yes |
| CoreS3 | nearest | interior | 320 | +30.21% | yes |
| CoreS3 | bilinear | interior | 8 | +86.89% | yes |
| CoreS3 | bilinear | interior | 320 | +93.42% | yes |

Correctness notes:

- Exact for `interior_inc`, `interior_dec`, `subtexel`, `edge_clamp`, and `wrap_pow2` cases in this benchmark.
- `large_step` produced mismatches in 18/22 rows per board for nearest and bilinear; max channel error was 5 for nearest and 1 for bilinear.
- `perspective_diag` was incorrect for all fixed16 rows; max channel error reached 38 in the broader matrix.

Decision:

- Do not integrate here. This is a real ESP lead, but it needs the same kind of guarded shader integration explored in the ARM work: affine-only, safe coordinate range, step-size/precision constraints, and fallback to the current path.

Data:

- `results/shader_coord_results.csv`
- `results/shader_coord_summary.csv`

## H. IRAM/code placement results

IRAM placement was tested with noinline normal-text and noinline `IRAM_ATTR` functions.

| Board | Kernel | IRAM speedup |
| --- | --- | ---: |
| Core2 | `fast_inv` | -0.03% |
| Core2 | `fast_invsqrt` | +2.09% |
| Core2 | fill RGB565 | -0.04% |
| Core2 | copy RGB565 | +0.01% |
| Core2 | fixed16 nearest texture | -0.00% |
| CoreS3 | `fast_inv` | -4.54% |
| CoreS3 | `fast_invsqrt` | -7.14% |
| CoreS3 | fill RGB565 | -0.64% |
| CoreS3 | copy RGB565 | -0.62% |
| CoreS3 | fixed16 nearest texture | -0.09% |

Decision:

- Do not add `IRAM_ATTR` to TGX hot helpers. The isolated evidence is neutral or negative.

Data:

- `results/iram_kernel_results.csv`
- `results/iram_summary.csv`

## I. Codegen analysis

See `codegen/CODEGEN_SUMMARY.md`.

Key observations:

- Math disassembly contains expected `recip0.s` and `rsqrt0.s` instructions.
- `sqrtf` remains a library path, so `fast_sqrt()` is not a useful ESP replacement here.
- Fixed16 texture-coordinate loops are smaller than float/floor loops:
  - Core2 nearest fixed/float: `0xca` / `0xde`
  - Core2 bilinear fixed/float: `0x1c4` / `0x217`
  - CoreS3 nearest fixed/float: `0xc3` / `0xdb`
  - CoreS3 bilinear fixed/float: `0x1bd` / `0x210`
- IRAM functions were genuinely placed in IRAM regions, so the negative/neutral timing is meaningful.

## J. Integration decision

No production source patch was attempted.

Reasons:

- Existing `fast_inv()` is already optimal for the safe two-step reciprocal pattern.
- Faster one-step reciprocal changes numerical behavior and needs path-specific visual validation.
- RGB565 color variants were either neutral/small or not exact.
- Span copy/fill wins are large but belong to a future Image/blit/fill integration pass with thresholds.
- Fixed16 texture-coordinate wins are real but require shader-level guards and fallback; this session only proved the ESP microbench side.
- IRAM placement did not justify source complexity.

Because no production patch survived microbench screening, TGX benchmark sanity and real examples were not run.

## K. Final source diff

Final checks:

```text
git status --short
?? validation/performance/builds/
?? validation/performance/investigations/2026-06-esp32-esp32s3-kernels/
?? validation/performance/investigations/2026-06-shader-fixedpoint-integration/
?? validation/performance/logs/
?? validation/performance/parsed/
?? validation/performance/telemetry/
```

```text
git diff --stat

<empty>
```

```text
git diff --name-only

<empty>
```

```text
git diff --check

<empty>
```

No production TGX source file is modified. The extra `2026-06-shader-fixedpoint-integration/` untracked directory was present before this session and was left untouched.

## L. Rejected candidates

- Local two-step reciprocal asm: rejected because it matches current `tgx::fast_inv()` performance/codegen.
- One-step reciprocal asm: rejected for production because it is approximate; useful future diagnostic for perspective paths.
- `rsqrt0.s` local variants: rejected because current `fast_invsqrt()` is already strong.
- `RGB565::blend256` alpha shortcut: rejected because it was slower.
- `mult256` extract scalar: rejected because it only helped Core2 modestly and was neutral on CoreS3.
- `RGBf -> RGB565` manual scalar: rejected because isolated gain did not justify changing generic color conversion without hot-path proof.
- Triangle direct component interpolation: rejected because it was not exact.
- Pairwise `meanColor4`: rejected because it was not exact.
- `dsps_memcpy` as immediate TGX patch: rejected for this session because it is span-copy specific, short-span sensitive, and needs Image/blit integration work.
- Fixed16 shader coordinates: rejected for immediate production because large-step and perspective cases are unsafe without guards.
- `IRAM_ATTR`: rejected because timing was neutral or negative.

## M. Next steps

Best next sessions:

1. ESP32-S3 Image/span copy/fill integration using `memcpy`/`dsps_memcpy`, unroll4, 32-bit stores, and length thresholds.
2. ESP shader fixed16 affine texture-coordinate integration with strict guard/fallback conditions.
3. Visual validation of one-step `recip0.s` in perspective-only shader microbenchmarks.
4. Deeper ESP-DSP/ESP32-S3 SIMD exploration for long span kernels, especially if a maintainable RGB565 vector pack path is identified.
