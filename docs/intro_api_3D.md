@page intro_api_3D Using the 3D API

**This page introduces the TGX 3D renderer and explains the normal path from a 3D object to pixels in a `tgx::Image`.**

- For the complete method reference, see the \ref tgx::Renderer3D class documentation.
- For the current compact meshlet container, see \ref tgx::Mesh3Dv2.
- For the legacy mesh container, see \ref tgx::Mesh3D.
- For ready-to-run code, see the 3D examples in `/examples/`.
- The previous version of this tutorial is temporarily kept at \ref intro_api_3D_legacy.

TGX is a small software rasterizer for embedded systems. It is not a scene graph and it does not own the display
driver. A sketch gives TGX a framebuffer, a Z-buffer if needed, a camera, a model transform, a shader state, material
and light state, then calls drawing methods. The sketch remains responsible for clearing buffers and uploading pixels
to the screen.

The tutorial is organized in the same order:

1. create a renderer and draw one frame;
2. choose where pixels and depth values are stored;
3. understand how coordinates become pixels;
4. choose the compiled and runtime rendering paths;
5. provide geometry;
6. choose textures, materials and lights;
7. use debug/wireframe helpers and performance rules.

![buddha](../buddha.png)

*The `buddha` mesh rendered by TGX's 3D engine. See `examples/CPU/buddhaOnCPU/`.*


@section sec_3D_overview Renderer3D Overview

The main 3D class is \ref tgx::Renderer3D. It transforms vertices, clips triangles, projects them to a 2D viewport
and writes pixels into a destination \ref tgx::Image.

`Renderer3D` is immediate-mode: it does not store a list of objects. It stores the current render state. You set that
state, draw one object, then change the state and draw the next object.

| State | What it controls | Main methods |
|-------|------------------|--------------|
| Destination | Destination image, viewport and tile offset. | \ref tgx::Renderer3D::setImage "setImage()", \ref tgx::Renderer3D::setViewportSize "setViewportSize()", \ref tgx::Renderer3D::setOffset "setOffset()" |
| Depth buffer | Optional per-pixel depth testing. | \ref tgx::Renderer3D::setZbuffer "setZbuffer()", \ref tgx::Renderer3D::clearZbuffer "clearZbuffer()" |
| Camera lens | Perspective, frustum or orthographic projection. | \ref tgx::Renderer3D::setPerspective "setPerspective()", \ref tgx::Renderer3D::setFrustum "setFrustum()", \ref tgx::Renderer3D::setOrtho "setOrtho()" |
| Camera pose | Where the camera is and where it looks. | \ref tgx::Renderer3D::setLookAt "setLookAt()", \ref tgx::Renderer3D::setViewMatrix "setViewMatrix()" |
| Object pose | Placement, scale and rotation of the object being drawn. | \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()", \ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" |
| Rendering path | Shading mode, texturing, Z-buffer use and texture sampling. | \ref tgx::Renderer3D::setShaders "setShaders()", \ref tgx::Renderer3D::setTextureQuality "setTextureQuality()", \ref tgx::Renderer3D::setTextureWrappingMode "setTextureWrappingMode()" |
| Appearance | Material, texture and lights. | \ref tgx::Renderer3D::setMaterial "setMaterial()", \ref tgx::Renderer3D::setLight "setLight()", \ref tgx::Renderer3D::setSpotLight "setSpotLight()" |
| Visibility/debug | Back-face culling, wireframe and point helpers. | \ref tgx::Renderer3D::setCulling "setCulling()", \ref tgx::Renderer3D::drawWireFrameMesh "drawWireFrameMesh()", \ref tgx::Renderer3D::drawDots "drawDots()" |

It is useful to keep ownership clear:

| Owner | Stores |
|-------|--------|
| The sketch | Framebuffer memory, Z-buffer memory, display upload and frame timing. |
| The renderer | Current camera, projection, model matrix, shader state, material, texture and light state. |
| Mesh data | Vertices, normals, texture coordinates, material tables and texture references. |
| A draw call | Consumes the current renderer state and the geometry passed to that call. |

Renderer state persists until it is changed. If two objects need different materials, shaders or model matrices, set
that state before each corresponding draw call.


@section sec_3D_minimal_loop Minimal Render Loop

A minimal frame can look like this:

~~~{.cpp}
#include <tgx.h>

constexpr int W = 320;
constexpr int H = 240;

tgx::RGB565 framebuffer[W * H];
uint16_t zbuffer[W * H];
tgx::Image<tgx::RGB565> image(framebuffer, W, H);

// Compile only the shader variants this renderer may use.
const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                                   tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE;

using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t>;
Renderer renderer({ W, H }, &image, zbuffer);

void drawFrame(float angle)
{
    image.clear(tgx::RGB565_Black);
    renderer.clearZbuffer();

    renderer.setPerspective(45.0f, (float)W / H, 0.1f, 100.0f);
    renderer.setLookAt({ 0.0f, 0.0f, 6.0f },
                       { 0.0f, 0.0f, 0.0f },
                       { 0.0f, 1.0f, 0.0f });

    renderer.setLightDirection({ -0.4f, -0.6f, -1.0f });
    renderer.setMaterial(tgx::RGBf(0.8f, 0.4f, 0.2f), 0.15f, 0.75f, 0.25f, 16);

    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 angle,
                                 { 0.0f, 1.0f, 0.0f });

    renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    renderer.drawCube();

    // Upload image to the display here.
}
~~~

