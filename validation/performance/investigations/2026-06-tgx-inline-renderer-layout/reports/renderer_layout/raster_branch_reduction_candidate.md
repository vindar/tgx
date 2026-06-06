# raster_branch_reduction_candidate

Patch: `tmp/renderer_layout_investigation/patches/raster_branch_reduction_candidate.patch`

## Change

Changed the `uint16_t` z-buffer `current_z` computation inside `uber_shader(...)` from a template-constant ternary to explicit `if constexpr (USE_ORTHO)` branches.

This tested whether the RP2350 compiler left less favorable code around the z-buffer path or register allocation when the orthographic/perspective choice was written as a ternary.

## Pico2 benchmark result

Run: `pico2_renderer_raster_branch_reduction_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were unchanged at displayed precision:

- score 1: `23.04`
- score 2: `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `+0.0265%`
- median: `0%`
- regressions below `-1%`: `0`
- worst: `Wire rib quad fast` 200x200, `-0.7195%`
- best: `Wire cube fast` 100x100, `+2.0077%`

Important rows:

- `Wire rib tri thick`: exactly neutral across sizes.
- `Point cloud dots/pixels`: neutral to small positive.
- `Bunny TEX_BILINEAR`: within +/- `0.08%`.
- `R2D2 far/farclip`: within +/- `0.06%` except no meaningful movement.
- `Sphere direct GOUR`: within `-0.23%` to `-0.04%`.

## Decision

Rejected as an optimization. The patch is safe on Pico2 but essentially neutral and does not capture the mesh-heavy gains sought in this session. It may be a readability cleanup later, but this campaign should not keep neutral churn.
