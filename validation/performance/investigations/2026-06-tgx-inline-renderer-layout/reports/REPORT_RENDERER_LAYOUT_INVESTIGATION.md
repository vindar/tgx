# TGX Renderer Layout Investigation

Date: 2026-06-06  
Branch: `feature/multi-directional-lights`  
HEAD: `5fbe8cf57342a0bb3dd343fa63866c7ba510ae42`

## A. Executive Summary

Final recommendation: **do not keep any source change from this session**.

The source tree was returned to the committed baseline. No robust Renderer3D / shader / rasterizer optimization survived the Pico2 hard gates.

The investigation found several real Pico2 performance signals, including very large gains in mesh-heavy rows, but every large-gain candidate also introduced severe local regressions in useful paths such as thick wire, bilinear texture, flat large triangles, ribbons, or point/dot paths. The stable local edits were neutral or noise-level.

Final source files changed: **none**.

Boards used:
- Pico2 / RP2350 on `COM21`: benchmark screening and codegen focus.
- Teensy 4.1 on `COM3`: hardware probe trusted, no finalist reached cross-board stage.
- Core2 on `COM5`: hardware probe trusted, no finalist reached cross-board stage.
- CoreS3 on `COM10`: hardware probe trusted, no finalist reached cross-board stage.

## B. Starting State And Previous Results

The session started from a clean tracked source tree at `5fbe8cf`. The broad inline-category macro layer had already been reverted. The previous `TGX_INLINE_IMAGE=` result was accepted as real but unsafe:

- Pico2 global gain about +4.4%.
- Severe regressions in `Wire rib tri thick` around -19.6%.
- Point cloud/dots paths around -15%.
- Narrow Image policy attempts showed that protecting wire/dots removed the gain.

This session therefore targeted direct Renderer3D / shader / rasterizer causes instead of Image helper inlining.

Artifacts created:
- `tmp/renderer_layout_investigation/SESSION_START.md`
- `tmp/renderer_layout_investigation/hardware_ready.md`
- `tmp/renderer_layout_investigation/baseline_reuse_audit.md`
- `tmp/renderer_layout_investigation/reports/previous_image_signal_attribution.md`
- `tmp/renderer_layout_investigation/reports/codegen_renderer_summary.md`

## C. Baselines

Existing baselines were reused because the commit, board setup, macros, benchmark sketch, and capture tooling matched the current source.

Sources:
- `tmp/inline_granularity/baseline_global_scores.csv`
- `tmp/inline_granularity/baseline_subtests.csv`
- `tmp/inline_granularity/baseline_binary_size.csv`
- `tmp/inline_granularity/baseline_example_telemetry.csv`

Copied/session outputs:
- `tmp/renderer_layout_investigation/baseline_global_scores.csv`
- `tmp/renderer_layout_investigation/baseline_subtests.csv`
- `tmp/renderer_layout_investigation/baseline_binary_size.csv`
- `tmp/renderer_layout_investigation/baseline_example_telemetry.csv`

Pico2 baseline used for candidate deltas:
- Global score set: about `23.04 / 16.54 / 14.01`.
- UF2 size: `2762752` bytes.
- Noise policy reused from previous repeated runs: values below 0.5% treated as noise unless repeated and consistent.

No full baseline rerun was performed because the existing baselines were valid.

## D. Previous Image Signal Attribution

The previous coarse Image no-inline result was not a direct Image improvement for mesh rendering.

Key attribution:
- Image and BSeg helpers shrank and moved, directly explaining wire/dots regressions.
- Mesh-heavy Renderer3D/shader paths also moved substantially, explaining indirect gains.
- The safe Image candidates restored wire/dots behavior but also removed the mesh-heavy gain.

Conclusion: the useful signal was a Renderer3D/shader layout sensitivity, not an Image helper optimization.

## E. Hot Path Inventory

Inventory files:
- `tmp/renderer_layout_investigation/hot_path_inventory.csv`
- `tmp/renderer_layout_investigation/reports/hot_path_map.md`

