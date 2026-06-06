# Renderer3D / Shader Hot Path Map

## Primary Mesh Path

Mesh draw calls enter `_drawMesh(...)`, then iterate triangles or meshlet chains.

Hot sequence for normal visible triangles:

1. `_drawMesh(Mesh3D)` or `_drawMesh(Mesh3Dv2)`
2. model/view transform and face culling
3. optional per-triangle clip test
4. Gouraud vertex lighting or flat face lighting
5. `rasterizeTriangle(...)`
6. `shader_select(...)`
7. selected `uber_shader(...)` variant
8. per-pixel texture/zbuffer/Gouraud/flat write loop

The Image-inline signal improved mesh-heavy rows indirectly, so the direct targets are steps 5-7 and possibly the hot part of step 1.

## Clipped Triangle Slow Path

If a triangle needs clipping:

1. `_drawTriangleClipped(...)`
2. `_drawTriangleClippedSub(...)`
3. `_triangleClip(...)` and helpers
4. final `rasterizeTriangle(...)`

This is a candidate for cold-path separation, but far/farclip benchmark rows can make this path important. Any split must be tested on far/farclip rows explicitly.

## Shader Dispatch

`shader_select(...)` is a large nested runtime dispatch over:

- zbuffer/no-zbuffer
- ortho/perspective
- texture/no-texture
- Gouraud/flat/unlit
- bilinear/nearest
- wrap/clamp

It is template-heavy and appears as a symbol in Pico2 codegen, so it is not simply inlined away. It may still influence layout and call target placement.

## Per-Pixel Shader Loop

`uber_shader(...)` is the core 3D per-pixel loop. Important expensive subpaths:

- zbuffer test/write
- perspective texture reciprocal
- bilinear texture sampling
- Gouraud color interpolation
- flat texture modulation
- `TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` on RP2350

This is the most sensitive target, but changes must avoid broad template bloat.

## Wire/Dots Guard Paths

Do not damage:

- `_drawPixels(...)`
- `_drawDots(...)`
- `_drawCircleZbuf(...)`
- `_drawWireFrame*`
- Image Bresenham/pixel helpers

These guard paths were harmed by coarse Image-inline changes and are explicit Pico2 rejection gates.

## Initial Candidate Priorities

1. Attribute/layout experiments on shader functions:
   - noinline/align `uber_shader`
   - noinline/align `shader_select`
   - noinline/align `rasterizeTriangle`
2. Cold clipping split:
   - keep common non-clipped path unchanged
   - move rare recursive clipping/code out of hot layout if possible
3. Shader selection fast-path experiments:
   - common perspective + zbuffer + texture cases
   - avoid changing wire/dots
4. Mesh setup code-size experiments:
   - Mesh3Dv2 slow material/clip branches
   - only if measured codegen points there
