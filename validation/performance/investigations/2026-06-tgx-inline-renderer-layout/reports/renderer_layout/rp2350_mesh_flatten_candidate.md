# rp2350_mesh_flatten_candidate

Patch: `tmp/renderer_layout_investigation/patches/rp2350_mesh_flatten_candidate.patch`

Intent: apply GCC `flatten` only to the RP2350 `Renderer3D::_drawMesh(Mesh3Dv2)` implementation. This was a targeted test to see whether the unsafe gains from forced `rasterizeTriangle()` inlining could be captured only in the Mesh3Dv2 path, without changing triangle/quad/ribbon/wire call sites.

Result on Pico2: rejected.

Global result:
- Run: `pico2_renderer_rp2350_mesh_flatten_candidate_bench_pico2`
- Status: `SUCCESS`
- Mean subtest delta: -4.52%.
- Median subtest delta: -1.89%.
- Subtests > +1%: 35.
- Subtests below -1%: 85.
- Subtests below -3%: 77.
- Subtests below -5%: 65.
- Binary size grew substantially: `uf2_bytes=2872832` versus baseline `2762752`, and larger than the unsafe forced-rasterizer-inline candidate.

Worst regressions:
- `R2D2 GOURAUD`, 100x100: -48.56%.
- `Bunny GOURAUD`, 100x100: -47.49%.
- `R2D2 GOURAUD`, 200x200: -46.46%.
- `Buddha GOURAUD`, 200x200: -45.96%.
- `Buddha GOURAUD`, 320x240: -45.40%.
- `Ribbon strip`, 100x100: -41.84%.
- `R2D2 TEX_BILINEAR`, 100x100: -34.62%.

Largest gains:
- `Bunny ORTHO TEX`, 100x100: +48.81%.
- `Sphere direct GOUR`, 100x100: +43.87%.
- `Bunny ORTHO TEX`, 200x200: +39.98%.
- `Wire rib quad thick`, 100x100: +37.38%.
- `R2D2 farclip`, 320x240: +34.66%.

Codegen notes:
- `_drawMesh(Mesh3Dv2)` exploded from 7680 bytes to 62692 bytes.
- Visible `shader_select` shrank from 1562 bytes to 630 bytes because much of its reachable code was cloned/inlined into `_drawMesh`.
- Several `uber_shader` instantiations appeared as new low-address symbols, indicating major code layout reshuffling rather than a small local improvement.
- Image/BSeg helpers retained the same sizes but were moved by a very large offset, with different alignments.

Conclusion:
The targeted `flatten` approach is not viable. It confirms that aggressive inlining can create spectacular gains in a few rows, but the cost is large code growth, unstable layout, and severe useful-path regressions.