In larger sketches, keep this separation:

| Moment | Typical work |
|--------|--------------|
| Initialization | Allocate buffers, create the image, create the renderer, choose compiled shader variants. |
| Beginning of a frame | Clear the image and Z-buffer, update camera/projection if needed. |
| For each object | Set model matrix, shader, material and texture state, then draw. |
| End of frame | Upload the image, display it on CPU, or save it. |


@section sec_3D_buffers Destination Image, Viewport and Depth Buffer

TGX draws into an existing \ref tgx::Image. The sketch decides where that image memory lives:

~~~{.cpp}
tgx::RGB565 framebuffer[W * H];
tgx::Image<tgx::RGB565> image(framebuffer, W, H);
renderer.setImage(&image);
~~~

The image is the memory that receives pixels. The viewport is the virtual screen size used by projection and pixel
mapping. In the simplest case they have the same size:

~~~{.cpp}
renderer.setViewportSize(W, H);
renderer.setOffset(0, 0);
~~~

Solid rendering usually also uses a Z-buffer:

~~~{.cpp}
uint16_t zbuffer[W * H];
renderer.setZbuffer(zbuffer);
renderer.clearZbuffer();
~~~

`ZBUFFER_t` can be `float` or `uint16_t`:

| Type | Cost | Notes |
|------|------|-------|
| `float` | 4 bytes per pixel | Better precision. |
| `uint16_t` | 2 bytes per pixel | Saves memory; can show more z-fighting. |

Clear the image and Z-buffer once at the beginning of each frame, not before every object.

To render without a Z-buffer, compile and select the `SHADER_NOZBUFFER` path. `SHADER_NOZBUFFER` is a real shader
variant and must be present in `LOADED_SHADERS` if the sketch uses it.


@subsection sec_3D_tiling Tile Rendering

The image can be smaller than the virtual viewport. This lets you render a large viewport in several tiles when a full
framebuffer and Z-buffer do not fit in memory. The projection and viewport describe the final screen; `setOffset()`
selects which part of that screen the current image represents:

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


@section sec_3D_coordinates From 3D Coordinates to Pixels

3D rendering is mostly about moving points through coordinate spaces. A mesh vertex is not written directly to the
framebuffer; it is transformed, clipped, projected and finally converted to pixels.

![3d_coordinate_spaces](../3d_coordinate_spaces.svg)

| Space | Meaning in TGX | Example |
|-------|----------------|---------|
| Model space | Coordinates stored in a mesh or passed to primitive drawing functions. | A unit cube centered at the origin. |
| World space | Scene coordinates after the model matrix places the object. | Several objects positioned relative to each other. |
| View space | Camera coordinates after the view matrix. | The camera is at the origin and looks along negative Z. |
| Clip space | Homogeneous coordinates after projection, before perspective divide. | Clipping happens here. |
| NDC | Normalized device coordinates after perspective division. | Visible x/y coordinates are roughly in `[-1, 1]`. |
| Image space | Destination pixel coordinates. | `(0,0)` is the upper-left pixel. |

The full pipeline is:

![3d_pipeline](../3d_pipeline.svg)

TGX uses a right-handed camera convention in view space: the camera is at the origin, looking toward negative Z, with
Y pointing upward. In image space, TGX uses the normal \ref tgx::Image convention: X goes right and Y goes down.


@subsection sec_3D_math Math Types, Points and Normals

TGX uses small value types for 3D math:

| Type | Purpose |
|------|---------|
| `tgx::fVec3` | A 3D point or vector using `float` coordinates. |
| `tgx::fVec4` | A homogeneous 4D vector, mostly useful around projection and clipping. |
| `tgx::fMat4` | A 4x4 floating-point matrix for transforms and projections. |
| `tgx::fBox3` | An axis-aligned 3D bounding box. |

A point and a direction both use `(x,y,z)`, but they are not transformed the same way. Homogeneous coordinates encode
the difference with `w`:

| Quantity | Homogeneous form | Matrix helper |
|----------|------------------|---------------|
| Point | `(x, y, z, 1)` | `fMat4::mult1()` |
| Direction/vector | `(x, y, z, 0)` | `fMat4::mult0()` |
| Explicit 4D vector | `(x, y, z, w)` | `fMat4::mult()` |

Use `mult1()` for positions and `mult0()` for directions. This matters for normals, light directions and camera axes.

Common vector operations:

| Operation | TGX method/function | Used for |
|-----------|---------------------|----------|
| Length | \ref tgx::Vec3::norm2 "norm2()", \ref tgx::Vec3::norm "norm()", \ref tgx::Vec3::norm_fast "norm_fast()" | Distances and normalization. |
| Inverse length | \ref tgx::Vec3::invnorm "invnorm()", \ref tgx::Vec3::invnorm_fast "invnorm_fast()" | Fast normalization. |
| Normalize | \ref tgx::Vec3::normalize "normalize()", \ref tgx::Vec3::normalize_fast "normalize_fast()" | Unit normals, light directions and camera vectors. |
| Dot product | `dotProduct(a,b)` | Lighting, angle tests and back-face tests. |
| Cross product | `crossProduct(a,b)` | Face normals and perpendicular axes. |

