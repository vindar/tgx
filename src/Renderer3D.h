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

#include "Mesh3D.h"








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
    * @tparam LOADED_SHADERS    list of all shaders that may be used. By default, all shaders are enabled but 
    *                           if is possible to select only a subset of shaders to improve rendering speed and 
    *                           decrease memory usage significantly. Must a `|` combination of the following flags:
    *                               - `SHADER_PERSPECTIVE`: enable perspective projection
    *                               - `SHADER_ORTHO`: enable orthographic projection
    *                               - `SHADER_NOZBUFFER`: enable rendering without using a z-buffer
    *                               - `SHADER_ZBUFFER`: enable rendering with a z-buffer
    *                               - `SHADER_FLAT`: enable flat shading
    *                               - `SHADER_GOURAUD`: enable gouraud shading
    *                               - `SHADER_NOTEXTURE`: enable rendering without texturing
    *                               - `SHADER_TEXTURE_NEAREST`: enable rendering with texturing using point sampling
    *                               - `SHADER_TEXTURE_BILINEAR`: enable rendering with texturing using bilinear sampling
    *                               - `SHADER_TEXTURE_WRAP_POW2`: texture can use 'wrap around' mode with dimensions of texture being power of two.
    *                               - `SHADER_TEXTURE_CLAMP`: texture can use 'clamping to edge' mode. 
    *
    * @tparam ZBUFFER_t :       Type used for storing z-buffer values. Must be either `float` or `uint16_t`. The z-buffer must be 
    *                           as large as the image (but can be smaller than the viewport when using an offset).
    *                               - `float`: higher quality but requires 4 bytes per pixel.
    *                               - `uint16_t` : lower quality (z-fighting may occur) but only 2 bytes per pixel.
    * @remark
    *          
    * 1. If a drawing call is made that requires a shader that was not enabled in the template parameter `LOADED_SHADERS` or
    *    if the Renderer3D object state is not valid (e.g. incorrect image size, enabled but missing z-buffer...) then the operation
    *    will fails silently. In particular, if drawing without a Z-buffer is performed, the flag `SHADER_NOZBUFFER` **must** be set
    *    in LOADED_SHADERS. Similarly, if drawing without texturing is performed, the flag `SHADER_NOTEXTURE` **must** be set in LOADED_SHADERS.
    *
    * 2. Z-buffer testing is enabled as soon as a valid z-buffer is provided (with `Renderer3D::setZbuffer()`).
    *    Do not forget to erase the z-buffer with `Renderer3D::clearZbuffer()` at the start of a new frame.
    *    
    * 3. Normal vectors are mandatory when using Gouraud shading and must always be normalized (unit lenght) !
    *
    * 4. Texture dimensions must be powers of two when flag `SHADER_TEXTURE_WRAP_POW2` is set.
    * 
    * 5. Back-face culling is set with `Renderer3D::setCulling()`. Triangles and quads must then be provided in the
    *    choosen winding order. 
    *    
    * 6. It is more efficient to use methods that draws several primitives at once rather than issuing
    *    multiple commands for drawing triangle/quads. For static geometry, `Renderer3D::drawMesh()` should be
    *    use whenever possible insteaed of `Renderer3D::drawQuads()`, `Renderer3D::drawTriangles()`...
    *    
    * 7. Wireframe drawing with 'high quality' is (currently) very slow. Use 'low quality' drawing if speed is required.    
    * 
    */
    template<typename color_t, Shader LOADED_SHADERS = TGX_SHADER_MASK_ALL, typename ZBUFFER_t = float>
    class Renderer3D
    {
       

        static constexpr int MAXVIEWPORTDIMENSION = 2048 * (1 << ((8 - TGX_RASTERIZE_SUBPIXEL_BITS) >> 1)); ///< maximum viewport size in each direction (depending on the value of TGX_RASTERIZE_SUBPIXEL_BITS). 

        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");
        static_assert((std::is_same<ZBUFFER_t, float>::value) || (std::is_same<ZBUFFER_t, uint16_t>::value), "The Z-buffer type must be either float or uint16_t");
                    
        // true if some kind of texturing may be used. 
        static constexpr int ENABLE_TEXTURING = (TGX_SHADER_HAS_ONE_FLAG(LOADED_SHADERS , (SHADER_TEXTURE | TGX_SHADER_MASK_TEXTURE_MODE | TGX_SHADER_MASK_TEXTURE_QUALITY)));
        
        static constexpr Shader ENABLED_SHADERS = LOADED_SHADERS | (ENABLE_TEXTURING ? SHADER_TEXTURE : SHADER_NOTEXTURE); // enable texturing when at least one texturing related flag is set
        
        // check that disabled shaders do not completely disable all drawing operations.         
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_PROJECTION), "At least one of the two shaders SHADER_PERSPECTIVE or SHADER_ORTHO must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_ZBUFFER), "At least one of the two shaders SHADER_NOZBUFFER or SHADER_ZBUFFER must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_SHADING), "At least one of the two shaders SHADER_FLAT or SHADER_GOURAUD must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE), "At least one of the two shaders SHADER_TEXTURE or SHADER_NOTEXTURE must be enabled");                              
        static_assert((~(TGX_SHADER_HAS_TEXTURE(ENABLED_SHADERS))) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE_QUALITY)),"When using texturing, at least one of the two shaders SHADER_TEXTURE_BILINEAR or SHADER_TEXTURE_NEAREST must be enabled");
        static_assert((~(TGX_SHADER_HAS_TEXTURE(ENABLED_SHADERS))) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS, TGX_SHADER_MASK_TEXTURE_MODE)), "When using texturing, at least one of the two shaders SHADER_TEXTURE_WRAP_POW2 or SHADER_TEXTURE_CLAMP must be enabled");



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
        * the viewport in order to perform 'tile rendering'. see `setOffset()`. 
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
         * @returns A copy ot hte current projection matrix.
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
        * - `SHADER_FLAT`:     Use flat shading (i.e. uniform color on faces). This is the fastest drawing method but it usually gives poor results when combined with 
        *   texturing. Lighting transitions between bright to dark aera may appear to flicker when using color with low resolution such as RGB565.
        *
        * - `SHADER_GOURAUD`:  Use gouraud shading (linear interpolation of vertex colors) This results in smoother color transitions and works well with texturing but is more CPU 
        *    intensive.
        *    
        * - `SHADER_TEXTURE`:  enable texture mapping. A texture must be stored in an Image<color_t> object.
        *                      - wrap mode can be set with setTextureWrappingMode() or passing along either `SHADER_TEXTURE_WRAP_POW2` or `SHADER_TEXTURE_CLAMP`.
        *                      - drawing quality can be set with setTextureQuality() or by passing along either `SHADER_TEXTURE_NEAREST` or `SHADER_TEXTURE_BILINEAR`.
        *
        * @remark                     
        * 1. If a shader flag is set with SetShaders() but is disabled in the template parameter LOADED_SHADER,
        *    then the drawing calls will silently fail (draw nothing).
        * 2. The color on the face (for flat shading) or on the vertices (for gouraud shading) is computed
        *    according to Phong's lightning https://en.wikipedia.org/wiki/Phong_reflection_model.
        * 3. Texture mapping is 'perspective correct' and can be done with either SHADER_FLAT or SHADER_GOURAUD   
        *    selected. The color of a pixel is obtained by combining to texture color at that pixel with the lightning 
        *    at the position according to phong's lightning model.
        * 4. For large textures stored in flash memory may be VERY slow to access when the texture is not 
        *    read linearly which will happens for some (bad) triangle orientations and then cache becomes useless... 
        *    This problem can be somewhat mitigated by:
        *    - splitting large textured triangles into smaller ones: then each triangle only accesses a small part 
        *      of the texture. This helps a lot wich cache optimization (this is why models with thousands of faces 
        *      may display faster that a simple textured cube in some cases).
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
         * @name Scene related methods.
         *
         * Methods related to properties specific to the scene: camera position, lightning...
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
         * @param   P   Point in the word coordinate system.
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
         * @param   P   Point in the word coordinate system.
         *
         * @returns the coordinates of the associated pixel on the image.
         * 
         * @remark
         * - The position returned may be outside of the image !
         * - Returns (0,0) if no image is inserted. 
         */
        iVec2 worldToImage(fVec3 P);


        /**
         * Set the light source direction.
         * 
         * The 3d rendered uses a single 'directional light' i.e. a light source comming from infinity (such as the sun). 
         * 
         * @param   direction   The direction the light point toward given in world coordinates.
         *
         * @sa setLight(), setLightAmbiant(), setLightDiffuse(), setLightSpecular()
         */
        void setLightDirection(const fVec3& direction);


        /**
         * Set the scene ambiant light color.
         *
         * See Phong's lightning model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         * 
         * @param   color   color for the ambiant light. 
         *
         * @sa setLight(), setLightDirection(), setLightDiffuse(), setLightSpecular()
         */
        void setLightAmbiant(const RGBf& color);


        /**
         * Set the scene diffuse light color.
         * 
         * See Phong's lightning model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * @param   color   color for the difuse light.
         *
         * @sa setLight(), setLightDirection(), setLightAmbiant(), setLightSpecular()
         */
        void setLightDiffuse(const RGBf& color);


        /**
         * Set the scene specular light color.
         * 
         * See Phong's lightning model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         *
         * @param   color   color for the specular light.
         *                  
         * @sa setLight(), setLightDirection(), setLightAmbiant(), setLightDiffuse()
         */
        void setLightSpecular(const RGBf& color);


        /**
         * Set all the lighting parameters of the scene at once.
         *
         * The 3d rendered uses a single 'directional light' i.e. a light source comming from infinity (such as the sun).
         *
         * See Phong's lightning model: https://en.wikipedia.org/wiki/Phong_reflection_model.
         * 
         * @param   direction       direction the light source point toward.
         * @param   ambiantColor    light ambiant color.
         * @param   diffuseColor    light diffuse color.
         * @param   specularColor   light specular color.
         *
         * @sa setLightDirection(), setLightAmbiant(), setLightDiffuse(), setLightSpecular()
         */
        void setLight(const fVec3 direction, const RGBf& ambiantColor, const RGBf& diffuseColor, const RGBf& specularColor);




        
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
         * Set the model tranformation matrix.
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
         * Return the model tranformation matrix.
         *
         * @returns a copy of the current model tranformation matrix.
         *
         * @sa  setModelMatrix(), setModelPosScaleRot()
        **/
        fMat4  getModelMatrix() const;


        /**
         * Set the model tranformation matrix to move an object to a given a given location, scale and
         * rotation.
         * 
         * The method is such that, if a model is initially centered around the origin in model
         * coordinate, then it will be placed at a given position/scale/rotation in the world
         * coordinates after multiplication by the model transformation matrix.
         * 
         * Transforms are done in the following order:
         * 
         * 1. The model is scaled in each direction in the model coord. according to `scale`
         * 2. The model is rotated in model coord. around direction `rot_dir` and with an angle `rot_angle` (in degree).
         * 3. The model is translated to position `center` in the world coord.
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
        * @returns the projection of `P` on the standard viewport `[-1,1]^2`` according 
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
         * @param   strenght    ambiant lightreflection strength in [0.0f,1.0f]. Default value 0.1f.
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
         * The epxonent should be range between 0 (no specular lightning) and 100 (very localized/glossy).
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
         * @param   ambiantStrength     The ambiant light reflection strength.
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
         * 1. Normal vector are mandatory when using Gouraud shadding and must be normalized (unit lenght).
         * 2. Texture dimension must be power of two when using flag `SHADER_TEXTURE_WRAP_POW2`.  
         * 3. Triangle and quads must be given in the correct winding order. The 4 vertices of a quads must be co-planar.  
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
         *   triangles/quads) so it should be the prefered method whenever working with static geometry. 
         * - The mesh can be located in any memory region (RAM, FLASH...) but using a fast memory will   
         *   improve renderin speed noticeably. The methods `cacheMesh()` (or `copyMeshEXTMEM()` on Teensy) are
         *   available to copy a mesh to a faster memory location before rendering. 
         */
        void drawMesh(const Mesh3D<color_t>* mesh, bool use_mesh_material = true, bool draw_chained_meshes = true);


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
         * - If Gouraud shading is enabled, the normal vector must all have have unit norm.
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
         * - If Gouraud shading is enabled, the normal vector must all have have unit norm.
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
         * - This method is useful for drawing a sky-box. 
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
         * - This method is useful for drawing a sky-box.
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
         * Draw a unit radius sphere centered at the origin `S(0,1)` in model space.
         * 
         * @remark
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere with a given number of sector and stacks.
         *
         * @param   nb_sectors  number of sectors of the UV sphere.
         * @param   nb_stacks   number of stacks of the UV sphere.
         */
        void drawSphere(int nb_sectors, int nb_stacks);


        /**
         * Draw a textured unit radius sphere centered at the origin S(0,1) in model space.
         * 
         * @remark
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere with a given number of sector and stacks.
         * - The texture is mapped using the Mercator projection.
         *
         * @param   nb_sectors  number of sectors of the UV sphere.
         * @param   nb_stacks   number of stacks of the UV sphere.
         * @param   texture     The texture (mapped via Mercoator projection)
         */
        void drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture);


        /**
         * Draw a unit radius sphere centered at the origin S(0,1) in model space.
         * 
         * @remark
         * - The model transform matrix may be used position the sphere anywhere in world space and
         * change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sector and stacks is adjusted
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
         * - The model transform matrix may be used position the sphere anywhere in world space   
         *   and change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sector and stacks is adjusted   
         *   automatically according to the apparent size on the screen.
         * - The texture is mapped using the Mercator projection.
         *
         * @param   texture The texture image mapped via Mercator projection. 
         * @param   quality (Optional) Quality of the mesh. Should be positive, typically between 0.5f
         *                  and 2.0f.
         *                  - `1` : default quality
         *                  - `>1`: finer mesh. Improve quality but decrease speed.
         *                  - `<1`: coarser mesh. Decrease quality but improve speed.
         */
        void drawAdaptativeSphere(const Image<color_t>* texture, float quality = 1.0f);








        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing wireframe geometric primitives. 
         *       
         * Methods for drawing wireframe meshes, triangles and quads.
         *
         * Two versions for each method :
         * 1. Fast but low quality drawing.
         * 2. Slow but high quality drawing (adjustable thickness, alpha-blending, anti-aliasing).  
         * 
         * @remark Wireframe methods do not take the scene lightning into account. 
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw a mesh in wireframe [*low quality*].
         * 
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The mesh is drawn with the current material color (not that of the mesh). This method does not
         *   require a zbuffer but back face culling is used if it is enabled.
         *
         * @param   mesh                The mesh to draw
         * @param   draw_chained_meshes True to draw also the chained meshes.
        **/
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes = true);


        /**
         * Draw a mesh in wireframe [*high quality*].
         * 
         * @remark
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing. 
         * - This method does not require a zbuffer but back face culling is used if it is enabled.
         *
         * @warning This method is very slow (may be slower that solid drawing) !  
         * 
         * @param   mesh                The mesh to draw
         * @param   draw_chained_meshes True to draw also the chained meshes.
         * @param   thickness           thickness of the lines.
         * @param   color               color to use.
         * @param   opacity             opacity multiplier in [0.0f, 1.0f].
        **/
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe line segment [*low quality*].
         * 
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The line is drawn with the current material color.
         *
         * @param   P1,P2  endpoints in model space.
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2);


        /**
         * Draw a wireframe line segment [*high quality*].
         *
         * @remark This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         *         
         * @warning This method is very slow (may be slower that solid drawing) !
         *
         * @param   P1,P2       endpoints in model space.
         * @param   thickness   thickness of the line
         * @param   color       color to use
         * @param   opacity     opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe line segments [*low quality*].
         * 
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         *
         * @param   nb_lines        number of lines to draw
         * @param   ind_vertices    array of vertex indices. The length of the array is `nb_lines*2` and each 2 consecutive values represent a line segment.
         * @param   vertices        The array of vertices in model space.
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe line segments [*high quality*].
         * 
         * @remark This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
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
         * Draw a wireframe triangle [*low quality*].
         *
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.  
         * - This method does not use the z-buffer but backface culling is used if enabled. 
         *
         * @param   P1, P2, P3      the triangle vertices in model space.
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3);


        /**
         * Draw a wireframe triangle [*high quality*].
         *
         * @remark 
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
         *
         * @param   P1, P2, P3      triangle vertices in model space.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe triangles [*low quality*].
         *
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.  
         * 
         * @param   nb_triangles    number of triangles to draw. 
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_triangles*3` and each 3 consecutive values represent a triangle.
         * @param   vertices        Array of vertices in model space. 
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe triangles [*high quality*].
         *
         * @remark
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
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
         * Draw a wireframe quad [*low quality*].
         *
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.  
         * - The four vertices of the quads must be co-planar. 
         *
         * @param   P1, P2, P3,P4       the quad vertices in model space.
        **/
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4);


        /**
         * Draw a wireframe quad [*high quality*].
         *
         * @remark
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of the quads must be co-planar.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
         *
         * @param   P1, P2, P3,P4       the quad vertices in model space.
         * @param   thickness           thickness of the lines
         * @param   color               color to use
         * @param   opacity             opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness, color_t color, float opacity);


        /**
         * Draw a collection of wireframe quads [*low quality*].
         *
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The lines are drawn with the current material color.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of the quads must be co-planar.
         *
         * @param   nb_quads        number of quads to draw.
         * @param   ind_vertices    Array of vertex indexes. The length of the array is `nb_quads*4` and each 4 consecutive values represent a quad.
         * @param   vertices        Array of vertices in model space.
         */
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices);


        /**
         * Draw a collection of wireframe quads [*high quality*].
         * 
         * @remark
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - This method does not use the z-buffer but backface culling is used if enabled.
         * - The four vertices of the quads must be co-planar.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
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
         * Methods for drawing wireframe cubes and spheres. 
         * 
         * Two versions for each method :
         * 1. Fast but low quality drawing.  
         * 2. Slow but high quality drawin (adjustable thickness, alpha-blending, anti-aliasing).     
         *
         * @remark Wireframe methods do not take the scene lightning into account.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/


        /**
         * Draw the wireframe cube [0,1]^3 (in model space) [*low quality*].
         *
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.
         */
        void drawWireFrameCube();


        /**
         * Draw the wireframe cube [0,1]^3 (in model space) [*high quality*].
         *
         * @remark
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - The model transform matrix may be used to scale, rotate and position the cube anywhere in world space.  
         *
         * @warning This method is very slow (may be slower that solid drawing) !
         *
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
         */
        void drawWireFrameCube(float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*low quality*].
         * 
         * @remark
         * - Create a UV-sphere with a given number of sector and stacks.
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         *
         * @param   nb_sectors  number of sectors in the UV sphere.
         * @param   nb_stacks   number of stacks in the UV sphere.
         */
        void drawWireFrameSphere(int nb_sectors, int nb_stacks);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*high quality*].
         *
         * @remark
         * - Create a UV-sphere with a given number of sector and stacks.
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
         * 
         * @param   nb_sectors      number of sectors in the UV sphere.
         * @param   nb_stacks       number of stacks in the UV sphere.
         * @param   thickness       thickness of the lines
         * @param   color           color to use
         * @param   opacity         opacity multiplier in [0.0f, 1.0f]
        **/
        void drawWireFrameSphere(int nb_sectors, int nb_stacks, float thickness, color_t color, float opacity);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*low quality*].
         * 
         * @remark
         * - This method use (fast) low quality drawing: no thickness, no blending, no anti-aliasing.
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         * - The mesh created is a UV-sphere and the number of sector and stacks is adjusted automatically according to the apparent size on the screen.
         *
         * @param quality   Quality of the mesh. Should be positive, typically between 0.5f and 2.0f.
         *                  - `1` : default quality
         *                  - `>1`: finer mesh. Improve quality but decrease speed.
         *                  - `<1`: coarser mesh. Decrease quality but improve speed.
         */
        void drawWireFrameAdaptativeSphere(float quality = 1.0f);


        /**
         * Draw a wireframe unit radius sphere centered at the origin (in model space) [*high quality*].
         *
         * @remark
         * - The model transform matrix may be used position the sphere anywhere in world space and change it to an ellipsoid.
         * - This method use high quality drawing: blending with opacity, thickness, and anti-aliasing.
         * - The mesh created is a UV-sphere and the number of sector and stacks is adjusted automatically according to the apparent size on the screen.
         *
         * @warning This method is very slow (may be slower that solid drawing) !
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





        ///@}
        /*****************************************************************************************
        *****************************************************************************************/
        /**
         * @name Drawing 3D point clouds
         *       
         * Methods for drawing pixels and dots.
         *
         * @remark the methods do not take the scene lightning into account.
         */
        ///@{
        /*****************************************************************************************
        ******************************************************************************************/



        /**
         * Draw a single pixel at a given position in model space.
         *
         * @remark
         * - Use the material color.
         * - The scene lightning is ignored.  
         * 
         * @param   pos Position (in model space).
         */
        void drawPixel(const fVec3& pos);


        /**
         * Draw a single pixel at a given position in model space.
         * 
         * @remark
         *  - Use blending with a given color and opacity factor.
         *  - The scene lightning is ignored.
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
         *  - The scene lightning is ignored.
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
         *  - The scene lightning is ignored.
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
         * - The scene lightning is ignored.
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
         * - The scene lightning is ignored.
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
         * - The scene lightning is ignored.
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
         * - The scene lightning is ignored.
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


        TGX_NOINLINE void _rectifyShaderTextureWrapping();


        TGX_NOINLINE void _rectifyShaderTextureQuality();
       

       /***********************************************************
        * DRAWING STUFF
        ************************************************************/


        /** draw a triangle and takes care of clipping (slow, called by the other methods only when necessary) */
        void _drawTriangleClipped(const int RASTER_TYPE,
            const fVec4* Q0, const fVec4* Q1, const fVec4* Q2,
            const fVec3* N0, const fVec3* N1, const fVec3* N2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2);


        /** used by _drawTriangleClipped() */
        void _drawTriangleClippedSub(const int RASTER_TYPE, const int plane,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3);


        /** draw a single triangle */
        void _drawTriangle(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2,
            const fVec3* N0, const fVec3* N1, const fVec3* N2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2);
        

        /** draw a single quad : the 4 points are assumed to be coplanar */
        void _drawQuad(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2, const fVec3* P3,
            const fVec3* N0, const fVec3* N1, const fVec3* N2, const fVec3* N3,
            const fVec2* T0, const fVec2* T1, const fVec2* T2, const fVec2* T3,
            const RGBf& Vcol0, const RGBf& Vcol1, const RGBf& Vcol2, const RGBf& Vcol3);


        /** Method called by drawMesh() which does the actual drawing. */
        void _drawMesh(const int RASTER_TYPE, const Mesh3D<color_t>* mesh);



        /***********************************************************
        * Drawing wireframe
        ************************************************************/


        template<bool DRAW_FAST> void _drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameLine(const fVec3& P1, const fVec3& P2, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> void _drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);




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


        template<bool CHECKRANGE, bool USE_BLENDING> TGX_INLINE inline void drawHLineZbuf(int x, int y, int w, color_t color, float opacity, float z)
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                const int lx = _uni.im->lx();
                const int ly = _uni.im->ly();
                if ((y < 0) || (y >= ly) || (x >= lx)) return;
                if (x < 0) { w += x; x = 0; }
                if (x + w > lx) { w = lx - x; }
                }
            while(w--) drawPixelZbuf<CHECKRANGE, USE_BLENDING>(x++, y, color, opacity, z);
            }


        template<bool CHECKRANGE, bool USE_BLENDING> void _drawCircleZbuf(int xm, int ym, int r, color_t color, float opacity, float z);


        /**
        * For adaptative mesh: compute the diameter (in pixels) of the unit sphere S(0,1) once projected the screen
        **/
        float _unitSphereScreenDiameter();


        template<bool WIREFRAME, bool DRAWFAST> void _drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture, float thickness, color_t color, float opacity);



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
        bool _discardBox(const fBox3 & bb, const fMat4 & M)
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
        bool _discardTriangle(const fVec4 & P1, const fVec4 & P2, const fVec4 & P3)
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
        bool _clip2(float clipboundXY, const fVec3 & P, const fMat4 & M)
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
        bool _clipTestNeeded(float clipboundXY, const fBox3 & bb, const fMat4 & M)
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
        inline float _cpdist(const tgx::fVec4& CP, float off, const tgx::fVec4& P)
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
        inline float _cpfactor(const tgx::fVec4& CP, const float sdistA, const float sdistB)
            {
            return sdistA / (sdistA - sdistB);
            }


        /** used by _triangleClip when only 1 point is inside the view */
        void _triangleClip1in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4);


        /** used by _triangleClip when only 2 points are inside the view */
        void _triangleClip2in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4);


        int _triangleClip(int shader, tgx::fVec4 CP, float off,
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


        /** compute a color according to Phong lightning model, use model color */
        template<bool TEXTURE> TGX_INLINE inline RGBf _phong(float v_diffuse, float v_specular) const
            {
            RGBf col = _r_ambiantColor;
            col += _r_diffuseColor * max(v_diffuse, 0.0f);
            col += _r_specularColor * _powSpecular(v_specular); // pow() this is too slow so we use a lookup table instead
            if (!(TEXTURE)) col *= _r_objectColor;
            col.clamp();
            return col;
            }


        /** compute a color according to Phong lightning model, use given color */
        TGX_INLINE  inline RGBf _phong(float v_diffuse, float v_specular, RGBf color) const
            {
            RGBf col = _r_ambiantColor;
            col += _r_diffuseColor * max(v_diffuse, 0.0f);
            col += _r_specularColor * _powSpecular(v_specular); // pow() this is too slow so we use a lookup table instead
            col *= color;
            col.clamp();
            return col;
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
        int   _texture_wrap_mode;   // wrapping mode (wrap_pow2 or clamp)
        int   _texture_quality;     // texturing quality (nearest or bilinear)

        // *** scene parameters ***

        fMat4   _viewM;             // view transform matrix

        fVec3   _light;             // light direction
        RGBf    _ambiantColor;      // light ambiant color
        RGBf    _diffuseColor;      // light diffuse color
        RGBf    _specularColor;     // light specular color


        // *** model specific parameters ***

        fMat4   _modelM;            // model transform matrix

        RGBf    _color;             // model color (use when texturing is disabled)
        float   _ambiantStrength;   // ambient light reflection strength
        float   _diffuseStrength;   // diffuse light reflection strength
        float   _specularStrength;  // specular light reflection strength
        int     _specularExponent;  // specular exponent


        // *** pre-computed values ***
        fMat4 _r_modelViewM;        // model-view matrix
        float _r_inorm;             // inverse of the norm of a unit vector after view transform
        fVec3 _r_light;             // light vector in view space (inverted and normalized)
        fVec3 _r_light_inorm;       // same as above but alreadsy muliplied by inorm
        fVec3 _r_H;                 // halfway vector.
        fVec3 _r_H_inorm;           // same as above but already muliplied by inorm
        RGBf _r_ambiantColor;       // ambient color multipled by object ambient strenght
        RGBf _r_diffuseColor;       // diffuse color multipled by object diffuse strenght
        RGBf _r_specularColor;      // specular color multipled by object specular strenght
        RGBf _r_objectColor;        // color to use for drawing the object (either _color or mesh->color).


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

