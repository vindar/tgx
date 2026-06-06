# shader_static_dispatch_candidate

Patch: `tmp/renderer_layout_investigation/patches/shader_static_dispatch_candidate.patch`

Intent: remove the runtime shader-function argument from `rasterizeTriangle()` by making the shader function a non-type template parameter. This tested whether the large Pico2 gain from forced `rasterizeTriangle()` inlining came primarily from eliminating an indirect call to `shader_select()`, without forcing the whole rasterizer into every caller.

Result on Pico2: rejected.

Global result:
- Run: `pico2_renderer_shader_static_dispatch_candidate_bench_pico2`
- Status: `SUCCESS`
- Mean subtest delta: -1.04%.
- Median subtest delta: -1.03%.
- Subtests > +1%: 35.
- Subtests below -1%: 82.
- Subtests below -3%: 56.
- Subtests below -5%: 37.
- Binary size: `uf2_bytes=2763264`, only slightly above baseline and far below the forced-inline rasterizer candidate.

Worst regressions:
- `Ribbon quads`, 100x100: -65.72%.
- `Ribbon quads`, 200x200: -52.00%.
- `Ribbon quads`, 320x240: -45.94%.
- `Sphere direct TEX`, 100x100: -39.66%.
- `Ribbon triangles`, 100x100: -37.03%.
- `R2D2 TEX_NEAREST`, 100x100: -36.32%.
- `Grid small TEX`, 100x100: -31.11%.
- `Bunny far`, 320x240: -30.96%.

Largest gains:
- `Bunny ORTHO TEX`, 100x100: +89.65%.
- `R2D2 farclip`, 320x240: +77.20%.
- `R2D2 far`, 320x240: +72.94%.
- `R2D2 farclip`, 200x200: +67.65%.
- `Bunny ORTHO TEX`, 200x200: +66.39%.

Codegen notes:
- `rasterizeTriangle` did not shrink. The old dynamic-symbol instance was 1110 bytes and the static-dispatch instance is also 1110 bytes.
- `_drawTriangle` grew from 3916 to 4028 bytes.
- `_drawQuad` grew from 5176 to 5300 bytes.
- `_drawTriangleStrip` grew from 4820 to 4872 bytes.
- `shader_select` stayed 1562 bytes but moved from `0x10019240` to `0x10011944`.
- BSeg/Image helpers did not change size, but their address/alignment shifted, e.g. `_bseg_draw_template<1>` align32 moved from 0 to 12.

Conclusion:
The hypothesis was false in this form: making the shader function a template parameter did not directly improve the rasterizer, and instead produced a highly unstable layout/codegen result with severe texture and ribbon regressions. This patch is not safe or maintainable as an optimization.
