# Spotlight optimization analysis - 2026-06-26

This note analyses whether the current spotlight optimization is a real
algorithmic/codegen win or just a lucky code-layout effect.

## Method

- Built the "before" code from the detached worktree
  `D:\Programmation\tgx_tmp_spot_head`.
- Built the current code from `D:\Programmation\tgx`.
- Forced Arduino library resolution with `--library <path>`.
  This is important: the Arduino mirror in
  `D:\Programmation\arduino\libraries\tgx` already contains the current code,
  so comparisons without explicit `--library` can silently compare the wrong
  implementation.
- Main runtime scene: `spotlights_room` with `MAX_SPOT_LIGHTS=2`.
- Additional Pico2 probe modes were measured from a temporary sketch in this
  investigation folder:
  - mode 1: two point lights, no local specular
  - mode 2: one point light, local specular enabled
  - mode 3: two moving cone spotlights, local specular enabled

## Controlled spotlights_room results

| Board | Old avg fps | Current avg fps | Delta |
|---|---:|---:|---:|
| Pico2 / RP2350 | 13.55 | 16.86 | +24.4% |
| Core2 / ESP32 | 20.56 | 20.22 | -1.7% |
| CoreS3 / ESP32-S3 | 37.57 | 37.86 | +0.8% |
| Teensy 4.1 | 101.36 | 100.45 | -0.9% |

The optimization is therefore clearly useful on Pico2, but neutral/noisy on
the other tested boards for this example.

## Pico2 probe modes

| Probe mode | Old avg fps | Current avg fps | Delta |
|---|---:|---:|---:|
| 2 point lights, no specular | 15.56 | 19.78 | +27.1% |
| 1 point light, specular | 17.78 | 23.11 | +30.0% |
| 2 cone spotlights, specular | 14.44 | 25.83 | +78.9% |

The two point-light probe modes are the most relevant result here: the gain
survives both disabling local specular and reducing the active light count.
That makes it unlikely to be a one-scene FPS accident.

The cone result is suspicious and is excluded from the optimization conclusion.
Within that row, old and current are intended to render the same cone-light
scene, so range/diffuse/cone rejects should be the same. Cone culling can
explain why a cone-light scene may be faster than a point-light scene, but it
does not explain a large old-vs-current delta for the same cone scene. This
needs a dedicated instrumented check before being used.

## Code size and symbol size

The current build is larger, not smaller. On Pico2:

| Symbol | Old size | Current size |
|---|---:|---:|
| `_drawTriangle` | 14592 B | 20950 B |
| `_drawQuad` | 25312 B | 34636 B |

The same pattern appears on the other targets. So the Pico2 speedup is not
explained by smaller code. The current code contains both a point path and a
generic cone-capable path; the point-light scene wins because the executed path
is shorter, even though the total binary is larger.

## Pico2 block-level instruction count

Representative first-light block inside `_drawTriangle`:

| Block | Instructions | `vmul.f32` | `vfma.f32` | `vldr` | Branches |
|---|---:|---:|---:|---:|---:|
| Old generic path | 122 | 19 | 14 | 28 | 14 |
| Current point path | 97 | 12 | 11 | 24 | 11 |

The current point path removes:

- the per-light cone flag test and cone dot product when no active cone exists;
- loads of `directionView`, `cosOuter`, and `invCosWidth` in point-light scenes;
- the full half-vector construction for local specular.

For local specular, the point path uses the camera direction approximation
already implied by the previous code:

```cpp
H = normalize(L + V), with V = (0, 0, 1)
```

Since `L` is normalized:

```cpp
h2 = dot(L + V, L + V) = 2 + 2 * Lz
dot(N, L + V) = dot(N, L) + Nz
```

This reuses `ndotl` and avoids several multiplies/adds and temporary values per
active light. This is an algorithmic simplification, not just a layout change.

## Why Pico2 benefits more

Pico2/RP2350 uses the C/FPU fast inverse-square-root trick selected by
`TGX_USE_FAST_INV_SQRT_TRICK=1`. The local-light vertex path is therefore a
meaningful part of the frame cost, especially with many Gouraud vertices.

ESP32/Core2 and ESP32-S3 use the Xtensa inline assembly path in
`fast_invsqrt()` (`rsqrt0.s` plus a Newton step). The saved scalar work around
the normalization is a smaller fraction of the total frame. The measured result
is correspondingly in the noise.

Teensy 4.1 is much faster overall and the measured scene is less dominated by
local-light vertex math. The larger generated function may slightly offset the
shorter point path, but the observed delta is below normal run variation.

## Rejected variants from the previous pass

The earlier variants are also useful evidence:

- applying the specular algebra to the generic cone path was slower on Pico2;
- a diffuse-only special path improved spec-off cases but badly hurt spec-on;
- the final retained version only specializes the no-cone point-light path.

This argues against accepting arbitrary code-layout changes just because one
FPS number improved.

## Current conclusion

Keep the point-light specialization for Pico2/RP2350 style targets: it has a
real per-light/per-vertex reduction and reproduced across multiple point-light
probe modes.

Do not claim a guaranteed improvement on Core2/CoreS3/Teensy for this scene.
The retained optimization is acceptable there only if the increased code size
for `MAX_SPOT_LIGHTS>0` is considered acceptable, because runtime performance is
essentially neutral.

Full assembly dumps were pruned after the symbol-size and instruction-count CSV
summaries were written.

The legacy `MAX_SPOT_LIGHTS=0` path remains the separate regression guard and
must continue to be checked against the baseline before accepting further
changes.