Normals describe surface orientation. For lighting and culling, normals and light directions should have length 1.
Non-normalized normals usually produce dark, overbright or unstable lighting. Non-uniform model scale is a special
case because normals cannot be transformed exactly like ordinary points; TGX keeps the runtime small and works best
with normalized mesh normals and mostly uniform scales.


@subsection sec_3D_model_view_projection Model, View and Projection

`Renderer3D` stores three main matrices:

| Matrix | Maps from | Maps to | Main methods |
|--------|-----------|---------|--------------|
| Model | Model space | World space | \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()", \ref tgx::Renderer3D::getModelMatrix "getModelMatrix()", \ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" |
| View | World space | View space | \ref tgx::Renderer3D::setViewMatrix "setViewMatrix()", \ref tgx::Renderer3D::getViewMatrix "getViewMatrix()", \ref tgx::Renderer3D::setLookAt "setLookAt()" |
| Projection | View space | Clip space | \ref tgx::Renderer3D::setProjectionMatrix "setProjectionMatrix()", \ref tgx::Renderer3D::getProjectionMatrix "getProjectionMatrix()", \ref tgx::Renderer3D::setPerspective "setPerspective()", \ref tgx::Renderer3D::setFrustum "setFrustum()", \ref tgx::Renderer3D::setOrtho "setOrtho()" |

The matrices are applied from right to left:

~~~{.cpp}
P_clip = projection * view * model * P_model;
~~~

In a normal render loop:

~~~{.cpp}
// Choose the camera lens.
renderer.setPerspective(fovy_degrees, aspect, zNear, zFar);

// Place the camera in the world.
renderer.setLookAt(eye, center, up);

// Place the current object in the world.
renderer.setModelPosScaleRot(position, scale, angle_degrees, axis);
~~~

\ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" is the convenient form for most objects. Use
\ref tgx::Renderer3D::setModelMatrix "setModelMatrix()" when you already have a full matrix.

`setLookAt()` builds the view matrix:

~~~{.cpp}
renderer.setLookAt({ 0.0f, 2.0f, 6.0f },    // eye: camera position in world space
                   { 0.0f, 0.0f, 0.0f },    // center: point being looked at
                   { 0.0f, 1.0f, 0.0f });   // up: screen vertical direction
~~~

The projection matrix acts like the lens. It controls what is visible and how depth changes apparent size:

| Projection | TGX call | Visual effect |
|------------|----------|---------------|
| Perspective | \ref tgx::Renderer3D::setPerspective "setPerspective(fovy, aspect, zNear, zFar)" | Distant objects become smaller. |
| Frustum | \ref tgx::Renderer3D::setFrustum "setFrustum(left, right, bottom, top, zNear, zFar)" | Explicit perspective volume. |
| Orthographic | \ref tgx::Renderer3D::setOrtho "setOrtho(left, right, bottom, top, zNear, zFar)" | Object size does not depend on distance. |
| Custom | \ref tgx::Renderer3D::setProjectionMatrix "setProjectionMatrix(M)" | Use your own projection matrix. |

After `setProjectionMatrix()`, call \ref tgx::Renderer3D::usePerspectiveProjection "usePerspectiveProjection()" or
\ref tgx::Renderer3D::useOrthographicProjection "useOrthographicProjection()" if the renderer cannot know which mode
your custom matrix represents. The standard `setPerspective()`, `setFrustum()` and `setOrtho()` helpers do this
automatically.

Changing the model matrix affects the next object. Changing the view matrix moves the camera. Changing the projection
matrix changes the lens.

When debugging transforms, these methods are useful:

| Method | Converts |
|--------|----------|
| \ref tgx::Renderer3D::modelToNDC "modelToNDC(P)" | A model-space point to normalized device coordinates. |
| \ref tgx::Renderer3D::modelToImage "modelToImage(P)" | A model-space point to image pixels. |
| \ref tgx::Renderer3D::worldToNDC "worldToNDC(P)" | A world-space point to normalized device coordinates. |
| \ref tgx::Renderer3D::worldToImage "worldToImage(P)" | A world-space point to image pixels. |


@subsection sec_3D_projection_viewport Projection and Viewport Mapping

Perspective projection uses a visible volume called a frustum, a truncated pyramid:

![3d_projection](../3d_projection.svg)

For perspective projection, `fovy` is the vertical field of view in degrees and `aspect` is normally
`viewport_width / viewport_height`. If the aspect ratio is wrong, objects look stretched.

`zNear` must be positive and should not be too close to zero. A very small near plane reduces depth precision and can
make Z-buffer artifacts more visible. `zFar` should be large enough for the scene, but not much larger than needed.

