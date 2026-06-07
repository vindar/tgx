# 2026-06 JPEG wrapper RGB565 row-copy investigation

This investigation tested one small portable optimization in the JPEGDEC wrapper:
use row-wise `memcpy` for safe opaque in-bounds RGB565 JPEG blocks instead of copying
one pixel at a time.

The production change is deliberately limited to `src/Image.inl` and does not use
platform-specific memcpy, DSP, DMA, SIMD, renderer layout changes, or shader changes.

## Key files

- `SESSION_START.md`: starting repository/hardware state.
- `notes/decoder_wrapper_audit.md`: JPEG/PNG/GIF wrapper semantics audit.
- `results/decoder_wrapper_inventory.csv`: wrapper inventory.
- `sketches/JpegRgb565CopyBench/`: display-free simulated JPEG RGB565 copy benchmark.
- `sketches/JpegDecoderTelemetry/`: display-free real JPEGDEC wrapper telemetry sketch.
- `tools/parse_jpeg_rgb565_copy_bench.py`: parser for copy benchmark telemetry.
- `results/jpeg_rgb565_copy_summary.csv`: local old-loop vs row-memcpy before source patch.
- `results/jpeg_memcpy_before_after_summary.csv`: post-patch run including the `production` variant.
- `results/row_memcpy_selected_speed_summary.csv`: speed summary for the selected threshold.
- `results/correctness_summary.csv`: mismatch summary.
- `results/jpeg_example_telemetry.csv`: real JPEGDEC post-patch telemetry.
- `patches/jpeg_rgb565_memcpy_candidate.patch`: production patch.
- `REPORT_JPEG_WRAPPER_COPY.md`: final report.

## Final decision

Keep the JPEG RGB565 optimization with a conservative guard:

- opacity is fully opaque;
- JPEG block is fully inside the destination image;
- block width is at least 16 pixels;
- block width is a multiple of 8.

Other cases keep the original pixel loop or `drawPixel` fallback.
