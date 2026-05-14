@page intro_api_3D Using the 3D API.


**This page introduces the TGX 3D renderer and shows the usual way to render solid 3D objects onto a `tgx::Image`.**

- For the complete list of methods, see the \ref tgx::Renderer3D class documentation.
- For the mesh container format, see \ref tgx::Mesh3D.
- For ready-to-run code, see the 3D examples in the `/examples/` directory.

The 3D engine is designed for small embedded targets. It does not try to be a full scene graph or a desktop GPU API.
Instead, it gives direct control over a software rasterizer that draws triangles, quads, meshes, cubes and spheres
into an image buffer. That image can then be displayed on a screen, copied to a DMA buffer, saved on a desktop target,
or used as a texture by other TGX drawing code.


![buddha](../buddha.png)

*The `buddha` mesh rendered by TGX's 3D engine.*

*See the example located in `examples/CPU/buddhaOnCPU/`.*


@section sec_3D_overview Rendering model

The main class is \ref tgx::Renderer3D. A renderer stores:

- a destination \ref tgx::Image;
- a viewport size and, optionally, an offset for tile rendering;
- an optional Z-buffer;
- projection, view and model matrices;
- light and material settings;
- the current shader state.

A typical frame is:

~~~{.cpp}
#include <tgx.h>

constexpr int W = 320;
constexpr int H = 240;

tgx::RGB565 framebuffer[W * H];
uint16_t zbuffer[W * H];

tgx::Image<tgx::RGB565> image(framebuffer, W, H);

// Keep only the shader variants that this program will actually use.
using Renderer = tgx::Renderer3D<
    tgx::RGB565,
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_FLAT |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_NOTEXTURE |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_WRAP_POW2,
    uint16_t>;

Renderer renderer({ W, H }, &image, zbuffer);

void drawFrame(float angle)
{
    image.clear(tgx::RGB565_Black);
    renderer.clearZbuffer();

    renderer.setPerspective(45.0f, (float)W / H, 0.1f, 100.0f);
    renderer.setLookAt({ 0.0f, 0.0f, 6.0f },   // camera position
                       { 0.0f, 0.0f, 0.0f },   // target point
                       { 0.0f, 1.0f, 0.0f });  // up direction

    renderer.setLightDirection({ -0.4f, -0.6f, -1.0f });
    renderer.setMaterial(tgx::RGBf(0.8f, 0.4f, 0.2f), 0.15f, 0.75f, 0.25f, 16);
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 angle,
                                 { 0.0f, 1.0f, 0.0f });

    renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    renderer.drawCube();
}
~~~

On an embedded display, the last step is usually an upload of `image` to the screen driver. On a desktop target,
the same image can be displayed with CImg, written to a BMP/PNG file, or compared against a reference image.


@section sec_3D_renderer_template Renderer template parameters

\ref tgx::Renderer3D has three template parameters:

~~~{.cpp}
tgx::Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>
~~~

- `color_t` is the destination color type, usually `tgx::RGB565` on MCU displays and `tgx::RGB24`,
  `tgx::RGB32` or `tgx::RGBf` on CPU.
- `LOADED_SHADERS` is the compile-time list of shader variants that may be used.
- `ZBUFFER_t` is either `float` or `uint16_t`.

The default `LOADED_SHADERS` enables every shader variant. This is convenient while experimenting, but on MCU
targets it costs code size and may reduce speed because more paths are kept alive. Production sketches should
usually keep only the variants they need.

For example, a fast textured mesh renderer using perspective projection, a Z-buffer, Gouraud lighting, nearest
texture sampling and power-of-two texture wrapping can be declared as:

~~~{.cpp}
using Renderer = tgx::Renderer3D<
    tgx::RGB565,
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_WRAP_POW2,
    uint16_t>;
~~~

@warning Runtime shader changes can only select variants that were enabled in `LOADED_SHADERS`. If a draw call
requires a missing variant, rendering may fail silently. This is intentional: the renderer avoids expensive runtime
diagnostics in hot drawing paths.


@section sec_3D_shaders Shader state

The renderer combines several independent shader choices:

- projection: `SHADER_PERSPECTIVE` or `SHADER_ORTHO`;
- depth mode: `SHADER_ZBUFFER` or `SHADER_NOZBUFFER`;
- lighting interpolation: `SHADER_FLAT` or `SHADER_GOURAUD`;
- texture usage: `SHADER_NOTEXTURE` or texture-enabled flags;
- texture sampling: `SHADER_TEXTURE_NEAREST` or `SHADER_TEXTURE_BILINEAR`;
- texture addressing: `SHADER_TEXTURE_WRAP_POW2` or `SHADER_TEXTURE_CLAMP`.

The most common runtime call is:

~~~{.cpp}
renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
~~~

Texture quality and wrapping may also be selected explicitly:

~~~{.cpp}
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
~~~

`SHADER_FLAT` is usually the fastest lighting mode. `SHADER_GOURAUD` interpolates vertex lighting and gives smoother
surfaces, especially on curved meshes. Textured Gouraud rendering is often the best visual compromise for embedded
solid 3D rendering.


@section sec_3D_projection Projection, camera and model transform

TGX uses the usual camera convention: in view space the camera is at the origin, looking toward negative Z, with Y
pointing upward.

Use one of the projection helpers:

~~~{.cpp}
renderer.setPerspective(45.0f, (float)W / H, 0.1f, 100.0f);
renderer.setOrtho(-2.0f, 2.0f, -1.5f, 1.5f, 0.1f, 100.0f);
renderer.setFrustum(left, right, bottom, top, zNear, zFar);
~~~

`setLookAt()` sets the view matrix:

~~~{.cpp}
renderer.setLookAt({ 0.0f, 2.0f, 6.0f },
                   { 0.0f, 0.0f, 0.0f },
                   { 0.0f, 1.0f, 0.0f });
~~~

`setModelMatrix()` gives full control over the model transform. For common object placement, use
`setModelPosScaleRot()`:

~~~{.cpp}
renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f },    // position
                             { 1.0f, 1.0f, 1.0f },    // scale
                             angle,                   // rotation angle in degrees
                             { 0.0f, 1.0f, 0.0f });   // rotation axis
~~~

Custom projection, view and model matrices can also be supplied directly. If a custom projection matrix is used,
make sure the renderer is told whether it is orthographic or perspective by calling `useOrthographicProjection()`
or `usePerspectiveProjection()` afterward.


@section sec_3D_zbuffer Z-buffer

A Z-buffer is required for normal solid rendering when triangles overlap. Its memory footprint is:

- `width * height * 4` bytes for a `float` Z-buffer;
- `width * height * 2` bytes for a `uint16_t` Z-buffer.

`float` gives more depth precision. `uint16_t` is often the best choice on MCU targets because it halves memory
traffic and memory usage.

~~~{.cpp}
uint16_t zbuffer[W * H];
tgx::Renderer3D<tgx::RGB565, MY_SHADERS, uint16_t> renderer({ W, H }, &image, zbuffer);

void drawFrame()
{
    image.clear(tgx::RGB565_Black);
    renderer.clearZbuffer();
    renderer.drawMesh(&mesh);
}
~~~

@warning Clear the Z-buffer at the start of every frame, and also after changing the viewport offset during tiled
rendering. Keeping old depth values is a very common cause of missing or partially erased geometry.


@section sec_3D_meshes Mesh3D and generated models

\ref tgx::Mesh3D is the preferred way to render static geometry. It stores the arrays of vertices, normals, texture
coordinates, face indices, material color and texture pointer. Meshes can also be chained, which is useful for OBJ
files that contain several material groups.

Typical generated mesh usage:

~~~{.cpp}
#include "buddha.h"

renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
renderer.drawMesh(&buddha, true, true);
~~~

The parameters of `drawMesh(mesh, use_mesh_material, draw_chained_meshes)` are:

- `mesh`: the first mesh to render;
- `use_mesh_material`: when true, use the material and texture stored in the mesh;
- `draw_chained_meshes`: when true, also draw linked meshes.

