# TGX skybox specialized renderer investigation

Date: 2026-06-07

Commit at session start: `c0f935dfcf02c8f6096af916475c156122dec450`

## A. Executive summary

This investigation implemented and tested a first production prototype of a dedicated `Renderer3D::drawSkyBox()` path.

Final recommendation for this v1: keep for review, with the explicit contract that the skybox must be rendered before the z-buffered scene.

Source files changed:

- `src/Renderer3D.h`
- `src/Renderer3D.inl`
- `examples/Teensy4/3D/mars/mars.ino`

The new path keeps the same cube face order and texture-coordinate convention as `drawCube()`:

- front
- back
- top
- bottom
- left
- right

The cube is still transformed by the current model matrix, so position, rotation and scale are set with the normal `setModelPosScaleRot()` workflow. This keeps the skybox in world coordinates and lets the caller scale it explicitly.

The prototype avoids the general `shader_select()` route for the skybox. It calls `rasterizeTriangle()` directly with the minimal shader configuration needed for an unlit textured perspective triangle:

- z-buffer disabled
- Gouraud disabled
- texture enabled
- perspective enabled
- unlit enabled
- nearest or bilinear texture quality selected from current renderer state
- texture wrap/clamp selected from current renderer state, but only variants enabled in the renderer template are instantiated

Measured result:

- CPU synthetic check: exact match with the old unlit cube reference, +24.86%.
- Teensy 4.1 synthetic check: exact framebuffer match, +27.61%.
- Teensy 4.1 `mars` demo: weighted real telemetry improved from 59.99 FPS to 62.17 FPS, about +3.64%.
- Best real scene gain: `movie`, +6.91%.
- Code cost in the `mars` build: +4,856 bytes flash code and +5,248 bytes RAM1/ITCM code. RAM variables are unchanged.

## B. Motivation

The old `mars` skybox used `drawCube()` with six textures. That was convenient but not specialized:

- it used the normal renderer shader selection path;
- it used the active z-buffer path;
- it used the normal lit/flat material setup unless carefully overridden;
- it participated in the normal clipping path, including far-plane clipping;
- disabling the z-buffer globally would have required adding or using a `NO_ZBUFFER` shader flag in the general renderer path.

The important design concern was avoiding a general `SHADER_NOZBUFFER` template expansion inside `shader_select()`. A no-z shader variant would mostly exist for the skybox, but it would add another branch/variant to the central triangle rasterization path used by the rest of the renderer.

The chosen prototype duplicates only the skybox-specific draw path instead of extending the generic shader matrix.

## C. API and implementation

Two public overloads were added:

```cpp
void drawSkyBox(
    const fVec2 v_front_ABCD[4] , const Image<color_t>* texture_front,
    const fVec2 v_back_EFGH[4]  , const Image<color_t>* texture_back,
    const fVec2 v_top_HADE[4]   , const Image<color_t>* texture_top,
    const fVec2 v_bottom_BGFC[4], const Image<color_t>* texture_bottom,
    const fVec2 v_left_HGBA[4]  , const Image<color_t>* texture_left,
    const fVec2 v_right_DCFE[4] , const Image<color_t>* texture_right);

void drawSkyBox(
    const Image<color_t>* texture_front,
    const Image<color_t>* texture_back,
    const Image<color_t>* texture_top,
    const Image<color_t>* texture_bottom,
    const Image<color_t>* texture_left,
    const Image<color_t>* texture_right);
```

The whole-image overload generates the same epsilon-adjusted texture coordinates used by `drawCube()`, so the face orientation and edge sampling convention are preserved.

Private helpers added in `Renderer3D.inl`:

- `_rasterizeSkyBoxTriangle<TEXTURE_BILINEAR, TEXTURE_WRAP>()`
- `_rasterizeSkyBoxTriangle()`
- `_drawSkyBoxTriangleClippedSub()`
- `_drawSkyBoxTriangleClipped()`
- `_drawSkyBoxQuad()`
- `_drawSkyBoxFace()`

The direct shader call is:

```cpp
uber_shader<color_t, ZBUFFER_t,
    false,              // USE_ZBUFFER
    false,              // USE_GOURAUD
    true,               // USE_TEXTURE
    false,              // USE_ORTHO
    TEXTURE_BILINEAR,
    TEXTURE_WRAP,
    true>               // USE_UNLIT
```

Important behavior:

- Requires textured perspective rendering to be enabled in the renderer template.
- Returns early in orthographic mode.
- Skips `nullptr` faces.
- Performs near-plane and screen clipping.
- Does not perform far-plane clipping.
- Does not test or write the z-buffer.

Because the new path does not use z-buffering, the skybox must be drawn before normal scene objects. Drawing it after the scene would overpaint the scene.

## D. Mars demo integration

The local `drawSkyBox()` helper in `examples/Teensy4/3D/mars/mars.ino` now does only the skybox setup:

```cpp
renderer.setModelPosScaleRot({ 0,skybox_ref_height,0 },
    { skybox_size,skybox_size,skybox_size }, 180, { 0,1,0 });
renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
renderer.drawSkyBox(&mars_front, &mars_back, &mars_top_neb,
    &mars_bottom, &mars_left, &mars_right);
```

The previous material and shader setup for the skybox was removed because the specialized path is already unlit and textured.

All `mars` scene call sites were reordered so the skybox is drawn after `setLookAt()` and before the normal scene geometry.

## E. CPU synthetic validation

Tool:

- `validation/performance/investigations/2026-06-skybox-specialized/tools/skybox_cpu_check.cpp`

Build/run commands:

```text
cmake -S validation\performance\investigations\2026-06-skybox-specialized\tools -B tmp\build_skybox_cpu_check -G "Visual Studio 17 2022" -A x64
cmake --build tmp\build_skybox_cpu_check --config Release
tmp\build_skybox_cpu_check\Release\tgx_skybox_cpu_check.exe
```

Result:

```text
WIDTH=320 HEIGHT=240 ITER=240
HASH_OLD=10147031527427794644 HASH_NEW=10147031527427794644
MISMATCHES=0
TIME_OLD_US=154.907 TIME_NEW_US=124.065
SPEEDUP=24.8593
```

The old reference was `drawCube()` configured as an unlit nearest-textured cube, so this validates face order, texture coordinates, projection and culling behavior without the unrelated lighting/z-buffer differences.

## F. Teensy synthetic validation

Sketch:

- `validation/performance/investigations/2026-06-skybox-specialized/sketches/SkyboxValidation/`

Upload/capture command:

```text
powershell -ExecutionPolicy Bypass -File validation\performance\tools\upload_and_capture.ps1 -Board teensy41 -Port COM3 -UploadPort usb:80000/3/0/6/4 -Sketch validation\performance\investigations\2026-06-skybox-specialized\sketches\SkyboxValidation -Label skybox_validation_teensy41_portfix -ParseMode probe -StartRegex TGX_SKYBOX_VALIDATION_BEGIN -EndRegex TGX_SKYBOX_VALIDATION_END -MinTelemetryLines 8 -TimeoutSeconds 80 -RetryCount 0
```

Result:

```text
BOARD=Teensy41
HASH_OLD=970234301
HASH_NEW=970234301
MISMATCHES=0
TIME_OLD_US=6152
TIME_NEW_US=4821
SPEEDUP_PCT=27.61
```

This confirms that the hardware-rendered framebuffer matches the old unlit cube reference exactly for the synthetic case.

## G. Real `mars` telemetry

Old baseline command:

```text
powershell -ExecutionPolicy Bypass -File validation\performance\tools\upload_and_capture.ps1 -Board teensy41 -Port COM3 -UploadPort usb:80000/3/0/6/4 -Sketch examples\Teensy4\3D\mars -Label mars_old_skybox_teensy41 -ParseMode example -MinTelemetryLines 8 -TimeoutSeconds 260 -RetryCount 0
```

Final candidate command:

```text
powershell -ExecutionPolicy Bypass -File validation\performance\tools\upload_and_capture.ps1 -Board teensy41 -Port COM3 -UploadPort usb:80000/3/0/6/4 -Sketch examples\Teensy4\3D\mars -Label mars_new_skybox_final_teensy41 -ParseMode example -MinTelemetryLines 8 -TimeoutSeconds 260 -RetryCount 0
```

