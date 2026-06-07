# TGX Performance Investigation History

This is the compact index of performance work already done. It is meant to avoid repeating broad searches while keeping the repository small.

For current benchmark comparisons, use `../baselines/current/`.
For reusable upload/capture and parsing scripts, use `../tools/`.

## Decisions To Carry Forward

| Investigation | Boards | Kept | Rejected / do not repeat |
| --- | --- | --- | --- |
| 2026-06 config / inline / renderer layout | Teensy 4.1, Core2, CoreS3, Pico2, Pico W partly unavailable | `TGX_INLINE_ZDIVIDE` policy was already committed before archive; RP2350 shader incremental pointer path remained baseline | Broad category inline macros; coarse `TGX_INLINE_IMAGE=`; Image helper splits; direct renderer/layout perturbations. Reasons: hidden local regressions, wire/dots regressions, texture/bilinear regressions, fragile layout-only gains. |
| 2026-06 ARM DSP RGB565 | Teensy 4.1, Pico2 | `meanColor(RGB565, RGB565)` exact packed average was committed separately | Most RGB565 DSP variants. Reasons: scalar code already strong, exactness issues, or gains not relevant to hot paths. |
| 2026-06 ARM RGB565 hot 3D | Teensy 4.1, Pico2 | No production patch | Bilinear/triangle/color modulation/RGBf conversion variants. Reasons: no robust exact speedup on actually hot 3D paths. |
| 2026-06 raster span kernels | Teensy 4.1, Pico2 | No production patch | Direct integration from the span benchmark. Reason: fixed-point texture-coordinate result was promising but did not yet match full shader semantics. |
| 2026-06 texture-coordinate fixed point | Teensy 4.1, Pico2 | No production patch | Perspective fixed approximation. Reason: errors too large. Safe subset found: affine orthographic interior spans, fixed16 accumulators, len >= 8; nearest exact in tested subset; bilinear bounded but approximate. |
| 2026-06 shader fixed-point integration | Teensy 4.1, Pico2 | No production patch | Nearest-only shader patch and follow-ups. Reasons: hidden benchmark regressions, especially bilinear texture rows, despite promising nearest rows. |
| 2026-06 ESP32 / ESP32-S3 kernels | Core2, CoreS3 | No production patch | Broad math, RGB565, IRAM, texture-coordinate integration. Reasons: `fast_inv()` already good; IRAM neutral/negative; RGB565 exact wins too small or not relevant; fixed16 shader work needs separate guarded integration. |
| 2026-06 ESP32-S3 Image span copy/fill | Core2, CoreS3 | Current WIP source patch in `src/Image.inl`: same-format `Image::blit()` uses direct 1x1 assignment and `memcpy()` only for provably non-overlapping ranges, keeping `memmove()` for overlap/tiny copies | `dsps_memcpy()` production path, RGB565 fill changes, manual 32-bit copy/unroll helpers. Reasons: overlap safety, dependency/threshold complexity, existing RGB565 fill already optimized. |

## Reports Retained

Only final reports or compact summaries are kept in dated subdirectories. Raw captures, build products, full objdump dumps, candidate matrices, and old per-candidate patch files have been pruned.

Retained files:

- `2026-06-tgx-inline-renderer-layout/SUMMARY.md`
- `2026-06-tgx-inline-renderer-layout/reports/REPORT_CONFIG_OPTIMIZATION.md`
- `2026-06-tgx-inline-renderer-layout/reports/REPORT_INLINE_GRANULARITY.md`
- `2026-06-tgx-inline-renderer-layout/reports/REPORT_IMAGE_INLINE_INVESTIGATION.md`
- `2026-06-tgx-inline-renderer-layout/reports/REPORT_RENDERER_LAYOUT_INVESTIGATION.md`
- `2026-06-arm-dsp-rgb565/REPORT_ARM_DSP_RGB565.md`
- `2026-06-arm-dsp-rgb565-hot3d/REPORT_ARM_DSP_RGB565_HOT3D.md`
- `2026-06-raster-span-kernels/REPORT_RASTER_SPAN_KERNELS.md`
- `2026-06-texture-coord-fixedpoint/REPORT_TEXTURE_COORD_FIXEDPOINT.md`
- `2026-06-shader-fixedpoint-integration/REPORT_SHADER_FIXEDPOINT_INTEGRATION.md`
- `2026-06-esp32-esp32s3-kernels/REPORT_ESP32_ESP32S3_KERNELS.md`
- `2026-06-esp32s3-image-span-copyfill/REPORT_ESP32S3_IMAGE_SPAN_COPYFILL.md`

## Future Policy

During an active session, temporary logs and build products may exist. Before committing, collapse them into:

- a final report;
- a small number of baseline/result CSVs only if they are reusable;
- scripts under `../tools/` if generally useful.

Do not commit bulky raw capture trees or generated build folders.
