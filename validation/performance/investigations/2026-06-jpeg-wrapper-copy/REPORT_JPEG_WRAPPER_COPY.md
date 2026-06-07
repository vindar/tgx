# JPEG wrapper RGB565 row-copy report

## A. Executive summary

Final recommendation: keep the small `src/Image.inl` JPEG RGB565 row-copy optimization.

The retained source patch changes only `_jpegdec_color_convert565()`:

- JPEGDEC RGB565 little-endian destination path only;
- opacity `>= 1.0f`;
- decoded block fully in bounds;
- `iWidth >= 16` and `iWidth` multiple of 8;
- row-wise `memcpy`;
- original per-pixel loop retained for narrow/partial-width in-bounds blocks;
- original `drawPixel<true>` fallback retained for clipped/opacity cases.

Boards tested with robust upload/capture:

- Core2 / ESP32 on `COM5`
- CoreS3 / ESP32-S3 on `COM10`
- Pico2 / RP2350 on `COM21`
- Teensy 4.1 on `COM3`

Correctness: 0 mismatching cases on all four boards in the simulated JPEG RGB565 copy bench, including the patched production helper.

Best selected-path microbench gains, old local pixel loop vs row `memcpy`:

| Board | Mean speedup for selected widths | Min selected speedup | Mismatches |
| --- | ---: | ---: | ---: |
| Core2 | +252.3% | +37.7% | 0 |
| CoreS3 | +239.8% | +30.3% | 0 |
| Pico2 | +165.0% | +5.8% | 0 |
| Teensy 4.1 | +16.6% | +0.8% | 0 |

## B. Source audit

JPEG/PNG/GIF wrappers were audited in `src/Image.inl`.

JPEG is the only simple safe row-copy candidate. `Image<color_t>::JPEGDecode()` requests JPEGDEC RGB565 little-endian output when the destination is `Image<RGB565>`, so `_jpegdec_color_convert565()` receives a decoder-owned RGB565 block. In the opaque fully in-bounds path, destination rows are contiguous for `iWidth` pixels and destination stride is explicit, making row `memcpy` equivalent to the old per-pixel copy.

PNG was not optimized. `_pngdec_color_convert()` handles grayscale, truecolor, truecolor+alpha, indexed palettes, multiple bit depths, clipping, and opacity blending.

GIF was not optimized. `gif_decode()` maps index bytes through RGB565/RGB888 palettes and handles transparency/disposal/opacity. It is not a raw RGB565 row copy.

Details:

- `notes/decoder_wrapper_audit.md`
- `results/decoder_wrapper_inventory.csv`

## C. Example porting

Created display-free portable validation sketches instead of duplicating display-driver examples:

- `sketches/JpegRgb565CopyBench/`
- `sketches/JpegDecoderTelemetry/`

`JpegDecoderTelemetry` is the practical cross-board JPEG wrapper example for this pass: it decodes the existing `batman.h` JPEG asset into an `Image<RGB565>`, measures decode time, and prints a framebuffer hash without needing ILI9341/LovyanGFX/TFT_eSPI setup.

Existing Teensy PNG/GIF examples were compile-checked:

- `examples/Teensy4/2D/extensions/PNG_test`: compile OK
- `examples/Teensy4/2D/extensions/GIF_test`: compile OK

PNG/GIF public ports for Core2/CoreS3/Pico2 were not created because no PNG/GIF production optimization was attempted and the GIF asset is large. Status is recorded in `results/png_gif_example_status.csv`.

## D. Microbench correctness

Display-free simulated JPEG RGB565 copy bench:

- rows parsed before patch: 1680
- summary cases before patch: 840
- rows parsed after patch: 2520
- summary cases after patch: 840
- mismatching cases: 0

The bench covered:

- widths `1,2,3,4,5,8,9,16,31,32,63,64,127,128,240,319,320`;
- heights `1,2,4,8,16,32`;
- tight stride and larger destination stride;
- odd widths;
- x/y offsets;
- in-bounds opaque path;
- opacity fallback;
- left/top/right/bottom clipping fallback.

Correctness summary:

```text
Core2     row_memcpy=0 mismatches, production=0 mismatches
CoreS3    row_memcpy=0 mismatches, production=0 mismatches
Pico2     row_memcpy=0 mismatches, production=0 mismatches
Teensy41  row_memcpy=0 mismatches, production=0 mismatches
```

## E. Microbench performance

Unconditional row `memcpy` was very fast for medium/large rows but regressed on very narrow rows, especially strided width `1..9` cases. Therefore the production patch is thresholded.