Overall result:

| Metric | Old | New | Delta |
| --- | ---: | ---: | ---: |
| Frames | 14959 | 15494 | +535 |
| Avg frame time | 16669.53 us | 16084.11 us | -585.42 us |
| FPS | 59.99 | 62.17 | +3.64% |

Per-scene result:

| Scene | Old FPS | New FPS | Speedup |
| --- | ---: | ---: | ---: |
| beginning | 188.33 | 188.56 | +0.12% |
| ending | 131.38 | 132.52 | +0.87% |
| falcon | 38.66 | 39.73 | +2.78% |
| intro | 469.71 | 466.43 | -0.70% |
| movie | 43.84 | 46.87 | +6.91% |

The gain is concentrated in the scenes where the skybox is a meaningful part of the frame cost. The small `intro` regression is treated as noise/layout because this scene is very fast and not dominated by the skybox.

## H. Code size and memory cost

From the old and final Teensy 4.1 `mars` size reports:

| Metric | Old | New | Delta |
| --- | ---: | ---: | ---: |
| Flash code | 219632 | 224488 | +4856 |
| Flash data | 1645704 | 1645704 | 0 |
| Flash headers | 8580 | 8844 | +264 |
| RAM1 variables | 329824 | 329824 | 0 |
| RAM1 code | 144616 | 149864 | +5248 |
| RAM2 variables | 452480 | 452480 | 0 |

The main cost is extra code. Data/buffer memory is unchanged.

Relevant final symbols from the Teensy `mars` build:

| Symbol | Size |
| --- | ---: |
| example `drawSkyBox()` wrapper | 132 bytes |
| `Renderer3D::drawSkyBox()` whole-image overload | 1472 bytes |
| `_drawSkyBoxQuad()` | 1624 bytes |
| `_drawSkyBoxTriangleClipped()` | 428 bytes |
| `_drawSkyBoxTriangleClippedSub()` | 948 bytes |
| no-z unlit nearest wrap `uber_shader` | 776 bytes |
| no-z unlit bilinear wrap `uber_shader` | 1094 bytes |

The selector was tightened so the `mars` instantiation does not emit unused clamp variants when only wrap is available. Because the `mars` renderer template still includes bilinear texture support, the skybox path emits both nearest and bilinear direct variants even though the example currently selects nearest for the skybox.

## I. Correctness validation

CPU 3D strict validation:

```text
powershell -ExecutionPolicy Bypass -File validation\3d\run_cpu.ps1 -Compare strict
```

Result:

```text
82 PASS, 0 FAIL
```

Notes:

- The existing strict CPU scenes do not directly exercise the new skybox API, so the separate CPU skybox synthetic check is the relevant direct validation.
- The skybox synthetic CPU and Teensy checks both produced exact framebuffer matches against the old unlit cube reference.
- The real `mars` visual result cannot be inspected by the script. The final candidate was uploaded and left running on the Teensy 4.1 for manual inspection.

## J. Risks and limitations

- The method is perspective-only in this v1. It returns early in orthographic mode.
- It skips far-plane clipping by design. The caller is expected to scale and place the skybox so near/screen clipping is enough.
- It performs no z-buffer test/write, so draw order matters. The skybox must be rendered before the rest of the scene.
- The API adds code size when instantiated. In `mars`, the measured cost is about +4.9 KB flash code and +5.2 KB RAM1/ITCM code.
- If a renderer template loads bilinear texture support, the skybox selector can instantiate the bilinear no-z unlit shader even when an example normally uses nearest. A future nearest-only overload or compile-time policy could reduce this if code size becomes a problem.
- Only Teensy 4.1 hardware runtime was tested in this pass. CPU validation also passed. Other boards were not benchmarked.

## K. Accepted and rejected design choices

Accepted:

- Add a dedicated `drawSkyBox()` path instead of adding a general no-zbuffer shader variant.
- Keep the `drawCube()` face and texture-coordinate conventions.
- Use the current model transform for world-space scaling/positioning.
- Skip far-plane clipping.
- Keep near/screen clipping.
- Draw skybox before normal z-buffered scene geometry.

Rejected or deferred:

- Adding `SHADER_NOZBUFFER` to the generic `Renderer3D` shader matrix. This would add a central shader variant for a rare case.
- A full-screen procedural/ray skybox. That may be faster later, but it is a different API and does not reuse the existing cube texture assets.
- A nearest-only compile-time skybox path. Useful if code size matters, but deferred to keep v1 general enough to match texture quality selection.
- Cross-board benchmark matrix. Deferred until the API and Mars integration are reviewed.

## L. Artifacts

Important files:

- `validation/performance/investigations/2026-06-skybox-specialized/SESSION_START.md`
- `validation/performance/investigations/2026-06-skybox-specialized/results/cpu_synthetic_check.txt`
- `validation/performance/investigations/2026-06-skybox-specialized/results/teensy_synthetic_validation_telemetry.txt`
- `validation/performance/investigations/2026-06-skybox-specialized/results/mars_scene_delta.csv`
- `validation/performance/investigations/2026-06-skybox-specialized/results/mars_overall_delta.csv`
- `validation/performance/investigations/2026-06-skybox-specialized/results/size_delta.csv`
- `validation/performance/investigations/2026-06-skybox-specialized/codegen/teensy41_mars_final_skybox_symbols.txt`
- `validation/performance/investigations/2026-06-skybox-specialized/tools/skybox_cpu_check.cpp`
- `validation/performance/investigations/2026-06-skybox-specialized/sketches/SkyboxValidation/SkyboxValidation.ino`

## M. Next steps

Recommended next work:

1. Manual visual review of `mars` on the Teensy display, especially culling/orientation of front/top/bottom/left/right faces.
2. Add a small CPU validation scene that directly exercises `drawSkyBox()`.
3. If code size is considered too high, test a nearest-only skybox overload or compile-time policy to avoid emitting the bilinear no-z shader in examples that never need it.
4. Benchmark the API on another board only after the v1 design is accepted.
5. Consider a separate future full-screen skybox renderer only if cube-face rasterization remains a measurable bottleneck.

## N. 2026-06-08 API refinement

After the v1 source was committed, the API was refined so `drawSkyBox()` no longer depends on the current model transform. The skybox now accepts:

```cpp
rot_angle_y = 0.0f
reference_height = 0.0f
skybox_radius = 32768.0f
texture_quality = SHADER_TEXTURE_NEAREST
texture_mode = SHADER_TEXTURE_CLAMP
```

The renderer ignores camera X/Z translation, recovers camera Y from the view matrix, and places the skybox vertical center at `reference_height`. This keeps the skybox effectively infinite horizontally while preserving a world-space floor/sky reference when desired.

`mars` now calls:

```cpp
renderer.drawSkyBox(&mars_front, &mars_back, &mars_top_neb, &mars_bottom, &mars_left, &mars_right,
                    180.0f, 0.0f, 4000.0f, SHADER_TEXTURE_NEAREST, SHADER_TEXTURE_WRAP_POW2);
```

Validation for the refined API:

- CPU synthetic skybox check: exact framebuffer match, +23.97%.
- CPU 3D strict validation: 82 PASS, 0 FAIL.
- Teensy 4.1 `mars` upload/capture: SUCCESS, 286 parsed telemetry rows.

The final `mars` capture is longer than the earlier v1/old captures, so the raw overall weighted average is not directly comparable. Per-scene deltas vs the old `drawCube()` skybox are:

| Scene | Delta |
| --- | ---: |
| beginning | -0.42% |
| ending | +2.82% |
| falcon | +3.29% |
| intro | -2.11% |
| movie | +5.69% |

Final API note and artifacts:

- `validation/performance/investigations/2026-06-skybox-specialized/SKYBOX_RADIUS_API.md`
- `validation/performance/investigations/2026-06-skybox-specialized/results/cpu_synthetic_check_radius_api.txt`
- `validation/performance/investigations/2026-06-skybox-specialized/results/mars_skybox_radius_api_capture.json`
- `validation/performance/investigations/2026-06-skybox-specialized/results/mars_skybox_radius_api_scene_compare.csv`

Raw per-frame Mars telemetry CSVs and upload logs were pruned after the compact comparison CSVs and capture metadata
were recorded.
