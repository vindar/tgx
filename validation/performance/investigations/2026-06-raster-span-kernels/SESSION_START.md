# Session Start: Raster / Shader Span Kernels

Date: 2026-06-06

Branch: `feature/multi-directional-lights`

Commit:

```text
cb581ffa538118525d872424e7107028f43a31ab
```

Initial source diff:

```text
git status --short: clean
git diff --stat: clean
```

## Target Boards

| Board | Port | Role |
| --- | --- | --- |
| Teensy 4.1 / Cortex-M7 | `COM3` | ARM runtime target |
| Pico2 / RP2350 / Cortex-M33 | `COM21` | ARM runtime target |

Pico W / RP2040 is intentionally ignored because it is unavailable.
Core2/CoreS3 are compile sanity targets only if a generic production patch
survives the microbenchmark phase.

## Tools

Use the reusable capture/upload tooling in:

```text
validation/performance/tools/
```

In particular:

- `upload_and_capture.ps1`
- probe sketches under `validation/performance/tools/sketches/`

No ad-hoc serial capture should be used.

## Previous Results Not Repeated

- `TGX_INLINE_ZDIVIDE`: already committed and treated as baseline.
- Broad category inline macros: tested and rejected/reverted.
- Coarse `TGX_INLINE_IMAGE=`: rejected due hidden wire/dots regressions.
- Broad Renderer3D/shader/layout perturbations: rejected as fragile.
- Isolated RGB565 color-only kernels: `meanColor2` accepted earlier; hot 3D
  color kernels did not produce a robust exact patch.

## Planned Span Kernels

The benchmark will model per-pixel work from `src/Shaders.h` rather than the
full triangle pipeline:

- z-only depth test/write;
- flat RGB565 write, no z;
- flat RGB565 with z-test;
- Gouraud RGB565 color interpolation with optional z-test;
- texture nearest span with optional z-test;
- texture bilinear span using exact TGX RGB565 bilinear interpolation;
- texture x color/light modulation using `RGB565::mult256`;
- short-span and long-span behavior.

Span lengths:

```text
1, 2, 3, 4, 5, 8, 16, 32, 64, 128, 320
```

## Source Paths Mapped

Likely source correspondences:

- `src/Shaders.h`: `uber_shader`, `shader_select`, texture nearest/bilinear,
  z-buffer test/write, RGB565 modulation, Gouraud interpolation.
- `src/Renderer3D.inl`: wire/dots/pixel paths for context only; not the main
  target of this session.
- `src/Color.h`: exact RGB565 bilinear, triangle interpolation, and `mult256`
  helpers used inside shader paths.
- `src/tgx_config.h`: `TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` and target
  inlining/config policy.

Production TGX source must not be changed until an exact microbenchmark
candidate shows a useful, relevant speedup.