Selected production condition:

```cpp
(pDraw->iWidth >= 16) && ((pDraw->iWidth & 7) == 0)
```

Selected-path local row-copy speed summary:

- Core2: mean +252.3%, median +263.6%, min +37.7%
- CoreS3: mean +239.8%, median +235.8%, min +30.3%
- Pico2: mean +165.0%, median +154.4%, min +5.8%
- Teensy4.1: mean +16.6%, median +18.5%, min +0.8%

Files:

- `results/jpeg_rgb565_copy_summary.csv`
- `results/row_memcpy_selected_speed_summary.csv`
- `results/jpeg_memcpy_before_after_summary.csv`

## F. Production patch

Patch file:

- `patches/jpeg_rgb565_memcpy_candidate.patch`

Source file changed:

- `src/Image.inl`

The patch is portable and uses normal `memcpy`. It does not add:

- `dsps_memcpy`;
- ESP-DSP;
- DMA;
- SIMD;
- architecture-specific guards;
- renderer/shader/layout changes.

## G. Real JPEG example validation

`JpegDecoderTelemetry` ran successfully on all four boards after the patch:

| Board | Result | Open | Time us | FB hash | Nonzero |
| --- | ---: | ---: | ---: | --- | ---: |
| Core2 | 1 | 1 | 37207 | `90420DE9` | 51524 |
| CoreS3 | 1 | 1 | 21345 | `51B99722` | 52092 |
| Pico2 | 1 | 1 | 36422 | `90420DE9` | 51524 |
| Teensy4.1 | 1 | 1 | 5100 | `90420DE9` | 51524 |

CoreS3 produced a different hash/nonzero count from the other boards in the default build. A follow-up visual pass showed the image was visually correct and isolated the difference to JPEGDEC's ESP32-S3 SIMD YCbCr-to-RGB565 conversion path. Rebuilding the same CoreS3 sketch with `-DNO_SIMD` produced the same hash as Core2/Pico2/Teensy (`90420DE9` for the display-free decoder telemetry and `93FA7269` for the visual sketch). This is not caused by TGX's row-copy optimization.

Files:

- `results/jpeg_example_telemetry.csv`
- `results/jpeg_example_delta.csv`
- `results/jpeg_visual_upload_summary.csv`
- `notes/cores3_jpeg_hash_difference.md`

No same-session before-patch real JPEG decode baseline was collected; the before/after evidence is the display-free copy microbench.

## H. PNG/GIF status

PNG/GIF production code was audited but not changed.

Status file:

- `results/png_gif_example_status.csv`

Teensy compile-only checks passed for existing examples:

- PNG: compile OK
- GIF: compile OK

## I. CPU validation

CPU validation after the source patch:

- `validation/2d/run_cpu.ps1`: PASS
- `validation/3d/run_cpu.ps1`: PASS, 82 PASS / 0 FAIL

The 3D script ran in its default tolerant mode. The patch is in the JPEG wrapper, not in the renderer.

## J. Benchmark sanity

Full TGX renderer benchmark sanity was not run. The patch is isolated to JPEGDEC wrapper decode and does not affect Renderer3D code paths except possible code layout. CPU 3D validation passed, and the hardware JPEG/copy microbench ran on all four boards.

## K. Codegen summary

See `codegen/CODEGEN_SUMMARY.md`.

Summary: the selected path removes the inner per-pixel assignment loop and delegates the row copy to the platform C library `memcpy`. The fallback loop remains for narrow/partial blocks.

## L. Final source diff

```text
src/Image.inl | 18 +++++++++++++++---
```

```text
src/Image.inl
```

`git diff --check` reported no whitespace errors; Git emitted the usual local line-ending warning for `src/Image.inl`.

## M. Rejected alternatives

- Unconditional `memcpy`: rejected because narrow rows regressed on several boards.
- `dsps_memcpy`: intentionally out of scope for this portable JPEG pass.
- PNG production optimization: rejected for this pass because the wrapper is conversion/alpha/palette-heavy.
- GIF production optimization: rejected for this pass because source pixels are palette indices and transparency/disposal semantics matter.
- Architecture-specific paths: intentionally out of scope.

## N. Next steps

Useful follow-ups:

- Add public display-backed JPEG wrapper examples for M5Stack/Core2/CoreS3 and Pico2 using the telemetry style from `JpegDecoderTelemetry`.
- Consider a PNG truecolor opaque RGB888-to-RGB565 row conversion microbench.
- Consider a GIF palette-to-RGB565 span helper microbench if animated GIF examples become performance-sensitive.
