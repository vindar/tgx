# JPEG wrapper RGB565 row-copy session start

Date: 2026-06-07

## Repository state

- Branch: `feature/multi-directional-lights`
- HEAD: `3ac614a247e52a003a8e04c75e0eec68c688dc92`
- `git status --short`: clean at session start
- `git diff --stat`: empty at session start
- `git diff --check`: clean at session start

## Connected boards

| Board | Port | Notes |
| --- | --- | --- |
| Core2 / M5Stack Core2 / ESP32 | `COM5` | Runtime target for decoder/copy bench |
| CoreS3 / M5Stack CoreS3 / ESP32-S3 | `COM10` | Runtime target for decoder/copy bench |
| Pico2 / RP2350 + ILI9341 | `COM21` | Runtime target for decoder/copy bench |
| Teensy 4.1 + ILI9341 | `COM3` | Runtime target for decoder/copy bench |

Reusable upload/capture tooling lives in `validation/performance/tools/`, especially
`upload_and_capture.ps1`. This session will not use ad-hoc serial capture.

## Existing decoder wrappers found

All wrappers are implemented in `src/Image.inl`:

- PNGdec wrapper:
  - `_PNGDecWrapper`
  - `_pngdec_color_convert`
  - `Image<color_t>::PNGDecode`
  - `PNGDraw`
- JPEGDEC wrapper:
  - `_JPEGDECWrapper`
  - `Image<color_t>::JPEGDecode`
  - `_jpegdec_color_convert8888`
  - `_jpegdec_color_convert565`
  - `JPEGDraw`
- AnimatedGIF wrapper:
  - `_GIFWrapper`
  - `Image<color_t>::GIFplayFrame`
  - `gif_decode`
  - `GIFDraw`

## Existing examples/assets found

Existing decoder examples are currently Teensy-oriented:

- `examples/Teensy4/2D/extensions/JPEG_test/JPEG_test.ino`
  - asset: `batman.h`, about 69 KB
- `examples/Teensy4/2D/extensions/PNG_test/PNG_test.ino`
  - asset: `octocat_4bpp.h`, about 4.8 KB
- `examples/Teensy4/2D/extensions/GIF_test/GIF_test.ino`
  - asset: `earth_128x128.h`, about 4 MB

Board display setup references:

- M5Stack/Core2/CoreS3: `examples/M5Stack/TGX_2D_Showcase`
- generic ESP32: `examples/ESP32/TGX_2D_Showcase`
- Pico2/RP2350: `examples/Pico_RP2040_RP2350/TGX_2D_Showcase`
- Teensy 4.1: existing decoder examples use `ILI9341_T4`

## Decoder libraries expected

- JPEG: BitBank2 `JPEGDEC`
- PNG: BitBank2 `PNGdec`
- GIF: BitBank2 `AnimatedGIF`

If a library is missing on a board/toolchain, the example status will record the include/build failure and the example will be skipped rather than patched around in TGX.

## Intended production optimization

Only one production optimization is in scope:

- File: `src/Image.inl`
- Function: `_jpegdec_color_convert565`
- Condition:
  - destination is `Image<RGB565>`
  - JPEGDEC output has already been requested as RGB565 little endian
  - opacity is fully opaque
  - decoded JPEG block is fully inside the destination image
  - decoder source buffer and destination framebuffer do not overlap
- Change:
  - replace the in-bounds opaque inner per-pixel copy loop with row-wise `memcpy`
  - keep clipped and opacity/blending fallback unchanged

No architecture-specific `dsps_memcpy`, DMA, SIMD, renderer/shader/layout, or inline-policy optimization is in scope.

## Validation strategy

1. Audit JPEG/PNG/GIF wrapper semantics and record where row-copy is safe.
2. Create a display-free JPEG RGB565 copy microbenchmark that simulates JPEGDEC draw blocks.
3. Validate old per-pixel copy vs row-wise `memcpy` correctness across in-bounds, clipped, odd-width, strided, and opacity cases.
4. Run the microbench with robust upload/capture on Teensy 4.1, Core2, CoreS3, and Pico2 when available.
5. Apply the small `src/Image.inl` patch only if the microbench supports correctness and useful speedup.
6. Compile/run real JPEG examples where feasible, and record PNG/GIF example status for future coverage.
7. Run CPU validation and light benchmark sanity if a production patch is retained.
8. Produce `REPORT_JPEG_WRAPPER_COPY.md` with source audit, microbench results, example status, and final source diff.
