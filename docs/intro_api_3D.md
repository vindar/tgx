@page intro_api_3D Using the 3D API.


**This page introduces the TGX 3D renderer and shows how to render solid 3D objects onto a `tgx::Image`.**

- For the complete list of methods, see the \ref tgx::Renderer3D class documentation.
- For the legacy mesh container, see \ref tgx::Mesh3D.
- For the current compact meshlet container, see \ref tgx::Mesh3Dv2.
- For ready-to-run code, see the 3D examples in the `/examples/` directory.

The 3D engine is designed for small embedded targets. It is not a scene graph and it is not a desktop GPU API.
It is a compact software rasterizer that draws triangles, quads, meshes, cubes and spheres into a normal
\ref tgx::Image. Your sketch still owns the framebuffer, the display upload and the memory layout.

In practice, using the 3D API means:

- choose where pixels are written;
- choose a camera and a projection;
- place the object you want to draw;
- choose the shader, material, texture and light state;
- call a drawing method such as \ref tgx::Renderer3D::drawMesh "drawMesh()" or
  \ref tgx::Renderer3D::drawCube "drawCube()".

TGX keeps these steps visible because, on MCUs, memory placement, shader variants and draw-call cost matter a lot.


![buddha](../buddha.png)

*The `buddha` mesh rendered by TGX's 3D engine.*

*See the example located in `examples/CPU/buddhaOnCPU/`.*


@section sec_3D_overview What Renderer3D Does

The main class is \ref tgx::Renderer3D. It transforms 3D points, clips triangles, projects them to a 2D viewport,
and writes pixels into a destination \ref tgx::Image. It does not upload pixels to a display by itself: the sketch
keeps control of the framebuffer, the Z-buffer and the final display update.

A renderer stores the state needed to draw the next object:

| State | What it means | Main methods |
|-------|---------------|--------------|
| Destination | The image receiving the pixels, plus the virtual viewport and tile offset. | \ref tgx::Renderer3D::setImage "setImage()", \ref tgx::Renderer3D::setViewportSize "setViewportSize()", \ref tgx::Renderer3D::setOffset "setOffset()" |
| Depth buffer | Optional per-pixel depth storage used by solid rendering. | \ref tgx::Renderer3D::setZbuffer "setZbuffer()", \ref tgx::Renderer3D::clearZbuffer "clearZbuffer()" |
| Projection | Perspective or orthographic mapping from camera space to clip space, then to NDC. | \ref tgx::Renderer3D::setPerspective "setPerspective()", \ref tgx::Renderer3D::setOrtho "setOrtho()", \ref tgx::Renderer3D::setFrustum "setFrustum()", \ref tgx::Renderer3D::setProjectionMatrix "setProjectionMatrix()" |
| Camera/view | Placement and orientation of the camera. | \ref tgx::Renderer3D::setLookAt "setLookAt()", \ref tgx::Renderer3D::setViewMatrix "setViewMatrix()" |
| Current object | Placement, scale and rotation of the object currently being drawn. | \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()", \ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" |
| Shader state | The rendering path selected for subsequent draw calls. | \ref tgx::Renderer3D::setShaders "setShaders()", \ref tgx::Renderer3D::setTextureQuality "setTextureQuality()", \ref tgx::Renderer3D::setTextureWrappingMode "setTextureWrappingMode()" |
| Material and light | Colors and lighting coefficients used by flat/Gouraud shaders. | \ref tgx::Renderer3D::setLightDirection "setLightDirection()", \ref tgx::Renderer3D::setLight "setLight()", \ref tgx::Renderer3D::setMaterial "setMaterial()", \ref tgx::Renderer3D::setMaterialColor "setMaterialColor()" |
| Culling/debug | Back-face culling and diagnostic drawing modes. | \ref tgx::Renderer3D::setCulling "setCulling()", \ref tgx::Renderer3D::drawWireFrameMesh "drawWireFrameMesh()", \ref tgx::Renderer3D::drawDot "drawDot()", \ref tgx::Renderer3D::drawPixel "drawPixel()" |

A small frame can look like this:

~~~{.cpp}
#include <tgx.h>

constexpr int W = 320;
constexpr int H = 240;

tgx::RGB565 framebuffer[W * H];
uint16_t zbuffer[W * H];
tgx::Image<tgx::RGB565> image(framebuffer, W, H);

// Keep only the shader variants that this program will actually use.
const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD |
                                   tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2;

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

