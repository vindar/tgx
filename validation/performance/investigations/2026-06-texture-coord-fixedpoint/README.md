# Texture Coordinate Fixed-Point Investigation

This investigation follows the raster/span microbenchmark work. The previous
span benchmark showed that fixed-point incremental texture coordinates can be
much faster than per-pixel float texture coordinate handling, but it used a
controlled 8-bit fractional model rather than the real TGX shader semantics.

This pass audits the production texture-coordinate behavior in `src/Shaders.h`
and measures display-free equivalence/performance on:

- Teensy 4.1 / Cortex-M7, `COM3`
- Pico2 / RP2350 / Cortex-M33, `COM21`

Key result:

- A restricted affine interior fast path is promising.
- `fixed16_interior_nearest` is exact in the tested subset.
- `fixed16_interior_bilinear` is nearly exact: remaining differences are at
  most one RGB565 channel level, with no pixels above one quantization step.
- Perspective fixed-coordinate approximation is not safe.
- No production source patch was kept in this session. A real integration
  should be a focused `src/Shaders.h` patch with scanline safety/fallback and
  real render-hash/benchmark validation.

Important files:

- `SESSION_START.md`: session setup and constraints.
- `notes/texture_coord_semantics.md`: shader semantics audit.
- `results/texture_coord_equivalence.csv`: full parsed row data.
- `results/texture_coord_summary.csv`: grouped benchmark/error summary.
- `results/fixed16_interior_by_length.csv`: span-length threshold summary.
- `results/candidate_decision_summary.csv`: candidate decisions.
- `codegen/CODEGEN_SUMMARY.md`: symbol/codegen notes.
- `REPORT_TEXTURE_COORD_FIXEDPOINT.md`: final report.
