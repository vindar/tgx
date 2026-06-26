# Spotlight cone loop unification

Date: 2026-06-26

Goal: remove the duplicated spotlight loop that separated the global
no-cone path from the cone-capable path, without hurting the point-light path.

## Code variants

`unified_v1` used one loop and tested:

```cpp
if (localCone && (flags & SPOT_LIGHT_CONE_ENABLED))
```

This looked simple but was very bad on Pico2: it loaded flags early, kept more
state live in the hot loop, and dropped the point-light scene to 11.43 fps.

`unified_v2` keeps one loop but makes the cone work genuinely lazy:

- `localCone` and `localSpecular` are computed once before the loop;
- attenuation starts directly as `atten * atten`;
- per-light flags are loaded in the cone block only when `localCone` is true;
- for point lights, flags are loaded later only if local specular is enabled.

## Main point-light result

Reference is the previous accepted `invdot2` split-loop version on
`spotlights_room`.

| Board | invdot2 split-loop | unified_v2 | Delta | Flash delta |
|---|---:|---:|---:|---:|
| Pico2 | 23.22 fps | 28.88 fps | +24.38% | -33232 bytes |
| Core2 | 20.67 fps | 20.96 fps | +1.40% | -9288 bytes |
| CoreS3 | 38.38 fps | 38.15 fps | -0.60% | -8908 bytes |
| Teensy 4.1 | 102.23 fps | 102.58 fps | +0.34% | -21504 bytes |

CoreS3 was rerun for confirmation and remained essentially the same:
38.12 fps. The small negative delta is within the usual noise band for this
example, while the code-size reduction is consistent across boards.

## Cone probe

Pico2 `spotlights_room_probe` with `TGX_LIGHT_PROBE_MODE=3` and two cone
spotlights:

| Run | Avg fps |
|---|---:|
| Previous comparable probe result | 26.17 |
| unified_v2 cone probe | 29.38 |

So the unified lazy path does not appear to penalize cone spotlights either.

## Validation

Hardware upload and serial capture succeeded for:

- Pico2 `examples/Pico_RP2040_RP2350/spotlights_room`, v1 and v2;
- Core2 `examples/M5Stack/spotlights_room`, v2;
- CoreS3 `examples/M5Stack/spotlights_room`, v2 plus confirmation rerun;
- Teensy 4.1 `examples/Teensy4/3D/spotlights_room`, v2;
- Pico2 probe cone mode 3, using `-DTGX_LIGHT_PROBE_MODE=3`.

Legacy compile checks with `MAX_SPOT_LIGHTS == 0` also succeeded:

- Pico2 `examples/Pico_RP2040_RP2350/scream`;
- Teensy 4.1 `examples/Teensy4/3D/test-shading`.

Compact CSV summaries:

- `summary_cone_unification.csv`
- `comparison_unified_v2_vs_invdot2.csv`

Raw logs, parsed CSV/JSON files, and serial telemetry are under `raw/`.
