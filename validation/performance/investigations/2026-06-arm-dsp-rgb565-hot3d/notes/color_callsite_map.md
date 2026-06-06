# RGB565 Hot 3D Call-site Map

## Priority Ranking

| Rank | Kernel | Main call sites | Why it matters |
| ---: | --- | --- | --- |
| 1 | `interpolateColorsBilinear(RGB565, ...)` | `src/Shaders.h:336` in `uber_shader` textured bilinear path | Per-pixel in 3D bilinear texture sampling. Directly affects `TEX_BILINEAR` mesh benchmarks. |
| 2 | `RGB565::mult256(int,int,int)` | `src/Shaders.h:352`, `src/Shaders.h:356` | Per-pixel texture modulation by Gouraud/flat lighting. Affects textured lit paths, nearest and bilinear. |
| 3 | `interpolateColorsTriangle(RGB565, ...)` | `src/Shaders.h:363` | Per-pixel untextured Gouraud/color interpolation. Affects `GOURAUD` mesh benchmarks. |
| 4 | `RGB565::blend256` | `src/Renderer3D.inl:2502` etc. | Used in Renderer3D wireframe AA/thick line paths and heavily in 2D/Image. Current implementation is already strong. |
| 5 | `RGB565(const RGBf&)` / assignment from `RGBf` | `src/Shaders.h` setup casts and `src/Color.inl` definitions | Used mostly during triangle setup for flat/Gouraud colors, not normally per-pixel in RGB565 mesh shader. |
| 6 | `meanColor4` | `src/Image.inl:1559` | Image downsample path, not hot 3D. Only a follow-up if time permits. |

## 3D Opaque Mesh Path

The main `uber_shader` path in `src/Shaders.h` resolves shader flags at compile time.

Texture bilinear:

- `src/Shaders.h:336`
- Samples four texels, computes `ax/ay`, then calls `interpolateColorsBilinear(...)`.
- If lit/Gouraud, the sampled color is then modulated with `mult256(...)`.

Texture nearest:

- Does not call bilinear interpolation.
- Still calls `mult256(...)` when texture is combined with Gouraud or flat lighting.

Untextured Gouraud:

- `src/Shaders.h:363`
- Calls `interpolateColorsTriangle(...)` per pixel.
- This is the primary RGB565 triangle color interpolation path for 3D.

Flat untextured:

- Converts `data.facecolor` from `RGBf` to `color_t` during setup.
- Per-pixel output is just `flat_color`; no RGB565 color kernel appears after setup.

## 2D / Image Paths

The same helpers are also used by 2D rasterization:

- `shader_2D_gradient` calls `interpolateColorsTriangle(...)`.
- `shader_2D_texture` calls `interpolateColorsBilinear(...)` and `mult256(...)`.
- `Image` AA/blit/drawing paths call `blend256(...)`.
- Image resize/downsampling calls `meanColor(...)`.

These are useful for correctness and sanity, but this session should not optimize only 2D/Image functions unless the change also helps the verified 3D shader path.

## Microbenchmark Implications

Mandatory microbench kernels:

- exact RGB565 bilinear interpolation;
- exact RGB565 triangle interpolation;
- exact RGB565 `mult256` 3-channel and 4-argument overload;
- span-style variants for bilinear/triangle/mult to detect whether a helper could be useful in shader-like loops.

Control kernels:

- `blend256`, mostly to confirm no new exact variant beats the existing packed implementation in a relevant way;
- `RGBf -> RGB565`, mostly setup frequency, not per-pixel;
- exact `meanColor4`, low priority and not 3D-hot.

Potential production patch criteria:

- exact output match;
- measurable speedup on Teensy 4.1 and/or Pico2;
- no meaningful slowdown on the other ARM board;
- relevance to `Shaders.h` hot call sites;
- small and maintainable diff.

