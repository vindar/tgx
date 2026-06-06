# cold_shader_selection_candidate

Patch: `tmp/renderer_layout_investigation/patches/cold_shader_selection_candidate.patch`

## Change

Added an RP2350-only `noinline` attribute macro for `shader_select(...)` and applied it to the shader selection function. This tested whether taking the large shader selection dispatcher out of hot call-site layout would reproduce any of the indirect Pico2 gains seen with coarse Image de-inlining.

## Pico2 benchmark result

Run: `pico2_renderer_cold_shader_selection_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were unchanged at displayed precision:

- score 1: `23.04`, baseline `23.04`
- score 2: `16.54`, baseline `16.54`
- score 3: `14.01`, baseline `14.01`

Detailed subtests showed no mesh-heavy gain and clear guard-path regressions:

- worst: `Wire cube fast` 100x100, `-4.6211%`
- `Wire cube AA` 100x100, `-3.16045%`
- `Wire cube AA` 200x200, `-3.18991%`
- `Wire cube fast` 320x240, `-3.15139%`
- `Point cloud pixels` 200x200, `-2.95896%`

Useful mesh-heavy rows were effectively neutral:

- `R2D2 (far)`: approximately `0%` to `+0.08%`
- `R2D2 (farclip)`: approximately `-0.05%` to `+0.05%`
- `Bunny TEX_BILINEAR`: approximately `-0.08%` to `-0.02%`
- `Sphere direct GOUR`: approximately `-0.05%` to `+0.31%`

Summary row: mean `-0.0979%`, median `0%`, 6 regressions below `-1%`, 4 below `-3%`.

## Decision

Rejected. The patch fails Pico2 hard gates because wire guard rows regress below `-3%` while mesh-heavy paths do not benefit. It is not worth cross-board testing.
