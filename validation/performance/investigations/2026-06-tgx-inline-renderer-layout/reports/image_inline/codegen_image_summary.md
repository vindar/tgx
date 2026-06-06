# Image Inline Codegen Summary

Pico2/RP2350 codegen was extracted with:

- `arm-none-eabi-nm -C -S --size-sort`
- `arm-none-eabi-objdump -Cd`

Artifacts:

- `tmp/image_inline_investigation/codegen/pico2_image_binary_sizes.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbols.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbol_deltas.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_top_size_deltas.csv`
- `tmp/image_inline_investigation/codegen/*.objdump.txt`

## Binary Sizes

| Candidate | ELF bytes | BIN bytes | UF2 bytes |
| --- | ---: | ---: | ---: |
| baseline | 3,780,216 | 1,380,968 | 2,762,752 |
| categories default | 3,780,224 | 1,380,968 | 2,762,752 |
| coarse `TGX_INLINE_IMAGE=` | 3,764,140 | 1,378,824 | 2,758,656 |
| `PIXEL_API_DRAW=` | 3,769,444 | 1,380,112 | 2,761,216 |
| `_drawPixel=` | 3,767,984 | 1,379,320 | 2,759,168 |
| mesh-safe keep draw/wire/span | 3,780,264 | 1,380,968 | 2,762,752 |

## Observations

The default category split is codegen-neutral at program size level. It changes only ELF metadata/noise; `.bin` and `.uf2` sizes match baseline.

The unsafe candidates reduce code size and move hot renderer symbols:

- Coarse `TGX_INLINE_IMAGE=` shrinks `.bin` by about 2.1 KB and `.uf2` by 4 KB.
- `PIXEL_API_DRAW=` shrinks `.bin` by about 0.9 KB and `.uf2` by 1.5 KB.
- `_drawPixel=` shrinks `.bin` by about 1.6 KB and `.uf2` by 3.5 KB.

The biggest symbol changes are not in a direct 3D mesh shader function. They are in point/dot and Image/wire helpers:

- Coarse `TGX_INLINE_IMAGE=` changes `Renderer3D::_drawDots<...>` by -742 bytes and moves it by about +61 KB.
- `_drawPixel=` has the same -742 byte `Renderer3D::_drawDots<...>` change.
- Coarse `TGX_INLINE_IMAGE=` also shrinks multiple `Image<RGB565>::_bseg_*` helpers by 80 to 404 bytes.
- `_drawPixels<...>` shrinks by about 60 bytes in coarse and `_drawPixel=` variants.
- `PIXEL_API_DRAW=` mostly changes addresses/layout rather than large hot symbol sizes, but still moves `_drawDots`, `_drawPixels`, `_drawTriangle*`, `_drawQuad`, and `_drawMesh` by several KB to tens of KB.

## Attribution

The benchmark behavior matches codegen:

- `PIXEL_API_DRAW=` produces the best global score pattern but severely regresses point cloud and wire paths. The changed public `drawPixel/drawPixelf` wrappers alter hot renderer layout and likely introduce call/layout costs in point/dot-like paths.
- `_drawPixel=` also produces large mesh-heavy gains, but directly hurts `Wire rib tri thick` and point-cloud paths.
- The mesh-safe candidate keeps public draw wrappers, internal draw helpers, wire helpers, and span dispatchers forced inline. It removes the severe local regressions, but the global mesh-heavy gains disappear.

The conclusion is that the coarse Image signal is a layout/codegen accident coupled with real point/wire helper costs. There is no measured narrow Image inline policy that keeps the useful global gain while passing local subtest gates.
