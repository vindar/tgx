# rp2350_mesh_layout_candidate_1

Patch: `tmp/renderer_layout_investigation/patches/rp2350_mesh_layout_candidate_1.patch`

Intent: keep the strong Pico2 gain from forced `rasterizeTriangle()` inlining, while trying to protect the thick wire/BSeg paths by aligning `_bseg_draw_template()` on RP2350.

Result on Pico2: rejected.

Summary:
- Global scores stayed strongly above baseline, similar to `raster_pointer_increment_candidate`.
- Mean subtest delta: +5.31%.
- Median subtest delta: +0.62%.
- Binary size stayed in the larger forced-rasterizer-inline range (`uf2_bytes=2774016`).
- The thick wire regressions were not fixed; several became slightly worse.

Worst Pico2 regressions:
- `Wire rib strip thick`, 100x100: -16.60%.
- `Wire rib strip thick`, 200x200: -14.01%.
- `Wire rib strip thick`, 320x240: -13.10%.
- `Wire sphere thick`, 100x100: -9.97%.
- `Wire rib tri thick`, 100x100: -8.35%.
- `Wire rib tri thick`, 320x240: -6.30%.

Largest gains:
- `Bunny ORTHO TEX`, 100x100: +74.69%.
- `R2D2 farclip`, 320x240: +73.94%.
- `R2D2 farclip`, 200x200: +66.24%.
- `R2D2 far`, 320x240: +62.47%.
- `Sphere direct GOUR`, 100x100: +46.18%.

Conclusion:
The alignment perturbation did not stabilize the unsafe forced-rasterizer-inline result. The large mesh-heavy gains are real, but the thick-wire regressions remain well beyond the -3% guard threshold. This is evidence that the wire regression is not solved by a simple alignment attribute on `_bseg_draw_template()`.
