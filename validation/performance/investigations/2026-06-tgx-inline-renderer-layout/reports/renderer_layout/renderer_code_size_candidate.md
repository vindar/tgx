# renderer_code_size_candidate

Patch: `tmp/renderer_layout_investigation/patches/renderer_code_size_candidate.patch`

## Change

Added an RP2350-only `noinline` attribute for the two private `_drawMesh(...)` implementations (`Mesh3D` and `Mesh3Dv2`). These functions are already annotated `TGX_NOINLINE`, but that macro is empty on RP2350.

This tested whether taking large per-mesh traversal code out of the RP2350 hot layout could recover mesh-heavy gains without touching per-pixel or wire/dots code directly.

## Pico2 benchmark result

Run: `pico2_renderer_code_size_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were essentially unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `-0.0045%`
- median: `0%`
- regressions below `-1%`: `3`
- worst: `Wire cube fast` 200x200, `-2.0870%`
- best: `Wire cube fast` 100x100, `+1.9289%`

Mesh-heavy rows were neutral:

- `Buddha/R2D2/Bunny FLAT/GOURAUD`: near zero.
- `R2D2 far/farclip`: near zero.
- `Bunny TEX_BILINEAR`: within `+/-0.08%`.
- `Sphere direct GOUR`: small negative, up to `-0.318%`.

Guard rows:

- `Wire rib tri thick`: neutral.
- `Point cloud pixels`: mixed, `+0.889%`, `-1.226%`, `+0.604%`.
- `Wire cube fast`: `+1.929%`, `-2.087%`, `-1.868%`.

## Decision

Rejected. Forcing private `_drawMesh` out of line does not improve mesh-heavy paths and creates unrelated layout movement in wire/point-cloud rows.
