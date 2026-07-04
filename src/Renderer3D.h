/**
 * @file Renderer3D.h
 * Class that manages a 3D scene.
 */
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.

#ifndef _TGX_RENDERER3D_H_
#define _TGX_RENDERER3D_H_

// only C++, no plain C
#ifdef __cplusplus


#include "Misc.h"
#include "Color.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Box2.h"
#include "Box3.h"
#include "Mat4.h"
#include "Image.h"

#include "Shaders.h"
#include "Rasterizer.h"
#include "Renderer3DSpotLightData.h"

#include "Mesh3D.h"
#include "Mesh3Dv2.h"

namespace tgx
{

    // forward declaration for the vertices and faces of the unit cube [-1,1]^3

    extern const tgx::fVec3 UNIT_CUBE_VERTICES[8];
    extern const tgx::fVec3 UNIT_CUBE_NORMALS[6];
    extern const uint16_t UNIT_CUBE_FACES[6*4];
    extern const uint16_t UNIT_CUBE_FACES_NORMALS[6 * 4];


    /**
    * Class for drawing 3D objects onto a `Image` [**MAIN CLASS FOR THE 3D API**].
    *
    * A Renderer3D objects creates a "virtual viewport" and provides a set of methods to manage a scene and
    * draw 3D primitives onto this viewport which is  then mapped to a `tgx::Image`.
    *
    * @tparam LOADED_SHADERS    List of all shaders that may be used. This parameter is required so the renderer only
    *                           compiles the shader paths that are actually needed, improving rendering speed and
    *                           decreasing memory usage significantly. Must be a `|` combination of the following flags:
    *                               - `SHADER_PERSPECTIVE`: enable perspective projection
    *                               - `SHADER_ORTHO`: enable orthographic projection
    *                               - `SHADER_NOZBUFFER`: enable rendering without using a z-buffer
    *                               - `SHADER_ZBUFFER`: enable rendering with a z-buffer
    *                               - `SHADER_UNLIT`: enable unlit shading
    *                               - `SHADER_FLAT`: enable flat shading
    *                               - `SHADER_GOURAUD`: enable Gouraud shading
    *                               - `SHADER_NOTEXTURE`: enable rendering without texturing
    *                               - `SHADER_TEXTURE`: enable rendering with perspective-correct texturing
    *                               - `SHADER_TEXTURE_AFFINE`: enable rendering with affine texturing (faster but lower quality)
    *                               - `SHADER_TEXTURE_NEAREST`: enable rendering with texturing using point sampling
    *                               - `SHADER_TEXTURE_BILINEAR`: enable rendering with texturing using bilinear sampling
    *                               - `SHADER_TEXTURE_WRAP_POW2`: texture can use 'wrap around' mode with dimensions of texture being power of two.
    *                               - `SHADER_TEXTURE_CLAMP`: texture can use 'clamping to edge' mode.
    *
    * @tparam ZBUFFER_t :       Type used for storing z-buffer values. Must be either `float` or `uint16_t`.
    *                           The default is `uint16_t`. The z-buffer must be
    *                           as large as the image (but can be smaller than the viewport when using an offset).
    *                               - `float`: higher quality but requires 4 bytes per pixel.
    *                               - `uint16_t` : lower quality (z-fighting may occur) but only 2 bytes per pixel.
    *
    * @tparam MAX_DIRECTIONAL_LIGHTS : Maximum number of directional lights supported by this renderer.
    *                           The default value `1` preserves the historical one-light rendering path.
    *                           Instantiate with a value larger than 1 to use the advanced directional-light API.
    *
    * @tparam MAX_SPOT_LIGHTS : Maximum number of local spot lights supported by this renderer.
    *                           The default value `0` disables local spot-light support.
    *
    * @remark
    *
    * 1. If a drawing call is made that requires a shader that was not enabled in the template parameter `LOADED_SHADERS` or
    *    if the Renderer3D object state is not valid (e.g. incorrect image size, enabled but missing z-buffer...) then the operation
    *    will fail silently. In particular, if drawing without a Z-buffer is performed, the flag `SHADER_NOZBUFFER` **must** be set
    *    in LOADED_SHADERS. Similarly, if drawing without texturing is performed, the flag `SHADER_NOTEXTURE` **must** be set in LOADED_SHADERS.
    *
    * 2. Z-buffer testing is enabled as soon as a valid z-buffer is provided (with `Renderer3D::setZbuffer()`).
    *    Do not forget to erase the z-buffer with `Renderer3D::clearZbuffer()` at the start of a new frame.
    *
    * 3. Normal vectors are mandatory when using Gouraud shading and must always be normalized (unit length) !
    *
    * 4. Texture dimensions must be powers of two when flag `SHADER_TEXTURE_WRAP_POW2` is set.
    *
    * 5. Back-face culling is set with `Renderer3D::setCulling()`. Triangles and quads must then be provided in the
    *    chosen winding order.
    *
    * 6. It is more efficient to use methods that draw several primitives at once rather than issuing
    *    multiple commands for drawing triangle/quads. For static geometry, `Renderer3D::drawMesh()` should be
    *    used whenever possible instead of `Renderer3D::drawQuads()`, `Renderer3D::drawTriangles()`...
    *
    * 7. Wireframe drawing has a fast aliased path and a one-pixel antialiased path. Full overloads
    *    with adjustable thickness + AA use the general thick-line path and can be very slow.
    *
    */
    template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t = uint16_t, int MAX_DIRECTIONAL_LIGHTS = 1, int MAX_SPOT_LIGHTS = 0>
    class Renderer3D
    {


        static constexpr int MAXVIEWPORTDIMENSION = 2048 * (1 << ((8 - TGX_RASTERIZE_SUBPIXEL_BITS) >> 1)); ///< maximum viewport size in each direction (depending on the value of TGX_RASTERIZE_SUBPIXEL_BITS).

        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");
        static_assert((std::is_same<ZBUFFER_t, float>::value) || (std::is_same<ZBUFFER_t, uint16_t>::value), "The Z-buffer type must be either float or uint16_t");
        static_assert(MAX_DIRECTIONAL_LIGHTS >= 1, "MAX_DIRECTIONAL_LIGHTS must be at least 1");
        static_assert(MAX_SPOT_LIGHTS >= 0, "MAX_SPOT_LIGHTS must be non-negative");

        // true if some kind of texturing may be used.
        static constexpr int ENABLE_TEXTURING = (TGX_SHADER_HAS_ONE_FLAG(LOADED_SHADERS , (SHADER_TEXTURE | SHADER_TEXTURE_AFFINE | TGX_SHADER_MASK_TEXTURE_MODE | TGX_SHADER_MASK_TEXTURE_QUALITY)));
        static constexpr int EXPLICIT_TEXTURE_MODE = (TGX_SHADER_HAS_TEXTURING_ENABLED(LOADED_SHADERS));
        static constexpr Shader DEFAULT_TEXTURE_MODE = EXPLICIT_TEXTURE_MODE ? (Shader)0 : SHADER_TEXTURE;

