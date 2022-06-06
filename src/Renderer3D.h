/** @file Renderer3D.h */
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

    /** forward declaration for the vertices and faces of the unit cube [-1,1]^3 */
    extern const tgx::fVec3 UNIT_CUBE_VERTICES[8];
    extern const uint16_t UNIT_CUBE_FACES[6*4];



    /**
    * Class that manages the drawing of 3D objects.
    *
    * The class create a virtual viewport and provides methods to draw 3D primitives 
    * onto this viewport (to which is attached an Image<color_t> object)
    *
    * Template parameters:
    *
    * - color_t : the color type for the image to draw onto.
    *
    * - LOADED_SHADERS :   
    *  
    *   A list of all shaders that can be used with this object. By default, all shaders are loaded. 
    *    
    *    -> LOADING ONLY THE REQUIRED SHADERS SAVES *A LOT* OF MEMORY SPACE AND INCREASES PERFORMANCE.
    *                    
    *   A combination of the following flags:
    *                    
    *   - TGX_SHADER_PERSPECTIVE        when set, drawing with perspective projection is allowed
    *   - TGX_SHADER_ORTHO              when set, drawing with orthographic projection is allowed
    *   - TGX_SHADER_NOZBUFFER          when set, drawing without using a z-buffer is allowed
    *   - TGX_SHADER_ZBUFFER            when set, drawing while using a z-buffer is allowed
    *   - TGX_SHADER_FLAT               when set, drawing with flat shading is allowed
    *   - TGX_SHADER_GOURAUD            when set, drawing with gouraud shading is allowed
    *   - TGX_SHADER_NOTEXTURE          when set, drawing without using a texture is allowed
    *   - TGX_SHADER_TEXTURE_NEAREST    when set, drawing with a texture using nearest neighbour interpolation is allowed
    *   - TGX_SHADER_TEXTURE_BILINEAR   when set, drawing with a texture using bilinear interpolation is allowed
    *   - TGX_SHADER_TEXTURE_WRAP_POW2  when set, drawing with a texture using wrap around mode (with dimensions of texture being power of two) is allowed
    *   - TGX_SHADER_TEXTURE_CLAMP      when set, drawing with a texture using clamping to edge mode is allowed
    *                      
    *   NOTE: if a drawing call is made that requires a disabled shader, then
    *         the drawing operation fails silently (i.e. it is simply ignored). 
    *         
    *
    * - ZBUFFER_t : type used for storing z-buffer values. Must be either `float` or `uint16_t`.
    *               The z-buffer must be as large as the image (but can be smaller than the viewport when using an offset).
    *               -> float : higher quality but requires 4 bytes per pixel.
    *               -> uint16_t : lower quality (z-fighting may occur) but only 2 bytes per pixel.
    **/
    template<typename color_t, int LOADED_SHADERS = TGX_SHADER_MASK_ALL, typename ZBUFFER_t = float>
    class Renderer3D
    {
       
        static const int MAXVIEWPORTDIMENSION = 2048 * (1 << ((8 - TGX_RASTERIZE_SUBPIXEL_BITS) >> 1));
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");
        static_assert((std::is_same<ZBUFFER_t, float>::value) || (std::is_same<ZBUFFER_t, uint16_t>::value), "The Z-buffer type must be either float or uint16_t");
                    
        // true if some kind of texturing may be used. 
        static const int ENABLE_TEXTURING = (TGX_SHADER_HAS_ONE_FLAG(LOADED_SHADERS , (TGX_SHADER_TEXTURE | TGX_SHADER_MASK_TEXTURE_MODE | TGX_SHADER_MASK_TEXTURE_QUALITY)));
        
        static const int ENABLED_SHADERS = LOADED_SHADERS  
                                         | (ENABLE_TEXTURING ? TGX_SHADER_TEXTURE : TGX_SHADER_NOTEXTURE); // enable texturing when at least one texturing related flag is set
        
        // check that disabled shaders do not completely disable all drawing operations.         
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_PROJECTION), "At least one of the two shaders TGX_SHADER_PERSPECTIVE or TGX_SHADER_ORTHO must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_ZBUFFER), "At least one of the two shaders TGX_SHADER_NOZBUFFER or TGX_SHADER_ZBUFFER must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_SHADING), "At least one of the two shaders TGX_SHADER_FLAT or TGX_SHADER_GOURAUD must be enabled");        
        static_assert(TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE), "At least one of the two shaders TGX_SHADER_TEXTURE or TGX_SHADER_NOTEXTURE must be enabled");                              
        static_assert((~(TGX_SHADER_HAS_TEXTURE(ENABLED_SHADERS))) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS,TGX_SHADER_MASK_TEXTURE_QUALITY)),"When using texturing, at least one of the two shaders TGX_SHADER_TEXTURE_BILINEAR or TGX_SHADER_TEXTURE_NEAREST must be enabled");
        static_assert((~(TGX_SHADER_HAS_TEXTURE(ENABLED_SHADERS))) || (TGX_SHADER_HAS_ONE_FLAG(ENABLED_SHADERS, TGX_SHADER_MASK_TEXTURE_MODE)), "When using texturing, at least one of the two shaders TGX_SHADER_TEXTURE_WRAP_POW2 or TGX_SHADER_TEXTURE_CLAMP must be enabled");



       public:



        /*****************************************************************************************
        ******************************************************************************************
        *
        * General parameters
        *
        * The methods below define how 3D objects are rendered onto the viewport
        *
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Constructor. 
        *
        * Optionally set some parameters: viewport size, destination image and zbuffer (if used). 
        **/
        Renderer3D(const iVec2& viewportSize = {0,0}, Image<color_t> * im = nullptr, ZBUFFER_t * zbuffer = nullptr);



        /**
        * Set the size of the viewport, up to 4096x4096. 
        * The normalized coordinates in [-1,1]x[-1,1} are mapped to [0,lx-1]x[0,ly-1] just before rasterization.
        * It is possible to use a viewport larger than the image drawn onto by using an offset for the image inside 
        * the viewport in order to perform 'tile rendering'.        
        **/
        void setViewportSize(int lx, int ly)
            {
            _lx = clamp(lx, 0, MAXVIEWPORTDIMENSION);
            _ly = clamp(ly, 0, MAXVIEWPORTDIMENSION);
            }

        /**
        * Set the size of the viewport, up to 4096x4096. 
        * Same as above but in vector form. 
        **/
        void setViewportSize(const iVec2& viewport_dim)
            {
            setViewportSize(viewport_dim.x, viewport_dim.y);
            }



        /**
        * Set the image that will be drawn onto.
        * The image can be smaller than the viewport.
        **/
        void setImage(Image<color_t>* im)
            {
            _uni.im = im;            
            }


        /**
        * Set the offset of the image relative to the viewport.
        *
        * If the image has size (sx,sy), then during rasterization only the sub part
        * [ox, ox + sx[x[oy, oy+sy[ of the viewport will be drawn (onto the image).
        *
        * By changing the offset and redrawing several times it it possible to use an image
        * smaller than the viewport (and thus also save on zbuffer space).
        *
        * For example, to draw a 320x240 viewport with limited amount of memory. One can use an
        * image of size 160x120 (37.5kb) and a zbuffer of the same size (75Kb) and then call the
        * drawing method 4 times with offset (0,0), (0,120), (160,0) and (160,120) and upload
        * the resulting image at its correct position on the screen between each rendering.
        *
        * Note: do not forget to clear the z-buffer after changing the offset !
        **/
        void setOffset(int ox, int oy)
            {
            _ox = clamp(ox, 0, MAXVIEWPORTDIMENSION);
            _oy = clamp(oy, 0, MAXVIEWPORTDIMENSION);
            }


        /**
        * Set the offset of the image relative to the viewport.
        * Same as above but in vector form.
        **/
        void setOffset(const iVec2& offset)
            {
            this->setOffset(offset.x, offset.y);
            }


        /**
        * Set the projection matrix.
        *
        * This is the matrix that is used to project coordinate from
        * 'view space' to NDC.
        *
        * -> When using perspective projection, the projection matrix must
        *    store -z into the w component.
        *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
        **/
        void setProjectionMatrix(const fMat4& M)
            {
            _projM = M;
            _projM.invertYaxis();
            _recompute_wa_wb();
            }


        /**
        * Get a copy of the current projection matrix.
        **/
        fMat4 getProjectionMatrix() const
            {
            fMat4 M = _projM;
            M.invertYaxis();
            return M;
            }

        /**
        * Set projection mode to orthographic (ie no z-divide)
        **/
        void useOrthographicProjection()
            {
            static_assert(TGX_SHADER_HAS_ORTHO(ENABLED_SHADERS), "shader TGX_SHADER_ORTHO must be enabled to use useOrthographicProjection()");
            _ortho = true;
            _rectifyShaderOrtho();
            _recompute_wa_wb();
            }


        /**
        * Set projection mode to perspective (ie with z-divide)
        **/
        void usePerspectiveProjection()
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use usePerspectiveProjection()");
            _ortho = false;
            _rectifyShaderOrtho();
            _recompute_wa_wb();
            }


        /**
         * Set the projection matrix as an orthographic matrix:
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml
         *
         * This method automatically switches to orthographic projection mode.
         *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
         **/
        void setOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_ORTHO(ENABLED_SHADERS), "shader TGX_SHADER_ORTHO must be enabled to use setOrtho()");
            _projM.setOrtho(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
            useOrthographicProjection();
            }


        /**
         * Set the projection matrix as a perspective matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
        *
         * This method automatically switches to perspective projection mode.
         *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
        **/
        void setFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use setFrustrum()");
            _projM.setFrustum(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
            usePerspectiveProjection();
            }


        /**
        *  Set the projection matrix as a perspective matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
        *
        * This method is only available when ORTHO = false.
        *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
        **/
        void setPerspective(float fovy, float aspect, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use setPerspective()");
            _projM.setPerspective(fovy, aspect, zNear, zFar);
            _projM.invertYaxis();
            usePerspectiveProjection();
            }


        /**
        * Set the face culling strategy.
        *
        * - w > 0 : Vertices in front faces are ordered counter-clockwise CCW [default]. Clockwise faces are culled.
        * - w < 0 : Vertices in front faces are ordered clockwise (CW). Counter-clockwise faces are culled.
        * - w = 0 : Disable face culling: both clockwise and counter-clockwise faces are drawn.
        *
        * Remark: - When face culling is enabled (w != 0), and when gouraud shading is active, the normal vectors
        *           supplied for the vertices must, of course, be the normal vectors for the front side of the triangle.
        *         - When face culling is disabled (w = 0). Both face of a triangle are drawn so there is no more
        *           notion of 'front' and 'back' face. In this case, when using gouraud shading, by convention, the
        *           normal vector supplied must be those corresponding to the counter-clockwise face being shown
        *           (whatever this means since these normals vector are attached to  vertices and not faces anyway,
        *           but still...)
        **/
        void setCulling(int w)
            {
            _culling_dir = (w > 0) ? 1.0f : ((w < 0) ? -1.0f : 0.0f);
            }


        /**
        * Set the zbuffer.
        *
        * Warning: the zbuffer must be large enough to be used with the image that is being 
          drawn onto. This means that we must have length >= image.width()*image.height().
        *
        * - Setting a valid zbuffer automatically turns on z-buffering.
        * - Removing the z-buffer (by setting it to nullptr) turns off z-buffering.
        **/
        void setZbuffer(ZBUFFER_t * zbuffer)
            {
            static_assert(TGX_SHADER_HAS_ZBUFFER(ENABLED_SHADERS), "shader TGX_SHADER_ZBUFFER must be enabled to use setZbuffer()");
            _uni.zbuf = zbuffer;
            _rectifyShaderZbuffer();
            }


        /**
        * Clear the Zbuffer.
        *
        * This method should be called before drawing a new frame to erase
        * the previous zbuffer.
        *
        * The zbuffer is intentionally not clear between draw() calls to enable
        * the rendering of multiple objects on the same scene.
        **/
        void clearZbuffer()
            {
            static_assert(TGX_SHADER_HAS_ZBUFFER(ENABLED_SHADERS), "shader TGX_SHADER_ZBUFFER must be enabled to use clearZbuffer()");
            if ((_uni.zbuf) && (_uni.im != nullptr) && (_uni.im->isValid()))
                {
                memset(_uni.zbuf, 0, _uni.im->lx() * _uni.im->ly() * sizeof(ZBUFFER_t));        
                }
            }


        /**
        * Set the shaders to use for subsequent drawing operations. A combination of the following flags.
        * 
        * NOTE: If a shader flag is set with SetShaders but is disabled in the template parameter LOADED_SHADER, 
        *       then the drawing calls will silently fail (draw nothing) or, in the best case, sometime fall-back 
        *       using another shader that is loaded. 
        * 
        * - Choosing the shading algorithm: TGX_SHADER_FLAT or TGX_SHADER_GOURAUD (see https://en.wikipedia.org/wiki/Shading)
        *                                  
        *     TGX_SHADER_FLAT     Use flat shading (i.e. uniform color on faces). This is the fastest
        *                         drawing method but usually gives poor results when combined with texturing.
        *                         Lighting transition between bright to dark aera may appear to 'flicker'
        *                         when color_t = RGB565 because of the limited number of colors/shades
        *                         available.
        *
        *                      -> the color on the face is computed according to Phong's lightning
        *                         model use the triangle face normals computed via crossproduct.
        *                         https://en.wikipedia.org/wiki/Phong_reflection_model
        *                         
        *
        *     TGX_SHADER_GOURAUD  Give a color to each vertex and then use linear interpolation to
        *                         shade each triangle according to its vertex colors. This results in
        *                         smoother color transitions and works well with texturing but at a
        *                         higher CPU cost.
        *
        *                      -> In order to use gouraud shading, a normal vector must be attributed
        *                          to each vertex, which is then used to determine its color according
        *                          to phong's lightning model.
        *
        *                      -> *** NORMAL VECTORS MUST ALWAYS BE NORMALIZED (UNIT LENGHT) ***
        *
        *                      -> When backface culling is enable, the normal vector must be that of
        *                         the 'front face' (obviously). When backface culling is  disabled,
        *                         there is no more 'front' and 'back' face: by convention, the normal
        *                         vector supplied must then be that corresponding to the counter-
        *                         clockwise face.
        *
        *  - Enabling (perspective correct) texture mapping.  
        *    Texture mapping is perspective correct and is performed in combination with TGX_SHADER_FLAT or 
        *    TGX_SHADER_GOURAUD. The color of a pixel in the triangle is obtained by combining to texture color 
        *    at that pixel with the lightning at the position (according to phong's lightning model again).
        *    
        *    TGX_SHADER_TEXTURE   enable texture mapping. 
        *                         -> A texture must be stored in an Image<color_t> object
        *                         
        *                         -> wrap mode is set with `setTextureWrappingMode()`
        *                         
        *                         -> drawing quality is set with `setTextureQuality()`
        *
        *                         -> NOTE: Large textures stored in flash memory may be VERY slow to access when
        *                            the texture is not read linearly which will happens for some (bad) triangle
        *                            orientations and then cache becomes useless... This problem can be somewhat
        *                            mitigated by:
        *
        *                            (a) splitting large textured triangles into smaller ones: then each triangle
        *                                only accesses a small part of the texture. This helps a lot wich cache
        *                                optimizatrion [this is why models with thousands of faces may display
        *                                faster that a simple textured cube in some cases :-)].
        *
        *                            (b) moving the image into RAM if possible. Even moving the texture from
        *                                FLASH to EXTMEM (if available) will usually give a great speed boost !
        **/
        void setShaders(int shaders)
            {               
            _rectifyShaderShading(shaders);
            }


        /**
        * Set the wrap mode when using texturing. One of:
        * 
        * TGX_SHADER_TEXTURE_WRAP_POW2      Wrap around (repeat) the texture. This is the fastest mode.
        * 
        *                                   !!! WARNING : WHEN USING THIS FLAG, THE TEXTURE MUST HAVE DIMENSIONS 
        *                                       THAT ARE POWER OF 2 ALONG EACH AXIS !!!
        * 
        * 
        * TGX_SHADER_TEXTURE_CLAMP          Clamp to the edge. Slower than above but the texture can be any size. 
        **/
        void setTextureWrappingMode(int wrap_mode)
            {
            if (TGX_SHADER_HAS_TEXTURE_CLAMP(wrap_mode))
                {
                if (TGX_SHADER_HAS_TEXTURE_CLAMP(ENABLED_SHADERS))
                    _texture_wrap_mode = TGX_SHADER_TEXTURE_CLAMP;
                else
                    _texture_wrap_mode = TGX_SHADER_TEXTURE_WRAP_POW2; // fallback
                }
            else
                {
                if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(ENABLED_SHADERS))
                    _texture_wrap_mode = TGX_SHADER_TEXTURE_WRAP_POW2;
                else
                    _texture_wrap_mode = TGX_SHADER_TEXTURE_CLAMP; // fallback
                }
            _rectifyShaderTextureWrapping();
            }


        /**
        * Set the texturing quality. One of
        *
        * TGX_SHADER_TEXTURE_NEAREST      Use nearest neighbour point sampling when texturing. 
        *                                 (fastest method).
        *
        * TGX_SHADER_TEXTURE_BILINEAR     Use bilinear interpolation when texturing
        *                                 (slower but higher quality).
        **/
        void setTextureQuality(int quality)
            {
            if (TGX_SHADER_HAS_TEXTURE_BILINEAR(quality))
                {
                if (TGX_SHADER_HAS_TEXTURE_BILINEAR(ENABLED_SHADERS))
                    _texture_quality = TGX_SHADER_TEXTURE_BILINEAR;
                else
                    _texture_quality = TGX_SHADER_TEXTURE_NEAREST; // fallback
                }
            else
                {
                if (TGX_SHADER_HAS_TEXTURE_NEAREST(ENABLED_SHADERS))
                    _texture_quality = TGX_SHADER_TEXTURE_NEAREST;
                else
                    _texture_quality = TGX_SHADER_TEXTURE_BILINEAR; // fallback
                }
            _rectifyShaderTextureQuality();
            }



        /*****************************************************************************************
        ******************************************************************************************
        *
        * Scene specific parameters
        *
        * The method belows affect the rendering at the 'scene level': camera position, lightning..
        *
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Set the view tranformation matrix.
        *
        * This matrix is used to transform vertices from world coordinates to view coordinates
        * (ie from the point of view of the camera)
        *
        * Thus, changing this matrix changes the position of the camera in the world space.
        *
        * IMPORTANT: In view space (i.e. after transformation), the camera is assumed to be
        * centered at the origin, looking looking toward the negative Z axis with the Y axis
        * pointing up (as in opengl).
        **/
        void setViewMatrix(const fMat4& M)
            {
            _viewM = M;
            // recompute
            _r_modelViewM = _viewM * _modelM;
            _r_inorm = _r_modelViewM.mult0(fVec3{ 0,0,1 }).invnorm();
            _r_light = _viewM.mult0(_light);
            _r_light = -_r_light;
            _r_light.normalize();
            _r_light_inorm = _r_light * _r_inorm;
            _r_H = fVec3(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)...
            _r_H += _r_light;
            _r_H.normalize();
            _r_H_inorm = _r_H * _r_inorm;
            }


        /**
        * Get a copy of the current view matrix
        **/
        fMat4 getViewMatrix() const
            {
            return _viewM;
            }


        /**
        * Set the view matrix so that the camera is looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        *
        * eye       position of the camera in world space coords.
        * center    point the camera is looking toward in world space coords.
        * up        vector that tells the up direction for the camera (in world space coords).
        **/
        void setLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
            {
            fMat4 M;
            M.setLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
            setViewMatrix(M);
            }


        /**
        * Set the view matrix so that the camera is looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        *
        * eye       position of the camera in world space coords.
        * center    point the camera is looking toward in world space coords.
        * up        vector that tells the up direction for the camera (in world space coords).
        **/
        void setLookAt(const fVec3 eye, const fVec3 center, const fVec3 up)
            {
            setLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
            }


        /**
        * Convert from world coord. to the standard viewport coord.
        * 
        * - P : point given in the word coordinate system.    
        *
        * Return the projection of P on the standard viewport [-1,1]^2 according 
        * to the current position of the camera. 
        * 
        * Note: - the model matrix is not taken into account here.
        *       - the .w value can be used for depth testing. 
        **/
        fVec4 worldToViewPort(fVec3 P)
            {
            fVec4 Q = _projM * _viewM.mult1(P);
            if (!_ortho) Q.zdivide();
            return Q;
            }


        /**
        * Convert from world coord. to the corresponding image pixel.
        *
        * - P : point given in the word coordinate system.
        *
        * Return the position of the associated pixel on the image.
        * 
        * Note: - the position returned may be outside of the image ! 
        *       - return (0,0) if no image inserted. 
        **/
        iVec2 worldToImage(fVec3 P)
            {
            fVec4 Q = _projM * _viewM.mult1(P);
            if (!_ortho) Q.zdivide();
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
            return iVec2((int)roundfp(Q.x), (int)roundfp(Q.y));
            }


        /**
        * Set the light source direction (i.e. the direction the light points to).
        * Direction is given in world coordinates (hence transformed by the view Matrix
        * at rendering time so that the light does not move with the camera).
        **/
        void setLightDirection(const fVec3 & direction)
            {
            _light = direction;
            // recompute
            _r_light = _viewM.mult0(_light);
            _r_light = -_r_light;
            _r_light.normalize();
            _r_light_inorm = _r_light * _r_inorm;
            _r_H = fVec3(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)...
            _r_H += _r_light;
            _r_H.normalize();
            _r_H_inorm = _r_H * _r_inorm;
            }


        /**
        * Set the scene ambiant light color
        * (according to the Phong lightning model).
        **/
        void setLightAmbiant(const RGBf & color)
            {
            _ambiantColor = color;
            // recompute
            _r_ambiantColor = _ambiantColor * _ambiantStrength;
            }


        /**
        * Set the scene diffuse light color
        * (according to the Phong lightning model).
        **/
        void setLightDiffuse(const RGBf & color)
            {
            _diffuseColor = color;
            // recompute
            _r_diffuseColor = _diffuseColor * _diffuseStrength;
            }


        /**
        * Set the scene specular light color
        * (according to the Phong lightning model).
        **/
        void setLightSpecular(const RGBf & color)
            {
            _specularColor = color;
            // recompute
            _r_specularColor = _specularColor * _specularStrength;
            }


        /**
        * Set all the parameters of the scene light source at once.
        **/
        void setLight(const fVec3 direction, const RGBf & ambiantColor, const RGBf & diffuseColor, const RGBf & specularColor)
            {
            this->setLightDirection(direction);
            this->setLightAmbiant(ambiantColor);
            this->setLightDiffuse(diffuseColor);
            this->setLightSpecular(specularColor);
            }




        /*****************************************************************************************
        ******************************************************************************************
        *
        * Model specific parameters.
        *
        * The method below apply to the (current) object being draw: position in world space,
        * material properties...
        *
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Set the model tranformation matrix.
        *
        * This matrix describes the transformation from local object space to view space.
        * (i.e. the matrix specifies the position of the object in world space).
        **/
        void setModelMatrix(const fMat4& M)
            {
            _modelM = M;
            // recompute
            _r_modelViewM = _viewM * _modelM;
            _r_inorm = _r_modelViewM.mult0(fVec3{ 0,0,1 }).invnorm();
            _r_light_inorm = _r_light * _r_inorm;
            _r_H_inorm = _r_H * _r_inorm;
            }


        /**
        * Get a copy of the current model matrix
        **/
        fMat4  getModelMatrix() const
            {
            return _modelM;
            }



        /**
        * Set the model matrix in such way that a model centered at the origin (in model coordinate) 
        * will appear at a given position/scale/rotation on the world coordinates. 
        *
        * Transform are done in the following order:
        * 
        * (1) model is scaled in each direction in the model coord. according to 'scale'
        * (2) model is rotated in model coord. around direction 'rot_dir' and with an angle 'rot_angle' (in degree).
        * (3) model is translated to position 'center' in the world coord. 
        **/
        void setModelPosScaleRot(const fVec3& center = fVec3{ 0,0,0 }, const fVec3& scale = fVec3(1, 1, 1), float rot_angle = 0, const fVec3& rot_dir = fVec3{ 0,1,0 })
            {
            _modelM.setScale(scale);
            _modelM.multRotate(rot_angle, rot_dir);
            _modelM.multTranslate(center);
            // recompute
            _r_modelViewM = _viewM * _modelM;
            _r_inorm = _r_modelViewM.mult0(fVec3{ 0,0,1 }).invnorm();
            _r_light_inorm = _r_light * _r_inorm;
            _r_H_inorm = _r_H * _r_inorm;
            }


        /**
        * Convert from model coord. to the standard viewport coord.
        * 
        * - P : point given in the model coordinate system.    
        *
        * Return the projection of P on the standard viewport [-1,1]^2 according 
        * to the current position of the camera. 
        * 
        * Note:  the .w value can be used for depth testing. 
        **/
        fVec4 modelToViewPort(fVec3 P)
            {
            fVec4 Q = _projM * _r_modelViewM.mult1(P);
            if (!_ortho) Q.zdivide();
            return Q;
            }


        /**
        * Convert from model coord. to the corresponding image pixel.
        *
        * - P : point given in the model coordinate system.
        *
        * Return the position of the associated pixel on the image.
        **/
        iVec2 modelToImage(fVec3 P)
            {
            fVec4 Q = _projM * _r_modelViewM.mult1(P);
            if (!_ortho) Q.zdivide();
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
            return iVec2(roundfp(Q.x), roundfp(Q.y));
            }


        /**
        * Set the object material color.
        * This is the color used to render the object when texturing is not used.
        **/
        void setMaterialColor(RGBf color)
            {
            _color = color;
            // recompute
            _r_objectColor = _color;
            }


        /**
        * Set how much the object material reflects the ambient light.
        **/
        void setMaterialAmbiantStrength(float strenght = 0.1f)
            {
            _ambiantStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_ambiantColor = _ambiantColor * _ambiantStrength;
            }


        /**
        * Set how much the object material reflects the diffuse light.
        **/
        void setMaterialDiffuseStrength(float strenght = 0.6f)
            {
            _diffuseStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_diffuseColor = _diffuseColor * _diffuseStrength;
            }


        /**
        * Set how much the object material reflects the specular light.
        **/
        void setMaterialSpecularStrength(float strenght = 0.5f)
            {
            _specularStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_specularColor = _specularColor * _specularStrength;
            }


        /**
        * Set the object specular exponent.
        * Between 0 (no specular lightning) and 100 (very localized/glossy).
        **/
        void setMaterialSpecularExponent(int exponent = 16)
            {
            _specularExponent = clamp(exponent, 0, 100);
            }


        /**
        * Set all the object material properties all at once.
        **/
        void setMaterial(RGBf color, float ambiantStrength, float diffuseStrength, float specularStrength, int specularExponent)
            {
            this->setMaterialColor(color);
            this->setMaterialAmbiantStrength(ambiantStrength);
            this->setMaterialDiffuseStrength(diffuseStrength);
            this->setMaterialSpecularStrength(specularStrength);
            this->setMaterialSpecularExponent(specularExponent);
            }






        /*****************************************************************************************
        ******************************************************************************************
        *
        *                                  Drawing methods
        *
        ******************************************************************************************
        *
        * Methods to draw primitives (triangles/quads/meshes) onto the viewport.
        *
        * (1) The rasterizer uses the shaders set with setShaders(). If the requested shaders cannot
        *     be used because they are disabled in the template parameter DISABLED_SHADERS or because 
        *     the correct parameters are not supplied (e.g.  no normals for Gouraud shading or no image 
        *     for texturing) then the drawing operation silently fails and return without drawing anything
        *     (in the best case, it may fall back using another shader).
        *
        *     -> *** NORMAL VECTORS MUST ALWAYS BE NORMALIZED (UNIT LENGHT) ***
        *       
        *     -> *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        *
        *
        * (2) Depth testing is automatically performed when a z-buffer is available.
        *
        *     -> ***  DO NOT FORGET TO CLEAR THE ZBUFFER WITH clearZbuffer() BEFORE DRAWING A NEW SCENE ***
        *
        *
        * (3) Back face culling is automatically performed if enabled (via the setCulling() method)
        *
        *     -> *** TRIANGLES/QUADS MUST BE GIVEN TO THE DRAWING METHOD IN THEIR CORRECT WINDING ORDER ***
        *
        *
        * (4) It is much more efficient to use methods that draws several primitives at once than issuing
        *     multiple drawing commands for single triangles/quads. Use the following methods by order of
        *    preferences:
        *
        *    1 - drawMesh() is the 'most optimized' method and the one that will usually give the faster
        *        rendering so it should be used whenever possible (for static geometry).
        *
        *    3 - and drawQuads() Not as fast a drawing a mesh.
        *
        *    3 - drawTriangles() Even slower than the methods above.
        *
        *    4 - drawTriangle() / drawQuad()  Should only be used for drawing single triangles/quads
        *                                     slowest method because each draw call has an additional
        *                                     overhead.
        *
        ******************************************************************************************
        ******************************************************************************************/




        /**
        * Draw a mesh onto the image.
        *
        * ***************************************************************************************
        * THIS IS THE FASTEST METHOD FOR DRAWING AN OBJECT AND SHOULD BE USED WHENEVER POSSIBLE.
        * ***************************************************************************************
        *
        * - mesh    The mesh to draw. The meshes/vertices array/textures can be in RAM or in FLASH.
        *           Whenever possible, put vertex array and texture in RAM (or even EXTMEM).
        *
        * - use_mesh_material   If true, ignore the current object material properties set in the renderer
        *                       and use the mesh predefined material properties.
        *                       this flag affects also all the linked meshes if draw_chained_meshes=true.
        *
        * - draw_chained_meshes  If true, the meshes linked to this mesh (via the ->next member) are also drawn.
        **/
        void drawMesh(const Mesh3D<color_t>* mesh, bool use_mesh_material = true, bool draw_chained_meshes = true);




        /**
        * Draw a single triangle on the image.
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3)  pointers to the normals associated with (P1, P2, P3).  
        *               or nullptr if not using gouraud shading
        *               
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (T1,T2,T3)  pointers to the texture coords. associated with (P1, P2, P3).  
        *               or nullptr if not using texturing
        *
        * - texture     pointer to the texture image to use (or nullptr if not used)
        *
        *               *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        **/
        void drawTriangle(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                          const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr,
                          const fVec2 * T1 = nullptr, const fVec2 * T2 = nullptr, const fVec2 * T3 = nullptr,
                          const Image<color_t> * texture = nullptr);




        /**
        * Draw a single triangle on the image with a given color on each of its vertices.
        * The color inside the triangle is obtained by linear interpolation.
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (col1,col2,col3) Colors of the vertices.
        *
        * - (N1,N2,N3)  normals associated with (P1, P2, P3)
        *               or nullptr if not using gouraud shading.
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        **/
        void drawTriangleWithVertexColor(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                                         const RGBf & col1, const RGBf & col2, const RGBf & col3,
                                         const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr);



        /**
        * Draw a list of triangles on the image. If texture mapping is not used, then the current
        * material color is used to draw the triangles.
        *
        * - nb_triangles Number of triangles to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_triangles*3
        *                and each 3 consecutive values represent a triangle.
        *
        *               *** MAKE SURE THAT THE TRIANGLES ARE GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - vertices     The array of vertices (given in model space).
        *
        * - ind_normals  array of normal indexes. If specified, the array must
        *                have length nb_triangles*3.
        *
        * - normals      The array of normals vectors (in model space).
        *
        *                 *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - ind_texture  array of texture indexes. If specified, the array must
        *                have length nb_triangles*3.
        *
        * - textures     The array of texture coords.
        *
        * - texture_image The texture image to use.
        *
        *                 *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        **/
        void drawTriangles(int nb_triangles,
                           const uint16_t * ind_vertices, const fVec3 * vertices,
                           const uint16_t * ind_normals = nullptr, const fVec3* normals = nullptr,
                           const uint16_t * ind_texture = nullptr, const fVec2* textures = nullptr,
                           const Image<color_t> * texture_image = nullptr);



        /**
        * Draw a single quad on the image.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *                  *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3,N4)  normals associated with (P1, P2, P3, P4).
        *                  or nullptr if not using gouraud shading
        *
        *                  *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (T1,T2,T3,T4)  texture coords. associated with (P1, P2, P3).
        *                  or nullptr if not using texturing
        *
        * - texture     the texture image to use (or nullptr if unused).
        *
        *               *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        **/
        void drawQuad(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3 & P4,
                      const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr, const fVec3 * N4 = nullptr,
                      const fVec2 * T1 = nullptr, const fVec2 * T2 = nullptr, const fVec2 * T3 = nullptr, const fVec2 * T4 = nullptr,
                      const Image<color_t>* texture = nullptr);
        


        /**
        * Draw a single quad on the image with a given color on each of its four vertices.
        * The color inside the quad is obtained by linear interpolation.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (col1,col2,col3,col4) Colors of the vertices
        *
        * - (N1,N2,N3,N4)  normals associated with (P1, P2, P3, P4).
        *                  or nullptr if not using gouraud shading.
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        **/
        void drawQuadWithVertexColor(const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3 & P4,
                                     const RGBf & col1, const RGBf & col2, const RGBf & col3, const RGBf & col4,
                                     const fVec3 * N1 = nullptr, const fVec3 * N2 = nullptr, const fVec3 * N3 = nullptr, const fVec3 * N4 = nullptr);
         



        /**
        * Draw a list of quads on the image.  If texture mapping is not used, then the current
        * material color is used to draw the triangles.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - nb_quads     Number of quads to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_quads*4
        *                and each 4 consecutive values represent a quad.
        *
        *               *** MAKE SURE THAT THE QUADS ARE GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - vertices     The array of vertices (given in model space).
        *
        * - ind_normals  [Optional] array of normal indexes. If specified, the array must
        *                have length nb_quads*4.
        *
        * - normals      [Optional] The array of normals vectors (in model space).
        *
        *                 *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - ind_texture  [Optional] array of texture indexes. If specified, the array must
        *                have length nb_quads*4.
        *
        * - textures     [Optional] The array of texture coords.
        *
        * - texture_image [Optional] the texture image to use.
        *
        *                 *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        **/
        void drawQuads(int nb_quads,
                       const uint16_t * ind_vertices, const fVec3 * vertices,
                       const uint16_t * ind_normals = nullptr, const fVec3* normals = nullptr,
                       const uint16_t * ind_texture = nullptr, const fVec2* textures = nullptr,
                       const Image<color_t>* texture_image = nullptr);






        /*****************************************************************************************
        ******************************************************************************************
        *
        *                          Drawing simple geometric object
        *
        *
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Draw a single pixel on the image at a given position (in model space).
        *
        * Use the the material color.
        *
        * The scene lightning is ignored.
        **/
        void drawPixel(const fVec3& pos)
            {
            _drawPixel<false>(pos, _color, 1.0f);
            }



        /**
        * Draw a single pixel on the image at a given position (in model space).
        *
        * Use the specified color instead of the material color.
        *
        * The scene lightning is ignored.
        **/
        void drawPixel(const fVec3& pos, color_t color)
            {
            _drawPixel<false>(pos, color, 1.0f);
            }


        /**
        * Draw a single pixel on the image at a given position (given in model space).
        *
        * Use the specified color and opacity and use blending
        *         
        * The scene lightning is ignored.
        **/
        void drawPixel(const fVec3& pos, color_t color, float opacity)
            {
            _drawPixel<true>(pos, color, opacity);
            }


        /**
        * Draw a list of pixels on the image at given positions (in model space).
        *
        * Use the material color for all pixels.
        *
        * The scene lightning is ignored.
        **/
        void drawPixels(int nb_pixels, const fVec3* pos_list)
            {
            _drawPixels<false, false>(nb_pixels, pos_list, nullptr, nullptr, nullptr, nullptr);
            }


        /**
        * Draw a list of pixels on the image at given positions (in model space).
        *
        * Use a (possibly) different color for each pixel.
        * The color are given by a palette and a list of indices (one for each pixel).
        *
        * The scene lightning is ignored.
        **/
        void drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors)
            {
            _drawPixels<true,false>(nb_pixels, pos_list, colors_ind, colors, nullptr, nullptr);
            }


        /**
        * Draw a list of pixels on the image at given positions (in model space).
        *
        * Use different color/opacity for each pixel and use blending.
        * The color and opacities are both given by a palette and a list of indices
        * (one for each pixel)
        *
        * The scene lightning parameters are ignored.
        **/
        void drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            _drawPixels<true,true>(nb_pixels, pos_list, colors_ind, colors, opacities_ind, opacities);
            }




        /**
        * Draw a dot/circle on the image at a given position (in model space)
        * with a given radius in screen pixels.
        *
        * Use the the material color.
        *
        * The scene lightning is ignored.
        **/
        void drawDot(const fVec3& pos, int r)
            {
            _drawDot<false>(pos, r, _color, 1.0f);
            }


        /**
        * Draw a dot/circle on the image at a given position (in model space)
        * with a given radius in screen pixels.
        *
        * Use the specified color instead of the material color.
        *
        * The scene lightning is ignored.
        **/
        void drawDot(const fVec3& pos, int r, color_t color)
            {
            _drawDot<false>(pos, r, color, 1.0f);
            }


        /**
        * Draw a dot/circle on the image at a given position (in model space).
        *
        * Use the specified color and opacity and use blending
        *
        * The scene lightning is ignored.
        **/
        void drawDot(const fVec3& pos, int r, color_t color, float opacity)
            {
            _drawDot<true>(pos, r, color, opacity);
            }



        /**
        * Draw a list of dots/circles on the image at given positions (in model space).
        *
        * Use the material color for all dot.
        * Use the same radius for each dot.
        *
        * The scene lightning is ignored.
        **/
        void drawDots(int nb_dots, const fVec3* pos_list, const int radius)
            {
            _drawDots<false, false, false>(nb_dots, pos_list, nullptr, &radius, nullptr, nullptr, nullptr, nullptr);
            }


        /**
        * Draw a list of dots/circles on the image at given positions (in model space).
        *
        * Use the material color for all dot.
        * Use a possible different radius for each dot.
        *
        * The scene lightning is ignored.
        **/
        void drawDots(int nb_dots, const fVec3* pos_list, const int * radius_ind, const int * radius)
            {
            _drawDots<true, false,false>(nb_dots, pos_list, radius_ind, radius, nullptr, nullptr, nullptr, nullptr);
            }


        /**
        * Draw a list of dots/circles on the image at given positions (in model space).
        *
        * Use a possible different radius for each dot
        * Use a (possibly) different color for each dot.
        *
        * The scene lightning is ignored.
        **/
        void drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const fVec3* colors_ind, const color_t* colors)
            {
            _drawDots<true, true,false>(nb_dots, pos_list, radius_ind, radius, colors_ind, colors, nullptr, nullptr);
            }


        /**
        * Draw a list of dots/circles on the image at given positions (in model space).
        *
        * Use a possible different radius for each dot
        * Use a (possibly) different color for each dot
        * Use a (possibly) different opacity for each dot (and use blending when drawing)
        *
        * The scene lightning parameters are ignored.
        **/
        void drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int * colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            _drawDots<true, true,true>(nb_dots, pos_list, radius_ind, radius, colors_ind, colors, opacities_ind, opacities);
            }







        /**
        * Draw the cube [-1,1]^3 (in model space). 
        * 
        * -> Use the Model matrix to draw a rectangle and/or choose the position 
        *    in world coord.
        **/
        void drawCube()
            {
            // set culling direction = -1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES);
            // restore culling direction
            _culling_dir = save_culling; 
            }
            


        /**
        * draw a textured unit cube [-1,1]^3 (in model space)
        * 
        * -> Use the Model matrix to draw a rectangle and/or choose the position
        *    in world coord.
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
        *
        * - Each face may use a different texture (or set the image to nullptr to disable texturing a face).
        *
        * - the texture coordinate for each face are given ordered by their name
        *   (e.g. 'v_front_A_B_C_D' means vertex in order: A, B, C, D for the front face)
        *
        *    *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
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
        * Same as above for use the whole texture image given for each face.
        **/
        void drawCube(
            const Image<color_t>* texture_front,
            const Image<color_t>* texture_back,
            const Image<color_t>* texture_top,
            const Image<color_t>* texture_bottom,
            const Image<color_t>* texture_left,
            const Image<color_t>* texture_right
            );





        /**
        * Draw a unit radius sphere centered at the origin S(0,1).
        *
        * Create a UV-sphere with a given number of sector and stacks.
        *
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawSphere(int nb_sectors, int nb_stacks)
            {
            drawSphere(nb_sectors, nb_stacks, nullptr);
            }


        /**
        * Draw a textured unit radius sphere centered at the origin S(0,1).
        *
        * Create a UV-sphere with a given number of sector and stacks.
        *
        * The texture is mapped using the Mercator projection. 
        * 
        * *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        *
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture)
            {
            _drawSphere<false,false>(nb_sectors, nb_stacks, texture, 1.0f, color_t(_color), 1.0f);
            }


        /**
        * Draw a unit radius sphere centered at the origin S(0,1).
        *
        * Draw a UV-sphere where the number of sector and stacks is computed 
        * automatically according to the apparent size on the screen.
        * 
        * - quality > 0 is a multiplier (typically between 0.5 and 2) used to   
        *   imcrease or  decrease the number of faces in the tesselation:
        *     - quality < 1 : decrease quality but improve speed  
        *     - quality > 1 : improve quality but decrease speed  
        * 
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawAdaptativeSphere(float quality = 1.0f)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawSphere(nb_stacks * 2 - 2, nb_stacks, nullptr);
            }   


        /**
        * Draw a textured unit radius sphere centered at the origin S(0,1).
        *
        * Draw a UV-sphere where the number of sector and stacks is computed
        * automatically according to the apparent size on the screen.
        *
        * - quality > 0 is a multiplier (typically between 0.5 and 2) used to
        *   imcrease or  decrease the number of faces in the tesselation:
        *     - quality < 1 : decrease quality but improve speed
        *     - quality > 1 : improve quality but decrease speed
        *
        * The texture is mapped using the mercator projection.
        *
        *  *** TEXTURE DIMENSIONS MUST BE POWERS OF 2 WHEN USING 'FAST' WRAP MODE FOR TEXTURING ****
        *
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawAdaptativeSphere(const Image<color_t>* texture, float quality = 1.0f)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawSphere(nb_stacks * 2 - 2, nb_stacks, texture);
            }






        /*****************************************************************************************
        ******************************************************************************************
        *
        *                            WireFrame drawing methods
        *
        * Each methods comes in two flavors: without or with a specified line thickness.
        * 
        * - methods without thickness are fast but are not accurate (single pixel line, without  
        *   blending, anti-aliasing or sub-pixel precision.
        * 
        * - methods with a thickness specified are currently VERY SLOW (even slower than texturing!)   
        *   but the line is drawn in high quality, using anti-aliasing, sub-pixel precision, blending
        *   and the given thickness in pixels (a positive float value).
        * 
        *  *** WIREFRAME DRAWING METHODS DO NOT USE DEPTH TESTING (even if a z-buffer is on) ***
        *  
        *  *** WIREFRAME DRAWING METHODS DO NOT TAKE LIGHTNING INTO ACCOUNT FOR LINE COLORS ***
        *  
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Draw a mesh onto the image using 'wireframe' lines (FAST BUT LOW QUALITY METHOD). 
        * 
        * The mesh is drawn with the current material color (not that of the mesh). This method does not 
        * require a zbuffer but back face culling is used if it is enabled.
        *
        * - mesh:    The mesh to draw. The meshes/vertices array/textures can be in RAM or in FLASH.
        *           Whenever possible, put vertex array and texture in RAM (or even EXTMEM).
        *
        * - draw_chained_meshes:  If true, the meshes linked to this mesh (via the ->next member) are also drawn.
        **/
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes = true)
            {
            _drawWireFrameMesh<true>(mesh, draw_chained_meshes, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a mesh onto the image using 'wireframe' lines (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * 
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/        
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness)
            {
            _drawWireFrameMesh<false>(mesh, draw_chained_meshes, color_t(_color), 1.0f, thickness);
            }
            

        /**
        * Draw a mesh onto the image using 'wireframe' lines (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/        
        void drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness, color_t color, float opacity)
            {
            _drawWireFrameMesh<false>(mesh, draw_chained_meshes, color, opacity, thickness);
            }
            

        /**
        * Draw a 'wireframe' 3D line segment  (FAST BUT LOW QUALITY METHOD).
        *
        * the line is drawn with the current material color. This method does not require a zbuffer.
        *
        * - (P1, P2) coordinates (in model space) of the segment to draw.
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2)
            {
            _drawWireFrameLine<true>(P1, P2, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' 3D line segment. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness)
            {
            _drawWireFrameLine<false>(P1, P2, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a 'wireframe' 3D line segment. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness, color_t color, float opacity)
            {
            _drawWireFrameLine<false>(P1, P2, color, opacity, thickness);
            }


        /**
        * Draw a list of wireframe lines on the image (FAST BUT LOW QUALITY METHOD). 
        * the lines are drawn with the current material color. This method does not require a zbuffer.
        *
        * - nb_lines Number of lines to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_lines*2
        *                and each 2 consecutive values represent a line segment.
        *
        * - vertices     The array of vertices (given in model space).
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameLines<true>(nb_lines, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a list of wireframe lines on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            _drawWireFrameLines<false>(nb_lines, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a list of wireframe lines on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameLines<false>(nb_lines, ind_vertices, vertices, color, opacity, thickness);
            }


        /**
        * Draw a 'wireframe' triangle (FAST BUT LOW QUALITY METHOD).
        *
        * the triangle is drawn with the current material color. This method does not require a zbuffer
        * but back face culling is used (if enabled).
        *
        * - (P1, P2, P3) coordinates (in model space) of the triangle to draw.
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3)
            {
            _drawWireFrameTriangle<true>(P1, P2, P3, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' triangle. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness)
            {
            _drawWireFrameTriangle<false>(P1, P2, P3, color_t(_color), 1.0f, thickness);
            }
    

        /**
        * Draw a 'wireframe' triangle. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness, color_t color, float opacity)
            {
            _drawWireFrameTriangle<false>(P1, P2, P3, color, opacity, thickness);
            }


        /**
        * Draw a list of wireframe triangles on the image (FAST BUT LOW QUALITY METHOD).
        * 
        * the triangles are drawn with the current material color. This method does not require a zbuffer.
        * but back face culling is used (if enabled).
        * 
        * - nb_triangles Number of triangles to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_triangles*3
        *                and each 3 consecutive values represent a triangle.
        *
        *               *** MAKE SURE THAT THE TRIANGLES ARE GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - vertices     The array of vertices (given in model space).
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameTriangles<true>(nb_triangles, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a list of wireframe triangles on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            _drawWireFrameTriangles<false>(nb_triangles, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }
          

        /**
        * Draw a list of wireframe triangles on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameTriangles<false>(nb_triangles, ind_vertices, vertices, color, opacity, thickness);
            }


        /**
        * Draw a 'wireframe' quad (FAST BUT LOW QUALITY METHOD). 
        *
        * the quad is drawn with the current material color. This method does not require a zbuffer
        * but back face culling is used (if enabled).
        *
        * - (P1, P2, P3, P4) coordinates (in model space) of the quad to draw.
        **/
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4)
            {
            _drawWireFrameQuad<true>(P1, P2, P3, P4, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' quad. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness)
            {
            _drawWireFrameQuad<false>(P1, P2, P3, P4, color_t(_color), 1.0f, thickness);
            }
           

        /**
        * Draw a 'wireframe' quad. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness, color_t color, float opacity)
            {
            _drawWireFrameQuad<false>(P1, P2, P3, P4, color, opacity, thickness);
            }
           

        /**
        * Draw a list of wireframe quads on the image (FAST BUT LOW QUALITY METHOD). 
        * 
        * the quads are drawn with the current material color. This method does not require a zbuffer
        * but back face culling is used (if enabled).
        *
        * - nb_quads Number of triangles to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_triangles*3
        *                and each 3 consecutive values represent a triangle.
        *
        *               *** MAKE SURE THAT THE QUADS ARE GIVEN WITH THE CORRECT WINDING ORDER ***
        *               *** QUADS MUST BE COPLANAR ***
        *
        * - vertices     The array of vertices (given in model space).
        **/
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameQuads<true>(nb_quads, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a list of quads on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            _drawWireFrameQuads<false>(nb_quads, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a list of quads on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameQuads<false>(nb_quads, ind_vertices, vertices, color, opacity, thickness);
            }




        /*****************************************************************************************
        ******************************************************************************************
        *
        *                          Drawing simple geometric object in wireframe
        *
        *
        ******************************************************************************************
        ******************************************************************************************/


        /**
        * Draw the cube [0,1]^3 (in model space) in wireframe (FAST BUT LOW QUALITY METHOD). 
        **/
        void drawWireFrameCube()
            {
            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawWireFrameQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES);
            // restore culling direction
            _culling_dir = save_culling;
            }


        /**
        * Draw the cube [0,1]^3 (in model space) in wireframe (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameCube(float thickness)
            {
            drawWireFrameCube(thickness, color_t(_color), 1.0f);
            }


        /**
        * Draw the cube [0,1]^3 (in model space) in wireframe (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameCube(float thickness, color_t color, float opacity)
            {
            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawWireFrameQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES, thickness, color, opacity);
            // restore culling direction
            _culling_dir = save_culling;
            }



        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (FAST BUT LOW QUALITY METHOD).
        * 
        * Create a UV-sphere with a given number of sector and stacks.
        **/
        void drawWireFrameSphere(int nb_sectors, int nb_stacks)
            {
            _drawSphere<true, true>(nb_sectors, nb_stacks, nullptr, 1.0f, color_t(_color), 1.0f);
            }


        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameSphere(int nb_sectors, int nb_stacks, float thickness)
            {
            drawWireFrameSphere(nb_sectors, nb_stacks, thickness, color_t(_color), 1.0f);
            }


        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameSphere(int nb_sectors, int nb_stacks, float thickness, color_t color, float opacity)
            {
            _drawSphere<true,false>(nb_sectors, nb_stacks, nullptr, thickness, color, opacity);
            }



        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (FAST BUT LOW QUALITY METHOD).
        *
        * Draw a UV-sphere wherethe number of sector and stacks is
        * computed automatically according to the apparent size on the screen.
        *
        * - quality > 0 is a multiplier (typically between 0.5 and 2) used to
        *   imcrease or  decrease the number of faces in the tesselation:
        *     - quality < 1 : decrease quality but improve speed
        *     - quality > 1 : improve quality but decrease speed
        *
        * As usual, model-view transform is applied a drawing time.
        **/
        void drawWireFrameAdaptativeSphere(float quality = 1.0f)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawWireFrameSphere(nb_stacks*2 - 2, nb_stacks);
            }   


        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameAdaptativeSphere(float quality, float thickness)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawWireFrameSphere(nb_stacks * 2 - 2, nb_stacks, thickness);
            }   


        /**
        * Draw a wireframe unit radius sphere centered at the origin (in model space). (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        **/
        void drawWireFrameAdaptativeSphere(float quality, float thickness, color_t color, float opacity)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawWireFrameSphere(nb_stacks * 2 - 2, nb_stacks, thickness, color, opacity);
            }






    private:



        /*****************************************************************************************
        ******************************************************************************************
        *
        * BE CAREFUL PAST THIS POINT... HERE BE DRAGONS !
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
        void _recompute_wa_wb()
            {
            if (_ortho)
                { // orthographic projection
                if (std::is_same<ZBUFFER_t, float>::value)
                    { // float zbuffer : no normalization needed
                    _uni.wa = 1.0f;
                    _uni.wb = 0.0f;
                    }
                else
                    { // uint16_t zbuffer : normalize in [0,65535]
                    _uni.wa = 32767.4f;
                    _uni.wb = 0;
                    }
                }
            else
                { // perspective projection
                if (std::is_same<ZBUFFER_t, float>::value)
                    { // float zbuffer : no normalization needed
                    _uni.wa = 1.0f;
                    _uni.wb = 0.0f;
                    }
                else
                    { // uint16_t zbuffer : normalize in [0,65535]
                    _uni.wa = -32768 * _projM[14];
                    _uni.wb = 32768 * (_projM[10] + 1);
                    }
                }
            }



        /***********************************************************
        * Making sure shader flags are coherent
        ************************************************************/

        void _rectifyShaderOrtho()
            {
            if (_ortho)
                {
                TGX_SHADER_ADD_ORTHO(_shaders)
                TGX_SHADER_REMOVE_PERSPECTIVE(_shaders)
                }
            else
                {
                TGX_SHADER_ADD_PERSPECTIVE(_shaders)
                TGX_SHADER_REMOVE_ORTHO(_shaders)
                }
            }


        void _rectifyShaderZbuffer()
            {
            if (_uni.zbuf)
                {
                TGX_SHADER_ADD_ZBUFFER(_shaders)
                TGX_SHADER_REMOVE_NOZBUFFER(_shaders)
                }
            else
                {
                TGX_SHADER_ADD_NOZBUFFER(_shaders)
                TGX_SHADER_REMOVE_ZBUFFER(_shaders)
                }
            }


        void _rectifyShaderShading(int new_shaders)
            {
            if (TGX_SHADER_HAS_GOURAUD(new_shaders))
                {
                TGX_SHADER_ADD_GOURAUD(_shaders)
                TGX_SHADER_REMOVE_FLAT(_shaders)
                }
            else 
                {
                TGX_SHADER_ADD_FLAT(_shaders)
                TGX_SHADER_REMOVE_GOURAUD(_shaders)
                }

            bool tex = (TGX_SHADER_HAS_TEXTURE(new_shaders));

            if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(new_shaders))
                {
                setTextureWrappingMode(TGX_SHADER_TEXTURE_WRAP_POW2);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_CLAMP(new_shaders))
                {
                setTextureWrappingMode(TGX_SHADER_TEXTURE_CLAMP);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_NEAREST(new_shaders))
                {
                setTextureQuality(TGX_SHADER_TEXTURE_NEAREST);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_BILINEAR(new_shaders))
                {
                setTextureQuality(TGX_SHADER_TEXTURE_BILINEAR);
                tex = true;
                }
            if (tex)
                {
                TGX_SHADER_ADD_TEXTURE(_shaders)
                TGX_SHADER_REMOVE_NOTEXTURE(_shaders)
                }
            else
                {
                TGX_SHADER_ADD_NOTEXTURE(_shaders)
                TGX_SHADER_REMOVE_TEXTURE(_shaders)
                }
            }


        void _rectifyShaderTextureWrapping()
            {
            if (_texture_wrap_mode == TGX_SHADER_TEXTURE_WRAP_POW2)
                {
                TGX_SHADER_ADD_TEXTURE_WRAP_POW2(_shaders)
                TGX_SHADER_REMOVE_TEXTURE_CLAMP(_shaders)                    
                }
            else
                {
                TGX_SHADER_ADD_TEXTURE_CLAMP(_shaders)
                 TGX_SHADER_REMOVE_TEXTURE_WRAP_POW2(_shaders)
                }
            }


        void _rectifyShaderTextureQuality()
            {
            if (_texture_quality == TGX_SHADER_TEXTURE_BILINEAR)
                {
                TGX_SHADER_ADD_TEXTURE_BILINEAR(_shaders)
                TGX_SHADER_REMOVE_TEXTURE_NEAREST(_shaders)                    
                }
            else
                {
                TGX_SHADER_ADD_TEXTURE_NEAREST(_shaders)                    
                TGX_SHADER_REMOVE_TEXTURE_BILINEAR(_shaders)
                }
            }




       

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
            


        template<bool CHECKRANGE, bool USE_BLENDING> void drawPixelZbuf(int x, int y, color_t color, float opacity, float z)
            {
            if (CHECKRANGE && ((x < 0) || (x >= _uni.im->lx()) || (y < 0) || (y >= _uni.im->ly()))) return;           
            ZBUFFER_t& W = _uni.zbuf[x + _uni.im->lx() * y];
            const ZBUFFER_t aa =  (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(z * _uni.wa + _uni.wb)) : ((ZBUFFER_t)z);
            if (W < aa)
                {
                W = aa;
                if (USE_BLENDING) _uni.im->template drawPixel<false>(x, y, color, opacity); else _uni.im->template drawPixel<false>(x, y, color);
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
        void _clip(int & fl, const fVec4 & P, float bx, float Bx, float by, float By)
            {
            if (P.x >= bx) { fl &= (~(1)); }
            if (P.x <= Bx) { fl &= (~(2)); }
            if (P.y >= by) { fl &= (~(4)); }
            if (P.y <= By) { fl &= (~(8)); }
            if ((P.z >= -1.0f)&&(P.w > 0)) { fl &= (~(16)); }
            if (P.z <= +1.0f) { fl &= (~(32)); }
            }


        /** used by _discard() for testing a point position against the frustum planes */
        void _clip(int & fl, const fVec3 & P, float bx, float Bx, float by, float By, const fMat4 & M)
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

            const float ilx = 2.0f * fast_inv((float)_lx);
            const float bx = (_ox - 1) * ilx - 1.0f;
            const float Bx = (_ox + _uni.im->width() + 1) * ilx - 1.0f;
            const float ily = 2.0f * fast_inv((float)_ly);
            const float by = (_oy - 1) * ily - 1.0f;
            const float By = (_oy + _uni.im->height() + 1) * ily - 1.0f;

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
            const float ilx = 2.0f  * fast_inv((float)_lx);
            const float bx = (_ox - 1) * ilx - 1.0f;
            const float Bx = (_ox + _uni.im->width() + 1) * ilx - 1.0f;
            const float ily = 2.0f  * fast_inv((float)_ly);
            const float by = (_oy - 1) * ily - 1.0f;
            const float By = (_oy + _uni.im->height() + 1) * ily - 1.0f;

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
        TGX_INLINE void _precomputeSpecularTable(int exponent)
            {
            if (_currentpow == exponent) return;
            _precomputeSpecularTable2(exponent);
            }

        void _precomputeSpecularTable2(int exponent);


        /** compute pow(x, exponent) using linear interpolation from the pre-computed table */
        TGX_INLINE float _powSpecular(float x) const
            {
            const float indf = (_powmax - x) * _POWTABSIZE;
            const int indi = max(0,(int)indf);
            return (indi >= (_POWTABSIZE - 1)) ? 0.0f : (_fastpowtab[indi] + (indf - indi) * (_fastpowtab[indi + 1] - _fastpowtab[indi]));;
            }


        /** compute a color according to Phong lightning model, use model color */
        template<bool TEXTURE> TGX_INLINE  RGBf _phong(float v_diffuse, float v_specular) const
            {
            RGBf col = _r_ambiantColor;
            col += _r_diffuseColor * max(v_diffuse, 0.0f);
            col += _r_specularColor * _powSpecular(v_specular); // pow() this is too slow so we use a lookup table instead
            if (!(TEXTURE)) col *= _r_objectColor;
            col.clamp();
            return col;
            }


        /** compute a color according to Phong lightning model, use given color */
        TGX_INLINE  RGBf _phong(float v_diffuse, float v_specular, RGBf color) const
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

