# Accepted Spotlight First Version

Date: 2026-06-25

Branch: `feature/point-spot-lights`

Purpose: compact validation record for the first accepted local point/spot light implementation and the `spotlights_room` examples.

## Baseline Reference

The reference baseline taken before adding private spot-light data/runtime code is preserved in:

```text
validation/performance/baselines/spotlights_prechange_2026-06-25/
```

Use that baseline as the non-regression reference for historical rendering paths while continuing the spotlight work. It contains:

- `benchmark_global_scores.csv`
- `benchmark_subtests.csv`
- `example_telemetry.csv`
- `example_telemetry_summary.csv`
- `run_manifest.csv`
- `README.md`

## Example FPS

The accepted demo examples were tested on real hardware:

| Board | Sketch | Config | FPS |
| --- | --- | --- | ---: |
| Core2 | `examples/M5Stack/spotlights_room` | full geometry, local specular | 20.2 |
| CoreS3 | `examples/M5Stack/spotlights_room` | full geometry, local specular | 37.5 |
| Pico2 | `examples/Pico_RP2040_RP2350/spotlights_room` | reduced floor/spheres, local specular | 13.4 |

Pico2 was first tested with the full geometry and measured 9.4 FPS average. The accepted Pico2 example reduces only the floor and sphere resolution, while keeping the same animation, two local lights, camera orbit, material setup and local specular.

Detailed rows are in `spotlights_room_fps.csv`.

## Legacy Path Checks

Historical examples were measured with `MAX_SPOT_LIGHTS=0` and `MAX_SPOT_LIGHTS=2` to keep an eye on legacy path regressions while the spotlight work continues.

Compact results are kept in:

- `legacy_example_summary_deltas.csv`
- `legacy_example_scene_deltas.csv`
- `legacy_example_size_deltas.csv`

The notable expected cost in this first implementation is when compiling with `MAX_SPOT_LIGHTS=2`, even without active lights, because the renderer carries the local-light-ready path. The `MAX_SPOT_LIGHTS=0` path stayed within measurement noise against the pre-change baseline.

## Pico2 Specular Cost

On Pico2, with the accepted reduced geometry:

| Local specular | Avg FPS | Min | Max |
| --- | ---: | ---: | ---: |
| on | 13.17 | 12 | 14 |
| off | 15.42 | 15 | 17 |

Disabling local specular gives about `+17.1%` FPS on this scene. The source was restored to the specular-enabled version after the measurement.

Detailed rows are in `pico2_specular_impact.csv`.

## Build Commands Used

```powershell
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o3std --libraries D:\Programmation\arduino\libraries examples\Teensy4\3D\spotlights_room
arduino-cli compile --fqbn esp32:esp32:m5stack_core2 --libraries D:\Programmation\arduino\libraries --port COM5 --upload examples\M5Stack\spotlights_room
arduino-cli compile --fqbn esp32:esp32:m5stack_cores3 --libraries D:\Programmation\arduino\libraries --port COM10 --upload examples\M5Stack\spotlights_room
arduino-cli compile --fqbn rp2040:rp2040:rpipico2:opt=Fast --libraries D:\Programmation\arduino\libraries --port COM21 --upload examples\Pico_RP2040_RP2350\spotlights_room
```

## Build Size Notes

Last observed builds:

| Board | Flash / code | RAM/global |
| --- | ---: | ---: |
| Teensy 4.1 | code `187780`, data `42612` | RAM1 free `331904`, RAM2 free `34848` |
| Core2 | sketch `466423` bytes | globals `38576` bytes |
| CoreS3 | sketch `453175` bytes | globals `38172` bytes |
| Pico2 | sketch `221156` bytes | globals `442876` bytes, 84% |

Pico2 emits the usual `TFT_eSPI` touch warning and low dynamic memory warning. Upload and runtime telemetry were successful.