On an embedded display, the last step is often an upload of `image` to the screen driver. On a desktop target,
the same image can be displayed in a window or written to a file.

In a larger sketch, separate the state that changes rarely from the state that changes every frame:

| When | Typical work |
|------|--------------|
| Initialization | Create the image, create or assign the Z-buffer, choose the shader variants. |
| Beginning of a frame | Clear the image and Z-buffer, update the camera if it moves. |
| For each object | Set the model matrix, material, shader and texture state, then draw. |
| End of the frame | Send the image buffer to the display, or save/inspect it on CPU. |

The next sections connect these calls to the main 3D concepts.

@section sec_3D_pipeline Coordinate Spaces

A vertex is not drawn directly from the coordinates stored in the mesh. It first moves through a few coordinate
spaces: local object coordinates, scene coordinates, camera coordinates, projected coordinates, and finally pixels.
This is the part that the model, view and projection matrices control.

![3d_coordinate_spaces](../3d_coordinate_spaces.svg)

| Space | Meaning in TGX | Typical values |
|-------|----------------|----------------|
| Model space | Coordinates stored in a mesh or passed to primitive drawing functions. They are local to the object. | A cube centered around `(0,0,0)`, a mesh normalized to a unit box. |
| World space | A common scene coordinate system after the model matrix has placed the object. | Several objects can now be positioned relative to each other. |
| View space | Coordinates after the camera transform. The camera is at the origin. | TGX uses a right-handed convention and the camera looks along negative Z. |
| Clip space | Homogeneous coordinates after projection, before the perspective divide. | Points still have a `w` component. Clipping is performed here. |
| NDC | Normalized device coordinates after division by `w` for perspective projection. | Visible x/y coordinates are approximately in `[-1, 1]`. |
| Image space | Pixel coordinates in the destination image. | `(0,0)` is the upper-left pixel, X goes right, Y goes down. |

In view space, TGX uses the common right-handed convention: the camera is at the origin and looks along negative Z.
Once the point reaches image space, it uses the normal \ref tgx::Image convention: `(0,0)` is the upper-left pixel,
X goes right and Y goes down.

TGX does not keep a list of objects. For each draw call, the current renderer state is enough:

| Transform | Matrix/API | Role |
|-----------|------------|------|
| Object placement | Model matrix, \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()", \ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" | Converts model coordinates to world coordinates. |
| Camera placement | View matrix, \ref tgx::Renderer3D::setViewMatrix "setViewMatrix()", \ref tgx::Renderer3D::setLookAt "setLookAt()" | Converts world coordinates to camera/view coordinates. |
| Camera lens | Projection matrix, \ref tgx::Renderer3D::setPerspective "setPerspective()", \ref tgx::Renderer3D::setOrtho "setOrtho()", \ref tgx::Renderer3D::setFrustum "setFrustum()" | Converts view coordinates to clip coordinates; perspective projection then divides by `w` to get NDC. |
| Pixel mapping | Viewport and offset, \ref tgx::Renderer3D::setViewportSize "setViewportSize()", \ref tgx::Renderer3D::setOffset "setOffset()" | Converts NDC coordinates to image pixels. |

The full pipeline is:

![3d_pipeline](../3d_pipeline.svg)

The mesh data itself is not modified. Each frame, TGX reads the original vertices and applies the current state. The
same mesh can be drawn many times with different model matrices, materials or shaders.


@subsection sec_3D_math Vectors, Points and Matrices

TGX uses small value types for 3D math:

| Type | Purpose |
|------|---------|
| `tgx::fVec3` | A 3D vector or point using `float` coordinates. |
| `tgx::fVec4` | A homogeneous 4D vector, mostly useful when working with projected coordinates. |
| `tgx::fMat4` | A 4x4 floating-point matrix used for 3D transformations and projections. |
| `tgx::fBox3` | An axis-aligned 3D bounding box, used by meshes and clipping tests. |

Most sketches only need `fVec3` and the `Renderer3D` matrix setters. Use `fMat4` directly when you want a custom
camera, a custom projection, or when you want to reuse a transform between several objects.

The same `(x,y,z)` triplet can represent either a point or a direction:

- a **point** is a position in space, and translations affect it;
- a **direction** is an orientation or displacement, and translations should not affect it.

Homogeneous coordinates encode this distinction with `w`:

| Quantity | Homogeneous form | Matrix helper |
|----------|------------------|---------------|
| Point | `(x, y, z, 1)` | `fMat4::mult1()` |
| Direction/vector | `(x, y, z, 0)` | `fMat4::mult0()` |
| Explicit 4D vector | `(x, y, z, w)` | `fMat4::mult()` |

