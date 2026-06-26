# Spotlight invD-dot optimization

Date: 2026-06-26

Goal: avoid normalizing the local-light vector with `L *= invD` in the
spotlight runtime. The renderer now keeps `L` unnormalized, computes raw dot
products first, and applies `invD` only to scalar quantities that need a unit
light direction.

Current code shape:

- diffuse: `diffRaw = normalScale * dot(N, L)`, then `diff = diffRaw * invD`;
- cone: `cone = -dot(spotDir, L) * invD`;
- specular half-vector length: `h2 = 2 + 2 * L.z * invD`;
- specular dot: reuse `diff` as the normalized `N.L` term instead of recomputing
  another normalized dot product.

The first `invdot` attempt removed `L *= invD` but still recomputed an extra
normalized `N.L` value for specular. That version regressed Pico2 and is kept
only as a rejected intermediate measurement.

## Main result

Reference is the previous accepted `fVec3` version of the same `spotlights_room`
example.

| Board | fVec3 avg fps | invdot2 avg fps | Delta | fVec3 flash | invdot2 flash | Flash delta |
|---|---:|---:|---:|---:|---:|---:|
| Pico2 | 16.70 | 23.22 | +39.04% | 242908 | 240940 | -1968 |
| Core2 | 20.42 | 20.67 | +1.22% | 475627 | 475323 | -304 |
| CoreS3 | 38.12 | 38.38 | +0.68% | 462223 | 461887 | -336 |
| Teensy 4.1 | 101.77 | 102.23 | +0.45% | 247804 | 247804 | 0 |

Pico2 was rerun for confirmation with the same current code:

- first invdot2 run: 23.21 fps average;
- confirmation run used in the table: 23.22 fps average.

## Rejected intermediate result

| Board | fVec3 avg fps | first invdot avg fps | Delta |
|---|---:|---:|---:|
| Pico2 | 16.70 | 16.26 | -2.63% |
| Core2 | 20.42 | 20.42 | 0.00% |
| CoreS3 | 38.12 | 38.31 | +0.50% |
| Teensy 4.1 | 101.77 | 101.69 | -0.08% |

This supports keeping only the `invdot2` form, where the specular computation
reuses `diff`.

## Validation

Hardware uploads and captures succeeded for:

- Pico2 + ILI9341, `examples/Pico_RP2040_RP2350/spotlights_room`, 23 FPS lines;
- Core2, `examples/M5Stack/spotlights_room`, 24 FPS lines;
- CoreS3, `examples/M5Stack/spotlights_room`, 26 FPS lines;
- Teensy 4.1 + ILI9341, `examples/Teensy4/3D/spotlights_room`, 26 FPS lines.

Legacy compile checks with `MAX_SPOT_LIGHTS == 0` also succeeded:

- Pico2: `examples/Pico_RP2040_RP2350/scream`;
- Teensy 4.1: `examples/Teensy4/3D/test-shading`.

Compact CSV summaries:

- `summary_invdot2.csv`
- `comparison_invdot2_vs_fvec3.csv`
- `comparison_invdot_vs_fvec3.csv`

Raw logs, parsed files, and serial telemetry were pruned after the compact CSV
summaries were written.
