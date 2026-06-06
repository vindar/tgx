# Previous Image Signal Attribution

Sources:

- `REPORT_IMAGE_INLINE_INVESTIGATION.md`
- `tmp/image_inline_investigation/reports/codegen_image_summary.md`
- `tmp/image_inline_investigation/codegen/pico2_image_binary_sizes.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbol_deltas.csv`
- `tmp/image_inline_investigation/pico2_subtest_matrix.csv`

## Reproduced Performance Pattern

Coarse `TGX_INLINE_IMAGE=` on Pico2/RP2350:

- Global score 1: `23.04 -> 24.05`, `+4.38%`
- Global score 2: `16.545 -> 17.31`, `+4.62%`
- Global score 3: `14.01 -> 14.63`, `+4.43%`

Large mesh-heavy gains:

| Renderer size | Subtest | Delta |
| --- | --- | ---: |
| 100 x 100 | `R2D2 (far)` | `+27.45%` |
| 100 x 100 | `R2D2 (farclip)` | `+35.35%` |
| 100 x 100 | `Bunny TEX_BILINEAR` | `+16.87%` |
| 100 x 100 | `Sphere direct GOUR` | `+21.71%` |
| 200 x 200 | `R2D2 (far)` | `+56.22%` |
| 200 x 200 | `R2D2 (farclip)` | `+54.68%` |
| 320 x 240 | `R2D2 (far)` | `+63.92%` |
| 320 x 240 | `R2D2 (farclip)` | `+60.92%` |

Severe guard-path regressions:

| Renderer size | Subtest | Delta |
| --- | --- | ---: |
| 100 x 100 | `Wire rib tri thick` | `-19.62%` |
| 200 x 200 | `Wire rib tri thick` | `-17.31%` |
| 320 x 240 | `Wire rib tri thick` | `-16.28%` |
| 100 x 100 | `Point cloud pixels` | `-8.97%` |
| 200 x 200 | `Point cloud pixels` | `-9.37%` |
| 320 x 240 | `Point cloud pixels` | `-8.45%` |
| 100 x 100 | `Point cloud dots` | `-13.98%` |
| 200 x 200 | `Point cloud dots` | `-14.86%` |
| 320 x 240 | `Point cloud dots` | `-15.11%` |

## Code Size and Symbol Movement

Pico2 program sizes:

| Candidate | ELF bytes | BIN bytes | UF2 bytes |
| --- | ---: | ---: | ---: |
| Baseline | 3,780,216 | 1,380,968 | 2,762,752 |
| Coarse `TGX_INLINE_IMAGE=` | 3,764,140 | 1,378,824 | 2,758,656 |
| `PIXEL_API_DRAW=` | 3,769,444 | 1,380,112 | 2,761,216 |
| `_drawPixel=` | 3,767,984 | 1,379,320 | 2,759,168 |
| Mesh-safe keep draw/wire/span | 3,780,264 | 1,380,968 | 2,762,752 |

The unsafe candidates shrink binary size, but the safe mesh-protected candidate returns to baseline size and loses the gain.

Key hot symbol changes:

- `Renderer3D::_drawDots<true,true,true>`:
  - Baseline size: `2096` bytes.
  - Coarse `TGX_INLINE_IMAGE=` size: `1354` bytes.
  - Delta: `-742` bytes.
  - Address moved from `0x1000ac40` to `0x10019d18`, about `+61 KB`.
- `Renderer3D::_drawPixels<true,true>`:
  - Baseline size: `680` bytes.
  - Coarse `TGX_INLINE_IMAGE=` size: `620` bytes.
  - Delta: `-60` bytes.
  - Address moved by about `+76 KB`.
- Multiple `Image<RGB565>::_bseg_*` helpers shrank by `80` to `404` bytes in the coarse candidate.
- `PIXEL_API_DRAW=` mainly moved hot Renderer3D/shader symbols and changed layout, with less direct symbol-size change.
- The mesh-safe candidate restored forced inline for draw/wire/span helpers, removed the severe local regressions, and also removed the global mesh-heavy gain.

## Direct vs Layout Effects

Direct call/inlining effects:

- Wire and dots regressions are direct. Pixel/Bresenham helpers stopped being forced inline, and the affected subtests are exactly the paths that depend on them.
- `_drawDots` and `_drawPixels` shrinkage correlates with point-cloud regressions.
- `_bseg_*` shrinkage correlates with thick/AA wire regressions.

Layout/code-size effects:

- Mesh-heavy 3D gains are indirect. Textured and Gouraud mesh paths mostly execute `Renderer3D`, `shader_select`, `rasterizeTriangle`, and `uber_shader`, not public `Image` pixel wrappers.
- Coarse Image no-inline changes large amounts of template code placement. It moves renderer and shader symbols by KB to tens of KB and slightly shrinks `.bin`.
- The giant far/farclip gains are too large to be explained by a direct Image accessor improvement. They are likely layout/codegen sensitivity in large Renderer3D/shader template instantiations.

## Target for This Session

Do not touch Image inline policy.

Instead target direct Renderer3D/shader/rasterizer causes:

- Reduce or split cold code from `_drawMesh`, `_drawTriangle`, `_drawTriangleClipped`, `_drawQuad`, `shader_select`, and selected `uber_shader` instantiations.
- Avoid damaging `_drawDots`, `_drawPixels`, wire helpers, or Image Bresenham paths.
- Prefer changes that alter mesh-heavy hot code directly rather than using Image helper layout as an accidental lever.