TGX follows this distinction in the matrix helpers: use `mult1()` for positions and `mult0()` for directions.

The vector operations used most often in 3D rendering are:

| Operation | TGX methods/functions | Used for |
|-----------|-----------------------|----------|
| Length | \ref tgx::Vec3::norm2 "norm2()", \ref tgx::Vec3::norm "norm()", \ref tgx::Vec3::norm_fast "norm_fast()" | Distances and normalization. |
| Inverse length | \ref tgx::Vec3::invnorm "invnorm()", \ref tgx::Vec3::invnorm_fast "invnorm_fast()" | Fast normalization and lighting. |
| Normalize | \ref tgx::Vec3::normalize "normalize()", \ref tgx::Vec3::normalize_fast "normalize_fast()" | Unit normals, light directions, camera vectors. |
| Dot product | <a class="el" href="_vec3_8h.html#aa66986f6d4db428a3fca04c45507c860"><code>dotProduct(a,b)</code></a> | Lighting, back-face tests, angle tests. |
| Cross product | <a class="el" href="_vec3_8h.html#a3bdc106d9df1a0355e71a94b2257f9ea"><code>crossProduct(a,b)</code></a> | Building perpendicular axes and face normals. |

For lighting and culling, normals and light directions should have length 1. If a normal is not normalized, lighting
will be wrong because the dot product no longer only measures an angle.

`fMat4` stores its coefficients in column-major order, matching the OpenGL-style formulas used by the helper methods.
Most sketches should use the named helpers (`setPerspective()`, `setLookAt()`, `setTranslate()`...) instead of
filling the `M[16]` array manually.

Matrices are applied from right to left in this expression:

~~~{.cpp}
P_clip = projection * view * model * P_model;
~~~

So `model` is applied first, then `view`, then `projection`:

1. `P_world = model * P_model`
2. `P_view = view * P_world`
3. `P_clip = projection * P_view`

If the object moves when the camera should move, or if rotations happen around the wrong point, check this order first.

The `set...()` methods replace a matrix with a new transform. The `mult...()` methods pre-multiply the current matrix,
which is useful when building a transform step by step:

~~~{.cpp}
tgx::fMat4 M;
M.setIdentity();
M.multScale(1.0f, 1.0f, 1.0f);
M.multRotate(angle, 0.0f, 1.0f, 0.0f);
M.multTranslate(0.0f, 0.0f, -3.0f);
renderer.setModelMatrix(M);
~~~

Here the model points are scaled, then rotated, then translated.

The most useful \ref tgx::Mat4 helpers are:

| Method | Meaning |
|--------|---------|
| \ref tgx::Mat4::setIdentity "setIdentity()" | Reset to the identity matrix. |
| \ref tgx::Mat4::setScale "setScale()", \ref tgx::Mat4::multScale "multScale()" | Build or pre-multiply a scale transform. |
| \ref tgx::Mat4::setRotate "setRotate()", \ref tgx::Mat4::multRotate "multRotate()" | Build or pre-multiply a rotation transform. |
| \ref tgx::Mat4::setTranslate "setTranslate()", \ref tgx::Mat4::multTranslate "multTranslate()" | Build or pre-multiply a translation transform. |
| \ref tgx::Mat4::setLookAt "setLookAt()" | Build a camera/view matrix. |
| \ref tgx::Mat4::setPerspective "setPerspective()", \ref tgx::Mat4::setFrustum "setFrustum()" | Build perspective projection matrices. |
| \ref tgx::Mat4::setOrtho "setOrtho()" | Build an orthographic projection matrix. |
| \ref tgx::Mat4::invertYaxis "invertYaxis()" | Flip the Y axis; used internally by TGX projections to match image coordinates. |
| \ref tgx::Mat4::mult0 "mult0()", \ref tgx::Mat4::mult1 "mult1()" | Transform a direction or a point. |


@subsection sec_3D_matrices Renderer Matrices

`Renderer3D` stores three user-visible matrices:

| Matrix | Maps from | Maps to | Main TGX methods |
|--------|-----------|---------|------------------|
| Model matrix | Model space | World space | \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()", \ref tgx::Renderer3D::getModelMatrix "getModelMatrix()", \ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" |
| View matrix | World space | View/camera space | \ref tgx::Renderer3D::setViewMatrix "setViewMatrix()", \ref tgx::Renderer3D::getViewMatrix "getViewMatrix()", \ref tgx::Renderer3D::setLookAt "setLookAt()" |
| Projection matrix | View space | Clip space | \ref tgx::Renderer3D::setProjectionMatrix "setProjectionMatrix()", \ref tgx::Renderer3D::getProjectionMatrix "getProjectionMatrix()", \ref tgx::Renderer3D::setPerspective "setPerspective()", \ref tgx::Renderer3D::setOrtho "setOrtho()", \ref tgx::Renderer3D::setFrustum "setFrustum()" |

Internally, the renderer also keeps a **model-view matrix** derived from `view * model`. You normally do not set it
yourself; it is recomputed when the model or view matrix changes.

The short version is:

| Matrix | Question it answers |
|--------|---------------------|
| Model | Where is this object, and how is it rotated or scaled? |
| View | Where is the camera, and where is it looking? |
| Projection | How does the camera volume become a flat screen image? |

These calls only set renderer state; they do not draw anything. A clear render loop sets the camera/projection state
first, then the per-object model state just before drawing:

~~~{.cpp}
// 1. Choose the camera lens.
renderer.setPerspective(fovy_degrees, aspect, zNear, zFar);

// 2. Place the camera in the world.
renderer.setLookAt(eye, center, up);

// 3. Place the current object in the world.
renderer.setModelPosScaleRot(position, scale, angle_degrees, axis);
~~~

\ref tgx::Renderer3D::setModelPosScaleRot "setModelPosScaleRot()" is the convenient form for most objects: position,
scale, rotation angle and rotation axis. Use \ref tgx::Renderer3D::setModelMatrix "setModelMatrix()" when you already
have a full matrix.

`setLookAt()` builds the view matrix:

~~~{.cpp}
renderer.setLookAt({ 0.0f, 2.0f, 6.0f },    // eye: camera position in world space
                   { 0.0f, 0.0f, 0.0f },    // center: point being looked at
                   { 0.0f, 1.0f, 0.0f });   // up: screen vertical direction
~~~

After this call, objects are seen from `eye`, looking toward `center`.

The projection matrix acts like the camera lens. It does not move the camera; it controls the visible volume and how
depth affects the projected image:

| Method | Use |
|--------|-----|
| \ref tgx::Renderer3D::setPerspective "setPerspective(fovy, aspect, zNear, zFar)" | Usual perspective camera; distant objects appear smaller. |
| \ref tgx::Renderer3D::setFrustum "setFrustum(left, right, bottom, top, zNear, zFar)" | Explicit perspective frustum. |
| \ref tgx::Renderer3D::setOrtho "setOrtho(left, right, bottom, top, zNear, zFar)" | Orthographic view; object size does not depend on distance. |
| \ref tgx::Renderer3D::setProjectionMatrix "setProjectionMatrix(M)" | Use a custom projection matrix. |
| \ref tgx::Renderer3D::usePerspectiveProjection "usePerspectiveProjection()" | Tell the renderer that a custom projection is perspective. |
| \ref tgx::Renderer3D::useOrthographicProjection "useOrthographicProjection()" | Tell the renderer that a custom projection is orthographic. |

The last two methods are only needed after `setProjectionMatrix()`. The standard helpers call them automatically.

Changing the model matrix affects the next object. Changing the view matrix moves the camera. Changing the projection
matrix changes the lens.

All solid primitive calls (`drawTriangle()`, `drawCube()`, `drawSphere()`, `drawMesh()`...) take coordinates in model
space. In a render loop, update the model matrix just before drawing the object it belongs to.

When debugging transforms, these methods are useful:

| Method | Converts |
|--------|----------|
| \ref tgx::Renderer3D::modelToNDC "modelToNDC(P)" | A model-space point to normalized device coordinates. |
| \ref tgx::Renderer3D::modelToImage "modelToImage(P)" | A model-space point to image pixels. |
| \ref tgx::Renderer3D::worldToNDC "worldToNDC(P)" | A world-space point to normalized device coordinates. |
| \ref tgx::Renderer3D::worldToImage "worldToImage(P)" | A world-space point to image pixels. |

These methods are useful when placing labels, debugging a camera, or checking why an object is outside the view.

Normals describe surface orientation. Non-uniform scale is a special case because normals cannot be transformed like
ordinary points. TGX keeps the runtime small and works best with normalized mesh normals and mostly uniform scales.


@subsection sec_3D_projection Projection and Viewport Mapping

