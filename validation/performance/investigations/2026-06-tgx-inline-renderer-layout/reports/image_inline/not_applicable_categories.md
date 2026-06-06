# Not Applicable Image Inline Categories

The requested image categories were inspected against `src/Image.h` and `src/Image.inl`.

## Blit

No existing `TGX_INLINE_IMAGE` use directly wraps the large blit implementations. The relevant blit functions are normal out-of-line/template implementations in `Image.inl`, not tiny inline helpers in `Image.h`.

The closest applicable category is `TGX_INLINE_IMAGE_SPAN`, which controls tiny `_drawFastHLine/_drawFastVLine` dispatchers used by span/fill-like paths. This was tested as `image_span_empty_pico2`.

## Dots

The public `Image` class does not expose a separate `TGX_INLINE_IMAGE` helper dedicated to dots. The benchmark `Point cloud pixels` and `Point cloud dots` paths are driven through `Renderer3D::drawPixels()` and `Renderer3D::drawDots()`.

The image inline policy still affects those paths indirectly through code layout and through public pixel wrappers, especially `drawPixel/drawPixelf`, but there is no clean Image-only `TGX_INLINE_IMAGE_DOTS` helper to isolate.

## Mesh

The 3D mesh rasterization paths are primarily in `Renderer3D` and shader code. Image helper changes affect mesh-heavy benchmark rows indirectly through binary layout/codegen, not through a direct Image mesh helper. The `TGX_INLINE_IMAGE_TRIANGLE` category only applies to 2D Image textured/gradient triangle coordinate helpers.