After projection and perspective division, TGX maps NDC coordinates to the virtual viewport described in
\ref sec_3D_buffers "Destination Image, Viewport and Depth Buffer". In a full-frame render, the viewport and image have
the same size. In a tiled render, the projection is still computed for the full viewport, and only the image offset
changes from tile to tile.


@section sec_3D_render_paths Compile-time and Runtime Rendering Paths

TGX separates two questions that are easy to confuse:

| Question | Where it is answered | Example |
|----------|----------------------|---------|
| Which code paths exist in this binary? | `LOADED_SHADERS`, a template parameter. | Compile Gouraud + textured + Z-buffer paths. |
| Which path is active for the next draw call? | Runtime state, mainly `setShaders()`. | Draw this object as Gouraud textured. |


@subsection sec_3D_template Renderer Template Parameters

The renderer template is:

~~~{.cpp}
Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t, MAX_DIRECTIONAL_LIGHTS, MAX_SPOT_LIGHTS>
~~~

| Parameter | Meaning | Default |
|-----------|---------|---------|
| `color_t` | Pixel color type, usually `RGB565` for embedded displays. | required |
| `LOADED_SHADERS` | All shader variants that may be used by this renderer. | required |
| `ZBUFFER_t` | Z-buffer storage type, `float` or `uint16_t`. | `uint16_t` |
| `MAX_DIRECTIONAL_LIGHTS` | Compile-time capacity for directional lights. | `1` |
| `MAX_SPOT_LIGHTS` | Compile-time capacity for local point/spot lights. | `0` |

For small MCUs, do not instantiate with every feature if you do not need it. Fewer loaded shaders reduce code size and
can improve rendering speed.

Every runtime shader choice must be enabled in `LOADED_SHADERS`, including "disabled" choices such as
`SHADER_NOTEXTURE` and `SHADER_NOZBUFFER`. This point is important: a renderer that only compiles textured rendering
cannot later draw untextured geometry unless `SHADER_NOTEXTURE` was also loaded.

For example, this renderer can draw textured Gouraud meshes with a Z-buffer, but cannot draw untextured geometry:

~~~{.cpp}
const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                                   tgx::SHADER_GOURAUD |
                                   tgx::SHADER_TEXTURE |
                                   tgx::SHADER_TEXTURE_NEAREST |
                                   tgx::SHADER_TEXTURE_WRAP_POW2;
~~~

If the same sketch also draws untextured geometry, add `SHADER_NOTEXTURE`:

~~~{.cpp}
const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                                   tgx::SHADER_GOURAUD |
                                   tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE |
                                   tgx::SHADER_TEXTURE_NEAREST |
                                   tgx::SHADER_TEXTURE_WRAP_POW2;
~~~


@subsection sec_3D_shader_state Runtime Shader State

Use \ref tgx::Renderer3D::setShaders "setShaders()" before drawing to choose the current rendering path:

| Flag group | Choices |
|------------|---------|
| Shading | `SHADER_UNLIT`, `SHADER_FLAT`, `SHADER_GOURAUD` |
| Texture mode | `SHADER_NOTEXTURE`, `SHADER_TEXTURE`, `SHADER_TEXTURE_AFFINE` |
| Z-buffer mode | `SHADER_ZBUFFER`, `SHADER_NOZBUFFER` |
| Projection mode | `SHADER_PERSPECTIVE`, `SHADER_ORTHO` |
| Texture quality | `SHADER_TEXTURE_NEAREST`, `SHADER_TEXTURE_BILINEAR` |
| Texture addressing | `SHADER_TEXTURE_WRAP_POW2`, `SHADER_TEXTURE_CLAMP` |

`setShaders()` selects the active mode for subsequent draw calls. It does not allocate memory and it does not change
which shader variants were compiled into the binary.

~~~{.cpp}
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
~~~

`SHADER_TEXTURE` uses perspective-correct texture mapping. `SHADER_TEXTURE_AFFINE` interpolates texture coordinates
linearly in screen space; it is faster, but less accurate on large perspective triangles.

Shader state chooses the code path used by the rasterizer. Textures, materials and lights are separate state; they
provide the colors and lighting values consumed by that path.


@section sec_3D_geometry Geometry Sources

All normal solid drawing methods take coordinates in model space. Set the model matrix just before drawing the object
it belongs to:

~~~{.cpp}
renderer.setModelPosScaleRot(position, scale, angle, axis);
renderer.drawMesh(&mesh);
~~~

`drawSkyBox()` is the main exception: it is intended for distant backgrounds and ignores the current model matrix.


@subsection sec_3D_culling Back-face Culling

Back-face culling removes triangles that face away from the camera. It is often a large speed win on closed solid
meshes:

~~~{.cpp}
renderer.setCulling(1);   // keep one winding order
renderer.setCulling(-1);  // keep the opposite winding order
renderer.setCulling(0);   // disable culling
~~~

The correct sign depends on the winding order of the geometry after projection. If an object disappears completely,
try the opposite sign or disable culling while debugging. If a generated cylinder, cone or truncated cone is drawn
without all caps, TGX temporarily disables culling for that primitive so the inside remains visible, then restores the
previous culling state.


@subsection sec_3D_meshes Meshes