TGX uses a common camera convention: in view space the camera is at the origin, looking toward negative Z, with Y
pointing upward. The projection matrix defines what is visible. TGX then maps the projected coordinates to the
viewport and finally to pixels.

In perspective projection, the visible volume is a frustum, a truncated pyramid:

![3d_projection](../3d_projection.svg)

Keep `zNear` positive and not too close to zero. A very small near plane reduces depth precision and can make
Z-buffer artifacts more visible. Choose `zFar` large enough for the scene, but avoid making it much larger than
needed.

The two standard projections are:

| Projection | TGX call | Visual effect |
|------------|----------|---------------|
| Perspective | \ref tgx::Renderer3D::setPerspective "setPerspective(fovy, aspect, zNear, zFar)" | Distant objects become smaller. This is the standard 3D camera look. |
| Orthographic | \ref tgx::Renderer3D::setOrtho "setOrtho(left, right, bottom, top, zNear, zFar)" | Object size does not depend on distance. This is useful for CAD-like views, debug views or 3D overlays. |

For perspective projection, `fovy` is the vertical field of view in degrees and `aspect` is normally
`viewport_width / viewport_height`. If the aspect ratio is wrong, objects look stretched.

After projection and perspective division, TGX rescales NDC coordinates to the virtual viewport, then applies the
image offset:

| Concept | TGX method | Notes |
|---------|------------|-------|
| Destination image | \ref tgx::Renderer3D::setImage "setImage()" | Selects the \ref tgx::Image receiving pixels. |
| Virtual viewport size | \ref tgx::Renderer3D::setViewportSize "setViewportSize()" | The full projected pixel area. Often equal to the image size. |
| Image offset | \ref tgx::Renderer3D::setOffset "setOffset()" | Places the current image inside a larger viewport for tile rendering. |

For a normal full-frame render, the image and viewport have the same size and the offset is `(0,0)`. For tile
rendering, keep the viewport equal to the final screen size and draw smaller images at different offsets.


@section sec_3D_renderer_template Renderer template parameters

\ref tgx::Renderer3D has three template parameters:

~~~{.cpp}
tgx::Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>
~~~

- `color_t` is the destination color type, usually `tgx::RGB565` on MCU displays and `tgx::RGB24`,
  `tgx::RGB32` or `tgx::RGBf` on CPU.
- `LOADED_SHADERS` is the compile-time list of shader variants that may be used.
- `ZBUFFER_t` is either `float` or `uint16_t`.

The default `LOADED_SHADERS` enables every shader variant. This is convenient while experimenting. On MCU targets,
load only the variants you use: it reduces code size and can help speed.

This compile-time shader list is one of the ways TGX stays small. Unused rasterizer paths can be removed by the
compiler.

For example, a fast textured mesh renderer using perspective projection, a Z-buffer, Gouraud lighting, nearest
texture sampling and power-of-two texture wrapping can be declared as:

~~~{.cpp}
const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                                   tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2;
using Renderer = tgx::Renderer3D<tgx::RGB565, shaders_loaded, uint16_t>;
~~~

@warning Runtime shader changes can only select variants that were enabled in `LOADED_SHADERS`. If a draw call needs
a missing variant, it may draw nothing. This keeps hot drawing paths small and fast, so double-check the shader list
when a scene unexpectedly disappears.


@subsection sec_3D_shaders Shader state

The renderer combines several independent shader choices:

- projection: `SHADER_PERSPECTIVE` or `SHADER_ORTHO`;
- depth mode: `SHADER_ZBUFFER` or `SHADER_NOZBUFFER`;
- shading path: `SHADER_UNLIT`, `SHADER_FLAT` or `SHADER_GOURAUD`;
- texture usage: `SHADER_NOTEXTURE` or `SHADER_TEXTURE`;
- texture sampling: `SHADER_TEXTURE_NEAREST` or `SHADER_TEXTURE_BILINEAR`;
- texture addressing: `SHADER_TEXTURE_WRAP_POW2` or `SHADER_TEXTURE_CLAMP`.

These flags are combined in two places: first in the renderer template parameter, to decide which code paths are
compiled, and then at runtime with `setShaders()` and the texture setters, to choose which compiled path is active.

Runtime shader state describes the next draw call:

| Part of the draw call | Examples |
|-----------------------|----------|
| Projection path | perspective or orthographic. |
| Depth path | with or without Z-buffer. |
| Shading path | unlit, flat or Gouraud. |
| Texture path | no texture, nearest texture, or bilinear texture. |
| Addressing path | wrapping or clamping texture coordinates. |

