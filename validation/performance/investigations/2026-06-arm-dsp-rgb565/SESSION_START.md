# ARM DSP RGB565 Microbenchmark Session Start

## Repository State

- Branch: `feature/multi-directional-lights`
- Commit: `cd6c1d15fe0f721048d64f3f4becf35ba1a8d724`
- Starting source diff: clean tracked source tree; new artifacts for this investigation will live under `validation/performance/investigations/2026-06-arm-dsp-rgb565/`.
- Production TGX source must not be modified until a microbenchmark candidate proves a useful exact speedup.

## Boards

| Board | Port | Runtime role |
| --- | --- | --- |
| Teensy 4.1 / Cortex-M7 | `COM3` | Primary ARM DSP target |
| Pico2 / RP2350 / Cortex-M33 | `COM21` | Primary ARM DSP target |

Pico W / RP2040 is intentionally ignored for this session.

## Existing Tools

Reusable upload/capture tooling is under `validation/performance/tools/`.

Important tools for this session:

- `upload_and_capture.ps1`: robust compile/upload/serial capture with start/end marker checks.
- `run_probe_matrix.ps1`: historical probe matrix helper.
- `parse_benchmark_logs.py`, `parse_example_telemetry.py`, and aggregation scripts: useful patterns, but the RGB565 microbench gets a dedicated parser.

## Previous Approaches Not To Repeat

- `TGX_INLINE_ZDIVIDE` was already committed and is part of the baseline.
- Broad inline category macros were rejected and reverted.
- Coarse `TGX_INLINE_IMAGE=` and finer Image-helper splits were rejected because global gains hid wire/dots and point-cloud regressions.
- Direct Renderer3D/shader/layout perturbations were rejected as fragile.

This session is microbenchmark-first and focuses on exact RGB565 kernels only.

## Source Functions Inspected

Relevant source locations:

- `src/Color.h`
- `src/Color.inl`
- `src/Misc.h`
- `src/tgx_config.h`

Primary RGB565 kernels to test:

- `RGB565::blend256(const RGB565&, uint32_t)`
- `meanColor(RGB565, RGB565)`
- `meanColor(RGB565, RGB565, RGB565, RGB565)`
- `interpolateColorsTriangle(const RGB565&, int32_t, const RGB565&, int32_t, const RGB565&, int32_t)`
- `interpolateColorsBilinear(const RGB565&, const RGB565&, const RGB565&, const RGB565&, float, float)`
- `RGB565(const RGBf&)` and `RGB565::operator=(const RGBf&)`

Important current semantics:

- `blend256` maps alpha `[0,256]` to `a = alpha >> 3` and blends packed `R/G/B` lanes with a 32-step factor.
- `meanColor` currently averages unpacked components with truncating right shifts.
- Triangle interpolation rescales `C1` and `C2` to 5-bit weights using integer division before packed lane accumulation.
- Bilinear interpolation converts `ax` and `ay` to 8-bit fixed-point weights and truncates channels after a 16-bit shift.
- `RGBf` to `RGB565` conversion multiplies floats by `31/63/31` and truncates.

## Kernels Planned

1. RGB565 blend256, including fixed alpha and random alpha.
2. RGB565 meanColor2.
3. RGB565 meanColor4.
4. RGB565 triangle interpolation.
5. RGB565 bilinear interpolation.
6. RGBf to RGB565 conversion.
7. Span kernels for blend, mean, bilinear, fill/copy-style RGB565 loops.

## Correctness Policy

Every integration candidate must match the current TGX-compatible reference exactly. Approximate-only variants may be measured for notes, but they are not source integration candidates.
