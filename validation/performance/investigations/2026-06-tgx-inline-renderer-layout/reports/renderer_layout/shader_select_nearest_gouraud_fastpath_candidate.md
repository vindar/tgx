# shader_select_nearest_gouraud_fastpath_candidate

Patch: `tmp/renderer_layout_investigation/patches/shader_select_nearest_gouraud_fastpath_candidate.patch`

Intent: narrow the broad exact `shader_select()` fast path to only Gouraud/no-texture and Gouraud texture-nearest/wrap cases. This removed the flat and bilinear fast paths that caused the worst regressions in the previous candidate.

Result on Pico2: rejected.

Global result:
- Run: `pico2_renderer_shader_select_nearest_gouraud_fastpath_candidate_bench_pico2`
- Status: `SUCCESS`
- Mean subtest delta: +0.24%.
- Median subtest delta: +0.48%.
- Subtests > +1%: 62.
- Subtests below -1%: 44.
- Subtests below -3%: 27.
- Subtests below -5%: 25.
- Binary size: `uf2_bytes=2764288`, only +1536 bytes over baseline.

Largest gains:
- `R2D2 far`, 320x240: +55.25%.
- `R2D2 far`, 200x200: +48.61%.
- `R2D2 farclip`, 320x240: +42.80%.
- `R2D2 farclip`, 200x200: +36.72%.
- `Sphere direct GOUR`, 100x100: +31.21%.

Worst regressions:
- `Wire bunny thick`, 100x100: -20.74%.
- `Wire bunny thick`, 200x200: -20.06%.
- `Wire bunny thick`, 320x240: -19.75%.
- `Bunny TEX_BILINEAR`, 100x100: -18.81%.
- `R2D2 TEX_BILINEAR`, 100x100: -18.72%.
- `Bunny ORTHO TEX`, 100x100: -15.42%.
- `Large tris FLAT Z`: about -10% across sizes.
- `Wire rib tri/strip thick`: about -7% to -9%.

Conclusion:
Even the narrowed exact dispatch still creates unacceptable local regressions. The gains appear strongly layout-sensitive rather than a safe dispatch-cost improvement. This candidate fails the Pico2 hard gates and was not tested cross-board.