| Shader choice | Meaning | Typical use |
|---------------|---------|-------------|
| `SHADER_PERSPECTIVE` | Use perspective projection with division by `w`. | Normal 3D scenes. |
| `SHADER_ORTHO` | Use orthographic projection without perspective shrinking. | CAD-like views, debug views, 3D sprites. |
| `SHADER_ZBUFFER` | Use depth testing. | Solid objects that can overlap. |
| `SHADER_NOZBUFFER` | Draw without depth testing. | Ordered overlays or special effects. |
| `SHADER_UNLIT` | Use the material color or texture color directly, without lighting. | UI-like 3D, emissive objects, lightmaps, debug views. |
| `SHADER_FLAT` | One lighting result per triangle. | Fast faceted rendering. |
| `SHADER_GOURAUD` | Lighting at vertices, interpolated across triangles. | Smoother curved meshes. |
| `SHADER_NOTEXTURE` | Use material or vertex colors only. | Untextured models and debug views. |
| `SHADER_TEXTURE` | Enable texture mapping for the next draw call. | Textured meshes or textured primitives. |
| `SHADER_TEXTURE_NEAREST` | Nearest-neighbor texture lookup. | Fast textured rendering. |
| `SHADER_TEXTURE_BILINEAR` | Bilinear texture filtering. | Smoother textures when speed allows. |
| `SHADER_TEXTURE_WRAP_POW2` | Repeat power-of-two textures. | Fast tiling textures. |
| `SHADER_TEXTURE_CLAMP` | Clamp texture coordinates to the edge. | Non-power-of-two textures or non-repeating images. |

The most common runtime call is:

~~~{.cpp}
renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE);
~~~

Think of `LOADED_SHADERS` as "what exists in the binary", and `setShaders()` as "what I want to use now":

~~~{.cpp}
// Compiled once, at build time.
const tgx::Shader shaders_loaded =
    tgx::SHADER_PERSPECTIVE |
    tgx::SHADER_ZBUFFER |
    tgx::SHADER_UNLIT |
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

If a sketch only uses unlit textured geometry, do not load the flat or Gouraud lighting paths. If it switches between
a flat debug view and a textured view, load both `SHADER_FLAT` and the texture mode.

Texture quality and wrapping may also be selected explicitly:

~~~{.cpp}
renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
~~~

`SHADER_UNLIT` is the cheapest solid shading path because it skips lighting computations: textured geometry keeps
its texture colors, and untextured geometry uses the current material color. `SHADER_FLAT` is usually the fastest
lit mode. `SHADER_GOURAUD` interpolates vertex lighting and gives smoother surfaces, especially on curved meshes.
Textured Gouraud rendering is often the best visual compromise for embedded solid 3D rendering.

Internally, `Renderer3D` dispatches to templated shader variants. Keeping `LOADED_SHADERS` narrow saves flash and
helps the compiler remove unused branches.


@subsection sec_3D_zbuffer Z-buffer

A Z-buffer is required for normal solid rendering when triangles overlap. Its memory footprint is:

- `width * height * 4` bytes for a `float` Z-buffer;
- `width * height * 2` bytes for a `uint16_t` Z-buffer.

`float` gives more depth precision. `uint16_t` is often the best choice on MCU targets because it halves memory use
and memory traffic.

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

@warning Clear the Z-buffer at the start of every frame. Also clear it after changing the viewport offset during tile
rendering. If old depth values are left in place, parts of the next frame can disappear.


@section sec_3D_meshes Mesh3Dv2, Mesh3D and generated models

Most 3D objects are stored as meshes. A mesh is a list of triangles plus the data needed to draw them:

- **positions**: the 3D coordinates of its vertices;
- **normals**: directions perpendicular to the surface, used for lighting;
- **texture coordinates**: `(u, v)` coordinates telling which part of a texture image maps to each vertex;
- **material information**: color, lighting coefficients and optional texture image.

TGX can draw individual triangles directly. For static models, a mesh file is often better: the renderer can reuse
data, skip invisible parts and do less work each frame.

\ref tgx::Mesh3Dv2 is the preferred format for new static models. It stores compact 16-bit meshlet payloads,
materials, optional textures and precomputed visibility data. Compared with legacy \ref tgx::Mesh3D, it usually
uses less memory bandwidth and can skip invisible meshlets before decoding their triangles.

At a high level, a `Mesh3Dv2` model contains:

