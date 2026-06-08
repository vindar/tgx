# Decoder wrapper audit

## Scope

This audit covers the TGX decoder wrappers in `src/Image.inl`:

- PNGdec wrapper: lines around 67-286
- JPEGDEC wrapper: lines around 322-469
- AnimatedGIF wrapper: lines around 498-700

The only production optimization considered in this investigation is a portable row-wise `memcpy` in the JPEG RGB565 opaque/in-bounds path.

## JPEGDEC wrapper

`Image<color_t>::JPEGDecode()` creates `_JPEGDECWrapper`, stores the destination image type, stores the destination image pointer and opacity, and calls:

```cpp
jpeg.setUserPointer((void*)(&wrap));
jpeg.setPixelType((wrap.imgType == id_color_type<RGB565>::value) ? TGX_RGB565_LITTLE_ENDIAN : TGX_RGB8888);
return jpeg.decode(0, 0, options);
```

For `Image<RGB565>`, JPEGDEC is therefore asked to produce little-endian RGB565 pixels. The draw callback dispatches to `_jpegdec_color_convert565()`.

### Safe row-copy case

The existing `_jpegdec_color_convert565()` fast path is:

- opacity `op >= 1.0f`;
- destination rectangle fully inside `im`;
- source pointer `pDraw->pPixels` is decoder-owned input for the current JPEG block;
- destination row memory is contiguous for `pDraw->iWidth` pixels;
- destination stride may be larger than block width and must be respected.

In that fast path, the current inner loop:

```cpp
fb[i + str * j] = *(p++);
```

is equivalent to:

```cpp
memcpy(fb + str * j, p, pDraw->iWidth * sizeof(uint16_t));
p += pDraw->iWidth;
```

for every row. Odd widths work because `memcpy` copies bytes. Arbitrary JPEG block widths/heights work because the number of copied pixels remains `iWidth` by `iHeight`. Destination stride remains respected because each row uses `fb + str * j`. Source and destination do not overlap because `pDraw->pPixels` is the decoder output block, not a TGX subimage of the destination.

### Fallbacks that must stay unchanged

The fallback path handles clipping and opacity/blending with:

```cpp
im->template drawPixel<true>({ x + i, y + j }, RGB565(*(p++)), op);
```

This path must remain unchanged because it preserves clipping and opacity semantics.

### Non-RGB565 JPEG destinations

Non-RGB565 destinations use `_jpegdec_color_convert8888()`. JPEGDEC is configured for `TGX_RGB8888`, and the wrapper converts each pixel to the destination `color_t`. This is not a row-copy candidate because it performs a color conversion.

## PNGdec wrapper

PNGdec output can be grayscale, truecolor, truecolor+alpha, indexed palette, and several bit depths. `_pngdec_color_convert()` handles:

- grayscale with or without alpha;
- truecolor and truecolor+alpha;
- indexed 1/2/4/8 bpp palette images;
- palette alpha;
- clipping through `skipx` and `lx`;
- opacity blending through `blend()`.

Even an opaque RGB-like PNG row is not currently structured as a decoder-owned RGB565 row that maps directly to the destination. The wrapper performs pixel format conversion and may skip/clamp parts of the row. No production PNG optimization is attempted in this pass.

## AnimatedGIF wrapper

GIF output is palette/index based. `gif_decode()` handles:

- RGB565 little-endian palette;
- RGB565 big-endian palette;
- RGB888 palette;
- transparency index;
- disposal method 2 background restore;
- opacity blending;
- clipping to the destination row.

Although the no-transparency RGB565 little-endian palette path is simple, the source is still index bytes, not a contiguous RGB565 pixel row. It must map indices through the palette. No production GIF optimization is attempted in this pass.

## Conclusion

The JPEG RGB565 opaque/in-bounds path is the only clear, portable row-copy candidate. PNG and GIF are useful to port/compile for wrapper coverage, but their production paths are intentionally left unchanged.
