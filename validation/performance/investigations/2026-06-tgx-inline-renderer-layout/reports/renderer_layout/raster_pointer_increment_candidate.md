# raster_pointer_increment_candidate

Patch: `tmp/renderer_layout_investigation/patches/raster_pointer_increment_candidate.patch`

## Change

Added an RP2350-only `inline __attribute__((always_inline))` macro to `rasterizeTriangle(...)`. Despite the patch filename, this is a rasterizer-call inlining candidate, not a new pixel-pointer increment implementation.

This tested whether flattening the per-triangle rasterizer into the Renderer3D call sites can directly recover the mesh-heavy performance gains previously seen only through indirect layout changes.

## Pico2 benchmark result

Run: `pico2_renderer_raster_pointer_increment_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores improved substantially:

- score 1: `23.93` vs baseline `23.04`
- score 2: `17.22` vs baseline median `16.54`
- score 3: `14.55` vs baseline median `14.01`

Summary:

- mean subtest delta: `+5.388%`
- median: `+0.596%`
- improved > `+1%`: `76`
- regressions below `-1%`: `27`
- regressions below `-3%`: `10`
- regressions below `-5%`: `8`
- worst: `Wire rib strip thick` 100x100, `-16.013%`
- best: `Bunny ORTHO TEX` 100x100, `+74.623%`

Major gains:

- `Bunny ORTHO TEX`: `+49.7%` to `+74.6%`.
- `R2D2 far/farclip`: `+23.3%` to `+73.8%`.
- `Sphere direct GOUR`: `+12.7%` to `+45.8%`.
- `Bunny TEX_BILINEAR`: `+9.5%` to `+16.4%`.
- `Wire cube fast`: `+23.1%` to `+31.7%`.

Major regressions:

- `Wire rib strip thick`: `-12.4%` to `-16.0%`.
- `Wire sphere thick`: `-6.0%` to `-8.9%`.
- `Wire rib tri thick`: `-4.5%` to `-6.0%`.
- `Sphere direct TEX`: `-1.65%` to `-4.58%`.
- `Bunny GOURAUD`: `-1.97%` to `-2.73%`.

Binary size grew substantially: UF2 `2774016` vs baseline `2762752` (`+11264` bytes). Program bytes reported by Arduino increased from `1358560` to `1364240` (`+5680`).

## Decision

Promising but rejected in this form. This is the first direct Renderer/rasterizer change that captures the mesh-heavy Pico2 gains, but it fails hard guard gates due severe thick wire regressions. It needs a protected variant or must be rejected.
