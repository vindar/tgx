@page intro_api_3D Using the 3D API.


**This page introduces the TGX 3D renderer and shows the usual way to render solid 3D objects onto a `tgx::Image`.**

- For the complete list of methods, see the \ref tgx::Renderer3D class documentation.
- For the legacy mesh container, see \ref tgx::Mesh3D.
- For the current compact meshlet container, see \ref tgx::Mesh3Dv2.
- For ready-to-run code, see the 3D examples in the `/examples/` directory.

The 3D engine is designed for small embedded targets. It does not try to be a full scene graph or a desktop GPU API.
Instead, it gives direct control over a software rasterizer that draws triangles, quads, meshes, cubes and spheres
into an image buffer. That image can then be displayed on a screen, copied to a DMA buffer, saved on a desktop target,
or used as a texture by other TGX drawing code.

If you are new to 3D rendering, the most important idea is that TGX always draws triangles into a normal 2D
\ref tgx::Image. A 3D model is transformed by matrices, clipped to the camera view, projected to screen coordinates,
and finally rasterized as pixels. TGX exposes these steps because embedded programs often need direct control over
memory, shader variants and the exact cost of each draw call.


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
const tgx::Shader shaders_loaded =
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_FLAT |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_NOTEXTURE |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_WRAP_POW2;

using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t>;
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
the same image can be displayed in a window, written to a BMP/PNG file, or compared against a reference image.

The most useful `Renderer3D` methods are grouped below:

| Group | Purpose | Typical methods |
|-------|---------|-----------------|
| Target and viewport | Select the destination image, Z-buffer and virtual viewport. | `setImage()`, `setViewportSize()`, `setOffset()`, `setZbuffer()`, `clearZbuffer()` |
| Camera and projection | Define what part of the 3D world is visible. | `setPerspective()`, `setOrtho()`, `setFrustum()`, `setLookAt()`, `setViewMatrix()` |
| Object transform | Place the object currently being drawn. | `setModelMatrix()`, `setModelPosScaleRot()`, `modelToNDC()`, `modelToImage()` |
| Shader state | Select the compiled rendering path used by subsequent draw calls. | `setShaders()`, `setTextureQuality()`, `setTextureWrappingMode()` |
| Lighting and material | Control directional light, object color and specular highlights. | `setLightDirection()`, `setLight()`, `setMaterial()`, `setMaterialColor()` |
| Solid drawing | Draw optimized solid geometry. | `drawMesh()`, `drawTriangle()`, `drawQuad()`, `drawCube()`, `drawSphere()` |
| Debug drawing | Inspect geometry or transforms. | `drawWireFrameMesh()`, `drawWireFrameTriangle()`, `drawPixel()`, `drawDot()` |


@section sec_3D_pipeline Coordinate spaces and matrices

TGX follows the same conceptual pipeline as a small OpenGL-style renderer, but it exposes the steps directly so the
application can keep control over CPU time and memory:

1. **Model space**: coordinates stored in the mesh or passed to `drawTriangle()`, `drawCube()` and similar methods.
2. **World/view space**: the model matrix places the object, then the view matrix places the camera.
3. **Clip space**: the projection matrix maps the view frustum to homogeneous coordinates.
4. **NDC**: perspective projection divides by `w`, giving normalized coordinates in roughly `[-1, 1]`.
5. **Framebuffer space**: the rasterizer maps NDC to pixels in the target \ref tgx::Image.

![3d_coordinate_spaces](../3d_coordinate_spaces.svg)

TGX uses a right-handed convention for the 3D spaces used by the renderer. In view space, the camera is at the origin
and looks toward negative Z. Image coordinates are different: once projected, pixels use the usual TGX image convention
with `(0, 0)` at the upper-left corner, X increasing to the right and Y increasing downward.

The important point is that the mesh data itself is not modified. Each frame, TGX transforms the original vertices
through the current matrices and rasterizes the projected triangles:

![3d_pipeline](../3d_pipeline.svg)

The combined transform used by the renderer is:

~~~{.cpp}
clip_position = projection_matrix * view_matrix * model_matrix * model_position;
~~~

The most common calls are:

~~~{.cpp}
renderer.setPerspective(fovy_degrees, aspect, zNear, zFar);
renderer.setLookAt(eye, center, up);
renderer.setModelPosScaleRot(position, scale, angle_degrees, axis);
~~~

`setLookAt()` builds the view matrix. The camera is located at `eye`, looks toward `center`, and uses `up` to define
which direction should appear vertical on screen. In view space, the camera looks along the negative Z direction.

The projection can be perspective or orthographic:

- **Perspective** makes distant objects smaller. It is the usual choice for 3D scenes.
- **Orthographic** keeps object size independent of distance. It is useful for CAD-like views, sprites in 3D space
  or debugging a model without perspective distortion.

Use `modelToNDC()` when debugging transforms. It returns the projected position of a model-space point after the
current model, view and projection matrices have been applied.


@subsection sec_3D_vectors_matrices Short math reminder

A 3D point is usually written as `(x, y, z)`. Internally, transformations use homogeneous coordinates `(x, y, z, w)`.
For ordinary points, `w = 1`. Directions such as normals or light directions behave like vectors and do not represent
a position in space.

The three operations used most often are:

- **dot product**: `dot(a, b)` measures how aligned two directions are. Lighting and back-face tests use it heavily.
- **cross product**: `cross(a, b)` creates a direction perpendicular to two other directions. It is useful for normals.
- **normalization**: `normalize(v)` scales a vector to length 1. Normals and light directions should be normalized.

Matrices are applied from right to left in the usual expression:

~~~{.cpp}
P_clip = projection * view * model * P_model;
~~~

So `model` is applied first, then `view`, then `projection`. If the object moves when the camera should move, or the
camera seems to orbit the object in the wrong direction, the problem is often an inverted transform or a wrong matrix
order.

Normals describe surface orientation. When non-uniform scaling is used, normals should theoretically be transformed by
the inverse-transpose of the model matrix. TGX keeps the runtime small and assumes typical embedded use: normalized
mesh normals, mostly uniform scales, and explicit care from the sketch when using unusual transforms.


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
const tgx::Shader shaders_loaded =
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_WRAP_POW2;

using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t>;
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

These flags are combined in two places: first in the renderer template parameter, to decide which code paths are
compiled, and then at runtime with `setShaders()` and the texture setters, to choose which compiled path is active.

| Shader choice | Meaning | Typical use |
|---------------|---------|-------------|
| `SHADER_PERSPECTIVE` | Use perspective projection with division by `w`. | Normal 3D scenes. |
| `SHADER_ORTHO` | Use orthographic projection without perspective shrinking. | CAD-like views, debug views, 3D sprites. |
| `SHADER_ZBUFFER` | Use depth testing. | Solid objects that can overlap. |
| `SHADER_NOZBUFFER` | Draw without depth testing. | Ordered overlays or special effects. |
| `SHADER_FLAT` | One lighting result per triangle. | Fast faceted rendering. |
| `SHADER_GOURAUD` | Lighting at vertices, interpolated across triangles. | Smoother curved meshes. |
| `SHADER_NOTEXTURE` | Use material or vertex colors only. | Untextured models and debug views. |
| `SHADER_TEXTURE_NEAREST` | Nearest-neighbor texture lookup. | Fast textured rendering. |
| `SHADER_TEXTURE_BILINEAR` | Bilinear texture filtering. | Smoother textures when speed allows. |
| `SHADER_TEXTURE_WRAP_POW2` | Repeat power-of-two textures. | Fast tiling textures. |
| `SHADER_TEXTURE_CLAMP` | Clamp texture coordinates to the edge. | Non-power-of-two textures or non-repeating images. |

The most common runtime call is:

~~~{.cpp}
renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
~~~

Think of `LOADED_SHADERS` as the list of compiled code paths, and `setShaders()` as the currently selected path:

~~~{.cpp}
// Compiled once, at build time.
const tgx::Shader shaders_loaded =
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_FLAT |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_NOTEXTURE |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_BILINEAR |
    tgx::SHADER_TEXTURE_WRAP_POW2;

tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t> renderer({ W, H }, &image, zbuffer);

// Selected at runtime, before drawing.
renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
~~~

If a sketch only uses flat untextured geometry, do not load bilinear texture shaders. If it switches between a flat
debug view and a textured view, both `SHADER_FLAT` and the texture sampling mode must be present in `shaders_loaded`.

Texture quality and wrapping may also be selected explicitly:

~~~{.cpp}
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
~~~

`SHADER_FLAT` is usually the fastest lighting mode. `SHADER_GOURAUD` interpolates vertex lighting and gives smoother
surfaces, especially on curved meshes. Textured Gouraud rendering is often the best visual compromise for embedded
solid 3D rendering.

Internally, `Renderer3D` dispatches to templated shader variants. `LOADED_SHADERS` tells the compiler which variants
must exist in the binary. Runtime calls such as `setShaders()` then select among these already compiled variants.
Keeping `LOADED_SHADERS` narrow saves flash and helps the compiler remove unused branches. This is important on MCU
targets where code size and instruction cache behavior affect frame time.


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

