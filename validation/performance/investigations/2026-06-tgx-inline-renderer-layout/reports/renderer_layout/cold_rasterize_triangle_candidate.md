# cold_rasterize_triangle_candidate

Patch: `tmp/renderer_layout_investigation/patches/cold_rasterize_triangle_candidate.patch`

## Change

Added an RP2350-only `noinline` macro for `rasterizeTriangle(...)` and applied it to the triangle rasterizer template.

This tested whether keeping the per-triangle raster setup out of call-site layout could reduce Renderer3D/shader code pressure without adding per-pixel overhead.

## Pico2 benchmark result

Run: `pico2_renderer_cold_rasterize_triangle_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were essentially unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `-0.0198%`
- median: `0%`
- regressions below `-1%`: `1`
- worst: `Wire cube AA` 100x100, `-1.3549%`
- best: `Wire cube thick` 100x100, `+0.7730%`

Mesh-heavy rows were neutral:

- `R2D2 far/farclip`: within about `+/-0.11%`.
- `Bunny TEX_BILINEAR`: `-0.108%`, `+0.078%`, `-0.031%`.
- `Buddha/R2D2/Bunny FLAT`: neutral to small negative.
- `Sphere direct GOUR`: `-0.263%`, `+0.095%`, `-0.099%`.

Guard rows were mostly safe but not improved:

- `Wire rib tri thick`: neutral.
- `Point cloud dots/pixels`: within about `+/-0.37%`.
- `Wire cube AA`: one weak regression at `-1.35%`.

## Decision

Rejected. This is not harmful enough to trigger a hard gate, but it does not provide a measurable useful gain and adds an RP2350-specific attribute. No cross-board testing needed.