For static meshes on Teensy 4.x, `cacheMesh()` can copy the most frequently accessed data into faster memory:

~~~{.cpp}
const tgx::Mesh3D<tgx::RGB565>* cached =
    tgx::cacheMesh(&mesh, buf_DTCM, DTCM_buf_size, buf_DMAMEM, DMAMEM_buf_size);

renderer.drawMesh(cached);
~~~

Some examples also use `copyMeshEXTMEM()` to move large model data or textures to external memory when available.
This can improve speed compared with reading large textures directly from flash, depending on the board and memory
layout.


@section sec_3D_primitives Drawing primitives directly

The renderer can also draw individual primitives. These calls are useful for dynamic geometry, tests and debugging:

~~~{.cpp}
tgx::fVec3 P1(-1.0f, -1.0f, 0.0f);
tgx::fVec3 P2( 1.0f, -1.0f, 0.0f);
tgx::fVec3 P3( 0.0f,  1.0f, 0.0f);

tgx::fVec3 N(0.0f, 0.0f, 1.0f);

renderer.drawTriangle(P1, P2, P3, &N, &N, &N);
~~~

Available solid primitives include:

- `drawTriangle()`;
- `drawTriangleWithVertexColor()`;
- `drawTriangles()`;
- `drawQuad()`;
- `drawQuadWithVertexColor()`;
- `drawQuads()`;
- `drawCube()`;
- `drawSphere()`;
- `drawAdaptativeSphere()`.

When many triangles or quads share arrays of vertices, normals and texture coordinates, prefer `drawTriangles()` or
`drawQuads()` over many individual calls. For fully static geometry, prefer `drawMesh()`.

Normals must be unit vectors when Gouraud shading is used. Flat shading can compute a face normal from the geometry
when no normal is provided, but supplying correct normals is still recommended for predictable lighting.


@section sec_3D_textures Textures

Textures are regular \ref tgx::Image objects whose color type matches the renderer color type:

~~~{.cpp}
tgx::RGB565 texture_data[128 * 128];
tgx::Image<tgx::RGB565> texture(texture_data, 128, 128);

renderer.setShaders(tgx::SHADER_FLAT | tgx::SHADER_TEXTURE);
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);
~~~

There are two sampling modes:

- `SHADER_TEXTURE_NEAREST`: fastest, pixelated when magnified;
- `SHADER_TEXTURE_BILINEAR`: smoother, slower.

There are two addressing modes:

- `SHADER_TEXTURE_WRAP_POW2`: repeat texture coordinates; fastest, but both texture dimensions must be powers of two;
- `SHADER_TEXTURE_CLAMP`: clamp to the edge; slightly slower, but works with arbitrary texture dimensions.

@note On some MCUs, large textures stored in flash can be much slower to access than geometry. Smaller triangles,
power-of-two wrapping and moving textures to faster memory can make a large difference.


@section sec_3D_lighting Light and material

The built-in lighting model is intentionally compact. It combines a directional light, material color and ambient,
diffuse and specular strengths:

~~~{.cpp}
renderer.setLightDirection({ -0.3f, -0.7f, -1.0f });
renderer.setLightAmbiant(tgx::RGBf(1.0f, 1.0f, 1.0f));
renderer.setLightDiffuse(tgx::RGBf(1.0f, 1.0f, 1.0f));
renderer.setLightSpecular(tgx::RGBf(1.0f, 1.0f, 1.0f));

renderer.setMaterialColor(tgx::RGBf(0.7f, 0.5f, 0.3f));
renderer.setMaterialAmbiantStrength(0.15f);
renderer.setMaterialDiffuseStrength(0.75f);
renderer.setMaterialSpecularStrength(0.25f);
renderer.setMaterialSpecularExponent(16);
~~~

The convenience method `setLight()` sets all light colors and the direction at once. The convenience method
`setMaterial()` sets all material parameters at once.

If `drawMesh()` is called with `use_mesh_material = true`, the mesh material color and texture override the current
material settings for that mesh. This is the usual mode for generated OBJ models.


@section sec_3D_culling Back-face culling

Back-face culling removes triangles that face away from the camera. This is often a large speed win on closed solid
meshes.