Main hot paths studied:
- `Renderer3D::_drawMesh`
- `Renderer3D::_drawTriangle`
- `Renderer3D::_drawTriangleClipped`
- `Renderer3D::_drawQuad`
- `Renderer3D::_drawDots`
- wireframe helpers
- `rasterizeTriangle`
- `shader_select`
- `uber_shader`
- texture sampling paths
- z-buffer and RGB565 write paths

The most sensitive Pico2 areas were large template-generated shader/rasterizer regions and downstream function placement/alignment.

## F. Candidate Matrix

All patches were saved under `tmp/renderer_layout_investigation/patches/`.

| Candidate | Pico2 result | Decision | Reason |
| --- | ---: | --- | --- |
| `cold_shader_selection_candidate` | mean -0.10%, worst -4.62% | Rejected | Wire cube regressions, no mesh gain. |
| `cold_uber_shader_candidate` | mean -2.44%, worst -22.82% | Rejected | Major mesh/flat regressions. |
| `raster_branch_reduction_candidate` | mean +0.03%, worst -0.72% | Rejected as neutral | Safe but below useful threshold. |
| `texture_path_split_candidate` | mean -0.04%, worst -5.51% | Rejected | Wire cube regression. |
| `cold_rasterize_triangle_candidate` | mean -0.02%, worst -1.35% | Rejected | Neutral/slightly negative. |
| `cold_clipping_candidate` | mean +0.01%, worst -1.35% | Rejected | No useful far/farclip gain. |
| `renderer_code_size_candidate` | mean -0.00%, worst -2.09% | Rejected | Neutral. |
| `shader_select_hot_inline_candidate` | mean -0.06%, worst -2.87% | Rejected | No gain, guard-row regressions. |
| `shaderclip_inline_relax_candidate` | mean +0.02%, worst -1.02% | Rejected | Neutral. |
| `raster_pointer_increment_candidate` | mean +5.39%, best +74.62%, worst -16.01% | Rejected | Big mesh gains, severe thick-wire regressions. |
| `rp2350_mesh_layout_candidate_1` | mean +5.31%, worst -16.60% | Rejected | BSeg alignment did not fix thick-wire regressions. |
| `rp2350_mesh_layout_candidate_2` | mean +0.01%, worst -3.09% | Rejected | Plain inline lost gains and had guard regression. |
| `shader_static_dispatch_candidate` | mean -1.04%, best +89.65%, worst -65.72% | Rejected | Texture/ribbon regressions, no safe dispatch benefit. |
| `rp2350_mesh_flatten_candidate` | mean -4.52%, worst -48.56% | Rejected | `_drawMesh` grew to 62 KB, severe regressions. |
| `shader_select_exact_fastpath_candidate` | mean +1.11%, best +84.36%, worst -43.16% | Rejected | Bilinear/flat regressions too large. |
| `shader_select_nearest_gouraud_fastpath_candidate` | mean +0.24%, best +55.25%, worst -20.74% | Rejected | Still fails wire-thick, flat, bilinear gates. |

No candidate passed Pico2 screening, so no cross-board benchmark matrix or real-example finalist matrix was run. This avoided spending time on patches that already violated the primary hard gates.

## G. Detailed Pico2 Analysis

The strongest unsafe candidate was forced rasterizer inlining:

- Mean: +5.39%.
- Median: +0.60%.
- Best: `Bunny ORTHO TEX`, +74.62%.
- Strong gains also in `R2D2 far/farclip`, `Sphere direct GOUR`, and several textured mesh rows.
- Failures:
  - `Wire rib strip thick`: down to about -16%.
  - `Wire rib tri thick`: down to about -6%.
  - `Wire sphere thick`: down to about -9%.

The broad `shader_select` exact fast path was smaller but still unsafe:

- Mean: +1.11%.
- Best: `Bunny ORTHO TEX`, +84.36%.
- Failures:
  - `R2D2 TEX_BILINEAR`, 100x100: -43.16%.
  - `Bunny TEX_BILINEAR`, 100x100: -38.23%.
  - large flat triangles and cube flat: about -10%.

The narrowed nearest/Gouraud fast path did not solve the issue:

- Mean: +0.24%.
- Best: `R2D2 far`, 320x240: +55.25%.
- Failures:
  - `Wire bunny thick`: about -20%.
  - `Bunny/R2D2 TEX_BILINEAR`: -10% to -18%.
  - `Large tris FLAT Z`: about -10%.
  - `Wire rib tri/strip thick`: about -7% to -9%.

The local safe cleanups were neutral and did not justify source churn.

## H. Cross-Board Results

No candidate reached cross-board validation because none passed Pico2 hard gates.

Hardware readiness was still verified:

| Board | Port | Status |
| --- | --- | --- |
| Pico2 | `COM21` | Trusted |
| Teensy 4.1 | `COM3` | Trusted |
| Core2 | `COM5` | Trusted |
| CoreS3 | `COM10` | Trusted |

Pico W / RP2040 was intentionally ignored for runtime testing in this session.

## I. Real-Example Telemetry

No finalist candidate survived Pico2 screening, so no new real-example telemetry was run.

Existing baseline example telemetry remains available under:
- `tmp/renderer_layout_investigation/baseline_example_telemetry.csv`
- `tmp/renderer_layout_investigation/baseline_example_summary.csv`

This is intentional: running example matrices for candidates already failing synthetic guard rows would not make them acceptable.

## J. Codegen/Layout Analysis

Detailed codegen summary:
- `tmp/renderer_layout_investigation/reports/codegen_renderer_summary.md`

Key conclusions:

1. Forced `rasterizeTriangle()` inlining removes the standalone rasterizer symbol and grows multiple caller functions. It produces large mesh gains but also moves wire/Image/BSeg functions enough to cause severe local regressions.

2. Static shader dispatch did not reduce `rasterizeTriangle`; both old and new instantiations were 1110 bytes. The performance swings were therefore layout/codegen effects, not a clean dispatch-cost removal.

3. `flatten` on `_drawMesh(Mesh3Dv2)` made `_drawMesh` explode from 7680 bytes to 62692 bytes. It was clearly too aggressive.

4. Small `shader_select` fast paths can move performance a lot on Pico2 with only modest code-size growth, but the effect is still not stable: good rows and bad rows are interleaved across useful rendering modes.

Overall: Pico2/RP2350 is very layout-sensitive here. The safe changes were neutral; the high-gain changes were fragile or unsafe.

## K. Correctness Validation

No final source change was kept, so no new correctness run was required for a final candidate.

Previously established baseline correctness remains unchanged:
- CPU 3D validation had passed after the multi-light renderer restoration.
- This session leaves the source tree at the same committed code state.

## L. Final Source Diff

Final tracked source diff:

```text
git diff --stat
<empty>

git diff --name-only
<empty>

git diff --check
<clean>
```

Final `git status --short` shows only untracked report artifacts from previous/current investigations, not source modifications.

## M. Rejected Candidates

Per-candidate reports are under:
- `tmp/renderer_layout_investigation/reports/`

Patch files are under:
- `tmp/renderer_layout_investigation/patches/`

Useful rejected insights:

- Forced rasterizer inlining is the closest technical lead, but it must be solved without large layout fallout before it can be considered.
- Exact shader dispatch may be worth revisiting only with a much more controlled implementation, possibly generated per renderer configuration, but the current hand-written fast paths are too layout-sensitive.
- Generic noinline/cold marking did not help.
- Smaller source code is not automatically faster, and bigger code is not automatically slower; local row regressions matter more than global score.

## N. Remaining Risks And Next Steps

Remaining risks:
- RP2350 instruction/cache/layout sensitivity is real and not fully controllable with the current header-template structure.
- Existing benchmark rows can expose layout effects that real examples may or may not reproduce.
- No finalist reached real-example validation because no candidate passed Pico2 gates.

Recommended next steps:
- Consider a dedicated RP2350 microbenchmark that isolates `rasterizeTriangle` call overhead versus shader body cost without moving unrelated Image/BSeg code.
- If pursuing forced rasterizer inlining again, try a structural split that inlines only a small triangle setup wrapper while keeping BSeg/wire layout stable.
- Use map-file driven layout tracking before and after any future patch; do not rely on global score alone.
- Keep the source unchanged for review/merge at this point.