The frustum is the visible volume. In perspective projection it looks like a truncated pyramid:

![3d_projection](../3d_projection.svg)

`zNear` should be positive and not too close to zero. A very small near plane reduces depth precision and can make
Z-buffer artifacts more visible. `zFar` should be far enough for the scene, but not arbitrarily huge for the same
reason.


@section sec_3D_zbuffer Z-buffer

A Z-buffer is required for normal solid rendering when triangles overlap. Its memory footprint is:

- `width * height * 4` bytes for a `float` Z-buffer;
- `width * height * 2` bytes for a `uint16_t` Z-buffer.

`float` gives more depth precision. `uint16_t` is often the best choice on MCU targets because it halves memory
traffic and memory usage.

~~~{.cpp}
uint16_t zbuffer[W * H];
tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t> renderer({ W, H }, &image, zbuffer);

void drawFrame()
{
    image.clear(tgx::RGB565_Black);
    renderer.clearZbuffer();
    renderer.drawMesh(&mesh);
}
~~~

@warning Clear the Z-buffer at the start of every frame, and also after changing the viewport offset during tiled
rendering. Keeping old depth values is a very common cause of missing or partially erased geometry.


@section sec_3D_meshes Mesh3Dv2, Mesh3D and generated models

Most real 3D objects are stored as meshes. A mesh is made of triangles, and each triangle refers to:

- **positions**: the 3D coordinates of its vertices;
- **normals**: directions perpendicular to the surface, used for lighting;
- **texture coordinates**: `(u, v)` coordinates telling which part of a texture image maps to each vertex;
- **material information**: color, lighting coefficients and optional texture image.

TGX can draw individual triangles directly, but static models should usually be converted to a mesh format so that
the renderer can reuse data, cull invisible parts and reduce per-frame overhead.

\ref tgx::Mesh3Dv2 is the preferred format for new static models. It stores compact 16-bit meshlet payloads, material
records, optional textures and precomputed meshlet visibility data. Compared with legacy \ref tgx::Mesh3D, it usually
reduces memory traffic and can discard invisible meshlets before decoding their triangles.

At a high level, a `Mesh3Dv2` model contains:

- one material table: colors, lighting strengths and optional texture pointers;
- one meshlet table: small local groups of triangles, each attached to one material;
- one compact payload: quantized vertices, normals, UVs and triangle chains for the meshlets;
- visibility information used to skip meshlets that cannot contribute to the current view.

This organization is friendlier to MCU caches than a large global index soup. A meshlet that is not visible can be
discarded before its local payload is decoded, and a visible meshlet works mostly on a small local set of data.

Legacy \ref tgx::Mesh3D is still supported for existing projects. It stores global arrays of vertices, normals,
texture coordinates and chained triangle strips. Existing sketches do not need to be converted, but new examples and
new converted models should generally use `Mesh3Dv2`.

Typical generated mesh usage:

~~~{.cpp}
#include "buddha.h"

renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
renderer.drawMesh(&buddha);
~~~

For `Mesh3Dv2`, `drawMesh(mesh, use_mesh_material)` renders all meshlets in the model:

- `mesh`: the model to render;
- `use_mesh_material`: when true, use the material colors, lighting strengths and texture pointers stored in the mesh.

For legacy `Mesh3D`, `drawMesh(mesh, use_mesh_material, draw_chained_meshes)` also has:

- `draw_chained_meshes`: when true, also draw linked meshes.

For static meshes on Teensy 4.x, `cacheMesh()` can copy the most frequently accessed data into faster memory. With
`Mesh3Dv2`, the cache order string controls which parts are copied first:

- `M`: material table;
- `I`: meshlet table;
- `P`: meshlet payload;
- `L`: texture image pixels.

~~~{.cpp}
const tgx::Mesh3Dv2<tgx::RGB565>* cached =
    tgx::cacheMesh(&mesh, buf_DTCM, DTCM_buf_size, buf_DMAMEM, DMAMEM_buf_size, "LMPI");

renderer.drawMesh(cached);
~~~

Some examples also use `copyMeshEXTMEM()` to move large model data or textures to external memory when available.
This can improve speed compared with reading large textures directly from flash, depending on the board and memory
layout.

Use the \ref tools_mesh "mesh tools" to generate `Mesh3Dv2` headers from Wavefront OBJ files or to migrate existing
legacy `Mesh3D` headers.


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

