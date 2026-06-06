# shader_select_hot_inline_candidate

Patch: `tmp/renderer_layout_investigation/patches/shader_select_hot_inline_candidate.patch`

## Change

Added an RP2350-only `inline __attribute__((always_inline))` macro for `shader_select(...)` and applied it to the shader dispatcher.

This was the inverse of `cold_shader_selection_candidate`: test whether the dispatch callable was not being inlined aggressively enough through `rasterizeTriangle` on RP2350.

## Pico2 benchmark result

Run: `pico2_renderer_shader_select_hot_inline_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were essentially unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `-0.0606%`
- median: `0%`
- regressions below `-1%`: `6`
- worst: `Wire cube fast` 200x200, `-2.8709%`
- best: `Wire sphere fast` 100x100, `+2.5642%`

Important rows:

- `Buddha/R2D2/Bunny FLAT/GOURAUD`: near zero.
- `R2D2 far/farclip`: near zero.
- `Bunny TEX_BILINEAR`: near zero.
- `Point cloud pixels`: `-2.6295%` at 200x200.
- `Wire cube fast`: `-1.20%`, `-2.87%`, `-1.99%`.
- `Wire rib tri thick`: neutral.

Binary size did not meaningfully change: `bin_bytes=1380968`, same as baseline, with a slightly larger map file.

## Decision

Rejected. Explicitly forcing `shader_select` inline does not improve mesh/shader paths and creates guard-row regressions.
