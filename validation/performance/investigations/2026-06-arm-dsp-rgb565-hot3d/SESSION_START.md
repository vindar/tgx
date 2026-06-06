# ARM DSP RGB565 Hot 3D Session Start

## Repository State

- Branch: `feature/multi-directional-lights`
- Commit: `727956b2ad9bc4852f45ece75636cde830b4bd61`
- Starting tracked source diff: clean.
- Investigation artifacts live under `validation/performance/investigations/2026-06-arm-dsp-rgb565-hot3d/`.

## Carry-over Baseline

The previous ARM DSP RGB565 session accepted and committed one source optimization:

- `src/Color.h`
- `meanColor(RGB565, RGB565)` now uses the exact packed average:

```cpp
(a & b) + (((a ^ b) & 0xF7DEu) >> 1)
```

That work is treated as baseline and will not be repeated.

Previous rejected approaches not to repeat blindly:

- Naive `UHADD16` RGB565 averages, because halfword lanes do not match RGB565 channel boundaries.
- Coarse renderer/layout perturbations.
- Broad inline macro category experiments.
- Coarse `TGX_INLINE_IMAGE=` and Image-helper split attempts.
- The previous `blend256` scalar alternatives, which were exact but slower than the current packed implementation.

## Boards

| Board | Port | Role |
| --- | --- | --- |
| Teensy 4.1 / Cortex-M7 | `COM3` | Primary ARM DSP runtime target |
| Pico2 / RP2350 / Cortex-M33 | `COM21` | Primary ARM DSP runtime target |

Pico W / RP2040 is intentionally ignored. Core2/CoreS3 are compile-sanity targets only if a generic final source patch survives.

## Upload / Capture Tools

Use reusable tools from `validation/performance/tools/`, especially:

- `upload_and_capture.ps1`
- benchmark/example parsers and aggregation scripts as references

No ad-hoc serial capture should be used.

## Target Functions For This Session

The session focuses on exact RGB565 color kernels actually used by TGX 3D shader/rendering paths.

High-priority candidates to audit and microbenchmark:

- `interpolateColorsBilinear(RGB565, RGB565, RGB565, RGB565, float, float)`
- `interpolateColorsTriangle(RGB565, int32_t, RGB565, int32_t, RGB565, int32_t)`
- `RGB565::mult256(int, int, int)` and `RGB565::mult256(int, int, int, int)` when reached by texture x light/Gouraud modulation paths

Conditional candidates:

- `RGB565::blend256`, only if call sites show relevant 3D usage beyond 2D/Image paths
- `RGB565(const RGBf&)` and `RGB565::operator=(const RGBf&)`, only if call sites show per-pixel or per-vertex 3D relevance
- exact `meanColor4`, only after the hot 3D kernels have been covered

Expected lower priority:

- `meanColor`, because the source audit indicates it is mostly used for image reduction/downsampling rather than the main 3D shader path.

## Correctness Policy

Production integration requires exact output matching against current TGX behavior. Approximate variants may be measured and documented, but must not be integrated without explicit approval.

