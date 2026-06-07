# Skybox radius/reference-height API

Date: 2026-06-08

This note documents the skybox iteration after the initial committed v1.

## Goal

The first dedicated `drawSkyBox()` path avoided the normal `shader_select()` route, z-buffer test/write, lighting, culling state, and far clipping. It still used the current model transform, so `mars` had to do:

```cpp
renderer.setModelPosScaleRot(...);
renderer.drawSkyBox(...);
```

That made the skybox feel like a model. The new API makes the skybox independent from model state while keeping it in a world-scale coordinate convention.

## Public API

The overloads now accept optional skybox placement and texture parameters:

```cpp
renderer.drawSkyBox(front, back, top, bottom, left, right,
                    rot_angle_y,
                    reference_height,
                    skybox_radius,
                    texture_quality,
                    texture_mode);
```

Defaults:

```cpp
rot_angle_y = 0.0f
reference_height = 0.0f
skybox_radius = 32768.0f
texture_quality = SHADER_TEXTURE_NEAREST
texture_mode = SHADER_TEXTURE_CLAMP
```

Semantics:

- `rot_angle_y` rotates the skybox around world Y.
- `reference_height` is the world-space height of the skybox vertical center.
- `skybox_radius` is the cube half-size in world units.
- The current model transform, material, shader state and culling state are ignored.
- Camera X/Z translation is ignored, so the skybox remains effectively at infinity horizontally.
- Camera Y is recovered from the view matrix so floor/ceiling placement can remain tied to `reference_height`.
- Large radii reduce vertical parallax and make the skybox behave more like an infinitely distant background.

This keeps the API simple for the common case, while still allowing `mars` to preserve its previous ground/sky placement:

```cpp
renderer.drawSkyBox(&mars_front, &mars_back, &mars_top_neb, &mars_bottom, &mars_left, &mars_right,
                    180.0f, 0.0f, 4000.0f, SHADER_TEXTURE_NEAREST, SHADER_TEXTURE_WRAP_POW2);
```

## Implementation

`drawSkyBox()` builds the eight cube vertices in view space:

1. rotate the unit cube around world Y;
2. scale by `skybox_radius`;
3. offset vertical position by `reference_height - camera_y`;
4. transform by the view rotation only (`mult0`);
5. force `w = 1.0f`;
6. draw the six faces through the dedicated no-z/unlit textured path.

The main renderer triangle/rasterizer path remains unchanged.

Near and screen clipping are still used. Far clipping is intentionally skipped.

## Validation

CPU synthetic check:

```text
MISMATCHES=0
TIME_OLD_US=153.17
TIME_NEW_US=123.554
SPEEDUP=23.9707
```

CPU 3D strict validation:

```text
82 PASS, 0 FAIL
```

Teensy 4.1 `mars` upload/capture:

```text
FINAL_STATUS=SUCCESS
CAPTURE_STATUS=SUCCESS
parsed_telemetry_rows=286
```

The final `mars` capture is longer than the older v1/old captures, so the raw overall weighted average is not directly comparable. Per-scene deltas against the old `drawCube()` skybox are more useful:

| Scene | Radius API delta vs old |
| --- | ---: |
| beginning | -0.42% |
| ending | +2.82% |
| falcon | +3.29% |
| intro | -2.11% |
| movie | +5.69% |

The important heavy scenes still improve, while small intro/beginning differences are within the kind of variation seen in previous captures.

## Code size

The final Teensy `mars` upload reported:

```text
FLASH: code:226584, data:1645704, headers:8796
RAM1: variables:329824, code:151976, padding:11864, free:30624
RAM2: variables:452480, free:71808
```

The radius/reference-height API keeps the same code-size profile as the previous camera-centered prototype: runtime texture quality/mode parameters can instantiate several skybox texture variants.

## Artifacts

Key files:

- `results/cpu_synthetic_check_radius_api.txt`
- `results/mars_skybox_radius_api_capture.json`
- `results/mars_skybox_radius_api_scene_compare.csv`
- `results/mars_skybox_radius_api_overall_compare.csv`

The raw Mars telemetry CSV and upload log were pruned after the compact scene/overall comparison CSVs and capture
metadata were saved.

## Follow-up

If code size becomes too high, the next focused refinement should test a nearest/wrap compile-time or overload-specific skybox path for examples like `mars` that do not need bilinear/clamp skybox variants.