        static constexpr Shader ENABLED_SHADERS = LOADED_SHADERS | (ENABLE_TEXTURING ? DEFAULT_TEXTURE_MODE : SHADER_NOTEXTURE); // enable perspective texturing by default when only texturing options are set
        // check that disabled shaders do not completely disable all drawing operations.
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_PROJECTION), "At least one of the two shaders SHADER_PERSPECTIVE or SHADER_ORTHO must be enabled");
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_ZBUFFER), "At least one of the two shaders SHADER_NOZBUFFER or SHADER_ZBUFFER must be enabled");
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_SHADING), "At least one of the shaders SHADER_UNLIT, SHADER_FLAT or SHADER_GOURAUD must be enabled");
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE), "At least one of the shaders SHADER_NOTEXTURE, SHADER_TEXTURE or SHADER_TEXTURE_AFFINE must be enabled");
        static_assert((!TGX_SHADER_HAS_TEXTURING_ENABLED(ENABLED_SHADERS)) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE_QUALITY)),"When using texturing, at least one of the two shaders SHADER_TEXTURE_BILINEAR or SHADER_TEXTURE_NEAREST must be enabled");
        static_assert((!TGX_SHADER_HAS_TEXTURING_ENABLED(ENABLED_SHADERS)) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS, TGX_SHADER_MASK_TEXTURE_MODE)), "When using texturing, at least one of the two shaders SHADER_TEXTURE_WRAP_POW2 or SHADER_TEXTURE_CLAMP must be enabled");

        template<Shader SHADERS> static constexpr Shader _validatedDrawCallShaders();



       public:


         /**
         * Constructor.
         *
         * Some parameters may be set right way (but they may be also set independantly later).
         *
         * @param   viewportSize (Optional) Size of the viewport. See `setViewportSize()`.
         * @param   im           (Optional) the destination image. See `setImage()`.
         * @param   zbuffer      (Optional) the Z-buffer. See `setZbuffer()`.
         */
        TGX_NOINLINE Renderer3D(const iVec2& viewportSize = {0,0}, Image<color_t> * im = nullptr, ZBUFFER_t * zbuffer = nullptr);




        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Global settings.
         *
         * Methods use to query and set the global parameters of the renderer: viewport size, zbuffer, projection type...
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/


        /**
        * Set the size of the viewport.
        *
        * The normalized coordinates in `[-1,1]x[-1,1]` are mapped to `[0,lx-1]x[0,ly-1]` just before rasterization.
        *
        * It is possible to use a viewport larger than the image drawn onto by using an offset for the image inside
        * the viewport in order to perform 'tile rendering'. See `setOffset()`.
        *
        * the maximum viewport size depends on `TGX_RASTERIZE_SUBPIXEL_BITS` which specifies the sub-pixel precision
        * value used by the 3D rasterizer:
        *
        *  | subpixel bits | max viewport size LX*LY |
        *  |---------------|-------------------------|
        *  |      8        |      2048 x 2048        |
        *  |      6        |      4096 x 4096        |
        *  |      4        |      8192 x 8192        |
        *  |      2        |     16384 x 16384       |
        *
        * @param lx, ly     The viewport size.
        */
        void setViewportSize(int lx, int ly);


        /**
         * Set the size of the viewport.
         *
         * Same as `setViewportSize(int lx, int ly)`.
         *
         * @param viewport_dim  The viewport size.
        **/
        void setViewportSize(const iVec2& viewport_dim);


        /**
         * Set the image that will be drawn onto.
         *
         * @remark
         * - The image can be smaller than the viewport. In this case, use `setOffset()` to
         *   select the portion of the viewport that will be drawn.
         * - Passing `nullptr` remove the current image (and disables all drawing operation
         *   until a new image is inserted).
         *
         * @param im    the image to draw onto for subsequent renderings.
         */
        void setImage(Image<color_t>* im);


        /**
        * Set the offset of the image relative to the viewport.
        *
        * If the image has size `(sx,sy)`, then during rasterization the portion
        * `[ox, ox + sx[x[oy, oy+sy[` of the viewport will be drawn onto the image.
        *
        * By changing the offset and redrawing several times it it possible to use an image
        * smaller than the viewport (and also save on zbuffer space).
        *
        * For example, to draw a 320x240 viewport with limited amount of memory. One can use an
        * image of size 160x120 (37.5kb) and a z-buffer of the same size (35Kb for uint16_t) and
        * then call the drawing method 4 times with offsets (0,0), (0,120), (160,0) and (160,120)
        * and upload the resulting images at their correct positions on the screen between each rendering.
        *
        * 0warning Do not forget to clear the z-buffer after changing the offset !
        *
        * @param ox, oy     Offset of the image inside the viewport.
        */
        void setOffset(int ox, int oy);


        /**
        * Set the offset of the image relative to the viewport.
        *
        * Same as `setOffset(int ox, int oy)`.
        *
        * @param offset     Offset of the image inside the viewport.
        */
        void setOffset(const iVec2& offset);


        /**
         * Set the projection matrix.
         *
         * This is the matrix that is used to project coordinate from 'view space' to normalized device
         * coordinates (NDC).
         *
         * Call this method to set a "custom" projection matrix. For the usual perspective and
         * orthographic matrices, use instead setFrustum(), setPerspective(), setOrtho().
         *
         * @param M the projection matrix to use (an internal copy is made).
         *
         * @remark
         * - When using perspective projection, the projection matrix must store `-z` into the `w` component.
         * - In view space, the camera is assumed to be centered at the origin, looking looking toward the negative Z axis with the Y axis pointing up (as in opengl).
         *
         * @sa getProjectionMatrix()
         */
        void setProjectionMatrix(const fMat4& M);


        /**
         * Return the current projection matrix.
         *
         * @returns A copy of the current projection matrix.
        **/
        fMat4 getProjectionMatrix() const;


        /**
        * Set projection mode to orthographic (ie no z-divide).
        *
        * @remark This method is called automatically after `setOrtho()` so it needs only be called, when applicable, after `setProjectionMatrix()`.
        */
        void useOrthographicProjection();


        /**
        * Set projection mode to perspective (ie with z-divide).
        *
        * @remark This method is called automatically after `setPerspective()` or `setFrustum()` so it needs only be called, when applicable, after `setProjectionMatrix()`.
        */
        void usePerspectiveProjection();


        /**
         * Set the projection matrix as an orthographic matrix.
         *
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml
         *
         * This method automatically switches to orthographic projection mode by calling useOrthographicProjection().
         *
         * @remark In view space, the camera is assumed to be centered at the origin, looking looking toward the
         * negative Z axis with the Y axis pointing up (as in opengl).
         *
         * @param left, right   coordinates for the left and right vertical clipping planes
         * @param bottom, top   coordinates for the bottom and top horizontal clipping planes.
         * @param zNear, zFar   distances to the nearer and farther depth clipping planes.
         *
         * @sa setFrustum(), setPerspective(), , Mat4::setOrtho()
         */
        void setOrtho(float left, float right, float bottom, float top, float zNear, float zFar);


        /**
         * Set the projection matrix as a perspective matrix.
         *
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
         *
         * This method automatically switches to perspective projection mode by calling usePerspectiveProjection().
         *
         * @remark In view space, the camera is assumed to be centered at the origin, looking looking toward the
         * negative Z axis with the Y axis pointing up (as in opengl).
         *
         * @param left, right   coordinates for the left and right vertical clipping planes
         * @param bottom, top   coordinates for the bottom and top horizontal clipping planes.
         * @param zNear, zFar   distances to the nearer and farther depth clipping planes.
         *
         * @sa setOrtho(), setPerspective(), Mat4::setFrustum()
         */
        void setFrustum(float left, float right, float bottom, float top, float zNear, float zFar);


        /**
         *  Set the projection matrix as a perspective matrix
         *
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
         *
         * This method automatically switches to perspective projection mode by calling usePerspectiveProjection().
         *
         * @remark In view space, the camera is assumed to be centered at the origin, looking looking toward the
         * negative Z axis with the Y axis pointing up (as in opengl).
         *
         * @param   fovy   field of view angle, in degrees, in the y direction.
         * @param   aspect aspect ratio that determines the field of view in the x direction. The aspect ratio is the ratio of x (width) to y (height).
         * @param   zNear  distance from the viewer to the near clipping plane.
         * @param   zFar   distance from the viewer to the far clipping plane.
         *
         * @sa setFrustum(), setOrtho(), Mat4::setPerspective()
         */
        void setPerspective(float fovy, float aspect, float zNear, float zFar);


        /**
        * Set the face culling strategy.
        *
        * @param w  Culling direction
        *           - `w>0`: Vertices in front faces are ordered counter-clockwise [default]. Clockwise faces are culled.
        *           - `w<0`: Vertices in front faces are ordered clockwise. Counter-clockwise faces are culled.
        *           - `w=0`: Disables face culling: both clockwise and counter-clockwise faces are drawn.
        *
        * @remark
        * 1. When face culling is enabled (`w != 0`), and when Gouraud shading is active, the normal vectors
        *    supplied for the vertices must be the normal vectors for the front side of the triangle.
        * 2. When face culling is disabled (`w = 0`). Both faces of a triangle are drawn so there is no more
        *    notion of 'front' and 'back' face. In this case, when using Gouraud shading, by convention, the
        *    normal vector supplied must be those corresponding to the counter-clockwise face being shown
        *    (whatever this means since these normals vector are attached to vertices and not faces anyway,
        *    but still...)
        */
        void setCulling(int w);


        /**
        * Set the z-buffer.
        *
        * @warning The zbuffer must be large enough to be used with the image that is being drawn
        * onto. This means that we must have length at least `image.width()*image.height()`.
        *
        * @param zbuffer    pointer to the z-buffer to use from now one (replace the previous one if any) or nullptr.
        *
        * @remark
        * 1. Setting a valid zbuffer automatically turns on z-buffer depth test.
        * 2. Removing the z-buffer (by setting it to `nullptr`) turns off the z-buffer depth test.
        */
        void setZbuffer(ZBUFFER_t* zbuffer);


        /**
        * Clear the Zbuffer.
        *
        * This method must be called before drawing a new frame to erase the previous zbuffer.
        *
        * @remark The z-buffer is intentionally not cleared between draw() calls to enable
        * rendering of multiple objects on the same scene.
        */
        void clearZbuffer();


        /**
        * Set the shaders to use for subsequent drawing operations.
        *
        * @param shaders    flags to use (or'ed together with `|`).
        *
        * See enum tgx::Shader for a complete list of flags.
        *
        * **Main flags**
        *
        * - `SHADER_UNLIT`:   Use material color or texture color directly, without any lighting computation.
        *
        * - `SHADER_FLAT`:     Use flat shading (i.e. uniform color on faces). This is the fastest drawing method but it usually gives poor results when combined with
        *   texturing. Lighting transitions between bright to dark aera may appear to flicker when using color with low resolution such as RGB565.
        *
        * - `SHADER_GOURAUD`:  Use gouraud shading (linear interpolation of vertex colors) This results in smoother color transitions and works well with texturing but is more CPU
        *    intensive.
        *
        * - `SHADER_TEXTURE`:  enable perspective-correct texture mapping. A texture must be stored in an Image<color_t> object.
        *
        * - `SHADER_TEXTURE_AFFINE`:  enable affine texture mapping. Texture coordinates are interpolated linearly in screen space (fast but lower quality). A texture must be stored in an Image<color_t> object.
        *
        * Texturing also uses:
        *
        * - a wrap mode set with setTextureWrappingMode(), or passed as either `SHADER_TEXTURE_WRAP_POW2` or `SHADER_TEXTURE_CLAMP`.
        *
        * - a sampling quality set with setTextureQuality(), or passed as either `SHADER_TEXTURE_NEAREST` or `SHADER_TEXTURE_BILINEAR`.
        *
        * @remark
        * 1. Every runtime shader choice must be enabled in the template parameter `LOADED_SHADERS`,
        *    including "disabled" choices such as `SHADER_NOTEXTURE` or `SHADER_NOZBUFFER`.
        *    If a shader flag is set with setShaders() but is missing from `LOADED_SHADERS`,
        *    then the drawing calls may silently fail (draw nothing).
        * 2. `SHADER_UNLIT` skips lighting: textured pixels use the texture color directly and untextured pixels use
        *    the current material color.
        * 3. The color on the face (for flat shading) or on the vertices (for gouraud shading) is computed
        *    according to Phong's lighting https://en.wikipedia.org/wiki/Phong_reflection_model.
        * 4. `SHADER_TEXTURE` mapping is perspective correct. `SHADER_TEXTURE_AFFINE` skips perspective correction.
        *    With `SHADER_FLAT` or `SHADER_GOURAUD`, the texture color is combined with the lighting.
        *    With `SHADER_UNLIT`, the texture color is copied directly.
        * 5. Large textures stored in flash memory may be VERY slow to access when the texture is not
        *    read linearly, which happens for some triangle orientations and can make the cache ineffective.
        *    This problem can be somewhat mitigated by:
        *    - splitting large textured triangles into smaller ones: then each triangle only accesses a small part
        *      of the texture. This helps cache locality a lot (this is why models with thousands of faces
        *      may display faster than a simple textured cube in some cases).
        *    - moving the image into RAM if possible. Even moving the texture from FLASH to EXTMEM (if available) will usually give a great speed boost !
        */
        void setShaders(Shader shaders);


        /**
        * Set the wrap mode when for texturing.
        *
        * @param wrap_mode  Wrapping mode flag:
        *                   - `SHADER_TEXTURE_WRAP_POW2`:  Wrap around (repeat) the texture. This is the fastest mode but **the texture size must be a power of two along each dimension**.
        *                   - `SHADER_TEXTURE_CLAMP`:      Clamp to the edge. A bit slower than above but the texture can be any size.
        */
        void setTextureWrappingMode(Shader wrap_mode);


        /**
        * Set the texturing quality.
        *
        * @param quality    Texture quality flag:
        *                   - `SHADER_TEXTURE_NEAREST`:    Use simple point sampling when texturing (fastest method).
        *                   - `SHADER_TEXTURE_BILINEAR`:   Use bilinear interpolation when texturing (slower but higher quality).
        */
        void setTextureQuality(Shader quality);



        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Camera related methods.
         *
         * Methods related to the view matrix and camera position.
         */
         ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
        * Set the view transformation matrix.
        *
        * This matrix is used to transform vertices from 'world coordinates' to 'view coordinates'
        * (ie from the point of view of the camera).
        *
        * Changing this matrix changes the position of the camera in the world space.
        *
        * @remark In view space (i.e. after transformation), the camera is assumed to be
        * centered at the origin, looking toward the negative Z axis with the Y axis pointing up
        * (as in opengl).
        *
        * @param M the view matrix to use (a copy is made).
        *
        * @sa getViewMatrix(), setLookAt()
        */
        void setViewMatrix(const fMat4& M);


        /**
         * Return the current view matrix.
         *
         * @returns a copy of the view matrix.
         *
         * @sa  setViewMatrix(), setLookAt()
        **/
        fMat4 getViewMatrix() const;


        /**
        * Set the view matrix so that the camera is looking at a given direction.
        *
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        *
        * @param eyeX, eyeY, eyeZ          position of the camera in world space coords.
        * @param centerX, centerY, centerZ point the camera is looking toward in world space coords.
        * @param upX, upY, upZ             vector that tells the up direction for the camera (in world space coords).
        *
        * @sa setViewMatrix(), getViewMatrix(), Mat4::setLookAt()
        */
        void setLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);


        /**
        * Set the view matrix so that the camera is looking at a given direction.
        *
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        *
        * @param eye       position of the camera in world space coords.
        * @param center    point the camera is looking toward in world space coords.
        * @param up        vector that tells the up direction for the camera (in world space coords).
        *
        * @sa setViewMatrix(), getViewMatrix(), Mat4::setLookAt()
        **/
        void setLookAt(const fVec3 eye, const fVec3 center, const fVec3 up);


        /**
         * Convert from world coordinates to normalized device coordinates (NDC).
         *
         * @param   P   Point in the world coordinate system.
         *
         * @returns the coordinates of `P` on the standard viewport `[-1,1]^2` according to the current
         *          position of the camera.
         *
         * @remark
         * - the model matrix is not taken into account here.
         * - the `w` value returned can be used for depth testing.
         */
        fVec4 worldToNDC(fVec3 P);


        /**
         * Convert from world coordinates to the corresponding image pixel.
         *
         * @param   P   Point in the world coordinate system.
         *
         * @returns the coordinates of the associated pixel on the image.
         *
         * @remark
         * - The position returned may be outside of the image !
         * - Returns (0,0) if no image is inserted.
         */
        iVec2 worldToImage(fVec3 P);



        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Lighting related methods.
         *
         * Methods related to global ambiant lighting, directional lights and local spot lights.
         *
         * Directional lights are controlled by `MAX_DIRECTIONAL_LIGHTS`. Local spot lights are controlled
         * by `MAX_SPOT_LIGHTS`; when `MAX_SPOT_LIGHTS == 0`, spot light support is not compiled in.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/


        /*****************************************************************************************
         * Directional lights.
         ******************************************************************************************/


         /**
         * Set the scene ambiant light color.
         *
         * See Phong's lighting model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * Ambiant lighting is global and shared by all directional lights.
         *
         * @param   color   color for the global ambiant light.
         *
         * @sa setLight(), setLightDirection(), setLightDiffuse(), setLightSpecular(), setDirectionalLightAmbiant()
         */
        void setLightAmbiant(const RGBf& color);

        /**
         * Set the light source direction of the main directional light of the scene.
         *
         * See Phong's lighting model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * When using multiple directional lights (MAX_DIRECTIONAL_LIGHTS >1), the method controls directional light 0
         * hence is equivalent to `setDirectionalLightDirection(0, direction)`.
         *
         * @param   direction   The direction the light points toward, given in world coordinates.
         *
         * @sa setLight(), setLightAmbiant(), setLightDiffuse(), setLightSpecular(), setDirectionalLightDirection()
         */
        void setLightDirection(const fVec3& direction);




        /**
         * Set the diffuse light color of the main directional light of the scene.
         *
         * See Phong's lighting model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * When using multiple directional lights (MAX_DIRECTIONAL_LIGHTS >1), the method controls directional light 0
         * hence is equivalent to `setDirectionalLightDiffuse(0, color)`.
         *
         * @param   color   color for the diffuse light.
         *
         * @sa setLight(), setLightDirection(), setLightAmbiant(), setLightSpecular(), setDirectionalLightDiffuse()
         */
        void setLightDiffuse(const RGBf& color);


        /**
         * Set the specular light color of the main directional light of the scene.
         *
         * See Phong's lighting model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * When using multiple directional lights (MAX_DIRECTIONAL_LIGHTS >1), the method controls directional light 0
         * hence is equivalent to `setDirectionalLightSpecular(0, color)`.
         *
         * @param   color   color for the specular light.
         *
         * @sa setLight(), setLightDirection(), setLightAmbiant(), setLightDiffuse(), setDirectionalLightSpecular()
         */
        void setLightSpecular(const RGBf& color);


        /**
         * Set all lighting parameters of the main directional light of the scene at once.
         *
         * See Phong's lighting model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * When using multiple directional lights (MAX_DIRECTIONAL_LIGHTS >1), the method controls directional light 0
         * hence is equivalent to calling `setDirectionalLight(0, direction, diffuseColor, specularColor)` with the same ambiant color for all directional lights.
         *
         * @param   direction       direction the light source points toward.
         * @param   ambiantColor    light ambiant color.
         * @param   diffuseColor    light diffuse color.
         * @param   specularColor   light specular color.
         *
         * @sa setLightDirection(), setLightAmbiant(), setLightDiffuse(), setLightSpecular(), setDirectionalLight()
         */
        void setLight(const fVec3 direction, const RGBf& ambiantColor, const RGBf& diffuseColor, const RGBf& specularColor);


        /**
         * Set the number of active directional lights.
         *
         * The count is clamped to `[0, MAX_DIRECTIONAL_LIGHTS]`. A count of 0 disables diffuse and
         * specular directional lighting and leaves only the global ambiant term.
         *
         * Extra directional lights affect rendering only when the renderer is instantiated with
         * `MAX_DIRECTIONAL_LIGHTS > 1`. No shader flag is required for multi-directional lighting.
         *
         * @param count number of active directional lights.
         */
        void setDirectionalLightCount(int count);


        /**
         * Return the number of active directional lights.
         */
        int directionalLightCount() const;


        /**
         * Return the compile-time directional-light capacity.
         */
        static constexpr int maxDirectionalLightCount() { return MAX_DIRECTIONAL_LIGHTS; }


        /**
         * Set the global ambiant light shared by all directional lights.
         *
         * The ambiant lighting is shared by all directional lights.
         *
         * This method is equivalent to `setLightAmbiant()`.
         *
         * @param color color for the ambiant light.
         */
        void setDirectionalLightAmbiant(const RGBf& color);


        /**
         * Set the direction of one directional light.
         *
         * Directions are expressed in world coordinates and use the same convention as `setLightDirection()`:
         * the vector is the direction the light points toward. Directional light 0 is the default light.
         *
         * @param index index of the directional light in `[0, MAX_DIRECTIONAL_LIGHTS - 1]`.
         * @param direction direction the light points toward.
         */
        void setDirectionalLightDirection(int index, const fVec3& direction);


        /**
         * Set the diffuse color of one directional light.
         *
         * Directional light 0 is the default light (also controlled by `setLightDiffuse()`).
         *
         * @param index index of the directional light in `[0, MAX_DIRECTIONAL_LIGHTS - 1]`.
         * @param color diffuse color.
         */
        void setDirectionalLightDiffuse(int index, const RGBf& color);


        /**
         * Set the specular color of one directional light.
         *
         * Directional light 0 is the default light (aslo controlled by `setLightSpecular()`).
         *
         * @param index index of the directional light in `[0, MAX_DIRECTIONAL_LIGHTS - 1]`.
         * @param color specular color.
         */
        void setDirectionalLightSpecular(int index, const RGBf& color);


        /**
         * Configure one directional light at once.
         *
         * This method does not change the global ambiant term and does not change the active light count.
         * Use `setDirectionalLightCount()` to choose how many lights are rendered.
         *
         * @param index index of the directional light in `[0, MAX_DIRECTIONAL_LIGHTS - 1]`.
         * @param direction direction the light points toward.
         * @param diffuseColor diffuse color.
         * @param specularColor specular color.
         */
        void setDirectionalLight(int index, const fVec3& direction, const RGBf& diffuseColor, const RGBf& specularColor);



        /*****************************************************************************************
         * Spot lights.
         ******************************************************************************************/


        /**
         * Set the number of active spot lights.
         *
         * The count is clamped to `[0, MAX_SPOT_LIGHTS]`. A count of 0 disables all local spot lights.
         *
         * Spot lights affect rendering only when the renderer is instantiated with `MAX_SPOT_LIGHTS > 0`.
         * No shader flag is required for spot lighting, but lighting is visible only with flat or Gouraud
         * shading. With `SHADER_UNLIT`, spot lights are ignored.
         *
         * @param count number of active spot lights.
         */
        void setSpotLightCount(int count);


        /**
         * Return the number of active spot lights.
         */
        int spotLightCount() const;


        /**
         * Return the compile-time spot-light capacity.
         */
        static constexpr int maxSpotLightCount() { return MAX_SPOT_LIGHTS; }


        /**
         * Configure one omnidirectional local light.
         *
         * This overload configures a spot-light slot as a point light: the light emits in all directions
         * and does not use a cone test. The position is expressed in world coordinates.
         *
         * The range controls both the maximum influence distance and the smooth distance attenuation.
         * Use a strictly positive value for a finite local light. A range less than or equal to 0 disables
         * distance attenuation and gives the light infinite range (but you usually don't want that!).
         *
         * The local specular contribution is disabled by default. Pass a non-black `specularColor` to enable
         * local specular highlights for this light. The specular exponent is the current material specular
         * exponent.
         *
         * This method does not change the active spot-light count. Use `setSpotLightCount()` to choose how
         * many spot-light slots are rendered.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param position position of the light in world coordinates.
         * @param range maximum influence distance; `range <= 0` means infinite range.
         * @param diffuseColor diffuse color of the light.
         * @param specularColor specular color of the light. The default black color disables local specular.
         */
        void setSpotLight(int index, const fVec3& position, float range,
                          const RGBf& diffuseColor,
                          const RGBf& specularColor = RGBf(0.0f, 0.0f, 0.0f));


        /**
         * Configure one spot light with a hard cone edge.
         *
         * The position and direction are expressed in world coordinates. The direction is the direction the
         * light points toward. The cone is defined by its outer half-angle, in degrees. Points outside the
         * cone are not lit. Use the overload with `innerAngleDeg` to create a soft cone edge.
         *
         * The range controls both the maximum influence distance and the smooth distance attenuation.
         * Use a strictly positive value for a finite local light. A range less than or equal to 0 disables
         * distance attenuation and gives the light infinite range (but you usually don't want that!).
         *
         * The local specular contribution is disabled by default. Pass a non-black `specularColor` to enable
         * local specular highlights for this light. The specular exponent is the current material specular
         * exponent.
         *
         * This method does not change the active spot-light count. Use `setSpotLightCount()` to choose how
         * many spot-light slots are rendered.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param position position of the light in world coordinates.
         * @param direction direction the light points toward, in world coordinates.
         * @param range maximum influence distance; `range <= 0` means infinite range.
         * @param outerAngleDeg outer cone half-angle in degrees. Values greater than or equal to 180 behave
         *                      like an omnidirectional point light.
         * @param diffuseColor diffuse color of the light.
         * @param specularColor specular color of the light. The default black color disables local specular.
         */
        void setSpotLight(int index, const fVec3& position, const fVec3& direction,
                          float range, float outerAngleDeg,
                          const RGBf& diffuseColor,
                          const RGBf& specularColor = RGBf(0.0f, 0.0f, 0.0f));


        /**
         * Configure one spot light with a soft cone edge.
         *
         * The position and direction are expressed in world coordinates. The direction is the direction the
         * light points toward. The cone is defined by two half-angles, in degrees: lighting is full inside
         * `innerAngleDeg`, fades smoothly between `innerAngleDeg` and `outerAngleDeg`, and is zero outside
         * `outerAngleDeg`.
         *
         * The range controls both the maximum influence distance and the smooth distance attenuation.
         * Use a strictly positive value for a finite local light. A range less than or equal to 0 disables
         * distance attenuation and gives the light infinite range (but you usually don't want that!).
         *
         * The local specular contribution is disabled by default. Pass a non-black `specularColor` to enable
         * local specular highlights for this light. The specular exponent is the current material specular
         * exponent.
         *
         * This method does not change the active spot-light count. Use `setSpotLightCount()` to choose how
         * many spot-light slots are rendered.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param position position of the light in world coordinates.
         * @param direction direction the light points toward, in world coordinates.
         * @param range maximum influence distance; `range <= 0` means infinite range.
         * @param outerAngleDeg outer cone half-angle in degrees. Values greater than or equal to 180 behave
         *                      like an omnidirectional point light.
         * @param innerAngleDeg inner cone half-angle in degrees. Values less than 0 or greater than
         *                      `outerAngleDeg` are treated as a hard cone edge.
         * @param diffuseColor diffuse color of the light.
         * @param specularColor specular color of the light. The default black color disables local specular.
         */
        void setSpotLight(int index, const fVec3& position, const fVec3& direction,
                          float range, float outerAngleDeg, float innerAngleDeg,
                          const RGBf& diffuseColor,
                          const RGBf& specularColor = RGBf(0.0f, 0.0f, 0.0f));


        /**
         * Set the position of one spot light.
         *
         * The position is expressed in world coordinates.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param position position of the light in world coordinates.
         */
        void setSpotLightPosition(int index, const fVec3& position);


        /**
         * Set the direction of one spot light.
         *
         * The direction is expressed in world coordinates and points toward the lit area. It is ignored when
         * the light is configured as an omnidirectional point light.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param direction direction the light points toward, in world coordinates.
         */
        void setSpotLightDirection(int index, const fVec3& direction);


        /**
         * Set the diffuse color of one spot light.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param color diffuse color.
         */
        void setSpotLightDiffuse(int index, const RGBf& color);


        /**
         * Set the specular color of one spot light.
         *
         * A black specular color disables the local specular contribution for this light. The specular
         * exponent is the current material specular exponent.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param color specular color (default is black, which disables local specular).
         */
        void setSpotLightSpecular(int index, const RGBf& color = RGBf(0.0f, 0.0f, 0.0f));


        /**
         * Set the range of one spot light.
         *
         * The range controls both the maximum influence distance and the smooth distance attenuation.
         * Use a strictly positive value for a finite local light. A range less than or equal to 0 disables
         * distance attenuation and gives the light infinite range.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param range maximum influence distance; `range <= 0` means infinite range (but you usually don't want that).
         */
        void setSpotLightRange(int index, float range);


        /**
         * Set the cone angles of one spot light.
         *
         * The cone is defined by two half-angles, in degrees: lighting is full inside `innerAngleDeg`,
         * fades smoothly between `innerAngleDeg` and `outerAngleDeg`, and is zero outside `outerAngleDeg`.
         *
         * Values of `outerAngleDeg` greater than or equal to 180 configure the light as an omnidirectional
         * point light. Values of `innerAngleDeg` less than 0 or greater than `outerAngleDeg` create a hard
         * cone edge.
         *
         * @param index index of the spot light in `[0, MAX_SPOT_LIGHTS - 1]`.
         * @param outerAngleDeg outer cone half-angle in degrees.
         * @param innerAngleDeg inner cone half-angle in degrees. The default value creates a hard cone edge.
         */
        void setSpotLightCone(int index, float outerAngleDeg, float innerAngleDeg = -1.0f);




        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Model related methods.
         *
         * Methods related to properties specific to the model being drawn: position, material properties...
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Set the model transformation matrix.
         *
         * This matrix describes the transformation from local object space to view space. (i.e. the
         * matrix specifies the position of the object in world space).
         *
         * @param M the new model transformation matrix (a copy is made).
         *
         * @sa getModelMatrix(), setModelPosScaleRot()
         */
        void setModelMatrix(const fMat4& M);


        /**
         * Return the model transformation matrix.
         *
         * @returns a copy of the current model transformation matrix.
         *
         * @sa  setModelMatrix(), setModelPosScaleRot()
        **/
        fMat4  getModelMatrix() const;


        /**
         * Set the model transformation matrix to move an object to a given location, scale and
         * rotation.
         *
         * The method is such that, if a model is initially centered around the origin in model
         * coordinates, then it will be placed at a given position/scale/rotation in the world
         * coordinates after multiplication by the model transformation matrix.
         *
         * Transforms are done in the following order:
         *
         * 1. The model is scaled in each direction in model coordinates according to `scale`
         * 2. The model is rotated in model coordinates around direction `rot_dir` and with an angle `rot_angle` (in degrees).
         * 3. The model is translated to position `center` in world coordinates.
         *
         * @param   center      new center position after transformation.
         * @param   scale       scaling factor in each direction.
         * @param   rot_angle   rotation angle (in degrees).
         * @param   rot_dir     rotation axis.
         *
         * @sa  getModelMatrix(), setModelMatrix()
         */
        void setModelPosScaleRot(const fVec3& center = fVec3{ 0,0,0 }, const fVec3& scale = fVec3(1, 1, 1), float rot_angle = 0, const fVec3& rot_dir = fVec3{ 0,1,0 });


        /**
        * Convert from model coordinates to normalized device coordinates (NDC).
        *
        * @param P  Point given in the model coordinate system.
        *
        * @returns the projection of `P` on the standard viewport `[-1,1]^2` according
        *          to the current position of the camera.
        *
        * @remark the .w value can be used for depth testing.
        */
        fVec4 modelToNDC(fVec3 P);


        /**
         * Convert from model coordinates to the corresponding image pixel.
         *
         * @param   P   Point given in the model coordinate system.
         *
         * @returns the position of the associated pixel on the image.
         *
         * @remark
         * - The position returned may be outside of the image !
         * - Returns (0,0) if no image is inserted.
         */
        iVec2 modelToImage(fVec3 P);


        /**
         * Set the object material color.
         *
         * This is the color used to render the object when texturing is not used.
         *
         * @param   color   The material color.
         *
         * @sa  setMaterialAmbiantStrength(), setMaterialDiffuseStrength(), setMaterialSpecularStrength(),
         *      setMaterialSpecularExponent(), setMaterial()
        **/
        void setMaterialColor(RGBf color);


        /**
         * Set how much the object material reflects the ambient light.
         *
         * @param   strenght    ambient light reflection strength in [0.0f,1.0f]. Default value 0.1f.
         *
         * @sa  setMaterialColor(), setMaterialDiffuseStrength(),
         *      setMaterialSpecularStrength(), setMaterialSpecularExponent(), setMaterial()
         */
        void setMaterialAmbiantStrength(float strenght = 0.1f);


        /**
         * Set how much the object material reflects the diffuse light.
         *
         * @param   strenght    diffuse light reflection strength in [0.0f,1.0f]. Default value 0.6f.
         *
         * @sa  setMaterialColor(), setMaterialAmbiantStrength(),
         *      setMaterialSpecularStrength(), setMaterialSpecularExponent(), setMaterial()
         */
        void setMaterialDiffuseStrength(float strenght = 0.6f);


        /**
         * Set how much the object material reflects the specular light.
         *
         * @param   strenght    specular light reflection strength in [0.0f,1.0f]. Default value 0.5f.
         *
         * @sa  setMaterialColor(), setMaterialAmbiantStrength(), setMaterialDiffuseStrength(),
         *      setMaterialSpecularExponent(), setMaterial()
         */
        void setMaterialSpecularStrength(float strenght = 0.5f);


        /**
         * Set the object specular exponent.
         *
         * The exponent should be in the range between 0 (no specular lighting) and 100 (very localized/glossy).
         *
         * @param   exponent    Specular exponent in [0,100]. Default value 16.
         *
         * @sa  setMaterialColor(), setMaterialAmbiantStrength(), setMaterialDiffuseStrength(),
         *      setMaterialSpecularStrength(), setMaterial()
         */
        void setMaterialSpecularExponent(int exponent = 16);


        /**
         * Set all the object material properties at once.
         *
         * @param   color               The material color.
         * @param   ambiantStrength     The ambient light reflection strength.
         * @param   diffuseStrength     The diffuse light reflection strength.
         * @param   specularStrength    The specular light reflection strength.
         * @param   specularExponent    The specular exponent.
         *
         * @sa  setMaterialColor(), setMaterialAmbiantStrength(), setMaterialDiffuseStrength(),
         *      setMaterialSpecularStrength(), setMaterialSpecularExponent(),
         */
        void setMaterial(RGBf color, float ambiantStrength, float diffuseStrength, float specularStrength, int specularExponent);








        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing solid geometric primitives.
         *
         * Methods for drawing meshes, triangles and quads.
         *
         * @remark
         * 1. Normal vectors are mandatory when using Gouraud shading and must be normalized (unit length).
         * 2. Texture dimension must be power of two when using flag `SHADER_TEXTURE_WRAP_POW2`.
         * 3. Triangles and quads must be given in the correct winding order. The 4 vertices of a quad must be co-planar.
         * 4. The fastest method to display 'static' geometry is to use the drawMesh() method. Other methods are slower and should be reserved for 'dynamic' geometry.
         * 5. Do not forget to clear the z-buffer with `clearZbuffer()` at the beggining of each new frame.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw a Mesh3D object.
         *
         * @param   mesh                The mesh to draw.
         * @param   use_mesh_material   True (default) to use mesh material, otherwise use the current
         *                              material instead. this flag affects also all the linked meshes
         *                              if `draw_chained_meshes=true`.
         * @param   draw_chained_meshes True (default) to draw also the chained meshes, in any.
         *
         * @remark
         * - Drawing a mesh is the most effective way of drawing a 3D object (faster than drawing individual
         *   triangles/quads) so it should be the preferred method whenever working with static geometry.
         * - The mesh can be located in any memory region (RAM, FLASH...) but using a fast memory will
         *   improve rendering speed noticeably. The methods `cacheMesh()` (or `copyMeshEXTMEM()` on Teensy) are
         *   available to copy a mesh to a faster memory location before rendering.
         */
        void drawMesh(const Mesh3D<color_t>* mesh, bool use_mesh_material = true, bool draw_chained_meshes = true);


        /**
         * Draw a Mesh3Dv2 object.
         *
         * @param   mesh                The mesh to draw.
         * @param   use_mesh_material   True (default) to use Mesh3Dv2 material colors and
         *                              coefficients, otherwise use the current renderer material.
         *
         * @remark
         * Mesh3Dv2 stores geometry in compact meshlets. The renderer first decodes the
         * quantized meshlet sphere/cone metadata, optionally rejects the whole meshlet, then
         * reads and rasterizes the local 16-bit payload only for visible meshlets.
         */
        void drawMesh(const Mesh3Dv2<color_t>* mesh, bool use_mesh_material = true);


        /**
         * Draw a Mesh3Dv2 object using only the shader subset listed in `SHADERS`.
         *
         * This overload is useful when a renderer is compiled with several shader paths but
         * one specific mesh draw call only needs a smaller, known-at-compile-time subset.
         * `SHADERS` must be a complete subset of the renderer enabled shaders and must include
         * one flag from each mandatory category: projection, z-buffer mode, shading mode and
         * texture mode. In particular, include `SHADER_NOTEXTURE` if the mesh may be drawn
         * without texture data. At runtime, the current renderer shader state is intersected
         * with this subset; if the intersection does not select a valid shader path, nothing
         * is drawn.
         *
         * @tparam SHADERS              Compile-time shader subset available to this draw call.
         * @param   mesh                The mesh to draw.
         * @param   use_mesh_material   True (default) to use Mesh3Dv2 material colors and
         *                              coefficients, otherwise use the current renderer material.
         */
        template<Shader SHADERS>
        void drawMesh(const Mesh3Dv2<color_t>* mesh, bool use_mesh_material = true);


        /**
         * Draw a single triangle.
         *
         * @param   P1, P2, P3  coordinates (in model space) of the triangle to draw
         * @param   N1, N2, N3  pointers to the normals associated with `P1, P2, P3` or `nullptr` if not using Gouraud shading.
         * @param   T1, T2, T3  pointer to the texture coords. `nullptr` if not using texturing.
         * @param   texture     pointer to the texture image or `nullptr` if not using texturing.
         *
         * @remark
         * - The triangle is drawn with the current color/material.
         * - `P1,P2,P3` must be in the correct winding order (c.f. setCulling()).
         * - the normals `N1,N2,N3` are mandatory with Gouraud shading and must have unit norm.
         */
        void drawTriangle(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                          const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr,
                          const fVec2 * T1 = nullptr, const fVec2 * T2 = nullptr, const fVec2 * T3 = nullptr,
                          const Image<color_t> * texture = nullptr);


        /**
         * Draw a single triangle with a given colors on each of its vertices.
         *
         * @param   P1, P2, P3          coordinates (in model space) of the triangle to draw
         * @param   col1, col2, col3    color at each vertex.
         * @param   N1, N2, N3          pointers to the normals associated with `P1, P2, P3` or `nullptr` if not using Gouraud shading.
         *
         * @remark
         * - The color inside the triangle is obtained by linear interpolation.
         * - `P1,P2,P3` must be in the correct winding order (c.f. setCulling()).
         * - the normals `N1,N2,N3` are mandatory with Gouraud shading and must have unit norm.
         */
        void drawTriangleWithVertexColor(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                                         const RGBf & col1, const RGBf & col2, const RGBf & col3,
                                         const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr);


        /**
         * Draw a collection of triangles.
         *
         * @param   nb_triangles    number of triangles to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_triangles*3` and each 3 consecutive values represent a triangle.
         * @param   vertices        The array of vertices (in model space).
         * @param   ind_normals     Array of normal indexes. If specified, the array must have length `nb_triangles*3`.
         * @param   normals         The array of normals vectors (in model space).
         * @param   ind_texture     array of texture indexes. If specified, the array must have length `nb_triangles*3`.
         * @param   textures        The array of texture coords.
         * @param   texture_image   The texture image to use or `nullptr` if not used
         *
         * @remark
         * - If Gouraud shading is enabled, the normal vectors must all have unit norm.
         * - If Gouraud shading is disabled, `ind_normals` and `normals` should be set to `nullptr`.
         * - If texturing is disabled, `ind_texture`, `textures` and `texture_image` should be set to `nullptr`.
         * - If texturing is disabled, the current material color is used to draw the triangles.
         */
        void drawTriangles(int nb_triangles,
                           const uint16_t * ind_vertices, const fVec3 * vertices,
                           const uint16_t * ind_normals = nullptr, const fVec3* normals = nullptr,
                           const uint16_t * ind_texture = nullptr, const fVec2* textures = nullptr,
                           const Image<color_t> * texture_image = nullptr);


        /**
         * Draw a triangle strip.
         *
         * @param   nb_indices      Number of indexes in the strip.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_indices`.
         * @param   vertices        The array of vertices (in model space).
         * @param   ind_normals     Array of normal indexes. If specified, the array must have length `nb_indices`.
         * @param   normals         The array of normals vectors (in model space).
         * @param   ind_texture     Array of texture indexes. If specified, the array must have length `nb_indices`.
         * @param   textures        The array of texture coords.
         * @param   texture_image   The texture image to use or `nullptr` if not used.
         *
         * @remark
         * - The first triangle is `(0,1,2)`, then winding alternates like OpenGL triangle strips:
         *   `(2,1,3)`, `(2,3,4)`, `(4,3,5)`...
         * - Repeated vertex indexes create degenerate triangles that are skipped. This can be used
         *   to bridge parts of a strip without drawing connecting triangles.
         * - If Gouraud shading is enabled, the normal vectors must all have unit norm.
         * - If Gouraud shading is disabled, `ind_normals` and `normals` should be set to `nullptr`.
         * - If texturing is disabled, `ind_texture`, `textures` and `texture_image` should be set to `nullptr`.
         * - If texturing is disabled, the current material color is used to draw the strip.
         */
        void drawTriangleStrip(int nb_indices,
                               const uint16_t* ind_vertices, const fVec3* vertices,
                               const uint16_t* ind_normals = nullptr, const fVec3* normals = nullptr,
                               const uint16_t* ind_texture = nullptr, const fVec2* textures = nullptr,
                               const Image<color_t>* texture_image = nullptr);



        /**
         * Draw a single quad.
         *
         * **The four vertices of the quad must be co-planar !**
         *
         * @param P1, P2, P3, P4    coordinates (in model space) of the quad to draw
         * @param N1, N2, N3, N4    pointers to the normals associated with `P1, P2, P3, P4` or `nullptr` if not using Gouraud shading.
         * @param T1, T2, T3, T4    pointer to the texture coords. `nullptr` if not using texturing.
         * @param texture           pointer to the texture image or `nullptr` if not using texturing.
         *
         * @remark
         * - The quad is drawn with the current color/material.
         * - `P1,P2,P3,P4` must be in the correct winding order (c.f. setCulling()).
         * - the normals `N1,N2,N3,N4` are mandatory with Gouraud shading and must have unit norm.
         */
        void drawQuad(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3 & P4,
                      const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr, const fVec3 * N4 = nullptr,
                      const fVec2 * T1 = nullptr, const fVec2 * T2 = nullptr, const fVec2 * T3 = nullptr, const fVec2 * T4 = nullptr,
                      const Image<color_t>* texture = nullptr);



        /**
         * Draw a single quad with a given colors on each of its four vertices.
         *
         * **The four vertices of the quad must be co-planar !**
         *
         * @param   P1, P2, P3,P4           coordinates (in model space) of the quad to draw
         * @param   col1, col2, col3, col4  color at each vertex.
         * @param   N1, N2, N3, N4          pointers to the normals associated with `P1, P2, P3, P4` or `nullptr` if not using Gouraud shading.
         *
         * @remark
         * - The color inside the quad is obtained by linear interpolation.
         * - `P1,P2,P3,P4` must be in the correct winding order (c.f. setCulling()).
         * - the normals `N1,N2,N3,N4` are mandatory with Gouraud shading and must have unit norm.
         *
         */
        void drawQuadWithVertexColor(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3 & P4,
                                     const RGBf & col1, const RGBf & col2, const RGBf & col3, const RGBf & col4,
                                     const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr, const fVec3 * N4 = nullptr);




        /**
         * Draw a collection of quads.
         *
         * **The four vertices of a quad must always be co-planar !**
         *
         * @param   nb_quads        number of quads to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_quads*4` and each 4 consecutive values represent a quad.
         * @param   vertices        The array of vertices (in model space).
         * @param   ind_normals     Array of normal indexes. If specified, the array must have length `nb_quads*4`.
         * @param   normals         The array of normals vectors (in model space).
         * @param   ind_texture     array of texture indexes. If specified, the array must have length `nb_quads*4`.
         * @param   textures        The array of texture coords.
         * @param   texture_image   The texture image to use or `nullptr` if not used
         *
         * @remark
         * - If Gouraud shading is enabled, the normal vectors must all have unit norm.
         * - If Gouraud shading is disabled, `ind_normals` and `normals` should be set to `nullptr`.
         * - If texturing is disabled, `ind_texture`, `textures` and `texture_image` should be set to `nullptr`.
         * - If texturing is disabled, the current material color is used to draw the quads.
        **/
        void drawQuads(int nb_quads,
                       const uint16_t * ind_vertices, const fVec3 * vertices,
                       const uint16_t * ind_normals = nullptr, const fVec3* normals = nullptr,
                       const uint16_t * ind_texture = nullptr, const fVec2* textures = nullptr,
                       const Image<color_t>* texture_image = nullptr);




        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing solid basic shapes.
         *
         * Methods for drawing cube and spheres.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw the unit cube `[-1,1]^3` in model space.
         *
         * @remark The model transform matrix may be used to scale, rotate and position the cube
         * anywhere in world space.
         */
        void drawCube();


        /**
         * Draw a textured unit cube `[-1,1]^3` in model space.
         *
         * ```
         *                                                                   H--------E
         *                                                                   |        |
         *                                                                   |  top   |
         *                          H-------------E                          |        |
         *                         /.            /|                 H--------A--------D--------E
         *                        / .   top     / |                 |        |        |        |
         *                       /  .          /  |                 |  left  | front  |  right |
         *                      A------------D    |  right          |        |        |        |
         *                      |   .        |    |                 G--------B--------C--------F
         *                      |   G .......|....F                          |        |
         *                      |  .         |   /                           | bottom |
         *                      | .  front   |  /                            |        |
         *                      |.           | /                             G--------F
         *                      B------------C                               |        |
         *                                                                   |  back  |
         *                                                                   |        |
         *                                                                   H--------E
         * ```
         *
         * @remark
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.
         * - Each face may use a different texture (or set the image to `nullptr` to disable texturing a face).
         *
         * @param   v_front_ABCD    texture coords array for the front face in order ABCD
         * @param   texture_front   texture for the front face
         * @param   v_back_EFGH     texture coords array for the back face in order EFGH
         * @param   texture_back    texture for the back face
         * @param   v_top_HADE      texture coords array for the top face in order HADE
         * @param   texture_top     texture for the top face
         * @param   v_bottom_BGFC   texture coords array for the bottom face in order BGFC
         * @param   texture_bottom  texture for the bottom face
         * @param   v_left_HGBA     texture coords array for the left face in order HGBA
         * @param   texture_left    texture for the left face
         * @param   v_right_DCFE    texture coords array for the right face in order DCFE
         * @param   texture_right   texture for the right face
         */
        void drawCube(
            const fVec2 v_front_ABCD[4] , const Image<color_t>* texture_front,
            const fVec2 v_back_EFGH[4]  , const Image<color_t>* texture_back,
            const fVec2 v_top_HADE[4]   , const Image<color_t>* texture_top,
            const fVec2 v_bottom_BGFC[4], const Image<color_t>* texture_bottom,
            const fVec2 v_left_HGBA[4]  , const Image<color_t>* texture_left,
            const fVec2 v_right_DCFE[4] , const Image<color_t>* texture_right
            );


        /**
         * draw a textured unit cube [-1,1]^3 (in model space)
         *
         * ```
         *                                                                   H--------E
         *                                                                   |        |
         *                                                                   |  top   |
         *                          H-------------E                          |        |
         *                         /.            /|                 H--------A--------D--------E
         *                        / .   top     / |                 |        |        |        |
         *                       /  .          /  |                 |  left  | front  |  right |
         *                      A------------D    |  right          |        |        |        |
         *                      |   .        |    |                 G--------B--------C--------F
         *                      |   G .......|....F                          |        |
         *                      |  .         |   /                           | bottom |
         *                      | .  front   |  /                            |        |
         *                      |.           | /                             G--------F
         *                      B------------C                               |        |
         *                                                                   |  back  |
         *                                                                   |        |
         *                                                                   H--------E
         * ```
         *
         * @remark
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.
         * - Each face uses a 'whole' image. Set the texture image to `nullptr` to disable texturing a given face.
         *
         * @param   texture_front   texture for the front face.
         * @param   texture_back    texture for the back face.
         * @param   texture_top     texture for the top face.
         * @param   texture_bottom  texture for the bottom face.
         * @param   texture_left    texture for the left face.
         * @param   texture_right   texture for the right face.
         */
        void drawCube(
            const Image<color_t>* texture_front,
            const Image<color_t>* texture_back,
            const Image<color_t>* texture_top,
            const Image<color_t>* texture_bottom,
            const Image<color_t>* texture_left,
            const Image<color_t>* texture_right
            );


        /**
         * Draw a textured sky-box around the current camera.
         *
         * This method uses the same unit-cube geometry and face/texture-coordinate conventions
         * as drawCube(), but it renders through a minimal unlit textured path without z-buffer
         * testing/writing and without far-plane clipping. It is intended for sky-box/background
         * rendering and should be called before drawing the z-buffered scene.
         *
         * Face naming and layout:
         *
         * ```
         *                                                                   H--------E
         *                                                                   |        |
         *                                                                   |  top   |
         *                          H-------------E                          |        |
         *                         /.            /|                 H--------A--------D--------E
         *                        / .   top     / |                 |        |        |        |
         *                       /  .          /  |                 |  left  | front  |  right |
         *                      A------------D    |  right          |        |        |        |
         *                      |   .        |    |                 G--------B--------C--------F
         *                      |   G .......|....F                          |        |
         *                      |  .         |   /                           | bottom |
         *                      | .  front   |  /                            |        |
         *                      |.           | /                             G--------F
         *                      B------------C                               |        |
         *                                                                   |  back  |
         *                                                                   |        |
         *                                                                   H--------E
         * ```
         *
         * @remark
         * - The current model transform, material, culling and shader state are ignored.
         * - `rot_angle_y` rotates the sky-box around the world Y axis.
         * - `skybox_radius` is the half-size of the cube in world units.
         * - `reference_height` is the world-space height of the sky-box vertical center.
         * - The renderer ignores the current camera X/Z translation, but uses the current camera
         *   height so the sky-box floor/ceiling can stay tied to `reference_height`. Large radii
         *   make this vertical parallax very small and make the sky-box look farther away.
         * - `nullptr` texture faces are skipped.
         *
         * @param   v_front_ABCD    texture coords array for the front face in order ABCD.
         * @param   texture_front   texture for the front face.
         * @param   v_back_EFGH     texture coords array for the back face in order EFGH.
         * @param   texture_back    texture for the back face.
         * @param   v_top_HADE      texture coords array for the top face in order HADE.
         * @param   texture_top     texture for the top face.
         * @param   v_bottom_BGFC   texture coords array for the bottom face in order BGFC.
         * @param   texture_bottom  texture for the bottom face.
         * @param   v_left_HGBA     texture coords array for the left face in order HGBA.
         * @param   texture_left    texture for the left face.
         * @param   v_right_DCFE    texture coords array for the right face in order DCFE.
         * @param   texture_right   texture for the right face.
         * @param   rot_angle_y     additional sky-box rotation around the world Y axis, in degrees.
         * @param   reference_height world-space height of the sky-box vertical center.
         * @param   skybox_radius   sky-box cube half-size in world units.
         * @param   texture_quality either `SHADER_TEXTURE_NEAREST` or `SHADER_TEXTURE_BILINEAR`.
         * @param   texture_mode    either `SHADER_TEXTURE_WRAP_POW2` or `SHADER_TEXTURE_CLAMP`.
         */
        void drawSkyBox(
            const fVec2 v_front_ABCD[4] , const Image<color_t>* texture_front,
            const fVec2 v_back_EFGH[4]  , const Image<color_t>* texture_back,
            const fVec2 v_top_HADE[4]   , const Image<color_t>* texture_top,
            const fVec2 v_bottom_BGFC[4], const Image<color_t>* texture_bottom,
            const fVec2 v_left_HGBA[4]  , const Image<color_t>* texture_left,
            const fVec2 v_right_DCFE[4] , const Image<color_t>* texture_right,
            float rot_angle_y = 0.0f,
            float reference_height = 0.0f,
            float skybox_radius = 32768.0f,
            Shader texture_quality = SHADER_TEXTURE_NEAREST,
            Shader texture_mode = SHADER_TEXTURE_CLAMP
            );


        /**
         * Draw a textured sky-box using whole images for each face.
         *
         * Texture coordinates are generated with the same convention as drawCube().
         */
        void drawSkyBox(
            const Image<color_t>* texture_front,
            const Image<color_t>* texture_back,
            const Image<color_t>* texture_top,
            const Image<color_t>* texture_bottom,
            const Image<color_t>* texture_left,
            const Image<color_t>* texture_right,
            float rot_angle_y = 0.0f,
            float reference_height = 0.0f,
            float skybox_radius = 32768.0f,
            Shader texture_quality = SHADER_TEXTURE_NEAREST,
            Shader texture_mode = SHADER_TEXTURE_CLAMP
            );



        /**
         * Draw a unit radius sphere centered at the origin `S(0,1)` in model space.
         *
         * @remark
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere with a given number of sectors and stacks.
         *
         * @param   nb_sectors  number of sectors of the UV sphere.
         * @param   nb_stacks   number of stacks of the UV sphere.
         */
        void drawSphere(int nb_sectors, int nb_stacks);


        /**
         * Draw a textured unit radius sphere centered at the origin S(0,1) in model space.
         *
         * @remark
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere with a given number of sectors and stacks.
         * - The texture is mapped using the Mercator projection.
         *
         * @param   nb_sectors  number of sectors of the UV sphere.
         * @param   nb_stacks   number of stacks of the UV sphere.
         * @param   texture     The texture (mapped via Mercator projection)
         */
        void drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture);


        /**
         * Draw a unit radius sphere centered at the origin S(0,1) in model space.
         *
         * @remark
         * - The model transform matrix may be used to position the sphere anywhere in world space and
         * change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sectors and stacks is adjusted
         * automatically according to the apparent size on the screen.
         *
         * @param   quality Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         *                  - `1` : default quality
         *                  - `>1`: finer mesh. Improve quality but decrease speed.
         *                  - `<1`: coarser mesh. Decrease quality but improve speed.
         */
        void drawAdaptativeSphere(float quality = 1.0f);


        /**
         * Draw a textured unit radius sphere centered at the origin S(0,1) in model space.
         *
         * @remark
         * - The model transform matrix may be used to position the sphere anywhere in world space
         *   and change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sectors and stacks is adjusted
         *   automatically according to the apparent size on the screen.
         * - The texture is mapped using the Mercator projection.
         *
         * @param   texture The texture image mapped via Mercator projection.
         * @param   quality (Optional) Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         *                  - `1` : default quality
         *                  - `>1`: finer mesh. Improve quality but decrease speed.
         *                  - `<1`: coarser mesh. Decrease quality but improve speed.
         */
        void drawAdaptativeSphere(const Image<color_t>* texture, float quality = 1.0f);




        /**
         * Draw a unit cylinder in model space.
         *
         * The cylinder is centered at the origin, has radius `1`, and extends from
         * `y = -1` to `y = 1`. The model transform matrix may be used to scale,
         * rotate and position it anywhere in world space.
         *
         * @param nb_sectors  Number of sectors used around the cylinder. minimum `3`.
         * @param bottom_cap  Draw the bottom disk at `y = -1`.
         * @param top_cap     Draw the top disk at `y = 1`.
         *
         * @remark
         * - If both caps are present, the primitive is closed and uses the usual local
         *   culling convention for generated solids.
         * - If a cap is missing, culling is disabled locally while drawing this primitive
         *   so the inside remains visible. The renderer culling state is restored before
         *   returning.
         */
        void drawCylinder(int nb_sectors, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a textured unit cylinder in model space.
         *
         * @param nb_sectors      Number of sectors used around the cylinder.
         * @param texture_side    Texture for the lateral surface, or `nullptr` to draw it without texturing.
         * @param texture_bottom  Texture for the bottom disk, or `nullptr` to draw it without texturing.
         * @param texture_top     Texture for the top disk, or `nullptr` to draw it without texturing.
         * @param bottom_cap      Draw the bottom disk at `y = -1`.
         * @param top_cap         Draw the top disk at `y = 1`.
         *
         * @remark
         * - The side texture is unwrapped with `u` around the cylinder and `v` from bottom (`0`) to top (`1`).
         * - Cap textures use a radial projection of the disk into the full texture image.
         * - A `nullptr` texture disables texturing for that part only; use `bottom_cap` and `top_cap` to remove cap geometry.
         */
        void drawCylinder(int nb_sectors, const Image<color_t>* texture_side, const Image<color_t>* texture_bottom = nullptr, const Image<color_t>* texture_top = nullptr, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a unit cone in model space.
         *
         * The cone is centered on the Y axis, with base radius `1` at `y = -1` and
         * apex at `y = 1`.
         *
         * @param nb_sectors  Number of sectors used around the cone.
         * @param bottom_cap  Draw the bottom disk at `y = -1`.
         */
        void drawCone(int nb_sectors, bool bottom_cap = true);


        /**
         * Draw a textured unit cone in model space.
         *
         * @param nb_sectors      Number of sectors used around the cone.
         * @param texture_side    Texture for the lateral surface, or `nullptr` to draw it without texturing.
         * @param texture_bottom  Texture for the bottom disk, or `nullptr` to draw it without texturing.
         * @param bottom_cap      Draw the bottom disk at `y = -1`.
         *
         * @remark The side and cap texture mapping follows the same convention as `drawCylinder()`.
         */
        void drawCone(int nb_sectors, const Image<color_t>* texture_side, const Image<color_t>* texture_bottom = nullptr, bool bottom_cap = true);


        /**
         * Draw a truncated cone in model space.
         *
         * The shape is centered on the Y axis and extends from `y = -1` to `y = 1`.
         * `bottom_radius` is the radius of the bottom ring and `top_radius` is the
         * radius of the top ring. A radius less than or equal to zero creates an apex.
         *
         * @param nb_sectors      Number of sectors used around the shape.
         * @param bottom_radius   Radius of the bottom ring at `y = -1`.
         * @param top_radius      Radius of the top ring at `y = 1`.
         * @param bottom_cap      Draw the bottom disk when `bottom_radius > 0`.
         * @param top_cap         Draw the top disk when `top_radius > 0`.
         */
        void drawTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a textured truncated cone in model space.
         *
         * @param nb_sectors      Number of sectors used around the shape.
         * @param bottom_radius   Radius of the bottom ring at `y = -1`.
         * @param top_radius      Radius of the top ring at `y = 1`.
         * @param texture_side    Texture for the lateral surface, or `nullptr` to draw it without texturing.
         * @param texture_bottom  Texture for the bottom disk, or `nullptr` to draw it without texturing.
         * @param texture_top     Texture for the top disk, or `nullptr` to draw it without texturing.
         * @param bottom_cap      Draw the bottom disk when `bottom_radius > 0`.
         * @param top_cap         Draw the top disk when `top_radius > 0`.
         *
         * @remark The side and cap texture mapping follows the same convention as `drawCylinder()`.
         */
        void drawTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, const Image<color_t>* texture_side, const Image<color_t>* texture_bottom = nullptr, const Image<color_t>* texture_top = nullptr, bool bottom_cap = true, bool top_cap = true);






        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing wireframe geometric primitives.
         *
         * Methods for drawing wireframe meshes, triangles and quads.
         *
         * Two versions for each method:
         * 1. Fast drawing: no thickness, no blending, no anti-aliasing.
         * 2. Antialiased drawing: use the explicit `drawWireFrame...AA()` methods for optimized
         *    one-pixel antialiased lines, or the full overloads with adjustable thickness + AA.
         *    The latter use the general thick-line path and are very slow.
         *
         * @remark Wireframe methods do not take the scene lighting into account.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw a mesh in wireframe [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The mesh is drawn with the current material color (not that of the mesh). This method does not
         *   require a zbuffer but back face culling is used if it is enabled.
         *
         * @param   mesh                The mesh to draw
         * @param   draw_chained_meshes True to draw also the chained meshes.
        **/
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes = true);


        /**
         * Draw a Mesh3Dv2 object in wireframe [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The mesh is drawn with the current material color, not with the Mesh3Dv2 materials.
         * - This method does not require a zbuffer but back face culling is used if it is enabled.
         * - Meshlet visibility cones are used when `TGX_MESHLET_WIREFRAME_CULL` is enabled.
         *
         * @param   mesh                The Mesh3Dv2 object to draw.
        **/
        void drawWireFrameMesh(const Mesh3Dv2<color_t>* mesh);


        /**
         * Draw a mesh in wireframe [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   mesh                The mesh to draw.
         * @param   draw_chained_meshes True to draw also the chained meshes.
        **/
        void drawWireFrameMeshAA(const Mesh3D<color_t>* mesh, bool draw_chained_meshes = true);


        /**
         * Draw a Mesh3Dv2 object in wireframe [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   mesh        The Mesh3Dv2 object to draw.
        **/
        void drawWireFrameMeshAA(const Mesh3Dv2<color_t>* mesh);


        /**
         * Draw a mesh in wireframe [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - It is very slow compared with `drawWireFrameMesh()` and `drawWireFrameMeshAA()`.
         * - This method does not require a zbuffer but back face culling is used if it is enabled.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   mesh                The mesh to draw
         * @param   draw_chained_meshes True to draw also the chained meshes.
         * @param   thickness           thickness of the lines.
         * @param   color               color to use.
         * @param   opacity             opacity multiplier in [0.0f, 1.0f].
        **/
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness, color_t color, float opacity);


        /**
         * Draw a Mesh3Dv2 object in wireframe [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - It is very slow compared with `drawWireFrameMesh()` and `drawWireFrameMeshAA()`.
         * - This method does not require a zbuffer but back face culling is used if it is enabled.
         * - Meshlet visibility cones are used when `TGX_MESHLET_WIREFRAME_CULL` is enabled.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   mesh                The Mesh3Dv2 object to draw.
         * @param   thickness           thickness of the lines.
         * @param   color               color to use.
         * @param   opacity             opacity multiplier in [0.0f, 1.0f].
        **/
        void drawWireFrameMesh(const Mesh3Dv2<color_t>* mesh, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe line segment [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The line is drawn with the current material color.
         *
         * @param   P1,P2  endpoints in model space.
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2);


        /**
         * Draw a wireframe line segment [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   P1,P2       endpoints in model space.
        **/
        void drawWireFrameLineAA(const fVec3& P1, const fVec3& P2);


        /**
         * Draw a wireframe line segment [*adjustable thickness + AA*].
         *
         * @remark This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   P1,P2       endpoints in model space.
         * @param   thickness   thickness of the line
         * @param   color       color to use
         * @param   opacity     opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe line segments [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         *
         * @param   nb_lines        number of lines to draw
         * @param   ind_vertices    array of vertex indices. The length of the array is `nb_lines*2` and each 2 consecutive values represent a line segment.
         * @param   vertices        The array of vertices in model space.
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw wireframe line segments [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   nb_lines        number of lines to draw.
         * @param   ind_vertices    array of vertex indices.
         * @param   vertices        The array of vertices in model space.
        **/
        void drawWireFrameLinesAA(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe line segments [*adjustable thickness + AA*].
         *
         * @remark This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   nb_lines        number of lines to draw
         * @param   ind_vertices    array of vertex indices. The length of the array is `nb_lines*2` and each 2 consecutive values represent a line segment.
         * @param   vertices        The array of vertices in model space.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe triangle [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @param   P1, P2, P3      the triangle vertices in model space.
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3);


        /**
         * Draw a wireframe triangle [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   P1, P2, P3      triangle vertices in model space.
        **/
        void drawWireFrameTriangleAA(const fVec3& P1, const fVec3& P2, const fVec3& P3);


        /**
         * Draw a wireframe triangle [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   P1, P2, P3      triangle vertices in model space.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe triangles [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @param   nb_triangles    number of triangles to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_triangles*3` and each 3 consecutive values represent a triangle.
         * @param   vertices        Array of vertices in model space.
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw wireframe triangles [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   nb_triangles    number of triangles to draw.
         * @param   ind_vertices    Array of vertex indexes.
         * @param   vertices        Array of vertices in model space.
        **/
        void drawWireFrameTrianglesAA(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe triangles [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   nb_triangles    number of triangles to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_triangles*3` and each 3 consecutive values represent a triangle.
         * @param   vertices        Array of vertices in model space.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity);


        /**
         * Draw a triangle strip in wireframe [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method uses the same strip winding convention as `drawTriangleStrip()`.
         * - Repeated vertex indexes create degenerate triangles that are skipped.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @param   nb_indices      Number of indexes in the strip.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_indices`.
         * @param   vertices        Array of vertices in model space.
        **/
        void drawWireFrameTriangleStrip(int nb_indices, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a triangle strip in wireframe [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         * - This method uses the same strip winding convention as `drawTriangleStrip()`.
         * - Repeated vertex indexes create degenerate triangles that are skipped.
         *
         * @param   nb_indices      Number of indexes in the strip.
         * @param   ind_vertices    Array of vertex indexes.
         * @param   vertices        Array of vertices in model space.
        **/
        void drawWireFrameTriangleStripAA(int nb_indices, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a triangle strip in wireframe [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - This method uses the same strip winding convention as `drawTriangleStrip()`.
         * - Repeated vertex indexes create degenerate triangles that are skipped.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   nb_indices      Number of indexes in the strip.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_indices`.
         * @param   vertices        Array of vertices in model space.
         * @param   thickness       Thickness of the lines.
         * @param   color           Color to use.
         * @param   opacity         Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawWireFrameTriangleStrip(int nb_indices, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe quad [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of each quad must be co-planar.
         *
         * @param   P1, P2, P3,P4       the quad vertices in model space.
        **/
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4);


        /**
         * Draw a wireframe quad [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   P1, P2, P3,P4   the quad vertices in model space.
         */
        void drawWireFrameQuadAA(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4);


        /**
         * Draw a wireframe quad [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of each quad must be co-planar.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   P1, P2, P3,P4       the quad vertices in model space.
         * @param   thickness           thickness of the lines
         * @param   color               color to use
         * @param   opacity             opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe quads [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of each quad must be co-planar.
         *
         * @param   nb_quads        number of quads to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_quads*4` and each 4 consecutive values represent a quad.
         * @param   vertices        Array of vertices in model space.
        */
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw wireframe quads [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   nb_quads        number of quads to draw.
         * @param   ind_vertices    Array of vertex indexes.
         * @param   vertices        Array of vertices in model space.
         */
        void drawWireFrameQuadsAA(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe quads [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of each quad must be co-planar.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   nb_quads        number of quads to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_quads*4` and each 4 consecutive values represent a quad.
         * @param   vertices        Array of vertices in model space.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity);




        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing basic shapes in wireframe.
         *
         * Methods for drawing wireframe cubes, spheres, cylinders and cones.
         *
         * Two versions for each method:
         * 1. Fast drawing: no thickness, no blending, no anti-aliasing.
         * 2. Antialiased drawing: use the explicit `drawWireFrame...AA()` methods for optimized
         *    one-pixel antialiased lines, or the full overloads with adjustable thickness + AA.
         *    The latter use the general thick-line path and are very slow.
         *
         * @remark Wireframe methods do not take the scene lighting into account.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/


        /**
         * Draw the wireframe cube [0,1]^3 (in model space) [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.
         */
        void drawWireFrameCube();


        /**
         * Draw the wireframe cube [0,1]^3 [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         */
        void drawWireFrameCubeAA();


        /**
         * Draw the wireframe cube [0,1]^3 (in model space) [*adjustable thickness + AA*].
         *
         * @remark
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameCube(float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*fast*].
         *
         * @remark
         * - Creates a UV-sphere with a given number of sectors and stacks.
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         *
         * @param   nb_sectors  number of sectors in the UV sphere.
         * @param   nb_stacks   number of stacks in the UV sphere.
         */
        void drawWireFrameSphere(int nb_sectors, int nb_stacks);


        /**
         * Draw a wireframe unit radius sphere [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param   nb_sectors      number of sectors in the UV sphere.
         * @param   nb_stacks       number of stacks in the UV sphere.
        **/
        void drawWireFrameSphereAA(int nb_sectors, int nb_stacks);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*adjustable thickness + AA*].
         *
         * @remark
         * - Creates a UV-sphere with a given number of sectors and stacks.
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param   nb_sectors      number of sectors in the UV sphere.
         * @param   nb_stacks       number of stacks in the UV sphere.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameSphere(int nb_sectors, int nb_stacks, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*fast*].
         *
         * @remark
         * - This method uses fast drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sectors and stacks is adjusted automatically according to the apparent size on the screen.
         *
         * @param quality   Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         *                  - `1` : default quality
         *                  - `>1`: finer mesh. Improve quality but decrease speed.
         *                  - `<1`: coarser mesh. Decrease quality but improve speed.
         */
        void drawWireFrameAdaptativeSphere(float quality = 1.0f);


        /**
         * Draw an adaptive wireframe sphere [*antialiased*] with the current material color.
         *
         * @remark
         * - This method uses the optimized one-pixel antialiased wireframe path.
         * - Use the adjustable thickness + AA overload only when line thickness or opacity is needed.
         *
         * @param quality       Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         */
        void drawWireFrameAdaptativeSphereAA(float quality = 1.0f);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*adjustable thickness + AA*].
         *
         * @remark
         * - The model transform matrix may be used to position the sphere anywhere in world space and change it to an ellipsoid.
         * - This method uses the general adjustable thickness + AA line path with explicit color and opacity.
         * - The mesh created is a UV-sphere and the number of sectors and stacks is adjusted automatically according to the apparent size on the screen.
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param quality       Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         *                      - `1` : default quality
         *                      - `>1`: finer mesh. Improve quality but decrease speed.
         *                      - `<1`: coarser mesh. Decrease quality but improve speed.
         * @param thickness     thickness of the lines
         * @param color         color to use
         * @param opacity       opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameAdaptativeSphere(float quality, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit cylinder in model space [*fast*].
         *
         * @remark The cylinder is centered at the origin, has radius `1`, and extends from `y = -1` to `y = 1`.
         *
         * @param nb_sectors  Number of sectors used around the cylinder. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         * @param top_cap     Draw the top disk triangulation at `y = 1`.
         */
        void drawWireFrameCylinder(int nb_sectors, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a wireframe unit cylinder in model space [*antialiased*].
         *
         * @param nb_sectors  Number of sectors used around the cylinder. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         * @param top_cap     Draw the top disk triangulation at `y = 1`.
         */
        void drawWireFrameCylinderAA(int nb_sectors, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a wireframe unit cylinder in model space [*adjustable thickness + AA*].
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param nb_sectors  Number of sectors used around the cylinder. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         * @param top_cap     Draw the top disk triangulation at `y = 1`.
         * @param thickness   thickness of the lines
         * @param color       color to use
         * @param opacity     opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameCylinder(int nb_sectors, bool bottom_cap, bool top_cap, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit cone in model space [*fast*].
         *
         * @remark The cone has base radius `1` at `y = -1` and its apex at `y = 1`.
         *
         * @param nb_sectors  Number of sectors used around the cone. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         */
        void drawWireFrameCone(int nb_sectors, bool bottom_cap = true);


        /**
         * Draw a wireframe unit cone in model space [*antialiased*].
         *
         * @param nb_sectors  Number of sectors used around the cone. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         */
        void drawWireFrameConeAA(int nb_sectors, bool bottom_cap = true);


        /**
         * Draw a wireframe unit cone in model space [*adjustable thickness + AA*].
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param nb_sectors  Number of sectors used around the cone. Minimum `3`.
         * @param bottom_cap  Draw the bottom disk triangulation at `y = -1`.
         * @param thickness   thickness of the lines
         * @param color       color to use
         * @param opacity     opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameCone(int nb_sectors, bool bottom_cap, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe truncated cone in model space [*fast*].
         *
         * @param nb_sectors      Number of sectors used around the shape. Minimum `3`.
         * @param bottom_radius   Radius of the bottom ring at `y = -1`.
         * @param top_radius      Radius of the top ring at `y = 1`.
         * @param bottom_cap      Draw the bottom disk triangulation when `bottom_radius > 0`.
         * @param top_cap         Draw the top disk triangulation when `top_radius > 0`.
         */
        void drawWireFrameTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a wireframe truncated cone in model space [*antialiased*].
         *
         * @param nb_sectors      Number of sectors used around the shape. Minimum `3`.
         * @param bottom_radius   Radius of the bottom ring at `y = -1`.
         * @param top_radius      Radius of the top ring at `y = 1`.
         * @param bottom_cap      Draw the bottom disk triangulation when `bottom_radius > 0`.
         * @param top_cap         Draw the top disk triangulation when `top_radius > 0`.
         */
        void drawWireFrameTruncatedConeAA(int nb_sectors, float bottom_radius, float top_radius, bool bottom_cap = true, bool top_cap = true);


        /**
         * Draw a wireframe truncated cone in model space [*adjustable thickness + AA*].
         *
         * @warning This method is very slow and may be slower than solid drawing.
         *
         * @param nb_sectors      Number of sectors used around the shape. Minimum `3`.
         * @param bottom_radius   Radius of the bottom ring at `y = -1`.
         * @param top_radius      Radius of the top ring at `y = 1`.
         * @param bottom_cap      Draw the bottom disk triangulation when `bottom_radius > 0`.
         * @param top_cap         Draw the top disk triangulation when `top_radius > 0`.
         * @param thickness       thickness of the lines
         * @param color           color to use
         * @param opacity         opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, bool bottom_cap, bool top_cap, float thickness, color_t color, float opacity);



        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing 3D point clouds
         *
         * Methods for drawing pixels and dots.
         *
         * @remark the methods do not take the scene lighting into account.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw a single pixel at a given position in model space.
         *
         * @remark
         * - Use the material color.
         * - The scene lighting is ignored.
         *
         * @param   pos Position (in model space).
         */
        void drawPixel(const fVec3& pos);


        /**
         * Draw a single pixel at a given position in model space.
         *
         * @remark
         *  - Use blending with a given color and opacity factor.
         *  - The scene lighting is ignored.
         *
         * @param   pos     Position (in model space).
         * @param   color   color to use.
         * @param   opacity opacity multiplier in `[0.0f, 1.0f]`.
         */
        void drawPixel(const fVec3& pos, color_t color, float opacity);


        /**
         * Draw a list of pixels at given positions in model space.
         *
         * @remark
         *  - Use the material color for all pixels.
         *  - The scene lighting is ignored.
         *
         * @param   nb_pixels   number of pixels to draw.
         * @param   pos_list    array of positions (in model space).
         */
        void drawPixels(int nb_pixels, const fVec3* pos_list);


        /**
         * Draw a list of pixels at given positions in model space with different colors and opacities.
         *
         * @remark
         *  - Use blending with given colors and opacities given by a palette and a list of indices.
         *  - The scene lighting is ignored.
         *
         * @param   nb_pixels       number of pixels to draw.
         * @param   pos_list        array of positions (in model space).
         * @param   colors_ind      array of color indices.
         * @param   colors          array of colors.
         * @param   opacities_ind   array of opacities indices.
         * @param   opacities       array of opacities
         */
        void drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities);


        /**
         * Draw a dot/circle at a given position in model space
         *
         * @remark
         * - Use the the material color.
         * - The scene lighting is ignored.
         *
         * @param   pos Position in model space.
         * @param   r   radius in pixels (integer valued).
         */
        void drawDot(const fVec3& pos, int r);



        /**
         * Draw a dot/circle at a given position in model space
         *
         * @remark
         * - Use blending with the given color and opacity.
         * - The scene lighting is ignored.
         *
         * @param   pos     Position in model space.
         * @param   r       radius in pixels.
         * @param   color   color to use.
         * @param   opacity The opacity for blending in [0.0f, 1.0f].
         */
        void drawDot(const fVec3& pos, int r, color_t color, float opacity);


        /**
         * Draw a list of dots/circles at given positions in model space.
         *
         * @remark
         * - Use the material color and the same radius for every dot.
         * - The scene lighting is ignored.
         *
         * @param   nb_dots     number of dots to draw
         * @param   pos_list    array of positions in model space.
         * @param   radius      radius in pixels
         */
        void drawDots(int nb_dots, const fVec3* pos_list, const int radius);


        /**
         * Draw a list of dots/circles at given positions in model space.
         *
         * @remark
         * - Use a different radius and colors for every dot.
         * - Use blending with given colors and opacities given by a palette and a list of indices.
         * - The scene lighting is ignored.
         *
         * @param   nb_dots         number of dots to draw.
         * @param   pos_list        array of positions in model space.
         * @param   radius_ind      array of radius indices.
         * @param   radius          array of radiuses.
         * @param   colors_ind      array of color indices.
         * @param   colors          array of colors.
         * @param   opacities_ind   array of opacity indices.
         * @param   opacities       array of opacities value in [0.0f, 1.0f].
         */
        void drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities);





        ///@}





    private:



        /*****************************************************************************************
        ******************************************************************************************
        *
        * BE CAREFUL PAST THIS POINT... FOR HERE BE DRAGONS !
        *
        ******************************************************************************************
        ******************************************************************************************/


        /** Choose bounds so that we have some margin of error on both side.  */
        TGX_INLINE float _clipbound_xy() const
            {
            return (256 + 3*((MAXVIEWPORTDIMENSION * 256) / ((_lx > _ly) ? _lx : _ly))) / 1024.0f; // use integer computation up to the last divide
            //return (1.0f + 3.0f * (((float)MAXVIEWPORTDIMENSION) / ((_lx > _ly) ? _lx : _ly))) / 4.0f;
            }


        /** Make sure we can perform a drawing operation */
        TGX_INLINE bool _validDraw() const
            {
            return ((_lx > 0) && (_ly > 0) && (_uni.im != nullptr) && (_uni.im->isValid()));
            }


        /** recompute the wa and wa scaling constants. called after every change of the projection matrix or projection mode */
        TGX_NOINLINE void _recompute_wa_wb();


        /***********************************************************
        * Making sure shader flags are coherent
        ************************************************************/

        TGX_NOINLINE void _rectifyShaderOrtho();


        TGX_NOINLINE void _rectifyShaderZbuffer();


        TGX_NOINLINE void _rectifyShaderShading(Shader new_shaders);


        TGX_NOINLINE void _rectifyShaderTextureMode();


        TGX_NOINLINE void _rectifyShaderTextureWrapping();


        TGX_NOINLINE void _rectifyShaderTextureQuality();


       /***********************************************************
        * DRAWING STUFF
        ************************************************************/


        /** draw a triangle with a restricted compile-time shader set and takes care of clipping */
        template<Shader RASTERIZER_SHADERS = ENABLED_SHADERS>
        void _drawTriangleClipped(const int RASTER_TYPE,
            const fVec4* Q0, const fVec4* Q1, const fVec4* Q2,
            const fVec3* N0, const fVec3* N1, const fVec3* N2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2);


        /** used by _drawTriangleClipped() */
        template<Shader RASTERIZER_SHADERS = ENABLED_SHADERS>
        void _drawTriangleClippedSub(const int RASTER_TYPE, const int plane,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3);


        /** draw a single triangle */
        void _drawTriangle(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2,
            const fVec3* N0, const fVec3* N1, const fVec3* N2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2);


        /** draw a triangle strip */
        void _drawTriangleStrip(const int RASTER_TYPE, int nb_indices,
            const uint16_t* ind_vertices, const fVec3* vertices,
            const uint16_t* ind_normals, const fVec3* normals,
            const uint16_t* ind_texture, const fVec2* textures);


        /** draw a single quad : the 4 points are assumed to be coplanar */
        void _drawQuad(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2, const fVec3* P3,
            const fVec3* N0, const fVec3* N1, const fVec3* N2, const fVec3* N3,
            const fVec2* T0, const fVec2* T1, const fVec2* T2, const fVec2* T3,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2, const RGBf& Vcol3);

        /** draw one sky-box triangle with a direct minimal shader */
        template<bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
        void _rasterizeSkyBoxTriangle(const RasterizerVec4& V0, const RasterizerVec4& V1, const RasterizerVec4& V2);

        /** select the requested texture quality/wrap mode for a sky-box triangle */
        void _rasterizeSkyBoxTriangle(const RasterizerVec4& V0, const RasterizerVec4& V1, const RasterizerVec4& V2,
            Shader texture_quality, Shader texture_mode);

        /** draw a clipped sky-box triangle; clips against screen and near planes but not far plane */
        void _drawSkyBoxTriangleClippedSub(const int plane,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            Shader texture_quality, Shader texture_mode);

        /** draw a sky-box triangle and takes care of near/screen clipping */
        void _drawSkyBoxTriangleClipped(
            const fVec4* Q0, const fVec4* Q1, const fVec4* Q2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            Shader texture_quality, Shader texture_mode);

        /** draw a single sky-box quad with the dedicated no-z unlit texture path */
        void _drawSkyBoxQuad(
            const fVec4* P0, const fVec4* P1, const fVec4* P2, const fVec4* P3,
            const fVec2* T0, const fVec2* T1, const fVec2* T2, const fVec2* T3,
            Shader texture_quality, Shader texture_mode);

        /** draw one named sky-box face */
        void _drawSkyBoxFace(const fVec4 skybox_vertices[8], const uint16_t* face, const fVec2 texture_coords[4], const Image<color_t>* texture,
            Shader texture_quality, Shader texture_mode);


        /** Method called by drawMesh() which does the actual drawing. */
        void _drawMesh(const int RASTER_TYPE, const Mesh3D<color_t>* mesh);

        /** Method called by drawMesh<SHADERS>() which does the actual Mesh3Dv2 drawing. */
        template<Shader RASTERIZER_SHADERS = ENABLED_SHADERS>
        void _drawMesh(const int RASTER_TYPE, const Mesh3Dv2<color_t>* mesh, bool use_mesh_material);

        /** Return true when a decoded Mesh3Dv2 visibility cone rejects the meshlet for the current view. */
        inline bool _discardMeshlet16b(const fVec3& sphere_center, float sphere_radius, const fVec3& cone_dir, float cone_cos) const
            {
            if (cone_cos <= -1.0f) return false;

            const fVec4 D = _r_modelViewM.mult0(cone_dir);
            const float dd = D.x * D.x + D.y * D.y + D.z * D.z;
            if (dd <= 1.0e-20f) return false;

            float dot;
            float len2;
            if (_ortho)
                {
                dot = D.z; // object-to-camera direction is +Z in view space.
                len2 = dd;
                }
            else
                {
                const fVec3 anchor = sphere_center - (cone_dir * sphere_radius);
                const fVec4 A = _r_modelViewM.mult1(anchor);
                dot = -(D.x * A.x + D.y * A.y + D.z * A.z);
                const float aa = A.x * A.x + A.y * A.y + A.z * A.z;
                if (aa <= 1.0e-20f) return false;
                len2 = dd * aa;
                }

            const float c2len2 = cone_cos * cone_cos * len2;
            const float dot2 = dot * dot;
            return (cone_cos >= 0.0f) ? ((dot < 0.0f) || (dot2 < c2len2))
                                      : ((dot < 0.0f) && (dot2 > c2len2));
            }



        /***********************************************************
        * Drawing wireframe
        ************************************************************/

        inline void _drawWireFrameLineFast(iVec2 P0, iVec2 P1, color_t color);

        template<bool CHECK_NEIGHBOR> inline void _drawWireFrameLineAAFast(const fVec2& P0, const fVec2& P1, color_t color, int32_t op);

        template<int MODE> void _drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameMesh(const Mesh3Dv2<color_t>* mesh, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameLine(const fVec3& P1, const fVec3& P2, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameTriangleStrip(int nb_indices, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, color_t color, float opacity, float thickness);

        template<int MODE> void _drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);




        /***********************************************************
        * Simple geometric objects
        ************************************************************/


        template<bool USE_BLENDING> void _drawPixel(const fVec3& pos, color_t color, float opacity);


        template<bool USE_COLORS, bool USE_BLENDING> void _drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities);


        template<bool USE_BLENDING> void _drawDot(const fVec3& pos, int r, color_t color, float opacity);


        template<bool USE_RADIUS, bool USE_COLORS, bool USE_BLENDING> void _drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities);



        template<bool CHECKRANGE, bool USE_BLENDING> TGX_INLINE inline void drawPixelZbuf(int x, int y, color_t color, float opacity, float z)
            {
            if (CHECKRANGE && ((x < 0) || (x >= _uni.im->lx()) || (y < 0) || (y >= _uni.im->ly()))) return;
            ZBUFFER_t& W = _uni.zbuf[x + _uni.im->lx() * y];
            const ZBUFFER_t aa =  (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(z * _uni.wa + _uni.wb)) : ((ZBUFFER_t)z);
            if (W < aa)
                {
                W = aa;
                if (USE_BLENDING) _uni.im->template drawPixel<false>({ x, y }, color, opacity); else _uni.im->template drawPixel<false>({ x, y }, color);
                }
            }


        template<bool CHECKRANGE, bool USE_BLENDING> inline void drawHLineZbuf(int x, int y, int w, color_t color, float opacity, float z)
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                const int lx = _uni.im->lx();
                const int ly = _uni.im->ly();
                if ((y < 0) || (y >= ly) || (x >= lx)) return;
                if (x < 0) { w += x; x = 0; }
                if (x + w > lx) { w = lx - x; }
                if (w <= 0) return;
                }
            while(w--) drawPixelZbuf<CHECKRANGE, USE_BLENDING>(x++, y, color, opacity, z);
            }


        template<bool CHECKRANGE, bool USE_BLENDING> void _drawCircleZbuf(int xm, int ym, int r, color_t color, float opacity, float z);


        /**
        * For adaptive mesh: compute the diameter (in pixels) of the unit sphere S(0,1) once projected on the screen
        **/
        float _unitSphereScreenDiameter();


        template<bool WIREFRAME, int MODE> void _drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture, float thickness, color_t color, float opacity);

        void _drawTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, const Image<color_t>* texture_side, const Image<color_t>* texture_bottom, const Image<color_t>* texture_top, bool bottom_cap, bool top_cap);

        template<int MODE> void _drawWireFrameTruncatedCone(int nb_sectors, float bottom_radius, float top_radius, bool bottom_cap, bool top_cap, float thickness, color_t color, float opacity);

        struct ExtVec4;

        void _drawTruncatedConeGouraudCachedTriangle(const int cone_shader,  ExtVec4& E0, fVec3& N0, ExtVec4& E1, fVec3& N1, ExtVec4& E2, fVec3& N2, bool textured, bool ortho, float CLIPBOUND_XY, bool cliptestneeded);

        void _drawTruncatedConeGouraudSideStrip(const int cone_shader, int nb_sectors, float bottom_radius, float top_radius, const float* cosTheta, const float* sinTheta, float inv_side_normal, float side_normal_y, float dtx, float CLIPBOUND_XY, bool cliptestneeded);

        void _drawTruncatedConeGouraudConeFan(const int cone_shader, int nb_sectors, bool top_apex, float ring_radius, const float* cosTheta, const float* sinTheta, float inv_side_normal, float side_normal_y, float dtx, float CLIPBOUND_XY, bool cliptestneeded);

        void _drawTruncatedConeGouraudCapFan(const int cap_shader, int nb_sectors, bool top_cap, float radius, const float* cosTheta, const float* sinTheta, float CLIPBOUND_XY, bool cliptestneeded);

#if TGX_DRAWSPHERE_USE_STRIP_BANDS
        void _drawSphereGouraudStripBand(const int sphere_shader, int nb_sectors, const float* cosTheta, const float* sinTheta, float cosPhi, float sinPhi, float new_cosPhi, float new_sinPhi, float v, float vv, float dtx, float CLIPBOUND_XY, bool sphere_cliptestneeded);

        void _drawSphereGouraudCap(const int sphere_shader, int nb_sectors, bool top_cap, const float* cosTheta, const float* sinTheta, float ring_y, float ring_radius, float ring_v, float dtx, float CLIPBOUND_XY, bool sphere_cliptestneeded);
#endif



        /***********************************************************
        * CLIPPING
        ************************************************************/


        /** used by _discard() for testing a point position against the frustum planes */
        TGX_INLINE inline void _clip(int & fl, const fVec4 & P, float bx, float Bx, float by, float By)
            {
            if (P.x >= bx) { fl &= (~(1)); }
            if (P.x <= Bx) { fl &= (~(2)); }
            if (P.y >= by) { fl &= (~(4)); }
            if (P.y <= By) { fl &= (~(8)); }
            if ((P.z >= -1.0f)&&(P.w > 0)) { fl &= (~(16)); }
            if (P.z <= +1.0f) { fl &= (~(32)); }
            }


        /** used by _discard() for testing a point position against the frustum planes */
        TGX_INLINE inline void _clip(int & fl, const fVec3 & P, float bx, float Bx, float by, float By, const fMat4 & M)
            {
            fVec4 S = M.mult1(P);
            if (!_ortho) S.zdivide();
            return _clip(fl, S, bx, Bx, by, By);
            }


        /* test if a box is outside the image and should be discarded.
           transform the box coords with M then z-divide. */
        inline bool _discardBox(const fBox3 & bb, const fMat4 & M)
            {
            if ((bb.minX == 0) && (bb.maxX == 0) && (bb.minY == 0) && (bb.maxY == 0) && (bb.minZ == 0) && (bb.maxZ == 0))
                return false; // do not discard if the bounding box is uninitialized.

            const float bx = (_ox - 1) * _ilx - 1.0f;
            const float Bx = (_ox + _uni.im->width() + 1) * _ilx - 1.0f;
            const float by = (_oy - 1) * _ily - 1.0f;
            const float By = (_oy + _uni.im->height() + 1) * _ily - 1.0f;

            int fl = 63; // every bit set
            _clip(fl, fVec3(bb.minX, bb.minY, bb.minZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.minY, bb.maxZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.maxY, bb.minZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.maxY, bb.maxZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.minY, bb.minZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.minY, bb.maxZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.maxY, bb.minZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.maxY, bb.maxZ), bx, Bx, by, By, M);
            if (fl == 0) return false;
            return true;
            }


        /* test if a triangle is completely outside the image and should be discarded.
         * coords are given after z-divide. */
        inline bool _discardTriangle(const fVec4 & P1, const fVec4 & P2, const fVec4 & P3)
            {
            const float bx = (_ox - 1) * _ilx - 1.0f;
            const float Bx = (_ox + _uni.im->width() + 1) * _ilx - 1.0f;
            const float by = (_oy - 1) * _ily - 1.0f;
            const float By = (_oy + _uni.im->height() + 1) * _ily - 1.0f;

            int fl = 63; // every bit set
            _clip(fl, P1, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, P2, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, P3, bx, Bx, by, By);
            if (fl == 0) return false;
            return true;
            }


        /** used by _clipTestNeeded() */
        TGX_INLINE bool _clip2(float clipboundXY, const fVec3 & P, const fMat4 & M)
            {
            fVec4 S = M.mult1(P);
            if (!_ortho)
                {
                S.zdivide();
                if (S.w <= 0) S.z = -2;
                }
            return ((S.x <= -clipboundXY) || (S.x >= clipboundXY)
                 || (S.y <= -clipboundXY) || (S.y >= clipboundXY)
                 || (S.z <= -1) || (S.z >= 1));
            }


        /** test if the mesh may possibly need clipping. If it return false, then cliptest can be skipped. */
        inline bool _clipTestNeeded(float clipboundXY, const fBox3 & bb, const fMat4 & M)
            {
            return (_clip2(clipboundXY, fVec3(bb.minX, bb.minY, bb.minZ), M)
                 || _clip2(clipboundXY, fVec3(bb.minX, bb.minY, bb.maxZ), M)
                 || _clip2(clipboundXY, fVec3(bb.minX, bb.maxY, bb.minZ), M)
                 || _clip2(clipboundXY, fVec3(bb.minX, bb.maxY, bb.maxZ), M)
                 || _clip2(clipboundXY, fVec3(bb.maxX, bb.minY, bb.minZ), M)
                 || _clip2(clipboundXY, fVec3(bb.maxX, bb.minY, bb.maxZ), M)
                 || _clip2(clipboundXY, fVec3(bb.maxX, bb.maxY, bb.minZ), M)
                 || _clip2(clipboundXY, fVec3(bb.maxX, bb.maxY, bb.maxZ), M));
            }


        /** Return true when AA wireframe lines still need to range-check their secondary pixel. */
        inline bool _wireFrameAANeighborCheckNeeded(const fBox3& bb, const fMat4& proj_modelview)
            {
            if ((_uni.im == nullptr) || (_uni.im->lx() <= 2) || (_uni.im->ly() <= 2)) return true;

            const float bx = (_ox + 1.0f) * _ilx - 1.0f;
            const float Bx = (_ox + (float)(_uni.im->lx() - 2)) * _ilx - 1.0f;
            const float by = (_oy + 1.0f) * _ily - 1.0f;
            const float By = (_oy + (float)(_uni.im->ly() - 2)) * _ily - 1.0f;

            const fVec3 C[8] = {
                fVec3(bb.minX, bb.minY, bb.minZ),
                fVec3(bb.minX, bb.minY, bb.maxZ),
                fVec3(bb.minX, bb.maxY, bb.minZ),
                fVec3(bb.minX, bb.maxY, bb.maxZ),
                fVec3(bb.maxX, bb.minY, bb.minZ),
                fVec3(bb.maxX, bb.minY, bb.maxZ),
                fVec3(bb.maxX, bb.maxY, bb.minZ),
                fVec3(bb.maxX, bb.maxY, bb.maxZ)
                };

            for (int i = 0; i < 8; i++)
                {
                fVec4 P = proj_modelview.mult1(C[i]);
                if (!_ortho)
                    {
                    if (P.w <= 0) return true;
                    P.zdivide();
                    }
                if ((P.z < -1.0f) || (P.z > 1.0f)) return true;
                if ((P.x < bx) || (P.x > Bx) || (P.y < by) || (P.y > By)) return true;
                }
            return false;
            }


#if TGX_MESHLET_SPHERE_CLIP
        /** Build object-space clip planes from the current projection-modelview matrix. */
        inline void _meshletClipPlanes(float clipboundXY, const fMat4& M, fVec4* planes, float* plane_norms) const
            {
            const float cx = clipboundXY;
            planes[0] = fVec4(M.M[0] + cx * M.M[3], M.M[4] + cx * M.M[7], M.M[8] + cx * M.M[11], M.M[12] + cx * M.M[15]);
            planes[1] = fVec4(-M.M[0] + cx * M.M[3], -M.M[4] + cx * M.M[7], -M.M[8] + cx * M.M[11], -M.M[12] + cx * M.M[15]);
            planes[2] = fVec4(M.M[1] + cx * M.M[3], M.M[5] + cx * M.M[7], M.M[9] + cx * M.M[11], M.M[13] + cx * M.M[15]);
            planes[3] = fVec4(-M.M[1] + cx * M.M[3], -M.M[5] + cx * M.M[7], -M.M[9] + cx * M.M[11], -M.M[13] + cx * M.M[15]);
            planes[4] = fVec4(M.M[2] + M.M[3], M.M[6] + M.M[7], M.M[10] + M.M[11], M.M[14] + M.M[15]);
            planes[5] = fVec4(-M.M[2] + M.M[3], -M.M[6] + M.M[7], -M.M[10] + M.M[11], -M.M[14] + M.M[15]);
            for (int i = 0; i < 6; i++)
                {
                const fVec4& P = planes[i];
                plane_norms[i] = sqrtf(P.x * P.x + P.y * P.y + P.z * P.z);
                }
            }


        /** Return -1 when outside, 0 when fully inside, 1 when intersecting the clipping frustum. */
        inline int _meshletSphereClip(const fVec3& center, float radius, const fVec4* planes, const float* plane_norms) const
            {
            bool intersects = false;
            for (int i = 0; i < 6; i++)
                {
                const fVec4& P = planes[i];
                const float d = P.x * center.x + P.y * center.y + P.z * center.z + P.w;
                const float r = radius * plane_norms[i];
                if (d < -r) return -1;
                if (d < r) intersects = true;
                }
            return intersects ? 1 : 0;
            }
#endif




        /***********************************************************
        * TRIANGLE CLIPPING AGAINST A CLIP-PLANE
        ************************************************************/

        /**
        * Signed distance from a clip-plane
        *
        * The positive half space delimited by the clip-plane are the points (X,Y,Z)
        * such that X * CP.x  +  Y * C.y  +  W * CP.z  +  CP.w >= 0
        *
        * Return > 0 if inside, return < 0 if outside (and 0 on the plane).
        **/
        TGX_INLINE inline float _cpdist(const tgx::fVec4& CP, float off, const tgx::fVec4& P)
            {
            return (CP.x * P.x) + (CP.y * P.y) + (CP.z * P.z) + (CP.w * P.w) + off;
            }

        /**
        * Compute the barycentric weight of the point P on the segment [A,B]
        * that intersect a given clip-plane CP.
        *
        * - CP     : the clip-plane
        * - sdistA : signed distance from A to the clip-plane CP (computed with cpdist())
        * - sdistB : signed distance from B to the clip-plane CP (computed with cpdist())
        *
        * return alpha such that P = A + alpha * (B - A)
        **/
        TGX_INLINE inline float _cpfactor(const tgx::fVec4& CP, const float sdistA, const float sdistB)
            {
            return sdistA / (sdistA - sdistB);
            }


        /** used by _triangleClip when only 1 point is inside the view */
        TGX_RENDERER3D_CLIP_NOINLINE void _triangleClip1in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4);


        /** used by _triangleClip when only 2 points are inside the view */
        TGX_RENDERER3D_CLIP_NOINLINE void _triangleClip2in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4);


        TGX_RENDERER3D_CLIP_NOINLINE int _triangleClip(int shader, tgx::fVec4 CP, float off,
            const RasterizerVec4 & P1, const RasterizerVec4 & P2, const RasterizerVec4 & P3,
            RasterizerVec4 & nP1, RasterizerVec4 & nP2, RasterizerVec4 & nP3, RasterizerVec4 & nP4);



        /***********************************************************
        * PHONG LIGHTNING
        ************************************************************/

        static const int _POWTABSIZE = 32;      // number of entries in the precomputed power table for specular exponent.
        int _currentpow;                        // exponent for the currently computed table (<0 if table not yet computed)
        float _powmax;                          // used to compute exponent
        float _fastpowtab[_POWTABSIZE];         // the precomputed power table.

        /** Pre-compute the power table for computing specular light component (if needed). */
        TGX_INLINE inline void _precomputeSpecularTable(int exponent)
            {
            if (_currentpow == exponent) return;
            _precomputeSpecularTable2(exponent);
            }

        TGX_NOINLINE void _precomputeSpecularTable2(int exponent);


        /** compute pow(x, exponent) using linear interpolation from the pre-computed table */
        TGX_INLINE inline float _powSpecular(float x) const
            {
            const float indf = (_powmax - x) * _POWTABSIZE;
            const int indi = max(0,(int)indf);
            return (indi >= (_POWTABSIZE - 1)) ? 0.0f : (_fastpowtab[indi] + (indf - indi) * (_fastpowtab[indi + 1] - _fastpowtab[indi]));;
            }




        /** recompute a directional light after a view or light change */
        inline void _updateDirectionalLightTransform(int index)
            {
            _r_light[index] = _viewM.mult0(_light[index]);
            _r_light[index] = -_r_light[index];
            _r_light[index].normalize();
            _r_light_inorm[index] = _r_light[index] * _r_inorm;
            _r_H[index] = fVec3(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)...
            _r_H[index] += _r_light[index];
            _r_H[index].normalize();
            _r_H_inorm[index] = _r_H[index] * _r_inorm;
            }


        /** recompute directional light normal-scaled vectors after a model change */
        inline void _updateActiveDirectionalLightInorms()
            {
            for (int i = 0; i < _directionalLightCount; i++)
                {
                _r_light_inorm[i] = _r_light[i] * _r_inorm;
                _r_H_inorm[i] = _r_H[i] * _r_inorm;
                }
            }


        /** recompute all active directional light transforms after a view change */
        inline void _updateActiveDirectionalLightTransforms()
            {
            for (int i = 0; i < _directionalLightCount; i++)
                {
                _updateDirectionalLightTransform(i);
                }
            }



        static TGX_INLINE inline bool _spotLightColorIsBlack(const RGBf& color)
            {
            return ((color.R <= 0.0f) && (color.G <= 0.0f) && (color.B <= 0.0f));
            }


        /** precompute the finite or infinite range constants of one spot light */
        inline void _setSpotLightRangeValues(int index, float range)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                if ((range > 0.0f) && (range < 1.0e15f))
                    {
                    _spotLights.range2[index] = range * range;
                    _spotLights.invRange2[index] = 1.0f / _spotLights.range2[index];
                    }
                else
                    {
                    _spotLights.range2[index] = 1.0e30f;
                    _spotLights.invRange2[index] = 0.0f;
                    }
                }
            }


        /** precompute the cone constants of one spot light */
        inline void _setSpotLightConeValues(int index, float outerAngleDeg, float innerAngleDeg)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                if (outerAngleDeg >= 180.0f)
                    {
                    _spotLights.flags[index] &= ~(Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED | Renderer3D_detail::SPOT_LIGHT_SOFT_CONE);
                    _spotLights.cosOuter[index] = -1.0f;
                    _spotLights.invCosWidth[index] = 0.0f;
                    return;
                    }

                const float outer = clamp(outerAngleDeg, 0.0f, 180.0f);
                _spotLights.flags[index] |= Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED;
                _spotLights.flags[index] &= ~Renderer3D_detail::SPOT_LIGHT_SOFT_CONE;

                _spotLights.cosOuter[index] = tgx_fast_cos_deg_clamped(outer);
                _spotLights.invCosWidth[index] = 0.0f;

                if ((innerAngleDeg >= 0.0f) && (innerAngleDeg < outer))
                    {
                    const float inner = clamp(innerAngleDeg, 0.0f, outer);
                    const float cosInner = tgx_fast_cos_deg_clamped(inner);
                    const float cosWidth = cosInner - _spotLights.cosOuter[index];
                    if (cosWidth > 0.00001f)
                        {
                        _spotLights.flags[index] |= Renderer3D_detail::SPOT_LIGHT_SOFT_CONE;
                        _spotLights.invCosWidth[index] = 1.0f / cosWidth;
                        }
                    }
                }
            }


        /** recompute one spot-light position after a view or light change */
        inline void _updateSpotLightPositionTransform(int index)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                _spotLights.positionView[index] = _viewM.mult1(_spotLights.position[index]);
                }
            }


        /** recompute one spot-light direction after a view or light change */
        inline void _updateSpotLightDirectionTransform(int index)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                fVec3 D = _viewM.mult0(_spotLights.direction[index]);
                const float d2 = dotProduct(D, D);
                if (d2 > 0.000001f) { D *= tgx::fast_invsqrt(d2); } else { D = fVec3(0.0f, 0.0f, -1.0f); }
                _spotLights.directionView[index] = D;
                }
            }


        /** recompute one spot-light transform after a view or light change */
        inline void _updateSpotLightTransform(int index)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                _updateSpotLightPositionTransform(index);
                _updateSpotLightDirectionTransform(index);
                }
            }


        /** recompute all active spot-light transforms after a view change */
        inline void _updateActiveSpotLightTransforms()
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                for (int i = 0; i < _spotLights.count; i++)  { _updateSpotLightTransform(i); }
                }
            }


        /** recompute compact runtime flags for the active spot lights */
        inline void _updateActiveSpotLightFlags()
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                int globalFlags = 0;
                for (int i = 0; i < _spotLights.count; i++)
                    {
                    const int flags = _spotLights.flags[i];
                    if (flags & Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED) { globalFlags |= Renderer3D_detail::SPOT_LIGHT_GLOBAL_RUNTIME_SPECULAR; }
                    if (flags & Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED) { globalFlags |= Renderer3D_detail::SPOT_LIGHT_GLOBAL_ACTIVE_CONE; }
                    }
                _spotLights.globalFlags = globalFlags;
                }
            }


        /** recompute one spot-light runtime color for the current material strengths */
        inline void _updateSpotLightColor(int index, float diffuseStrength, float specularStrength, int specularExponent)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                _spotLights.runtimeDiffuseColor[index] = _spotLights.diffuseColor[index] * diffuseStrength;
                _spotLights.runtimeSpecularColor[index] = _spotLights.specularColor[index] * specularStrength;
                if ((specularStrength > 0.0f) && (specularExponent > 0) && (!_spotLightColorIsBlack(_spotLights.specularColor[index])))
                    {
                    _spotLights.flags[index] |= Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED;
                    }
                else
                    {
                    _spotLights.flags[index] &= ~Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED;
                    }
                }
            }


        /** recompute runtime spot-light colors for the current material strengths */
        inline void _updateActiveSpotLightColors(float diffuseStrength, float specularStrength, int specularExponent)
            {
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                for (int i = 0; i < _spotLights.count; i++) { _updateSpotLightColor(i, diffuseStrength, specularStrength, specularExponent); }
                _updateActiveSpotLightFlags();
                }
            }


        /** recompute runtime light colors for the current material strengths */
        inline void _setRuntimeMaterialLighting(float ambiantStrength, float diffuseStrength, float specularStrength, int specularExponent)
            {
            _r_ambiantColor = _ambiantColor * ambiantStrength;
            for (int i = 0; i < _directionalLightCount; i++)
                {
                _r_diffuseColor[i] = _diffuseColor[i] * diffuseStrength;
                _r_specularColor[i] = _specularColor[i] * specularStrength;
                }
            _updateActiveSpotLightColors(diffuseStrength, specularStrength, specularExponent);
            }


        /** recompute runtime light colors for the current material strengths */
        inline void _setRuntimeMaterialLighting(float ambiantStrength, float diffuseStrength, float specularStrength)
            {
            _setRuntimeMaterialLighting(ambiantStrength, diffuseStrength, specularStrength, _specularExponent);
            }


        /** recompute one directional light runtime color for the current material strengths */
        inline void _updateDirectionalLightColor(int index)
            {
            _r_diffuseColor[index] = _diffuseColor[index] * _diffuseStrength;
            _r_specularColor[index] = _specularColor[index] * _specularStrength;
            }



            //
        

        /** compute a vertex light factor or object color according to the current lighting setup */
        template<bool TEXTURE> TGX_RENDERER3D_SHADING_INLINE inline RGBf _shadeVertex(const float icu, const fVec3 & N, const fVec4 & P) const
            {
            RGBf col = _r_ambiantColor;
            if constexpr (MAX_DIRECTIONAL_LIGHTS == 1)
                {
                col += _r_diffuseColor[0] * max(icu * dotProduct(N, _r_light_inorm[0]), 0.0f);
                col += _r_specularColor[0] * _powSpecular(icu * dotProduct(N, _r_H_inorm[0]));
                }
            else
                {
                for (int i = 0; i < _directionalLightCount; i++)
                    {            
                    col += _r_diffuseColor[i] * max(icu * dotProduct(N, _r_light_inorm[i]), 0.0f);
                    col += _r_specularColor[i] * _powSpecular(icu * dotProduct(N, _r_H_inorm[i]));
                    }
                }
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                const float localNormalScale = icu * _r_inorm;
                const int spotGlobalFlags = _spotLights.globalFlags;
                const bool localSpecular = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_RUNTIME_SPECULAR) != 0;
                const bool localCone = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_ACTIVE_CONE) != 0;
                const fVec3 Pv(P.x, P.y, P.z);
                for (int i = 0; i < _spotLights.count; i++)
                    {
                    fVec3 L = _spotLights.positionView[i] - Pv;
                    const float d2 = dotProduct(L, L);
                    if ((d2 <= 1.0e-12f) || (d2 >= _spotLights.range2[i])) continue;

                    const float ndotlRaw = dotProduct(N, L);
                    const float diffRaw = localNormalScale * ndotlRaw;
                    if (diffRaw <= 0.0f) continue;

                    const float invD = tgx::fast_invsqrt(d2);
                    const float diff = diffRaw * invD;
                    const float atten = 1.0f - d2 * _spotLights.invRange2[i];
                    float lightFactor = atten * atten;

                    int flags = 0;
                    if (localCone)
                        {
                        flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED)
                            {
                            const float cone = -dotProduct(_spotLights.directionView[i], L) * invD;
                            if (cone <= _spotLights.cosOuter[i]) continue;
                            if (flags & Renderer3D_detail::SPOT_LIGHT_SOFT_CONE)
                                {
                                float spot = (cone - _spotLights.cosOuter[i]) * _spotLights.invCosWidth[i];
                                if (spot > 1.0f) spot = 1.0f;
                                lightFactor *= spot * spot;
                                }
                            }
                        }

                    col += _spotLights.runtimeDiffuseColor[i] * (diff * lightFactor);

                    if (localSpecular)
                        {
                        if (!localCone) flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED)
                            {
                            const float specRaw = diff + (localNormalScale * N.z);
                            if (specRaw > 0.0f)
                                {
                                const float h2 = 2.0f + (2.0f * L.z * invD);
                                if (h2 > 1.0e-12f)
                                    {
                                    const float invH = tgx::fast_invsqrt(h2);
                                    const float spec = _powSpecular(specRaw * invH);
                                    col += _spotLights.runtimeSpecularColor[i] * (spec * lightFactor);
                                    }
                                }
                            }
                        }
                    }
                }
            else
                {
                (void)P;
                }
            if (!(TEXTURE)) col *= _r_objectColor;
            col.clamp();
            return col;
            }


        /** compute a vertex color according to the current lighting setup and a supplied material color */
        TGX_RENDERER3D_SHADING_INLINE inline RGBf _shadeVertex(const float icu, const fVec3 & N, const fVec4 & P, const RGBf & color) const
            {
            RGBf col = _r_ambiantColor;
            if constexpr (MAX_DIRECTIONAL_LIGHTS == 1)
                {
                col += _r_diffuseColor[0] * max(icu * dotProduct(N, _r_light_inorm[0]), 0.0f);
                col += _r_specularColor[0] * _powSpecular(icu * dotProduct(N, _r_H_inorm[0]));
                }
            else
                {
                for (int i = 0; i < _directionalLightCount; i++)
                    {            
                    col += _r_diffuseColor[i] * max(icu * dotProduct(N, _r_light_inorm[i]), 0.0f);
                    col += _r_specularColor[i] * _powSpecular(icu * dotProduct(N, _r_H_inorm[i]));
                    }
                }
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                const float localNormalScale = icu * _r_inorm;
                const int spotGlobalFlags = _spotLights.globalFlags;
                const bool localSpecular = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_RUNTIME_SPECULAR) != 0;
                const bool localCone = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_ACTIVE_CONE) != 0;
                const fVec3 Pv(P.x, P.y, P.z);
                for (int i = 0; i < _spotLights.count; i++)
                    {
                    fVec3 L = _spotLights.positionView[i] - Pv;
                    const float d2 = dotProduct(L, L);
                    if ((d2 <= 1.0e-12f) || (d2 >= _spotLights.range2[i])) continue;

                    const float ndotlRaw = dotProduct(N, L);
                    const float diffRaw = localNormalScale * ndotlRaw;
                    if (diffRaw <= 0.0f) continue;

                    const float invD = tgx::fast_invsqrt(d2);
                    const float diff = diffRaw * invD;
                    const float atten = 1.0f - d2 * _spotLights.invRange2[i];
                    float lightFactor = atten * atten;

                    int flags = 0;
                    if (localCone)
                        {
                        flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED)
                            {
                            const float cone = -dotProduct(_spotLights.directionView[i], L) * invD;
                            if (cone <= _spotLights.cosOuter[i]) continue;
                            if (flags & Renderer3D_detail::SPOT_LIGHT_SOFT_CONE)
                                {
                                float spot = (cone - _spotLights.cosOuter[i]) * _spotLights.invCosWidth[i];
                                if (spot > 1.0f) spot = 1.0f;
                                lightFactor *= spot * spot;
                                }
                            }
                        }

                    col += _spotLights.runtimeDiffuseColor[i] * (diff * lightFactor);

                    if (localSpecular)
                        {
                        if (!localCone) flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED)
                            {
                            const float specRaw = diff + (localNormalScale * N.z);
                            if (specRaw > 0.0f)
                                {
                                const float h2 = 2.0f + (2.0f * L.z * invD);
                                if (h2 > 1.0e-12f)
                                    {
                                    const float invH = tgx::fast_invsqrt(h2);
                                    const float spec = _powSpecular(specRaw * invH);
                                    col += _spotLights.runtimeSpecularColor[i] * (spec * lightFactor);
                                    }
                                }
                            }
                        }
                    }
                }
            else
                {
                (void)P;
                }

            col *= color;
            col.clamp();
            return col;
            }


        /** compute a face light factor or object color according to the current lighting setup */
        template<bool TEXTURE> TGX_RENDERER3D_SHADING_INLINE inline RGBf _shadeFace(const float icu, const fVec3 & N, const fVec4 & P) const
            {
            RGBf col = _r_ambiantColor;
            if constexpr (MAX_DIRECTIONAL_LIGHTS == 1)
                {
                col += _r_diffuseColor[0] * max(icu * dotProduct(N,  _r_light[0]), 0.0f);
                col += _r_specularColor[0] * _powSpecular(icu * dotProduct(N, _r_H[0]));
                }
            else
                {
                for (int i = 0; i < _directionalLightCount; i++)
                    {            
                    col += _r_diffuseColor[i] * max(icu * dotProduct(N,  _r_light[i]), 0.0f);
                    col += _r_specularColor[i] * _powSpecular(icu * dotProduct(N, _r_H[i]));
                    }
                }
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                const int spotGlobalFlags = _spotLights.globalFlags;
                const bool localSpecular = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_RUNTIME_SPECULAR) != 0;
                const bool localCone = (spotGlobalFlags & Renderer3D_detail::SPOT_LIGHT_GLOBAL_ACTIVE_CONE) != 0;
                const fVec3 Pv(P.x, P.y, P.z);
                for (int i = 0; i < _spotLights.count; i++)
                    {
                    fVec3 L = _spotLights.positionView[i] - Pv;
                    const float d2 = dotProduct(L, L);
                    if ((d2 <= 1.0e-12f) || (d2 >= _spotLights.range2[i])) continue;

                    const float ndotlRaw = dotProduct(N, L);
                    const float diffRaw = icu * ndotlRaw;
                    if (diffRaw <= 0.0f) continue;

                    const float invD = tgx::fast_invsqrt(d2);
                    const float diff = diffRaw * invD;
                    const float atten = 1.0f - d2 * _spotLights.invRange2[i];
                    float lightFactor = atten * atten;

                    int flags = 0;
                    if (localCone)
                        {
                        flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_CONE_ENABLED)
                            {
                            const float cone = -dotProduct(_spotLights.directionView[i], L) * invD;
                            if (cone <= _spotLights.cosOuter[i]) continue;
                            if (flags & Renderer3D_detail::SPOT_LIGHT_SOFT_CONE)
                                {
                                float spot = (cone - _spotLights.cosOuter[i]) * _spotLights.invCosWidth[i];
                                if (spot > 1.0f) spot = 1.0f;
                                lightFactor *= spot * spot;
                                }
                            }
                        }

                    col += _spotLights.runtimeDiffuseColor[i] * (diff * lightFactor);

                    if (localSpecular)
                        {
                        if (!localCone) flags = _spotLights.flags[i];
                        if (flags & Renderer3D_detail::SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED)
                            {
                            const float specRaw = diff + (icu * N.z);
                            if (specRaw > 0.0f)
                                {
                                const float h2 = 2.0f + (2.0f * L.z * invD);
                                if (h2 > 1.0e-12f)
                                    {
                                    const float invH = tgx::fast_invsqrt(h2);
                                    const float spec = _powSpecular(specRaw * invH);
                                    col += _spotLights.runtimeSpecularColor[i] * (spec * lightFactor);
                                    }
                                }
                            }
                        }
                    }
                }
            else
                {
                (void)P;
                }
            if (!(TEXTURE)) col *= _r_objectColor;
            col.clamp();
            return col;
            }


        /** compute the uniform face color for flat or unlit shading */
        TGX_RENDERER3D_SHADING_INLINE inline void _setFlatOrUnlitFaceColor(int raster_type, bool texture, fVec3& faceN, float cu,
                                                        const fVec4& Q0, const fVec4& Q1, const fVec4& Q2)
            {
            if constexpr (TGX_SHADER_HAS_UNLIT(ENABLED_SHADERS))
                {
                if (TGX_SHADER_HAS_UNLIT(raster_type))
                    {
                    _uni.facecolor = texture ? RGBf(1.0f, 1.0f, 1.0f) : _r_objectColor;
                    return;
                    }
                }

            const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
            faceN.normalize_fast();
            if constexpr (MAX_SPOT_LIGHTS > 0)
                {
                if (_spotLights.count > 0)
                    {
                    const float oneThird = 1.0f / 3.0f;
                    const fVec4 faceCenter((Q0.x + Q1.x + Q2.x) * oneThird,
                                           (Q0.y + Q1.y + Q2.y) * oneThird,
                                           (Q0.z + Q1.z + Q2.z) * oneThird,
                                           1.0f);
                    if (texture)
                        _uni.facecolor = _shadeFace<true>(icu, faceN, faceCenter);
                    else
                        _uni.facecolor = _shadeFace<false>(icu, faceN, faceCenter);
                    return;
                    }
                }
            if (texture)
                _uni.facecolor = _shadeFace<true>(icu, faceN, Q0);
            else
                _uni.facecolor = _shadeFace<false>(icu, faceN, Q0);
            }



        /***********************************************************
        * MEMBER VARIABLES
        ************************************************************/

        // *** general parameters ***

        int     _lx, _ly;           // viewport dimension
        float   _ilx, _ily;         // inverse viewport dimension

        int     _ox, _oy;           // image offset w.r.t. the viewport

        bool    _ortho;             // true to use orthographic projection and false for perspective projection

        fMat4   _projM;             // projection matrix

        RasterizerParams<color_t, color_t,ZBUFFER_t>  _uni; // rasterizer param (contain the image pointer and the zbuffer pointer).

        float _culling_dir;         // culling direction postive/negative or 0 to disable back face culling.

        int   _shaders;             // the shaders to use.
        int   _texture_mode;        // texture mapping mode (perspective-correct or affine)
        int   _texture_wrap_mode;   // wrapping mode (wrap_pow2 or clamp)
        int   _texture_quality;     // texturing quality (nearest or bilinear)

        // *** scene parameters ***

        fMat4   _viewM;             // view transform matrix

        fVec3   _light[MAX_DIRECTIONAL_LIGHTS];         // directional light directions
        RGBf    _ambiantColor;      // light ambiant color
        RGBf    _diffuseColor[MAX_DIRECTIONAL_LIGHTS];  // directional light diffuse colors
        RGBf    _specularColor[MAX_DIRECTIONAL_LIGHTS]; // directional light specular colors
        int     _directionalLightCount; // number of active directional lights

        // *** model specific parameters ***

        fMat4   _modelM;            // model transform matrix

        // material parameters
        RGBf    _color;             // model color (use when texturing is disabled)
        float   _ambiantStrength;   // ambient light reflection strength
        float   _diffuseStrength;   // diffuse light reflection strength
        float   _specularStrength;  // specular light reflection strength
        int     _specularExponent;  // specular exponent


        // *** pre-computed values ***
        fMat4 _r_modelViewM;        // model-view matrix
        float _r_inorm;             // Fast normal scaling for lighting. Assumes rotation/uniform scale;
                                    // Gouraud lighting is approximate with non-uniform model scaling.
        fVec3 _r_light[MAX_DIRECTIONAL_LIGHTS];         // light vectors in view space (inverted and normalized)
        fVec3 _r_light_inorm[MAX_DIRECTIONAL_LIGHTS];   // same as above but already multiplied by inorm
        fVec3 _r_H[MAX_DIRECTIONAL_LIGHTS];             // halfway vectors.
        fVec3 _r_H_inorm[MAX_DIRECTIONAL_LIGHTS];       // same as above but already multiplied by inorm
        RGBf _r_ambiantColor;       // ambient color multiplied by object ambient strength
        RGBf _r_diffuseColor[MAX_DIRECTIONAL_LIGHTS];   // diffuse colors multiplied by object diffuse strength
        RGBf _r_specularColor[MAX_DIRECTIONAL_LIGHTS];  // specular colors multiplied by object specular strength
        RGBf _r_objectColor;        // color to use for drawing the object (either _color or mesh->color).

        Renderer3D_detail::SpotLightData<MAX_SPOT_LIGHTS> _spotLights; // local spot-light data


        /**
        * Vector with additional attributes used by draw() methods.
        * **/
        struct ExtVec4 : public RasterizerVec4
            {
            fVec4 P;       // after model-view matrix multiplication
            fVec4 N;       // normal vector after model-view matrix multiplication
            bool missedP;  // true if the attributes should be computed
            int indn;      // index for normal vector in array
            int indt;      // index for texture vector in array
            };


    };



}




#include "Renderer3D.inl"


#endif

#endif


/** end of file */