For static models, \ref tgx::Renderer3D::drawMesh "drawMesh()" is the recommended path. It is usually faster and more
compact than issuing many triangle or quad calls by hand.

TGX supports two mesh containers:

| Mesh type | Status | Notes |
|-----------|--------|-------|
| \ref tgx::Mesh3Dv2 | Current recommended format. | Compact meshlet-based format, good for generated OBJ models. |
| \ref tgx::Mesh3D | Legacy format. | Still supported for older examples and assets. |

Use the \ref tools_mesh "TGX tools" to generate `Mesh3Dv2` headers from Wavefront OBJ files or to migrate existing
legacy `Mesh3D` headers.

~~~{.cpp}
#include "buddha.h"

renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
renderer.drawMesh(&buddha, true);
~~~

When `use_mesh_material` is `true`, mesh material color, coefficients and texture override the current renderer
material for that mesh. Most generated OBJ models use this mode.

On embedded boards, mesh data may be stored in flash, RAM, external RAM or other memory. Faster memory can noticeably
improve rendering speed. With `Mesh3Dv2`, \ref tgx::cacheMesh "cacheMesh()" can copy selected mesh sections to faster
memory:

~~~{.cpp}
const tgx::Mesh3Dv2<tgx::RGB565>* cached =
    tgx::cacheMesh(&mesh, buf_DTCM, DTCM_buf_size, buf_DMAMEM, DMAMEM_buf_size, "LMPI");

renderer.drawMesh(cached);
~~~

The cache order string controls which parts are copied first:

- `M`: material table and optional material extension table;
- `I`: texture image pixels referenced by material tables;
- `P`: meshlet payload;
- `L`: meshlet table.

On Teensy 4.x, some examples also use \ref tgx::copyMeshEXTMEM "copyMeshEXTMEM()" to move large model data or
textures to external memory. Whether this helps depends on where the data was stored before and how the sketch uses
the model.


@subsection sec_3D_generated_shapes Generated Solid Shapes

Renderer3D can generate simple shapes directly:

| Method | Geometry |
|--------|----------|
| \ref tgx::Renderer3D::drawCube "drawCube()" | Unit cube `[-1,1]^3`, optionally textured per face. |
| \ref tgx::Renderer3D::drawSphere "drawSphere()" | Unit-radius UV sphere with explicit sector/stack counts. |
| \ref tgx::Renderer3D::drawAdaptativeSphere "drawAdaptativeSphere()" | Unit-radius sphere with tessellation chosen from projected size. |
| \ref tgx::Renderer3D::drawCylinder "drawCylinder()" | Unit cylinder, radius `1`, from `y=-1` to `y=1`, optional caps/textures. |
| \ref tgx::Renderer3D::drawCone "drawCone()" | Cone with base radius `1` at `y=-1` and apex at `y=1`. |
| \ref tgx::Renderer3D::drawTruncatedCone "drawTruncatedCone()" | Cone frustum from `y=-1` to `y=1` with separate bottom/top radii. |

Use the model matrix to scale, rotate and position these shapes. For cylinders, cones and truncated cones,
`nb_sectors` controls circular tessellation. Caps are enabled by default and can be disabled to draw open shapes.

Textured cylinders, cones and truncated cones accept separate textures for the side and caps. Passing `nullptr` for a
part draws that part without texturing; disabling the cap removes the cap geometry.


@subsection sec_3D_low_level Low-level Primitives

Low-level primitive calls are useful for dynamic geometry, quick tests and custom procedural shapes:

~~~{.cpp}
tgx::fVec3 P1(-1.0f, -1.0f, 0.0f);
tgx::fVec3 P2( 1.0f, -1.0f, 0.0f);
tgx::fVec3 P3( 0.0f,  1.0f, 0.0f);
tgx::fVec3 N(0.0f, 0.0f, 1.0f);

renderer.drawTriangle(P1, P2, P3, &N, &N, &N);
~~~

| Method | Use |
|--------|-----|
| \ref tgx::Renderer3D::drawTriangle "drawTriangle()" | Draw one triangle with optional normals, texture coordinates and texture. |
| \ref tgx::Renderer3D::drawTriangleWithVertexColor "drawTriangleWithVertexColor()" | Draw one triangle with explicit per-vertex colors. |
| \ref tgx::Renderer3D::drawTriangles "drawTriangles()" | Draw an indexed array of triangles sharing arrays. |
| \ref tgx::Renderer3D::drawTriangleStrip "drawTriangleStrip()" | Draw an indexed triangle strip, reusing previous vertices. |
| \ref tgx::Renderer3D::drawQuad "drawQuad()" | Draw one quad, internally split into two triangles. |
| \ref tgx::Renderer3D::drawQuadWithVertexColor "drawQuadWithVertexColor()" | Draw one quad with explicit per-vertex colors. |
| \ref tgx::Renderer3D::drawQuads "drawQuads()" | Draw an indexed array of quads. |

When many triangles or quads share arrays of vertices, normals and texture coordinates, prefer `drawTriangles()`,
`drawTriangleStrip()` or `drawQuads()` over many individual calls. For static geometry, prefer `drawMesh()`.