~~~{.cpp}
renderer.setCulling(1);   // keep one winding order
renderer.setCulling(-1);  // keep the opposite winding order
renderer.setCulling(0);   // disable culling
~~~

The correct sign depends on the winding order of the model data after projection. If a mesh disappears completely,
try the opposite sign or disable culling while debugging.


@section sec_3D_tiling Tile rendering

The image can be smaller than the virtual viewport. This is useful when the full framebuffer and Z-buffer do not fit
in memory.

~~~{.cpp}
renderer.setViewportSize(320, 240);
renderer.setImage(&tileImage);       // for example 160 x 120

for (int oy = 0; oy < 240; oy += 120) {
    for (int ox = 0; ox < 320; ox += 160) {
        renderer.setOffset(ox, oy);
        tileImage.clear(tgx::RGB565_Black);
        renderer.clearZbuffer();
        renderer.drawMesh(&mesh);
        // upload tileImage at screen position (ox, oy)
    }
}
~~~

The projection and viewport remain the same for every tile. Only the image offset changes.


@section sec_3D_wireframe Wireframe and debug drawing

The renderer also contains wireframe, dot and normal-visualization methods. They are useful for inspecting geometry
and debugging transforms. They are not the main optimized path of the 3D engine, and high-quality wireframe drawing
can be much slower than solid rendering.

For performance-sensitive rendering, prefer the solid mesh path first:

~~~{.cpp}
renderer.drawMesh(&mesh);
~~~


@section sec_3D_performance Embedded performance checklist

For MCU targets, the largest wins usually come from these choices:

- restrict `LOADED_SHADERS` to the variants the program really uses;
- use `RGB565` for display rendering;
- use a `uint16_t` Z-buffer when depth precision is sufficient;
- use `drawMesh()` and `cacheMesh()` for static models;
- keep normals normalized and mesh data well formed;
- enable back-face culling for closed meshes;
- prefer `SHADER_TEXTURE_NEAREST` and `SHADER_TEXTURE_WRAP_POW2` when quality allows it;
- keep textures in faster memory when possible;
- split very large textured faces if texture cache locality is poor;
- clear the image and Z-buffer once per frame, not before every object;
- avoid drawing debug wireframe on top of every frame unless it is needed.


@section sec_3D_examples Useful examples

The repository contains several examples that exercise different parts of the 3D API:

- `examples/CPU/buddhaOnCPU/`: CPU rendering into an image using CImg for display.
- `examples/Teensy4/3D/buddha/`: shaded mesh rendering and mesh caching on Teensy 4.x.
- `examples/Teensy4/3D/borg_cube/`: dynamic texture generation and textured cube rendering.
- `examples/Teensy4/3D/test-shading/`: flat and Gouraud shading comparisons on several meshes.
- `examples/Teensy4/3D/test-texture/`: textured mesh rendering.
- `examples/Teensy4/3D/scream/`: dynamic textured surface built from quads.
- `examples/Teensy4/3D/characters/`: larger textured character models and chained meshes.
- `examples/Teensy4/3D/mars/`: a more complete scene with skybox-like rendering and textured objects.
- `examples/ESP32/naruto/`: ESP32 textured mesh rendering.
- `examples/Pico_RP2040_RP2350/bunny_fig/`: RP2040/RP2350 3D example.


@section sec_3D_pitfalls Common pitfalls

- Nothing is drawn: check that `setImage()`, `setViewportSize()` and the shader flags are valid.
- Geometry appears behind other objects incorrectly: clear the Z-buffer at the start of the frame.
- A mesh disappears when culling is enabled: reverse the culling sign or verify face winding.
- Textured rendering silently falls back or disappears: check that texture shader variants were enabled in
  `LOADED_SHADERS`.
- Wrapped textures look wrong: `SHADER_TEXTURE_WRAP_POW2` requires power-of-two texture dimensions.
- Gouraud lighting looks strange: verify that normals are normalized and match the model transform.
- Orthographic and perspective views do not match: compare the visible height at the object depth and keep the same
  camera/view matrix while switching projection.

