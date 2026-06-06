# Image Inline Path Map

Source inspected: `src/Image.h`, `src/Image.inl`, benchmark call sites in `examples/benchmark/benchmark.ino`, and validation references.

## Inline Helper Families

| Family | Helpers | Likely benchmark paths |
| --- | --- | --- |
| Accessors / bounds | `isValid`, `width`, `height`, `stride`, `dim`, `imageBox`, `data` | Setup, clipping, general image users. Tiny and should normally stay inline. |
| Public pixel wrappers | `drawPixel`, `drawPixelf`, `readPixel`, `readPixelf` | Direct pixel API, render-hash probes, primitive endpoints, some high-level examples. |
| Unchecked pixel access | `operator()(iVec2)`, `operator()(x,y)` | AA fill code, circle/ellipse/rounded-shape internals, direct image access. Per-pixel and should normally stay inline. |
| Internal pixel helpers | `_drawPixel`, `_readPixel` | Primitive internals, circles/ellipses, line endpoints, text/shape helpers. Per-pixel and strongly wire/dots related. |
| Bresenham wire helpers | `_bseg_update_pixel` overloads | `Wire cube`, `Wire rib`, `Wire sphere`, AA/thick wire and line paths. These are prime suspects for the coarse `TGX_INLINE_IMAGE=` local regressions. |
| Span dispatch wrappers | `_drawFastVLine(bool, ...)`, `_drawFastHLine(bool, ...)` | Fill/rect/circle/triangle span paths. These are small dispatchers and may interact with filled primitive layout. |
| Segment dispatcher | `_drawSeg` | Line/wire primitive setup before Bresenham drawing. |
| 2D triangle coordinate helpers | `_coord_texture`, `_coord_viewport` | 2D textured/gradient triangle helpers, not the main 3D mesh renderer. |
| Font pixel helper | `_drawFontPixel` | Text rendering only. |

## Benchmark Connections

- `Wire cube fast`, `Wire cube AA`, `Wire rib tri thick`, `Wire rib strip thick`, and `Wire sphere thick` depend heavily on `_bseg_update_pixel`, `_drawSeg`, `_drawPixel`, and fast-line/shape support.
- `Point cloud pixels` and `Point cloud dots` are called through `renderer.drawPixels(...)` and `renderer.drawDots(...)` in `examples/benchmark/benchmark.ino`; the previous coarse Image no-inline result regressed these strongly, probably because image/pixel access helpers changed code layout around the renderer's 2D/3D overlay paths.
- Mesh-heavy 3D paths such as `R2D2 (far)`, `R2D2 (farclip)`, `Bunny TEX_BILINEAR`, and `Sphere direct GOUR` improved with coarse `TGX_INLINE_IMAGE=`, suggesting a code-size/layout effect rather than a direct image-helper speedup.
- Textured 3D mesh rasterization is mostly in renderer/shader code, not the public `Image` API. Any Image inline effect on those paths is therefore likely indirect through code layout, template instantiation size, or shared helper placement.

## Working Hypothesis

The coarse `TGX_INLINE_IMAGE=` override improves some mesh-heavy Pico2 benchmark rows by changing code layout and reducing forced inlining in Image helpers. It simultaneously hurts wire/dots because the actual per-pixel helpers used by those paths stop being forced inline. A viable candidate would need to keep wire/dots pixel helpers forced inline while relaxing only less critical Image helpers.