Normals are mandatory for Gouraud shading and should be unit vectors. Flat shading can compute a face normal from
geometry when no normal is provided, but explicit normals give more predictable lighting.


@subsection sec_3D_skybox Skyboxes

Use \ref tgx::Renderer3D::drawSkyBox "drawSkyBox()" for distant textured backgrounds. Unlike `drawCube()`, it is not
a normal model draw call: it ignores the current model matrix, material, culling and Z-buffer state, and should
normally be drawn before the Z-buffered scene.

~~~{.cpp}
renderer.drawSkyBox(&front, &back, &top, &bottom, &left, &right,
                    yaw_degrees,
                    reference_height,
                    radius,
                    tgx::SHADER_TEXTURE_NEAREST,
                    tgx::SHADER_TEXTURE_WRAP_POW2);
~~~

Use `drawCube()` for real cubes in the scene. Use `drawSkyBox()` only for backgrounds.


@section sec_3D_appearance Appearance: Textures, Materials and Lighting

Geometry decides where triangles are. Appearance decides what color those triangles become.


@subsection sec_3D_textures Textures

Textures are regular \ref tgx::Image objects whose color type matches the renderer color type:

~~~{.cpp}
tgx::RGB565 texture_data[128 * 128];
tgx::Image<tgx::RGB565> texture(texture_data, 128, 128);

renderer.setShaders(tgx::SHADER_FLAT | tgx::SHADER_TEXTURE);
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);
~~~

Texture coordinates are usually called `(u, v)`:

- `(0, 0)` usually means one corner of the texture;
- `(1, 1)` means the opposite corner;
- values outside `[0, 1]` can repeat or clamp depending on the wrapping mode.

Texture mapping modes:

| Mode | Meaning |
|------|---------|
| `SHADER_TEXTURE` | Perspective-correct texture mapping. |
| `SHADER_TEXTURE_AFFINE` | Faster screen-space interpolation; can distort on large perspective triangles. |

Sampling modes:

| Mode | Meaning |
|------|---------|
| `SHADER_TEXTURE_NEAREST` | Fastest, pixelated when magnified. |
| `SHADER_TEXTURE_BILINEAR` | Smoother, slower. |

Addressing modes:

| Mode | Meaning |
|------|---------|
| `SHADER_TEXTURE_WRAP_POW2` | Repeat texture coordinates; fastest, but both texture dimensions must be powers of two. |
| `SHADER_TEXTURE_CLAMP` | Clamp to edge; works with arbitrary texture sizes, slightly slower. |

@note On some MCUs, large textures stored in flash can be much slower to access than geometry. Smaller triangles,
power-of-two wrapping and faster texture memory can noticeably improve speed.


@subsection sec_3D_materials Materials

The current material controls untextured color and lighting coefficients:

~~~{.cpp}
renderer.setMaterialColor(tgx::RGBf(0.7f, 0.5f, 0.3f));
renderer.setMaterialAmbiantStrength(0.15f);
renderer.setMaterialDiffuseStrength(0.75f);
renderer.setMaterialSpecularStrength(0.25f);
renderer.setMaterialSpecularExponent(16);
~~~

The convenience method \ref tgx::Renderer3D::setMaterial "setMaterial()" sets all of these at once:

~~~{.cpp}
renderer.setMaterial(tgx::RGBf(0.7f, 0.5f, 0.3f), 0.15f, 0.75f, 0.25f, 16);
~~~

With flat or Gouraud textured rendering, texture color is combined with lighting. With `SHADER_UNLIT`, textured
geometry uses texture color directly and untextured geometry uses the current material color.

Mesh3Dv2 can store emissive material metadata, but the current renderer does not render emissive materials yet.


@subsection sec_3D_lighting Lighting Model

TGX uses compact lighting evaluated before rasterization:

- `SHADER_UNLIT` skips lighting and uses material or texture color directly;
- `SHADER_FLAT` computes one lighted color per face;
- `SHADER_GOURAUD` computes lighting at vertices and interpolates the result;
- TGX does not do per-pixel Phong lighting.

| Mode | Cost | Result |
|------|------|--------|
| `SHADER_UNLIT` | Cheapest | No lighting. Texture or material color is used directly. |
| `SHADER_FLAT` | Low | One lighted color per face; faceted look. |
| `SHADER_GOURAUD` | Higher | Lighting at vertices, interpolated across triangles; smoother on curved objects. |

Use `SHADER_UNLIT` for emissive-looking helper objects, debug geometry, sky-like objects or the cheapest textured
path. Use `SHADER_FLAT` for low-poly/faceted rendering. Use `SHADER_GOURAUD` for smoother models.


@subsection sec_3D_directional_lights Directional Lights

The default renderer supports one directional light. The simple API controls directional light 0:

~~~{.cpp}
renderer.setLightDirection({ -0.3f, -0.7f, -1.0f });
renderer.setLightAmbiant(tgx::RGBf(1.0f, 1.0f, 1.0f));
renderer.setLightDiffuse(tgx::RGBf(1.0f, 1.0f, 1.0f));
renderer.setLightSpecular(tgx::RGBf(1.0f, 1.0f, 1.0f));
~~~

