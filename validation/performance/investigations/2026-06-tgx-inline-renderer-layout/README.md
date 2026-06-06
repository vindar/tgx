# June 2026 TGX Inline / Renderer Layout Investigation

This folder archives the June 2026 TGX performance investigation artifacts.

The work focused on inline policy, architecture-specific configuration, Image helper inlining, Renderer3D layout, shader dispatch, rasterizer behavior, and Pico2 / RP2350 codegen sensitivity.

## Boards Used

| Board | Port | Notes |
| --- | --- | --- |
| Pico2 / RP2350 + ILI9341 | `COM21` | Primary target and most sensitive board |
| Teensy 4.1 + ILI9341 | `COM3` | Cross-board validation board |
| Core2 | `COM5` | ESP32 validation board |
| CoreS3 | `COM10` | ESP32-S3 validation board |
| Pico W / RP2040 | `COM28` | Unavailable / ignored in final sessions |

## Source Optimization Kept

The only source optimization retained from this broader sequence was committed before the final renderer-layout investigation:

- `TGX_INLINE_ZDIVIDE` policy for Teensy4.x, ESP32-S3, and RP2350.
- RP2350 shader incremental pixel pointer path.

No additional source patch was retained after the inline-granularity, Image-inline, or direct Renderer3D/layout investigations archived here.

## Rejected Approaches

The following approaches were tested and rejected:

- Broad category inline macros such as `TGX_INLINE_VEC2`, `TGX_INLINE_VEC3`, `TGX_INLINE_VEC4`, `TGX_INLINE_COLOR`, `TGX_INLINE_RGBF`, `TGX_INLINE_MATH`, `TGX_INLINE_SHADER`, and `TGX_INLINE_IMAGE`.
- Coarse `TGX_INLINE_IMAGE=`.
- Finer Image helper splits.
- Direct Renderer3D / shader / rasterizer / layout candidates.
- Aggressive `rasterizeTriangle()` inlining.
- Static shader dispatch through template function pointers.
- RP2350 `flatten` on `_drawMesh(Mesh3Dv2)`.
- Exact and narrowed `shader_select()` fast paths.

## Why They Were Rejected

Several candidates produced large global gains on Pico2, but the gains hid unacceptable local regressions:

- wire and dots regressions;
- thick wire regressions;
- point-cloud regressions;
- texture bilinear regressions;
- flat and large-triangle regressions;
- ribbon regressions;
- layout/code-size fragility.

The final conclusion is that broad layout perturbations are too risky. Future work should use isolated microbenchmarks for rasterizer and shader kernels before attempting another source-level optimization.

## Directory Map

```text
reports/
  Top-level reports and per-candidate notes.

summaries/
  CSV/Markdown summaries copied from tmp/config_optimization,
  tmp/inline_granularity, tmp/image_inline_investigation, and
  tmp/renderer_layout_investigation.

patches/
  Rejected candidate patches grouped by investigation.

codegen/
  Compact codegen summaries and symbol CSVs. Multi-MB objdump dumps are omitted.

hardware/
  Compact upload/capture metadata, logs, and telemetry copied from
  tmp/hardware_validation. Build outputs are omitted.

raw_or_large_artifacts/
  Notes about omitted bulky files left in tmp/.
```

Reusable tools were archived separately in:

```text
validation/performance/tools/
```

## Key Reports

- `reports/REPORT_CONFIG_OPTIMIZATION.md`
- `reports/REPORT_INLINE_GRANULARITY.md`
- `reports/REPORT_IMAGE_INLINE_INVESTIGATION.md`
- `reports/REPORT_RENDERER_LAYOUT_INVESTIGATION.md`

Read them in that order for the full story.
