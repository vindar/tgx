# Texture Coordinate Semantics Audit

Date: 2026-06-06

Primary source: `src/Shaders.h::uber_shader(...)`.

## 1. Representation Before The Shader Loop

3D texture coordinates are stored in `RasterizerVec4::T` as `fVec2`.

For legacy `Mesh3D`, the coordinates are read directly from the mesh-provided
`fVec2` table. For `Mesh3Dv2`, the compact payload path decodes local texture
coordinates from signed 16-bit integers:

```cpp
return fVec2(((float)q[0]) * scale,
             ((float)q[1]) * scale);
```

with `scale = 4.0f / 32767.0f`, so Mesh3Dv2 coordinates are not inherently
limited to `[0,1]`. Negative coordinates and coordinates above 1 are possible.

Clipping preserves texture coordinates by linear interpolation in
`Renderer3D.inl::_triangleClip1in/_triangleClip2in`.

## 2. Screen-Space Or Perspective-Correct

Both cases exist.

In `USE_ORTHO`, the shader computes:

```cpp
T1 *= invaera; T2 *= invaera; T3 *= invaera;
T*.x *= texsize_x; T*.y *= texsize_y;
tx = T1.x*C1 + T2.x*C2 + T3.x*C3;
tx += dtx per pixel;
xx = tx;
```

This is affine in screen space and is the safest target for a fixed-point
incremental fast path.

In perspective mode, the shader computes:

```cpp
fP1a_p = fP1.w * invaera;
T1 *= fP1a_p;
cw_p = C1*fP1a_p + C2*fP2a_p + C3*fP3a_p;
xx = tx * fast_inv(cw_p);
```

So `tx` and `cw_p` are both linear, but the final sampled coordinate `xx` is a
rational function per pixel. A simple fixed affine increment cannot be exact for
general perspective-correct texturing.

## 3. Per-Pixel Increments

The shader increments:

- `tx += dtx`
- `ty += dty`
- `cw_p += dw_p` only in perspective mode
- `C2 += dx2`, `C3 += dx3`

The per-scanline starting `tx/ty/cw_p` values are recomputed from the clipped
start `bx`.

## 4. Nearest Texel Selection

Nearest does not use `floorf`.

```cpp
const int ttx = TEXTURE_WRAP ? ((int)(xx)) & texsize_x_mm
                             : shaderclip((int)(xx), texsize_x_mm);
```

C++ casts from float to int truncate toward zero. For positive coordinates this
matches floor for non-integers. For negative coordinates it differs: `-0.25f`
casts to `0`, while `floor(-0.25f)` is `-1`.

Any exact nearest fixed-point path must reproduce truncation-toward-zero, not
mathematical floor, unless it is restricted to non-negative interior
coordinates.

## 5. Bilinear Fractional Weights

Bilinear uses floor:

```cpp
const int ttx = lfloorf(xx);
const float ax = xx - ttx;
```

For negative coordinates, this keeps `ax` and `ay` in `[0,1)`.

The RGB565 bilinear color function then quantizes:

```cpp
const int iax = (int)(ax * 256);
const int iay = (int)(ay * 256);
```

So exact RGB565 bilinear equality only requires matching the integer texel
indices and these 8-bit fractional weights, not preserving the full float
fraction.

## 6. Edge Behavior

Clamp mode uses:

```cpp
shaderclip(v, maxv)
```

which clamps below 0 to 0 and above `maxv` to `maxv`.

Wrap mode uses:

```cpp
v & (texture_size - 1)
```

This assumes power-of-two texture dimensions. It is not a modulo operation for
arbitrary texture sizes.

For bilinear, both `ttx` and `ttx+1` are clamped or wrapped.
For nearest, only the selected integer coordinate is clamped or wrapped.

## 7. Negative Coordinates

Negative coordinates are possible because user-provided mesh texture
coordinates are not limited to `[0,1]`, Mesh3Dv2 coordinates can decode outside
`[0,1]`, and clipped triangles interpolate whatever values are present.

This matters most for nearest because truncation differs from floor for negative
fractional values.

## 8. Texture Dimensions

Clamp supports arbitrary texture dimensions.

Wrap is explicitly named `SHADER_TEXTURE_WRAP_POW2` and uses bit masks, so it is
safe only for power-of-two dimensions.

The shader scales normalized coordinates by `texsize_x` and `texsize_y`, not by
`texsize_x - 1`.

## 9. Exact Fixed-Point Opportunities

Likely exact or near-exact:

- affine/orthographic nearest for non-negative interior coordinates, if fixed
  conversion reproduces truncation;
- affine/orthographic bilinear if texel indices and `(int)(ax*256)` /
  `(int)(ay*256)` match;
- wrap power-of-two affine cases if mask behavior is preserved.

Risky or approximate:

- perspective-correct texturing with simple affine fixed increments;
- negative nearest coordinates unless truncation-toward-zero is reproduced;
- edge cases where a tiny fixed-point rounding difference changes clamp/wrap
  side;
- bilinear near exact integer texel boundaries.

## 10. Initial Safe-Subset Hypothesis

The first production candidate, if any, should be restricted to:

- `USE_ORTHO == true`;
- `USE_TEXTURE == true`;
- either nearest, or bilinear with enough fixed fractional precision;
- initially interior spans only, with fallback near edges;
- no change to color modulation or z-buffer logic.

Perspective approximation should be measured, but not integrated unless visual
error is very small and representative render-hash/image-difference tests pass.