| Method | Use |
|--------|-----|
| `drawTriangle()` | Draw one triangle with optional normals, texture coordinates and texture. |
| `drawTriangleWithVertexColor()` | Draw one triangle with explicit per-vertex colors. |
| `drawTriangles()` | Draw an indexed array of triangles sharing vertex, normal and texture-coordinate arrays. |
| `drawQuad()` | Draw one quad, internally split into two triangles. |
| `drawQuadWithVertexColor()` | Draw one quad with explicit per-vertex colors. |
| `drawQuads()` | Draw an indexed array of quads. |
| `drawCube()` | Draw the unit cube, optionally textured per face. |
| `drawSphere()` | Draw a generated sphere with a chosen tessellation. |
| `drawAdaptativeSphere()` | Draw a generated sphere with tessellation chosen from its projected size. |

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

Texture coordinates are usually called `(u, v)`. They are independent from pixel coordinates:

- `(0, 0)` usually means one corner of the texture;
- `(1, 1)` means the opposite corner;
- values outside `[0, 1]` can either repeat the texture or clamp to its edge, depending on the wrapping mode.

With `SHADER_TEXTURE_WRAP_POW2`, TGX can wrap very quickly, but the texture width and height must be powers of two.
With `SHADER_TEXTURE_CLAMP`, TGX accepts arbitrary texture sizes, but each lookup needs slightly more work.


@section sec_3D_lighting Light and material

The built-in lighting model is intentionally compact. It is a Phong-style lighting model evaluated per vertex for
Gouraud shading, or once per face for flat shading. It combines a directional light, material color and ambient,
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

Flat and Gouraud shading differ in where lighting is evaluated:

- **Flat shading** computes one color for the whole triangle. It is fast and gives a faceted look.
- **Gouraud shading** computes lighting at vertices and interpolates the resulting colors across the triangle. It is
  smoother on curved models, but costs more math and interpolation.

TGX does not implement per-pixel Phong normal interpolation; that would be much more expensive on the intended MCU
targets.

For a directional light, the diffuse part is controlled mostly by:

~~~{.cpp}
diffuse = max(0, dot(normal, -light_direction));
~~~

When the normal points toward the light, the dot product is close to 1 and the surface is bright. When it points away,
the value is 0 and only ambient or specular terms may remain. This is why wrong or non-normalized normals can make a
mesh look blotchy, dark or overbright.


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
- use `Mesh3Dv2`, `drawMesh()` and `cacheMesh()` for static models;
- keep normals normalized and mesh data well formed;
- enable back-face culling for closed meshes;
- prefer `SHADER_TEXTURE_NEAREST` and `SHADER_TEXTURE_WRAP_POW2` when quality allows it;
- keep textures in faster memory when possible;
- split very large textured faces if texture cache locality is poor;
- clear the image and Z-buffer once per frame, not before every object;
- avoid drawing debug wireframe on top of every frame unless it is needed.


@section sec_3D_complete_example Complete embedded example

The following sketch skeleton shows the usual structure for a textured `Mesh3Dv2` model on an MCU framebuffer. It
omits the display-driver upload, because that part depends on the screen library.

~~~{.cpp}
#include <tgx.h>
#include "my_model.h"     // generated Mesh3Dv2<RGB565> and texture headers

constexpr int W = 320;
constexpr int H = 240;

DMAMEM tgx::RGB565 framebuffer[W * H];
DMAMEM uint16_t zbuffer[W * H];

tgx::Image<tgx::RGB565> image(framebuffer, W, H);

const tgx::Shader shaders_loaded =
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_FLAT |
    tgx::SHADER_GOURAUD |
    tgx::SHADER_TEXTURE_NEAREST |
    tgx::SHADER_TEXTURE_BILINEAR |
    tgx::SHADER_TEXTURE_WRAP_POW2;

tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t> renderer({ W, H }, &image, zbuffer);

void setup3D()
{
    renderer.setPerspective(45.0f, float(W) / float(H), 0.1f, 100.0f);
    renderer.setLookAt({ 0.0f, 1.0f, 5.0f }, { 0.0f, 0.4f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    renderer.setLightDirection({ -0.35f, -0.55f, -1.0f });
    renderer.setCulling(1);
    renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
}

void drawFrame(float angle)
{
    image.clear(tgx::RGB565_Black);
    renderer.clearZbuffer();

    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 angle,
                                 { 0.0f, 1.0f, 0.0f });

    renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
    renderer.drawMesh(&my_model, true);

    // Upload image to the display here.
}
~~~


@section sec_3D_examples Useful examples

The repository contains several examples that exercise different parts of the 3D API:

- `examples/CPU/buddhaOnCPU/`: CPU rendering into an image and displaying the result in a small window.
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