The convenience method \ref tgx::Renderer3D::setLight "setLight()" sets the direction and all light colors at once.

If the renderer is instantiated with `MAX_DIRECTIONAL_LIGHTS > 1`, use the indexed API:

~~~{.cpp}
using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t, 4>;
Renderer renderer({ W, H }, &image, zbuffer);

renderer.setDirectionalLightCount(4);
renderer.setDirectionalLightAmbiant(tgx::RGBf(0.08f, 0.08f, 0.10f));
renderer.setDirectionalLight(0, { -0.45f, -0.60f, -1.0f },
                             tgx::RGBf(0.55f, 0.50f, 0.42f),
                             tgx::RGBf(0.22f, 0.20f, 0.18f));
renderer.setDirectionalLight(1, { 0.80f, -0.25f, -0.55f },
                             tgx::RGBf(0.18f, 0.34f, 0.90f),
                             tgx::RGBf(0.06f, 0.12f, 0.30f));
~~~

Ambient light is global across all directional lights. Diffuse and specular colors are per light. A runtime light
count of `0` gives ambient-only rendering.

For a directional light, the diffuse term is mostly:

~~~{.cpp}
diffuse = max(0, dot(normal, -light_direction));
~~~


@subsection sec_3D_spot_lights Point and Spot Lights

Local point lights and spot lights are optional. They are compiled in only when the renderer template is instantiated
with `MAX_SPOT_LIGHTS > 0`; the default value is `0`, which removes this code path. They do not need a shader flag,
but they are visible only with flat or Gouraud shading. `SHADER_UNLIT` ignores all lighting.

TGX uses the same `setSpotLight()` name for both local light types:

- a point light is a spot-light slot without a cone;
- a spot light is a local light with position, direction and cone angle.

~~~{.cpp}
using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t, 1, 2>;
Renderer renderer({ W, H }, &image, zbuffer);

renderer.setSpotLightCount(2);

// Point light: omnidirectional local light.
renderer.setSpotLight(0, tgx::fVec3(0.0f, 1.2f, -1.0f), 3.0f,
                      tgx::RGBf(1.0f, 0.55f, 0.25f));

// Spot light: position, direction, range, outer cone half-angle, diffuse color.
renderer.setSpotLight(1, tgx::fVec3(-1.0f, 1.5f, -0.5f),
                      tgx::fVec3(0.4f, -0.8f, -0.2f),
                      4.0f, 28.0f, tgx::RGBf(0.25f, 0.45f, 1.0f));
~~~

The `range` parameter controls both maximum influence distance and smooth attenuation. A value less than or equal to
zero gives an infinite-range light, but finite ranges are usually easier to tune and more visually useful.

Spot lights use cone half-angles in degrees. Values greater than or equal to 180 behave like point lights. Overloads
with `innerAngleDeg` create a soft cone edge.

An optional `specularColor` parameter enables local specular highlights for a local light. The default black value
disables local specular for that light. The specular exponent is still the current material specular exponent.

Local lights are evaluated per face in flat shading and per vertex in Gouraud shading. They are not per-pixel lights;
large triangles may miss small highlights unless the geometry is tessellated enough.


@section sec_3D_wireframe Wireframe and Debug Drawing

The renderer contains wireframe, pixel and dot helpers for diagnostics and special effects. Wireframe and point-cloud
methods ignore scene lighting and use the current material color or explicit colors.

There are three practical wireframe paths:

| Call style | Rendering path | Notes |
|------------|----------------|-------|
| `drawWireFrame...(object)` | Fast aliased wireframe. | No thickness, no blending, no antialiasing. |
| `drawWireFrame...AA(object)` | Optimized one-pixel antialiased wireframe. | Good default for readable debug wireframe. |
| `drawWireFrame...(object, thickness, color, opacity)` | Adjustable thickness + AA. | Uses the general thick-line path; can be very slow. |

Examples:

~~~{.cpp}
renderer.drawWireFrameMesh(&mesh);
renderer.drawWireFrameMeshAA(&mesh);
renderer.drawWireFrameMesh(&mesh, 1.6f, tgx::RGB565_Red, 0.9f);
~~~

Wireframe versions exist for meshes, low-level primitives and generated shapes such as cube, sphere, cylinder, cone
and truncated cone. Indexed wireframe helpers are also available for line lists, triangle lists, triangle strips and
quad lists.

Point-cloud helpers include \ref tgx::Renderer3D::drawPixel "drawPixel()", \ref tgx::Renderer3D::drawDot "drawDot()"
and \ref tgx::Renderer3D::drawDots "drawDots()".


@section sec_3D_performance Embedded Performance Checklist

For MCU targets, these choices often matter most:

- restrict `LOADED_SHADERS` to the variants the program really uses;
- include "disabled" modes such as `SHADER_NOTEXTURE` and `SHADER_NOZBUFFER` only when those paths are needed;
- use `RGB565` for display rendering;
- use a `uint16_t` Z-buffer when its precision is sufficient;
- reduce framebuffer, viewport or tile size when memory is tight;
- use `Mesh3Dv2`, \ref tgx::Renderer3D::drawMesh "drawMesh()" and \ref tgx::cacheMesh "cacheMesh()" for static models;
- use indexed arrays or triangle strips for dynamic geometry instead of many individual draw calls;
- enable back-face culling for closed meshes;
- keep normals normalized and mesh data well formed;
- choose generated primitive tessellation (`nb_sectors`, `nb_stacks`) according to screen size;
- prefer `SHADER_TEXTURE_NEAREST` and `SHADER_TEXTURE_WRAP_POW2` when quality allows it;
- keep textures in faster memory when possible;
- split very large textured faces if texture cache locality is poor;
- draw skyboxes with `drawSkyBox()`, not as ordinary cubes;
- keep `MAX_SPOT_LIGHTS` small and use finite ranges for local lights;
- disable local specular on spot lights unless highlights are useful;
- clear the image and Z-buffer once per frame, not before every object;
- avoid adjustable-thickness wireframe in every frame unless the visual effect is worth the cost.


@section sec_3D_complete_example Complete Embedded Example

This sketch is a compact starting point for a textured `Mesh3Dv2` model on an MCU framebuffer. The display upload is
left as a comment because it depends on the screen library.

~~~{.cpp}
#include <tgx.h>
#include "my_model.h"     // generated Mesh3Dv2<RGB565> and texture headers

constexpr int W = 150;
constexpr int H = 100;

tgx::RGB565 framebuffer[W * H];
uint16_t zbuffer[W * H];
tgx::Image<tgx::RGB565> image(framebuffer, W, H);

const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                                   tgx::SHADER_GOURAUD |
                                   tgx::SHADER_TEXTURE |
                                   tgx::SHADER_TEXTURE_NEAREST |
                                   tgx::SHADER_TEXTURE_WRAP_POW2;

using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t>;
Renderer renderer({ W, H }, &image, zbuffer);

void setup3D()
{
    renderer.setPerspective(45.0f, float(W) / float(H), 0.1f, 100.0f);
    renderer.setLookAt({ 0.0f, 1.0f, 5.0f },
                       { 0.0f, 0.4f, 0.0f },
                       { 0.0f, 1.0f, 0.0f });
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


@section sec_3D_examples Useful Examples

Useful starting points:

- `examples/CPU/buddhaOnCPU/`: CPU rendering into an image and displaying the result in a window.
- `examples/Teensy4/3D/buddha/`: shaded mesh rendering and mesh caching on Teensy 4.x.
- `examples/Teensy4/3D/borg_cube/`: dynamic texture generation and textured cube rendering.
- `examples/Teensy4/3D/test-shading/`: flat and Gouraud shading comparisons on several meshes.
- `examples/Teensy4/3D/test-texture/`: textured mesh rendering.
- `examples/Teensy4/3D/scream/`: dynamic textured surface built as a triangle strip.
- `examples/Teensy4/3D/characters/`: larger textured character models and chained meshes.
- `examples/Teensy4/3D/mars/`: a more complete scene using `drawSkyBox()` with textured objects.
- `examples/Teensy4/3D/pointlight_room/`: local point lights in a small scene.
- `examples/Teensy4/3D/pointlight_textured_meshes/`: point lights on textured meshes.
- `examples/Teensy4/3D/spotlight_checkerboard/`: moving spotlight cone on a textured floor.
- `examples/M5Stack/`, `examples/ESP32/` and `examples/Pico_RP2040_RP2350/`: board-specific ports of several 3D examples.


@section sec_3D_pitfalls Common Pitfalls

- **Nothing is drawn**: check that `setImage()`, `setViewportSize()` and the shader flags are valid.
- **A runtime shader path draws nothing**: make sure every required flag was compiled into `LOADED_SHADERS`, including
  `SHADER_NOTEXTURE` and `SHADER_NOZBUFFER` when those modes are used.
- **Geometry appears behind other objects incorrectly**: clear the Z-buffer at the start of each frame.
- **No-Z drawing does not work**: include and select `SHADER_NOZBUFFER`.
- **A mesh disappears when culling is enabled**: reverse the culling sign or verify face winding.
- **Textured geometry is missing or drawn without texture**: check that `SHADER_TEXTURE` or `SHADER_TEXTURE_AFFINE`,
  texture quality and texture wrapping flags were compiled into the renderer.
- **Wrapped textures look wrong**: `SHADER_TEXTURE_WRAP_POW2` requires power-of-two texture dimensions.
- **Gouraud lighting looks strange**: verify that normals are normalized and match the model transform.
- **Point/spot light methods do not compile**: instantiate `Renderer3D` with `MAX_SPOT_LIGHTS > 0`.
- **Point/spot lights have no visible effect**: check `setSpotLightCount()`, finite range, material diffuse strength
  and that the current shader is flat or Gouraud rather than unlit.
- **Spot lights overbrighten the scene**: reduce diffuse/specular color, reduce range, or lower ambient/material
  strengths.
- **Orthographic and perspective views do not match**: compare visible height at object depth and keep the same
  camera/view matrix while switching projection.
- **A custom projection behaves strangely**: after `setProjectionMatrix()`, call `usePerspectiveProjection()` or
  `useOrthographicProjection()` as appropriate.
