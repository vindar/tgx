# Renderer Codegen/Layout Summary

Primary board: Pico2 / RP2350.

Tools:
- `arm-none-eabi-nm -C -S --size-sort`
- Arduino build `.elf`, `.bin`, `.uf2`, and `.map` files under `tmp/hardware_validation/builds/`
- Extracted CSVs under `tmp/renderer_layout_investigation/codegen/`

## Baseline

Baseline reused from `pico2_baseline_bench_pico2_run2` at commit `5fbe8cf`.

Important baseline symbols:
- `rasterizeTriangle`: 1110 bytes, address `0x10006d84`, align32 `4`.
- `shader_select`: 1562 bytes, address `0x10019240`, align32 `0`.
- `_drawTriangle`: 3916 bytes, address `0x1000dc8c`.
- `_drawMesh(Mesh3Dv2)`: 7680 bytes, address `0x10012268`.
- `_drawQuad`: 5176 bytes, address `0x100101b8`.

## Forced Rasterizer Inline

Candidate: `renderer_raster_pointer_increment_candidate`

This was the strongest Pico2 speed signal but unsafe.

Codegen changes:
- `rasterizeTriangle` disappears as a standalone hot symbol.
- `_drawTriangle`: 3916 -> 5100 bytes.
- `_drawQuad`: 5176 -> 6300-ish range depending extracted symbol variant, with large growth.
- `_drawTriangleStrip`: 4820 -> 6070 bytes.
- `_drawTriangleClippedSub`: 1210 -> 2272 bytes.
- `_drawMesh(Mesh3Dv2)`: 7680 -> 8830 bytes.
- `shader_select` keeps the same size, but moves from `0x10019240` to `0x10011944`.
- Image/BSeg/wire symbols keep their sizes but move by about the same amount as the binary growth and their alignments shift.

Interpretation:
- The mesh-heavy gains are a direct consequence of removing the per-triangle rasterizer call and changing the surrounding layout.
- The thick-wire regressions are layout/alignment fallout, not direct changes to BSeg source code.
- The gain is not acceptable because `Wire rib strip/tri thick` regressions exceed the hard -3% guard by a wide margin.

## Static Shader Dispatch

Candidate: `renderer_shader_static_dispatch_candidate`

Codegen changes:
- The old dynamic `rasterizeTriangle` symbol is replaced by a static-dispatch instantiation, but both are 1110 bytes.
- `_drawTriangle`: 3916 -> 4028 bytes.
- `_drawQuad`: 5176 -> 5300 bytes.
- `_drawTriangleStrip`: 4820 -> 4872 bytes.
- `_drawMesh(Mesh3Dv2)`: essentially unchanged in size.
- `shader_select` stays 1562 bytes but moves to `0x10011944`.
- BSeg/Image helpers keep size but shift alignment/address.

Interpretation:
- Making the shader function a non-type template parameter does not reduce the rasterizer body.
- The large positive/negative benchmark swings are mostly layout/codegen placement effects.
- Severe texture/ribbon regressions make this unsafe.

## Mesh3Dv2 Flatten

Candidate: `renderer_rp2350_mesh_flatten_candidate`

Codegen changes:
- `_drawMesh(Mesh3Dv2)`: 7680 -> 62692 bytes.
- Visible `shader_select`: 1562 -> 630 bytes because substantial code was cloned into `_drawMesh`.
- New nearby `uber_shader` instantiations appear.
- `.uf2`: 2762752 -> 2872832 bytes.

Interpretation:
- This is too aggressive. It confirms that inlining/cloning can produce isolated gains, but it destroys instruction locality and creates broad regressions.

## Shader Select Fast Paths

Candidates:
- `renderer_shader_select_exact_fastpath_candidate`
- `renderer_shader_select_nearest_gouraud_fastpath_candidate`

Code size:
- Exact fast path: `.uf2` 2764800, about +2048 bytes.
- Narrow nearest/Gouraud fast path: `.uf2` 2764288, about +1536 bytes.

Interpretation:
- Small explicit dispatch changes can move performance significantly on Pico2.
- The exact fast path greatly improves several ORTHO/far rows but severely regresses bilinear and flat paths.
- The narrowed fast path still regresses wire-thick, flat, and bilinear rows beyond hard gates.
- These patches remain layout-sensitive and are not safe despite modest code-size growth.

## Overall Attribution

The RP2350 benchmark is highly sensitive to code layout in the renderer/shader region. The safe local edits either measured neutral or produced only noise-level changes. The large gains require either:
- inlining/cloning large hot templates, or
- changing dispatch/layout enough to move unrelated hot functions.

Both routes created hidden local regressions. No tested source change provided a stable improvement that preserved wire/dots, flat, bilinear, and real mesh paths simultaneously.
