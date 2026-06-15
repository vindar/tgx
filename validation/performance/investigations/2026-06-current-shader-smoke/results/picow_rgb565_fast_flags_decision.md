# Pico W RGB565 Fast Shader Flags

Date: 2026-06-15

Board: Pico W / RP2040 + ILI9341, COM19

Build flags used for all rows:

- `-DTGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=0`
- `-DTGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=0`

These two no-FPU overrides were kept fixed so the test only isolates:

- `TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE`
- `TGX_SHADER_RGB565_FAST_BILINEAR`

The `-D` values were verified in the robust-tool `COMPILE_UPLOAD_COMMAND` logs.

## Bunny Fig Nearest/Gouraud-Texture Matrix

Source CSV: `picow_rgb565_fast_flags_bunny_matrix.csv`

`bunny_fig` uses nearest texture, so it is useful for `FAST_TEXTURE_MODULATE` but not for `FAST_BILINEAR`.

| Scene | Best | Notes |
| --- | --- | --- |
| `flat` | `MODULATE=1` | `MODULATE=0` was about 6.1% slower. |
| `gouraud` | `MODULATE=1` | `MODULATE=0` was about 4.4% slower. |
| `gouraud_texture` | `MODULATE=1` | `MODULATE=0` was about 0.8% slower. |
| `wireframe` | neutral | `MODULATE=1` was not worse. |

Binary size also favored `MODULATE=1` in this build:

- `MODULATE=1`: 571684 bytes
- `MODULATE=0`: 571740 bytes

Decision: keep `TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE=1` on Pico W.

## Bilinear Micro-Smoke

Sketch:

`validation/performance/investigations/2026-06-current-shader-smoke/sketches/PicoWShaderBilinearFlagBench/`

This display-free sketch draws textured cubes through bilinear shader paths into a RAM framebuffer.

Fixed flags:

- `TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE=1`

Comparison:

| Case | `FAST_BILINEAR=1` time us | `FAST_BILINEAR=0` time us | Delta for `=1` | Hash |
| --- | ---: | ---: | ---: | --- |
| `bilinear_flat` | 672621 | 672324 | +0.04% | same |
| `bilinear_gouraud` | 671659 | 671426 | +0.03% | same |
| `bilinear_unlit` | 657329 | 656832 | +0.08% | same |

The flat/gouraud rows are expected to be mostly unaffected when `FAST_TEXTURE_MODULATE=1`,
because they use the combined `tgx_rgb565_bilinear_modulate256()` path.
The unlit row is the relevant row for `FAST_BILINEAR`.

Binary size with the unlit bilinear test:

- `FAST_BILINEAR=1`: 104816 bytes
- `FAST_BILINEAR=0`: 104832 bytes

Decision: timing difference is noise-level on Pico W, but `FAST_BILINEAR=1` preserves output and is slightly smaller. Keep `TGX_SHADER_RGB565_FAST_BILINEAR=1`.

## Final Recommendation For Pico W

Use:

```cpp
#define TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE 1
#define TGX_SHADER_RGB565_FAST_BILINEAR 1
```

Keep the no-FPU incremental overrides disabled on Pico W:

```cpp
#define TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL 0
#define TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL 0
```
