# Spotlight runtime optimization pass

Date: 2026-06-25

Branch: `feature/point-spot-lights`

Goal: optimize the local point-light path without regressing the historical
`MAX_SPOT_LIGHTS == 0` renderer path.

## Retained change

- Add a compact runtime flag telling whether any active local light uses a cone.
- Use a dedicated point-light loop when no active light has a cone.
- In the point-light specular path, reuse the fact that `L` is normalized:
  - `h2 = dot(L + V, L + V)` becomes `2 + 2 * Lz`
  - `dot(N, L + V)` becomes `dot(N, L) + Nz`
- Keep the generic cone/spotlight path on the original specular formula; the
  simplified formula was slower there on RP2350, likely due to register pressure.

## Spotlight room FPS

| Variant | Teensy 4.1 | Core2 | CoreS3 | Pico2 |
| --- | ---: | ---: | ---: | ---: |
| Baseline | 100.53 | 20.50 | 37.76 | 13.50 |
| Final | 99.25 | 20.17 | 38.00 | 17.07 |

The main useful gain is on Pico2/RP2350: about +26% on the specular point-light
scene. Teensy/Core2/CoreS3 remain within the observed capture noise for this
example.

## Rejected trials

| Trial | Result |
| --- | --- |
| Factor only `icu * _r_inorm` | Small/noisy gain on Pico2, neutral on Core2. Kept because it is harmless and used by the final point path. |
| Separate point-light loop without specular algebra | Pico2 improved to 15.07 FPS, Core2 stayed roughly flat. Kept as the base for the final path. |
| `allRuntimeSpecular` fast flag | Pico2 dropped to 10.86 FPS. Rejected. |
| Apply specular algebra in the generic cone-capable loop | Pico2 dropped to 11.46 FPS. Rejected; generic path keeps the original formula. |
| Diffuse-only point loop avoiding `L *= invD` | Specular-off Pico2 improved to 21.21 FPS, but specular-on dropped to 12.77 FPS. Rejected. |

Compact intermediate results are in `results/fps_summary.csv`. The raw serial
captures were pruned after the summary was written.

## Legacy MAX_SPOT_LIGHTS=0 check

One historical example per board was re-run. All use the default
`Renderer3D<...>` instantiation, therefore `MAX_SPOT_LIGHTS == 0`.

| Board | Example | Current | Pre-spot baseline |
| --- | --- | ---: | ---: |
| Pico2 | `scream` | 31.57 | 31.39 |
| Core2 | `borg_cube` | 64.75 | 66.05 |
| CoreS3 | `borg_cube` | 187.82 | 186.98 |
| Teensy 4.1 | `test-shading` selected scenes | matches baseline scene by scene | baseline |

The compact legacy summary is in `results/legacy0_summary.csv`.
