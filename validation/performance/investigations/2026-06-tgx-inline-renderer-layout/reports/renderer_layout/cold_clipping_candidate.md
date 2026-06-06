# cold_clipping_candidate

Patch: `tmp/renderer_layout_investigation/patches/cold_clipping_candidate.patch`

## Change

Added an RP2350-only `noinline` attribute for `_drawTriangleClipped(...)` and `_drawTriangleClippedSub(...)`, both already marked `TGX_NOINLINE` but not actually forced noinline on RP2350.

This tested whether moving recursive clipping code out of the hot Renderer3D layout could help mesh-heavy paths or far/farclip benchmarks.

## Pico2 benchmark result

Run: `pico2_renderer_cold_clipping_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were essentially unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `+0.0132%`
- median: `0%`
- regressions below `-1%`: `1`
- worst: `Point cloud pixels` 200x200, `-1.3498%`
- best: `Wire cube fast` 100x100, `+2.1654%`

Far/farclip rows were neutral:

- `R2D2 far/farclip`: around `-0.047%` to `+0.153%`.
- `Buddha far/farclip`: around `-0.047%` to `+0.049%`.
- `Bunny far/farclip`: around `-0.216%` to `+0.028%`.

Guard rows:

- `Wire rib tri thick`: neutral.
- `Point cloud pixels`: one `-1.35%` regression at 200x200, positives at other sizes.
- `Wire cube fast`: mixed, `+2.17%`, `+1.32%`, then `-0.79%`.

## Decision

Rejected. The patch does not improve the targeted clipping/far paths and only causes small layout-like movement elsewhere.
