# Regression deep dive report - 2026-07-01

Compared revisions:

- dev starting point: `feature/point-spot-lights` at `b1ef1ac36f7542672d4aebd96b81abd344a6c4d8`
- main/release worktree: `d46bdb8f89e79ac2e571de1cf97525c9d195510f`
- final candidate: same dev commit plus the source changes listed below.

Artifacts:

- `summaries/targeted_dev_vs_main_comparison.csv`: original targeted regressions.
- `summaries/final_candidate_vs_main_dev_comparison.csv`: final candidate vs main/dev for comparable targeted tests.
- `summaries/final_candidate_validation_all_results.csv`: final candidate runs on the six connected boards for `core_raster`, `wire_points`, and `skybox_noz`.
- `codegen/*.nm.txt`, `codegen/*symbol_size_delta.csv`, and `codegen/diff_*.patch`: compact codegen/source evidence retained after pruning the large raw disassembly dumps.

## 1. Core2 / ESP32 `core_raster`

Initial regression:

- `core_raster.far_clip.flat.z`: 918.69 fps -> 776.99 fps, -15.42%.
- `core_raster.big_tri.flat.z`: 101.18 fps -> 88.74 fps, -12.30%.
- Several flat/clipping tests were down by roughly 10-15%.
- Checksums were identical, so this was a speed/codegen issue, not a rendering change.

Root cause:

- `rasterizeTriangle()` used to be forced inline through `TGX_INLINE`.
- After the inline-policy cleanup, it became controlled by `TGX_RASTERIZE_TRIANGLE_INLINE`.
- For classic ESP32, `TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE` was empty, so the hot rasterizer entry point stopped being forced inline.
- Codegen proof:
  - `main_core2_core_raster.nm.txt`: no standalone `rasterizeTriangle<...>` symbol in the hot path.
  - `dev_core2_core_raster.nm.txt`: large standalone `rasterizeTriangle<&shader_select<191...>>` symbol, about `0xE5A` bytes.
  - `fix5_core2_core_raster.nm.txt`: the standalone rasterizer symbol disappears again after restoring forced inline.

Fix retained:

- In the classic ESP32 config block, set:
  - `TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE __attribute__((always_inline))`
- Keep local clipping helpers under `TGX_RENDERER3D_CLIP_NOINLINE` on classic ESP32 only.
- Do not restore global `TGX_NOINLINE` for ESP32. A global `noinline,noclone` test was measured and rejected.

Final candidate:

- `big_tri.flat.z`: 101.18 -> 101.62 fps, +0.44% vs main.
- `far_clip.flat.z`: 918.69 -> 916.40 fps, -0.25% vs main.
- `screen_clip.multi_plane.flat.z`: 73.84 -> 73.88 fps, +0.05% vs main.
- Checksums unchanged.

Conclusion:

- The Core2 raster regression is explained by `rasterizeTriangle()` no longer being forced inline on ESP32.
- The retained fix is local, justified by symbol-level codegen, and brings the raster module back to main-level performance.

## 2. Pico2 / RP2350 `scream` and `skybox_noz`

Initial regression:

- Real example `scream`: average around 30.38 fps on dev vs 31.38 fps on main.
- `skybox_noz.nearest.wrap`: 177.88 -> 169.71 fps, -4.59%.
- `skybox_noz.partial_faces`: 1569.76 -> 1078.61 fps, -31.29%.
- Checksums were identical except for one no-z overlay case in the intermediate dev comparison.

Root cause:

- `_triangleClip1in()`, `_triangleClip2in()`, and `_triangleClip()` had become `inline`.
- On RP2350, `TGX_INLINE` maps to `always_inline` in several nearby helpers, and the compiler duplicated clipping code into `_drawTriangleClippedSub()` and skybox clipped paths.
- Codegen proof:
  - main `scream`: `_drawTriangleClippedSub` clones were small, about `0x194/0x198`, plus a generic clipped path around `0x0A58`.
  - dev `scream`: the same clones grew to about `0x980`, and the generic path grew to about `0x11A8`.
  - main had separate `_triangleClip1in` and `_triangleClip2in` functions; dev largely duplicated this logic inside callers.

Fix retained:

- Introduce `TGX_RENDERER3D_CLIP_NOINLINE`.
- Use it on `_triangleClip1in()`, `_triangleClip2in()`, and `_triangleClip()`.
- On non-ESP32 it currently expands to empty, but the functions are no longer declared as plain `inline`, so RP2350 stops duplicating them into every clipped caller.
- On classic ESP32 it expands to `__attribute__((noinline, noclone))`, because Core2 needs the stronger barrier for the raster/clipping codegen graph.

Final candidate:

- `scream` Pico2: measured fps lines average 31.50 fps, +0.40% vs main and +3.70% vs dev.
- `skybox_noz.nearest.wrap`: 177.88 -> 183.54 fps, +3.18% vs main.
- `skybox_noz.partial_faces`: 1569.76 -> 1977.85 fps, +26.00% vs main, +83.37% vs dev.
- `skybox_noz.overlay.noz`: 152.20 -> 154.87 fps, +1.75% vs main.
- Checksums match main in final candidate for the main skybox tests.

Conclusion:

- The Pico2 `scream` and `skybox_noz` regression is explained by accidental inlining/duplication of clipping helpers.
- The retained fix restores main-like clipped-call structure and recovers the measured performance.

## 3. Pico2 / RP2350 `wire_points`

Initial regression:

- `wire.lines.batch.fast`: 3827.24 -> 2474.44 fps, -35.35%.
- `wire.lines.batch.thick`: 39.17 -> 32.28 fps, -17.59%.
- `wire.quads.batch.thick`: 7.78 -> 6.25 fps, -19.67%.
- Checksums were identical.

Root cause of the large line regression:

- `BSeg::move()` changed from plain `inline` to `TGX_INLINE inline`.
- On RP2350, `TGX_INLINE` is `always_inline`, so the runtime branch:
  - `if (_x_major) move<true>(); else move<false>();`
  was duplicated into Bresenham users.
- Codegen proof:
  - main `_drawWireFrameLineFast`: about `0x2F0` bytes.
  - dev `_drawWireFrameLineFast`: about `0x510` bytes.
  - main `BSeg::move_inside_box`: about `0x4E2` bytes.
  - dev `BSeg::move_inside_box`: about `0x616` bytes.
  - After removing forced inline on RP2350, `_drawWireFrameLineFast` returns to the main-size `0x2F0` body.

Fix retained:

- Add `TGX_BSEG_RUNTIME_MOVE_INLINE`.
- Set it empty on RP2040/RP2350.
- Set it to `TGX_INLINE` elsewhere.
- Use it only on the non-template runtime `BSeg::move()` wrapper.
- Keep the template `move<true/false>()` variants as simple `inline`.

Why this is board-specific:

- On RP2350, forced inlining the runtime wrapper expands both x-major and y-major code into hot users and hurts line drawing.
- On ESP32/Core2, leaving it as plain `inline` caused regressions in some wire/thick/dot tests. Restoring `TGX_INLINE` there recovered the ESP32 profile.

Final candidate:

- Pico2 `wire.lines.batch.fast`: 3827.24 -> 3667.46 fps, still -4.18% vs main but +48.21% vs dev.
- Pico2 `wire.lines.batch.thick`: 39.17 -> 38.33 fps, -2.15% vs main and +18.74% vs dev.
- Pico2 `wire.quads.batch.thick`: 7.78 -> 7.93 fps, +1.93% vs main.
- Core2 `wire.lines.batch.fast`: 1120.08 -> 1152.46 fps, +2.89% vs main.
- Core2 `wire.quads.batch.thick`: 33.89 -> 33.97 fps, +0.24% vs main after the board-specific macro.

Remaining wire notes:

- Pico2 `wire.mesh.v2.fast` remains slower than main in the final candidate: 4736.46 -> 4287.58 fps, -9.48%.
- This is not caused by the original `_drawWireFrameLineFast` expansion: after the fix, that function's normalized body matches the main-size compact version again.
- The remaining mesh-wire delta appears in `_drawWireFrameMesh<...>` codegen and should be investigated separately if mesh wire speed on RP2350 is critical.
- The rejected alternative `TGX_INLINE BSeg::move()` plus `noinline move_inside_box()` did not fix batch lines and was slower overall on the target line tests.

Conclusion:

- The major Pico2 wire regression is explained by `TGX_INLINE` on the runtime Bresenham move wrapper.
- The retained fix is deliberately granular and board-specific.
- It fixes the large line/thick regressions without penalizing ESP32/Core2, but one Pico2 mesh-wire synthetic subtest remains a follow-up item.

## Final candidate source changes

- `src/tgx_config.h`
  - Classic ESP32: force `TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE`.
  - Add `TGX_RENDERER3D_CLIP_NOINLINE`.
  - Add `TGX_BSEG_RUNTIME_MOVE_INLINE`.
- `src/Renderer3D.h` / `src/Renderer3D.inl`
  - Apply `TGX_RENDERER3D_CLIP_NOINLINE` to `_triangleClip1in()`, `_triangleClip2in()`, `_triangleClip()`.
  - Move `_spotLights` near the runtime/color data tail. This did not materially change measured performance, but avoids shifting older legacy hot members early in the object layout.
- `src/bseg.h`
  - Apply `TGX_BSEG_RUNTIME_MOVE_INLINE` to the runtime `BSeg::move()` wrapper only.

## Validation performed

Final candidate targeted modules:

- Pico2/RP2350: `core_raster`, `wire_points`, `skybox_noz`.
- Core2/ESP32: `core_raster`, `wire_points`, `skybox_noz`.
- CoreS3/ESP32-S3: `core_raster`, `wire_points`, `skybox_noz`.
- Teensy 4.1: `core_raster`, `wire_points`, `skybox_noz`.
- Pico W/RP2040: `core_raster`, `wire_points`, `skybox_noz`.
- ESP32P4: `core_raster`, `wire_points`, `skybox_noz`.

All final candidate runs completed with valid benchmark telemetry.

## Follow-up

- Pico2 `wire.mesh.v2.fast/aa` remains worth a separate investigation if wireframe mesh performance is important.
- Core2 `points.dots.batch.fixed` remains slower than main in both dev and final candidate; this was present before the retained fixes and is not explained by the three regressions studied here.
