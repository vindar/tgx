# shader_select_exact_fastpath_candidate

Patch: `tmp/renderer_layout_investigation/patches/shader_select_exact_fastpath_candidate.patch`

Intent: add RP2350-only exact `raster_type` fast paths at the top of `shader_select()` for common benchmark combinations. This tested whether skipping the nested shader-dispatch tree would recover some Pico2 mesh-heavy gains without changing `rasterizeTriangle()` or Image helpers.

Result on Pico2: rejected in this broad form.

Global result:
- Run: `pico2_renderer_shader_select_exact_fastpath_candidate_bench_pico2`
- Status: `SUCCESS`
- Mean subtest delta: +1.11%.
- Median subtest delta: +0.14%.
- Subtests > +1%: 65.
- Subtests below -1%: 47.
- Subtests below -3%: 26.
- Subtests below -5%: 16.
- Binary size grew modestly: `uf2_bytes=2764800` versus baseline `2762752`.

Largest gains:
- `Bunny ORTHO TEX`, 100x100: +84.36%.
- `Bunny ORTHO TEX`, 200x200: +62.11%.
- `Bunny ORTHO TEX`, 320x240: +54.47%.
- `Sphere direct GOUR`, 100x100: +39.73%.
- `R2D2 far`, 320x240: +38.93%.
- `R2D2 farclip`, 320x240: +30.28%.

Worst regressions:
- `R2D2 TEX_BILINEAR`, 100x100: -43.16%.
- `Bunny TEX_BILINEAR`, 100x100: -38.23%.
- `R2D2 TEX_BILINEAR`, 200x200: -33.80%.
- `R2D2 TEX_BILINEAR`, 320x240: -30.26%.
- `Bunny TEX_BILINEAR`, 200x200: -27.14%.
- `Cube direct FLAT`, 200x200: -10.50%.
- `Large tris FLAT Z`, 320x240: -10.50%.

Conclusion:
The idea is promising but the broad version is unsafe. The strongest regressions are concentrated in bilinear texture and flat paths, so a narrower Gouraud/nearest-only fast path should be tested separately.