- one material table: colors, lighting strengths and optional texture pointers;
- one meshlet table: small local groups of triangles, each attached to one material;
- one compact payload: quantized vertices, normals, UVs and triangle chains for the meshlets;
- visibility information used to skip meshlets that cannot contribute to the current view.

This layout works well on MCUs: a skipped meshlet costs little, and a visible meshlet mostly works on a small local
set of data.

Legacy \ref tgx::Mesh3D is still supported for existing projects. It stores global arrays of vertices, normals,
texture coordinates and chained triangle strips. Existing sketches can keep using it; new converted models will
generally be better served by `Mesh3Dv2`.

Typical generated mesh usage:

~~~{.cpp}
#include "buddha.h"

renderer.setShaders(tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
renderer.drawMesh(&buddha);
~~~

For `Mesh3Dv2`, \ref tgx::Renderer3D::drawMesh "drawMesh(mesh, use_mesh_material)" renders all meshlets in the model:

- `mesh`: the model to render;
- `use_mesh_material`: when true, use the material colors, lighting strengths and texture pointers stored in the mesh.

For legacy `Mesh3D`, \ref tgx::Renderer3D::drawMesh "drawMesh(mesh, use_mesh_material, draw_chained_meshes)" also has:

- `draw_chained_meshes`: when true, also draw linked meshes.

For static meshes, <code>%cacheMesh()</code> can copy selected mesh data into RAM buffers supplied by the sketch. This
is useful on boards where some RAM regions are faster than flash, external RAM, or another memory region. With
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

On Teensy 4.x, some examples also use <code>%copyMeshEXTMEM()</code> to move large model data or textures to external
memory. Whether this helps depends on where the data was stored before and on how the sketch uses the model.

Use the \ref tools_mesh "TGX tools" to generate `Mesh3Dv2` headers from Wavefront OBJ files or to migrate existing
legacy `Mesh3D` headers.


@subsection sec_3D_primitives Drawing primitives directly

The renderer can also draw individual primitives. These calls are useful for dynamic geometry, quick tests and
debugging:

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
| \ref tgx::Renderer3D::drawTriangle "drawTriangle()" | Draw one triangle with optional normals, texture coordinates and texture. |
| \ref tgx::Renderer3D::drawTriangleWithVertexColor "drawTriangleWithVertexColor()" | Draw one triangle with explicit per-vertex colors. |
| \ref tgx::Renderer3D::drawTriangles "drawTriangles()" | Draw an indexed array of triangles sharing vertex, normal and texture-coordinate arrays. |
| \ref tgx::Renderer3D::drawQuad "drawQuad()" | Draw one quad, internally split into two triangles. |
| \ref tgx::Renderer3D::drawQuadWithVertexColor "drawQuadWithVertexColor()" | Draw one quad with explicit per-vertex colors. |
| \ref tgx::Renderer3D::drawQuads "drawQuads()" | Draw an indexed array of quads. |
| \ref tgx::Renderer3D::drawCube "drawCube()" | Draw the unit cube, optionally textured per face. |
| \ref tgx::Renderer3D::drawSphere "drawSphere()" | Draw a generated sphere with a chosen tessellation. |
| \ref tgx::Renderer3D::drawAdaptativeSphere "drawAdaptativeSphere()" | Draw a generated sphere with tessellation chosen from its projected size. |

When many triangles or quads share arrays of vertices, normals and texture coordinates, use `drawTriangles()` or
`drawQuads()` instead of many individual calls. For fully static geometry, use `drawMesh()`.

Normals should be unit vectors for Gouraud shading. Flat shading can compute a face normal from the geometry when no
normal is provided, but explicit normals give more predictable lighting.


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

Two sampling modes are available:

- `SHADER_TEXTURE_NEAREST`: fastest, pixelated when magnified;
- `SHADER_TEXTURE_BILINEAR`: smoother, slower.

Two addressing modes are available:

- `SHADER_TEXTURE_WRAP_POW2`: repeat texture coordinates; fastest, but both texture dimensions must be powers of two;
- `SHADER_TEXTURE_CLAMP`: clamp to the edge; slightly slower, but works with arbitrary texture dimensions.

@note On some MCUs, large textures stored in flash can be much slower to access than geometry. Smaller triangles,
power-of-two wrapping and faster texture memory can noticeably improve speed.

Texture coordinates are often called `(u, v)`. They are not pixel coordinates:

- `(0, 0)` usually means one corner of the texture;
- `(1, 1)` means the opposite corner;
- values outside `[0, 1]` can either repeat the texture or clamp to its edge, depending on the wrapping mode.

With `SHADER_TEXTURE_WRAP_POW2`, TGX can wrap very quickly, but the texture width and height must be powers of two.
With `SHADER_TEXTURE_CLAMP`, TGX accepts arbitrary texture sizes, but each lookup needs slightly more work.


@subsection sec_3D_lighting Light and material

TGX uses a compact Phong-style lighting model. It is evaluated per vertex for Gouraud shading, or once per face for
flat shading. It combines a directional light, material color and ambient, diffuse and specular strengths:

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
material settings for that mesh. Most generated OBJ models use this mode.

Unlit, flat and Gouraud shading differ in how much lighting work they do:

- **Unlit shading** skips lighting. Textured geometry uses the texture color directly; untextured geometry uses the
  current material color. This is useful for emissive objects, lightmaps, debug views and the cheapest textured path.
- **Flat shading** computes one color for the whole triangle. It is fast and gives a faceted look.
- **Gouraud shading** computes lighting at vertices and interpolates the resulting colors across the triangle. It is
  smoother on curved models, but costs more math and interpolation.

TGX does not implement per-pixel Phong normal interpolation; it would be much more expensive on the intended MCU
targets.

For a directional light, the diffuse part is controlled mostly by:

~~~{.cpp}
diffuse = max(0, dot(normal, -light_direction));
~~~

When the normal points toward the light, the dot product is close to 1 and the surface is bright. When it points away,
the value is 0 and only ambient or specular terms may remain. Wrong or non-normalized normals usually show up as
strange dark or overbright areas.


@subsection sec_3D_culling Back-face culling

Back-face culling removes triangles that face away from the camera. This is often a large speed win on closed solid
meshes.

~~~{.cpp}
renderer.setCulling(1);   // keep one winding order
renderer.setCulling(-1);  // keep the opposite winding order
renderer.setCulling(0);   // disable culling
~~~

The correct sign depends on the winding order of the model data after projection. If a mesh disappears completely,
try the opposite sign or disable culling while debugging.


@subsection sec_3D_tiling Tile rendering

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


@subsection sec_3D_wireframe Wireframe and debug drawing

The renderer also contains wireframe, dot and normal-visualization methods. They are useful for inspecting geometry
and debugging transforms. They are not the main optimized path of the 3D engine, so high-quality wireframe drawing can
be much slower than solid rendering.

For performance-sensitive rendering, prefer the solid mesh path first:

~~~{.cpp}
renderer.drawMesh(&mesh);
~~~


@section sec_3D_performance Embedded performance checklist

For MCU targets, these choices often matter most:

- restrict `LOADED_SHADERS` to the variants the program really uses;
- use `RGB565` for display rendering;
- use a `uint16_t` Z-buffer when depth precision is sufficient;
- use `Mesh3Dv2`, \ref tgx::Renderer3D::drawMesh "drawMesh()" and <code>%cacheMesh()</code> for static models;
- keep normals normalized and mesh data well formed;
- enable back-face culling for closed meshes;
- prefer `SHADER_TEXTURE_NEAREST` and `SHADER_TEXTURE_WRAP_POW2` when quality allows it;
- keep textures in faster memory when possible;
- split very large textured faces if texture cache locality is poor;
- clear the image and Z-buffer once per frame, not before every object;
- avoid drawing debug wireframe on top of every frame unless it is needed.


@section sec_3D_complete_example Complete embedded example

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

const tgx::Shader shaders_loaded = tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                                   tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2;

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
    renderer.drawMesh(&my_model, true); // this is when the drawing actually happens !

    // Upload image to the display here....
}
~~~


@section sec_3D_examples Useful examples

Useful starting points:

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

- **Nothing is drawn**: check that `setImage()`, `setViewportSize()` and the shader flags are valid.
- **Geometry appears behind other objects incorrectly**: clear the Z-buffer at the start of the frame.
- **A mesh disappears when culling is enabled**: reverse the culling sign or verify face winding.
- **Textured geometry is missing or drawn without texture**: check that texture shader variants were enabled in
  `LOADED_SHADERS`.
- **Wrapped textures look wrong**: `SHADER_TEXTURE_WRAP_POW2` requires power-of-two texture dimensions.
- **Gouraud lighting looks strange**: verify that normals are normalized and match the model transform.
- **Orthographic and perspective views do not match**: compare the visible height at the object depth and keep the same
  camera/view matrix while switching projection.
