# Raster / Shader Span Path Map

This audit maps the current TGX 3D shader pixel loop to isolated span kernels.

## Main Source Path

The main target is `src/Shaders.h::uber_shader(...)`.

The function performs:

1. common framebuffer and triangle setup;
2. optional z-buffer setup;
3. optional Gouraud / flat color setup;
4. optional texture setup;
5. scanline clipping and per-span interpolation setup;
6. per-pixel loop:
   - z-test/write if enabled;
   - texture coordinate interpolation if textured;
   - nearest or bilinear texture sampling;
   - optional texture x Gouraud/flat color modulation;
   - untextured Gouraud/flat color selection;
   - framebuffer write;
   - per-pixel increments.

## Benchmark Kernel Mapping

| Kernel | Real TGX Path |
| --- | --- |
| A: z-only span | `USE_ZBUFFER` branch in `uber_shader`, without color work. |
| B: flat RGB565 no z | no-z flat store path. |
| C: flat RGB565 + z | `USE_ZBUFFER` + flat no-texture store. |
| D: Gouraud RGB565 | no-texture `USE_GOURAUD`, uses `interpolateColorsTriangle`. |
| E: texture nearest | `USE_TEXTURE`, `TEXTURE_BILINEAR == false`, one texel load. |
| F: texture bilinear | `TEXTURE_BILINEAR == true`, four texel loads and `interpolateColorsBilinear`. |
| G: texture x color/light modulation | texture sample followed by `RGB565::mult256`. |
| H/I: short/long spans | Same kernels measured at lengths 1..320. |

## RP2350-Specific Existing Path

`TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` is enabled for RP2350 in
`src/tgx_config.h`. In `uber_shader` this changes:

- framebuffer access from `buf[bx]` to `*pix` plus `pix++`;
- z-buffer access from `zbuf[bx]` to `*zpix` plus `zpix++`.

The microbenchmark therefore includes reference indexing loops and pointer
increment variants so this policy can be evaluated in isolation without
touching production shader templates.

## Non-Targets

Wireframe, dots, and circle-z paths live mainly in `src/Renderer3D.inl`.
They are intentionally not the main target because previous renderer/layout
investigations found global gains that hid severe wire/dots regressions. This
session focuses on triangle shader spans first.
