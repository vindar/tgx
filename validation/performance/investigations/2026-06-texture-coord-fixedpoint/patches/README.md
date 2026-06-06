# Patches

No production source patch was kept for this investigation.

The measured candidate that should be considered for a future focused patch is:

- affine/orthographic texture spans only;
- whole scanline segment safely inside the texture;
- `len >= 4`, preferably `len >= 8`;
- fixed 16 fractional-bit texture coordinates;
- nearest exact fast path;
- bilinear approximate fast path with max observed RGB565 channel error of 1;
- fallback to the current float path for perspective, edge, negative/wrap-risk,
  and very short spans.

The next patch should target `src/Shaders.h` only and should be validated with
CPU render hashes, hardware texture benchmark rows, and real textured examples.
