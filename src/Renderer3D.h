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
    * The class create a virtual viewport with a given size LX,LY and provides methods to draw
    * 3D primitives onto this viewport (to which is attached an Image<color_t> object)
    *
    * Template parameters:
    *
    * - color_t : the color type for the image to draw onto.
    *
    * - LX , LY : Viewport size (up to 2048x2048). The normalized coordinates in [-1,1]x[-1,1]
    *             are mapped to [0,LX-1]x[0,LY-1] just before rasterization.
    *             It is possible to use a viewport larger than the image drawn onto and set
    *             an offset for this image inside the viewport in order to perform 'tile rendering'.
    *
    * - ZBUFFER : (default true) Set this to use depth testing when drawing the mesh. In this case,
    *             a valid depth buffer MUST be supplied with setZbuffer() before drawing a mesh.
    *
    * - ORTHO   : (default false) Set this to use orthographic projection instead of perspective
    *             and thus disable the z-divide after projection.
    **/
    template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
    class Renderer3D
    {

        static const int MAXVIEWPORTDIMENSION = 2048;
        static_assert((LX > 0) && (LX <= MAXVIEWPORTDIMENSION), "Invalid viewport width.");
        static_assert((LY > 0) && (LY <= MAXVIEWPORTDIMENSION), "Invalid viewport height.");
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");

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
        * Constructor. Set some default parameters.
        **/
        Renderer3D();



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
         * Set the projection matrix as an orthographic matrix:
         * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml
         *
         * This method is only available when ORTHO = true.
         *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
         **/
        void setOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(ORTHO == true, "the setOrtho() method can only be used with template parameter ORTHO = true");
            _projM.setOrtho(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
            }


        /**
         * Set the projection matrix as a perspective matrix
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
        *
        * This method is only available when ORTHO = false.
         *
        * IMPORTANT :In view space, the camera is assumed to be centered
        * at the origin, looking looking toward the negative Z axis with
        * the Y axis pointing up (as in opengl).
        **/
        void setFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(ORTHO == false, "the setFrustum() method can only be used with template parameter ORTHO = false (use projectionMatrix().setFrustum() is you really want to...)");
            _projM.setFrustum(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
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
            static_assert(ORTHO == false, "the setPerspective() method can only be used with template parameter ORTHO = false (use projectionMatrix().setPerspective() is you really want to...)");
            _projM.setPerspective(fovy, aspect, zNear, zFar);
            _projM.invertYaxis();
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
        * Set the zbuffer and its size (in number of floats).
        *
        * The zbuffer must be large enough to be used with the image that is being drawn onto.
        * This means that we must have length >= image.width()*image.height().
        **/
        void setZbuffer(float* zbuffer, int length)
            {
            static_assert(ZBUFFER == true, "the setZbuffer() method can only be used with template parameter ZBUFFER = true");
            _uni.zbuf = zbuffer;
            _zbuffer_len = length;
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
            static_assert(ZBUFFER == true, "the clearZbuffer() method can only be used with template parameter ZBUFFER = true");
            if (_uni.zbuf) memset(_uni.zbuf, 0, _zbuffer_len*sizeof(float));
            }


        /**
        * Enable/disable the use of bilinear point sampling when using texture mapping.  
        * Enabling it increase the quality of the rendering but is much more compute expensive. 
        * default value = false. 
        **/
        void useBilinearTexturing(bool enable)
            {
            _uni.use_bilinear_texturing = enable;
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
            _r_inorm = 1.0f / _r_modelViewM.mult0(fVec3{ 0,0,1 }).norm();
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
            if (!ORTHO) Q.zdivide();
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
            if (!ORTHO) Q.zdivide();
            Q.x = ((Q.x + 1) * LX) / 2 - _ox;
            Q.y = ((Q.y + 1) * LY) / 2 - _oy;
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
            _r_inorm = 1.0f / _r_modelViewM.mult0(fVec3{ 0,0,1 }).norm();
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
            _r_inorm = 1.0f / _r_modelViewM.mult0(fVec3{ 0,0,1 }).norm();
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
            if (!ORTHO) Q.zdivide();
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
            if (!ORTHO) Q.zdivide();
            Q.x = ((Q.x + 1) * LX) / 2 - _ox;
            Q.y = ((Q.y + 1) * LY) / 2 - _oy;
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
        * (1) The rasterizer accept the following shader options.
        *
        *     - TGX_SHADER_FLAT   Use flat shading (i.e. uniform color on faces). This is the fastest
        *                     drawing method but usually gives poor results when combined with texturing.
        *                     Lighting transition between bright to dark aera may appear to 'flicker'
        *                     when color_t = RGB565 because of the limited number of colors/shades
        *                     available.
        *
        *                     -> the color on the face is computed according to Phong's lightning
        *                        model use the triangle face normals computed via crossproduct.
        *
        *     - TGX_SHADER_GOURAUD  Give a color to each vertex and then use linear interpolation to
        *                       shade each triangle according to its vertex colors. This results in
        *                       smoother color transitions and works well with texturing but at a
        *                       higher CPU cost.
        *
        *                       -> In order to use gouraud shading, a normal vector must be attributed
        *                          to each vertex, which is then used to determine its color according
        *                          to phong's lightning model.
        *
        *                       -> *** NORMAL VECTORS MUST ALWAYS BE NORMALIZED (UNIT LENGHT) ***
        *
        *                       -> When backface culling is enable, the normal vector must be that of
        *                          the 'front face' (obviously). When backface culling is  disabled,
        *                          there is no more 'front' and 'back' face: by convention, the normal
        *                          vector supplied must be then be those corresponding to the counter-
        *                          clockwise face.
        *
        *     - TGX_SHADER_TEXTURE  Use (perspective correct) texture mapping. This flag may be combined
        *                       with either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD. The color of a pixel in the
        *                       triangle is obtained by combining to texture color at that pixel with
        *                       the lightning at the position (according to phong's lightning model again).
        *
        *                       -> A texture must be stored in an Image<color_t> object:
        *
        *                           *** EACH TEXTURE DIMENSION MUST BE A POWER OF 2 ****
        *
        *                       -> Large textures stored in flash memory may be VERY slow to access when the
        *                          texture is not read linearly which will happens for some (bad) triangle
        *                          orientations and then cache becomes useless... This problem can be somewhat
        *                          mitigated by:
        *
        *                          (a) splitting large textured triangles into smaller ones: then each triangle
        *                              only accesses a small part of the texture. This helps a lot wich cache
        *                              optimizatrion [this is why models with thousands of faces may display
        *                              faster that a simple textured cube in some cases :-)].
        *
        *                          (b) moving the image into RAM if possible. Even moving the texture from
        *                              FLASH to EXTMEM (if available) will usually give a great speed boost !
        *
        *
        * (2) Depth testing is automoatically performed when drawing is the tmeplate parameter ZBUFFER is set
        *      (in this case, a valid z buffer must be supplied with setZbufffer() before calling any draw method).
        *
        *     ***  DO NOT FORGET TO CLEAR THE ZBUFFER WITH clearZbuffer() BEFORE DRAWING A NEW SCENE :-) ***
        *
        *
        * (3) Back face culling is automatically performed if enabled (via the setCulling() method)
        *
        *     *** TRIANGLES/QUADS MUST BE GIVEN TO THE DRAWING METHOD IN THEIR CORRECT WINDING ORDER ***
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
        * - shader  Type of shader to use. Combination of TGX_SHADER_GOURAUD, TGX_SHADER_FLAT, TGX_SHADER_TEXTURE
        *            - TGX_SHADER_FLAT    : use flat shading (uniform color on each triangle).
        *            - TGX_SHADER_GOURAUD : This flag overwrites TGX_SHADER_FLAT but requires the mesh to have a
        *                               normal array otherwise the renderer will fall back to flat shading.
        *            - TGX_SHADER_TEXTURE : use texture mapping. This flag complements TGX_SHADER_FLAT/TGX_SHADER_GOURAUD
        *                               but require the mesh to a a valid texture array and a valid texture
        *                               image otherwise the renderer will fall back to uniform color shading.
        *
        * - mesh    The mesh to draw. The meshes/vertices array/textures can be in RAM or in FLASH.
        *           Whenever possible, put vertex array and texture in RAM (or even EXTMEM).
        *
        * - use_mesh_material   If true, ignore the current object material properties set in the renderer
        *                       and use the mesh predefined material properties.
        *                       this flag affects also all the linked meshes if draw_chained_meshes=true.
        *
        * - draw_chained_meshes  If true, the meshes linked to this mesh (via the ->next member) are also drawn.
        *
        * The method returns  0 ok, (drawing performed correctly).
        *                    -1 invalid image
        *                    -2 invalid zbuffer (only when template parameter ZBUFFER=true)
        **/
        int drawMesh(const int shader, const Mesh3D<color_t>* mesh, bool use_mesh_material = true, bool draw_chained_meshes = true);




        /**
        * Draw a single triangle on the image. Use the current material color.
        *
        * - shader      Ignored: the triangle is draw with TGX_SHADER_FLAT anyway
        *               since no normal nor texture coord is given...
        *
        * - (P1,P2,P3)  Coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawTriangle(int shader,
                         const fVec3& P1, const fVec3& P2, const fVec3& P3)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            _drawTriangle(TGX_SHADER_FLAT, &P1, &P2, &P3, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }


        /**
        * Draw a single triangle on the image with a given color on each of its vertices.
        * The color inside the triangle is obtained by linear interpolation.
        *
        * - shader      Ignored: the triangle is drawn with TGX_SHADER_FLAT anyway
        *               since no normal nor texture coord is given...
        *
        * - (P1,P2,P3)  Coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (col1,col2,col3) Colors of the vertices
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawTriangleWithVertexColor(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3,
                     const RGBf & col1, const RGBf & col2, const RGBf & col3)
            {
            fVec3 dummyN(0,0,0); // does not matter, unused
            return drawTriangleWithVertexColor(TGX_SHADER_FLAT, P1, P2, P3, dummyN,dummyN,dummyN, col1, col2, col3);
            }


        /**
        * Draw a single triangle on the image (with gouraud shading). Use the current material color.
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD.
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3)  normals associated with (P1, P2, P3).
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawTriangle(int shader,
                         const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                         const fVec3 & N1, const fVec3 & N2, const fVec3 & N3)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed            
            TGX_SHADER_REMOVE_TEXTURE(shader) // disable texturing
            _drawTriangle(shader, &P1, &P2, &P3, &N1, &N2, &N3, nullptr, nullptr, nullptr, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }


        /**
        * Draw a single triangle on the image with a given color on each of its vertices.
        * The color inside the triangle is obtained by linear interpolation.
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD.
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3)  normals associated with (P1, P2, P3).
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (col1,col2,col3) Colors of the vertices
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawTriangleWithVertexColor(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3,
                     const fVec3& N1, const fVec3& N2, const fVec3& N3,
                     const RGBf & col1, const RGBf & col2, const RGBf & col3)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            if (TGX_SHADER_HAS_GOURAUD(shader))
                {
                _drawTriangle(TGX_SHADER_GOURAUD, &P1, &P2, &P3, &N1, &N2, &N3, nullptr, nullptr, nullptr, col1, col2, col3);
                }
            else
                {
                fVec3 N = crossProduct(P2 - P1, P3 - P1);// compute the triangle face normal
                N.normalize(); // normalize it
                _drawTriangle(TGX_SHADER_GOURAUD, &P1, &P2, &P3, &N, &N, &N, nullptr, nullptr, nullptr, col1, col2, col3); // call gouraud shader with the same normal for all 3 vertices
                }
            return 0;
            }




        /**
        * Draw a single triangle on the image (with texture mapping).
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_TEXTURE.
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (T1,T2,T3)  texture coords. associated with (P1, P2, P3).
        *
        * - texture     the texture image to use
        *
        *               *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid texture image
        **/
        int drawTriangle(int shader,
                        const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                        const fVec2 & T1, const fVec2 & T2, const fVec2 & T3,
                        const Image<color_t>* texture)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            TGX_SHADER_REMOVE_GOURAUD(shader)
            if (TGX_SHADER_HAS_TEXTURE(shader))
                { // store the texture
                if (texture == nullptr) return -3;
                _uni.tex = (const Image<color_t>*)texture;
                }
            _drawTriangle(shader, &P1, &P2, &P3, nullptr, nullptr, nullptr, &T1, &T2, &T3, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }



        /**
        * Draw a single triangle on the image (with gouraud shading and texture mapping)
        *
        * - shader      Combination of TGX_SHADER_FLAT/TGX_SHADER_GOURAUD and TGX_SHADER_TEXTURE
        *
        * - (P1,P2,P3)  coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE TRIANGLE IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3)  normals associated with (P1, P2, P3).
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (T1,T2,T3)  texture coords. associated with (P1, P2, P3).
        *
        * - texture     the texture image to use
        *
        *               *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid texture image
        **/
        int drawTriangle(int shader,
                        const fVec3 & P1, const fVec3 & P2, const fVec3 & P3,
                        const fVec3 & N1, const fVec3 & N2, const fVec3 & N3,
                        const fVec2 & T1, const fVec2 & T2, const fVec2 & T3,
                        const Image<color_t>* texture)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            if (TGX_SHADER_HAS_TEXTURE(shader))
                { // store the texture
                if (texture == nullptr) return -3;
                _uni.tex = (const Image<color_t>*)texture;
                }
            _drawTriangle(shader, &P1, &P2, &P3, &N1, &N2, &N3, &T1, &T2, &T3, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }



        /**
        * Draw a list of triangles on the image. If texture mapping is not used, then the current
        * material color is used to draw the triangles.
        *
        * - shader       Combination of TGX_SHADER_FLAT/TGX_SHADER_GOURAUD and TGX_SHADER_TEXTURE
        *                Some flag may be ignored if the corresponding required arrays
        *                are missing (i.e. a normal array for TGX_SHADER_GOURAUD, and a texture
        *                array and a texture image for TGX_SHADER_TEXTURE).
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
        * - ind_normals  [Optional] array of normal indexes. If specified, the array must
        *                have length nb_triangles*3.
        *
        * - normals      [Optional] The array of normals vectors (in model space).
        *
        *                 *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - ind_texture  [Optional] array of texture indexes. If specified, the array must
        *                have length nb_triangles*3.
        *
        * - textures     [Optional] The array of texture coords.
        *
        * - texture_image [Optional] the texture image to use.
        *
        *                  *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid vertice list
        **/
        int drawTriangles(int shader, int nb_triangles,
                            const uint16_t * ind_vertices, const fVec3 * vertices,
                            const uint16_t * ind_normals = nullptr, const fVec3* normals = nullptr,
                            const uint16_t * ind_texture = nullptr, const fVec2* textures = nullptr,
                            const Image<color_t> * texture_image = nullptr);




        /**
        * Draw a single quad on the image.
        * Use the current material color to draw the quad.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Ignored: the quad is drawn with TGX_SHADER_FLAT anyway
        *               since no normal nor texture coord is given...
        *
        * - (P1,P2,P3,P4)  Coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawQuad(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            _drawQuad(shader, &P1, &P2, &P3, &P4, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }


        /**
        * Draw a single quad on the image with a given color on each of its four vertices.
        * The color inside the quad is obtained by linear interpolation.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Ignored: the quad is drawn with TGX_SHADER_FLAT anyway
        *               since no normal nor texture coord is given...
        *
        * - (P1,P2,P3,P4)  Coordinates (in model space) of the triangle to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (col1,col2,col3,col4) Colors of the vertices
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawQuadWithVertexColor(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4,
                     const RGBf & col1, const RGBf & col2, const RGBf & col3, const RGBf & col4)
            {
            fVec3 dummyN(0,0,0); // does not matter, unused
            return drawQuadWithVertexColor(TGX_SHADER_FLAT, P1, P2, P3, P4, dummyN,dummyN,dummyN,dummyN, col1, col2, col3, col4);
            }


        /**
        * Draw a single quad on the image (with gouraud shading).
        * Use the current material color to draw the quad.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD.
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3,N4)  normals associated with (P1, P2, P3, P4).
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawQuad(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4,
                     const fVec3& N1, const fVec3& N2, const fVec3& N3, const fVec3& N4)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            TGX_SHADER_REMOVE_TEXTURE(shader) // disable texturing
            _drawQuad(shader, &P1, &P2, &P3, &P4, &N1, &N2, &N3, &N4, nullptr, nullptr, nullptr, nullptr, _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }


        /**
        * Draw a single quad on the image with a given color on each of its four vertices.
        * The color inside the quad is obtained by linear interpolation.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD.
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3,N4)  normals associated with (P1, P2, P3, P4).
        *
        *               *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (col1,col2,col3,col4) Colors of the vertices
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        **/
        int drawQuadWithVertexColor(int shader,
                     const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4,
                     const fVec3& N1, const fVec3& N2, const fVec3& N3, const fVec3& N4,
                     const RGBf & col1, const RGBf & col2, const RGBf & col3, const RGBf & col4)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            if (TGX_SHADER_HAS_GOURAUD(shader))
                {
                _drawQuad(TGX_SHADER_GOURAUD, &P1, &P2, &P3, &P4, &N1, &N2, &N3, &N4, nullptr, nullptr, nullptr, nullptr, col1, col2, col3, col4);
                }
            else
                {
                fVec3 N = crossProduct(P2 - P1, P3 - P1);// compute the quad normal (quad is assumed to be planar)
                N.normalize(); // normalize it
                _drawQuad(TGX_SHADER_GOURAUD, &P1, &P2, &P3, &P4, &N, &N, &N, &N, nullptr, nullptr, nullptr, nullptr, col1, col2, col3, col4);
                }
            return 0;
            }


        /**
        * Draw a single quad on the image (with texture mapping).
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Either TGX_SHADER_FLAT or TGX_SHADER_TEXTURE.
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *               *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (T1,T2,T3,T4)  texture coords. associated with (P1, P2, P3, P4).
        *
        * - texture     the texture image to use
        *
        *               *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid texture image
        **/
        int drawQuad(int shader,
                     const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3& P4,
                     const fVec2 & T1, const fVec2 & T2, const fVec2 & T3, const fVec2& T4,
                     const Image<color_t>* texture)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            TGX_SHADER_REMOVE_GOURAUD(shader) // disable gouraud
            if (TGX_SHADER_HAS_TEXTURE(shader))
                { // store the texture
                if (texture == nullptr) return -3;
                _uni.tex = (const Image<color_t>*)texture;
                }
            _drawQuad(shader, &P1, &P2, &P3, &P4, nullptr, nullptr, nullptr, nullptr, &T1, &T2, &T3, &T4, _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }



        /**
        * Draw a single quad on the image (with texture mapping and gouraud shading)
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader      Combination of TGX_SHADER_FLAT/TGX_SHADER_GOURAUD and TGX_SHADER_TEXTURE
        *
        * - (P1,P2,P3,P4)  coordinates (in model space) of the quad to draw
        *
        *                  *** MAKE SURE THAT THE QUAD IS GIVEN WITH THE CORRECT WINDING ORDER ***
        *
        * - (N1,N2,N3,N4)  normals associated with (P1, P2, P3, P4).
        *
        *                  *** THE NORMAL VECTORS MUST HAVE UNIT NORM ***
        *
        * - (T1,T2,T3,T4)  texture coords. associated with (P1, P2, P3).
        *
        * - texture     the texture image to use
        *
        *               *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid texture image
        **/
        int drawQuad(int shader,
                     const fVec3 & P1, const fVec3 & P2, const fVec3 & P3, const fVec3& P4,
                     const fVec3 & N1, const fVec3 & N2, const fVec3 & N3, const fVec3& N4,
                     const fVec2 & T1, const fVec2 & T2, const fVec2 & T3, const fVec2& T4,
                     const Image<color_t>* texture)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            if (TGX_SHADER_HAS_TEXTURE(shader))
                { // store the texture
                if (texture == nullptr) return -3;
                _uni.tex = (const Image<color_t>*)texture;
                }
            _drawQuad(shader, &P1, &P2, &P3, &P4,   &N1, &N2, &N3, &N4,    &T1, &T2, &T3, &T4, _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
            return 0;
            }



        /**
        * Draw a list of quads on the image.  If texture mapping is not used, then the current
        * material color is used to draw the triangles.
        *
        *     *** THE 4 VERTEX OF A QUAD MUST ALWAYS BE CO-PLANAR ***
        *
        * - shader       Combination of TGX_SHADER_FLAT/TGX_SHADER_GOURAUD and TGX_SHADER_TEXTURE
        *                Some flag may be ignored if the corresponding required arrays
        *                are missing (i.e. a normal array for TGX_SHADER_GOURAUD, and a texture
        *                array and a texture image for TGX_SHADER_TEXTURE).
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
        *                  *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid detph buffer
        *          -3 invalid vertice list
        **/
        int drawQuads(int shader, int nb_quads,
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
            int index = 0;
            _drawPixels<false>(nb_pixels, pos_list, &index, &_color, nullptr, nullptr);
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
            _drawPixels<false>(nb_pixels, pos_list, colors_ind, colors, nullptr, nullptr);
            }


        /**
        * Draw a list of pixels on the image at given positions (in model space).
        *
        * Use a (possibly) different color/opacity for each pixel and use blending.
        * The color and opacities are both given by a palette and a list of indices
        * (one for each pixel)
        *
        * The scene lightning parameters are ignored.
        **/
        void drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            _drawPixels<true>(nb_pixels, pos_list, colors_ind, colors, opacities_ind, opacities);
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
        * Use a possible different radius for each dot.
        *
        * The scene lightning is ignored.
        **/
        void drawDots(int nb_dots, const fVec3* pos_list, const int * radius_ind, const int * radius)
            {
            int index = 0;
            _drawDots<false>(nb_dots, pos_list, radius_ind, radius, &index, &_color, nullptr, nullptr);
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
            _drawDots<false>(nb_dots, pos_list, radius_ind, radius, colors_ind, colors, nullptr, nullptr);
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
            _drawDots<false>(nb_dots, pos_list, radius_ind, radius, colors_ind, colors, opacities_ind, opacities);
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
            _culling_dir = 1;
            drawQuads(TGX_SHADER_FLAT, 6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES);
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
        *   *** THE TEXTURE SIZE MUST BE A POWER OF 2 IN EVERY DIMENSION ****
        */

        void drawCube(int shader,
            fVec2 v_front_ABCD[4] , const Image<color_t>* texture_front,
            fVec2 v_back_EFGH[4]  , const Image<color_t>* texture_back,
            fVec2 v_top_HADE[4]   , const Image<color_t>* texture_top,
            fVec2 v_bottom_BGFC[4], const Image<color_t>* texture_bottom,
            fVec2 v_left_HGBA[4]  , const Image<color_t>* texture_left,
            fVec2 v_right_DCFE[4] , const Image<color_t>* texture_right
            )
            {

            if (!(TGX_SHADER_HAS_TEXTURE(shader)))
                {
                drawCube();
                return;
                }

            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            _culling_dir = 1;

            const uint16_t list_v[4] = { 0,1,2,3 };
            if (texture_front) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_front_ABCD, texture_front); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES); }
            if (texture_back) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES + 4, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_back_EFGH, texture_back); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES + 4, UNIT_CUBE_VERTICES); }
            if (texture_top) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES + 8, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_top_HADE, texture_top); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES + 8, UNIT_CUBE_VERTICES); }
            if (texture_bottom) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES + 12, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_bottom_BGFC, texture_bottom); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES + 12, UNIT_CUBE_VERTICES); }
            if (texture_left) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES + 16, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_left_HGBA, texture_left); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES + 16, UNIT_CUBE_VERTICES); }
            if (texture_right) { drawQuads(TGX_SHADER_TEXTURE, 1, UNIT_CUBE_FACES + 20, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_right_DCFE, texture_right); } else { drawQuads(TGX_SHADER_FLAT, 1, UNIT_CUBE_FACES + 20, UNIT_CUBE_VERTICES); }

            // restore culling direction
            _culling_dir = save_culling;
            }



        /**
        * Draw a unit radius sphere centered at the origin S(0,1).
        *
        * Create a UV-sphere with a given number of sector and stacks.
        *
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawSphere(int shader, int nb_sectors, int nb_stacks)
            {
            drawSphere(shader, nb_sectors, nb_stacks, nullptr);
            }


        /**
        * Draw a textured unit radius sphere centered at the origin S(0,1).
        *
        * Create a UV-sphere with a given number of sector and stacks.
        *
        * The texture is mapped using the Mercator projection. 
        * 
        *                  *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        * 
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawSphere(int shader, int nb_sectors, int nb_stacks, const Image<color_t>* texture)
            {
            _drawSphere<false,false>(shader, nb_sectors, nb_stacks, texture, 1.0f, color_t(_color), 1.0f);
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
        void drawAdaptativeSphere(int shader, float quality = 1.0f)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)sqrtf(l * quality); // Why this formula ? Well, why not...
            drawSphere(shader, nb_stacks * 2 - 2, nb_stacks, nullptr);
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
        *                  *** THE DIMENSION OF THE TEXTURE MUST BE POWERS OF 2 ***
        *                  
        * -> Use the Model matrix to draw an ellipsoid instead and/or choose the position
        *    in world coord.
        **/
        void drawAdaptativeSphere(int shader, const Image<color_t>* texture, float quality = 1.0f)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)sqrtf(l * quality); // Why this formula ? Well, why not...
            drawSphere(shader, nb_stacks * 2 - 2, nb_stacks, texture);
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
        *        *
        * The method returns  0 ok, (drawing performed correctly).
        *                    -1 invalid image
        **/
        int drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes = true)
            {
            return _drawWireFrameMesh<true>(mesh, draw_chained_meshes, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a mesh onto the image using 'wireframe' lines (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * 
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        * 
        **/        
        int drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness)
            {
            return _drawWireFrameMesh<false>(mesh, draw_chained_meshes, color_t(_color), 1.0f, thickness);
            }
            


        /**
        * Draw a mesh onto the image using 'wireframe' lines (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        * 
        **/        
        int drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameMesh<false>(mesh, draw_chained_meshes, color, opacity, thickness);
            }
            



        /**
        * Draw a 'wireframe' 3D line segment  (FAST BUT LOW QUALITY METHOD).
        *
        * the line is drawn with the current material color. This method does not require a zbuffer.
        *
        * - (P1, P2) coordinates (in model space) of the segment to draw.
        *
        * The method returns  0 ok, (drawing performed correctly).
        *                    -1 invalid image
        **/
        int drawWireFrameLine(const fVec3& P1, const fVec3& P2)
            {
            return _drawWireFrameLine<true>(P1, P2, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' 3D line segment. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness)
            {
            return _drawWireFrameLine<false>(P1, P2, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a 'wireframe' 3D line segment. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameLine<false>(P1, P2, color, opacity, thickness);
            }





        /**
        * Draw a list of wireframe lines on the image (FAST BUT LOW QUALITY METHOD). 
        * the triangles are drawn with the current material color. This method does not require a zbuffer.
        *
        * - nb_lines Number of lines to draw.
        *
        * - ind_vertices Array of vertex indexes. The length of the array is nb_lines*2
        *                and each 2 consecutive values represent a line segment.
        *
        * - vertices     The array of vertices (given in model space).
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid vertice list
        **/
        int drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            return _drawWireFrameLines<true>(nb_lines, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a list of wireframe lines on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            return _drawWireFrameLines<false>(nb_lines, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a list of wireframe lines on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameLines<false>(nb_lines, ind_vertices, vertices, color, opacity, thickness);
            }


        /**
        * Draw a 'wireframe' triangle (FAST BUT LOW QUALITY METHOD).
        *
        * the triangle is drawn with the current material color. This method does not require a zbuffer
        * but back face culling is used (if enabled).
        *
        * - (P1, P2, P3) coordinates (in model space) of the triangle to draw.
        *
        * The method returns  0 ok, (drawing performed correctly).
        *                    -1 invalid image
        **/
        int drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3)
            {
            return _drawWireFrameTriangle<true>(P1, P2, P3, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' triangle. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness)
            {
            return _drawWireFrameTriangle<false>(P1, P2, P3, color_t(_color), 1.0f, thickness);
            }
    

        /**
        * Draw a 'wireframe' triangle. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameTriangle<false>(P1, P2, P3, color, opacity, thickness);
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
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid vertice list
        **/
        int drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            return _drawWireFrameTriangles<true>(nb_triangles, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a list of wireframe triangles on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            return _drawWireFrameTriangles<false>(nb_triangles, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }
          

        /**
        * Draw a list of wireframe triangles on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameTriangles<false>(nb_triangles, ind_vertices, vertices, color, opacity, thickness);
            }





        /**
        * Draw a 'wireframe' quad (FAST BUT LOW QUALITY METHOD). 
        *
        * the quad is drawn with the current material color. This method does not require a zbuffer
        * but back face culling is used (if enabled).
        *
        * - (P1, P2, P3, P4) coordinates (in model space) of the quad to draw.
        *
        * The method returns  0 ok, (drawing performed correctly).
        *                    -1 invalid image
        **/
        int drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4)
            {
            return _drawWireFrameQuad<true>(P1, P2, P3, P4, color_t(_color), 1.0f, 1.0f);
            }


        /**
        * Draw a 'wireframe' quad. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness)
            {
            return _drawWireFrameQuad<false>(P1, P2, P3, P4, color_t(_color), 1.0f, thickness);
            }
           

        /**
        * Draw a 'wireframe' quad. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameQuad<false>(P1, P2, P3, P4, color, opacity, thickness);
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
        *
        * Returns: 0  OK
        *          -1 invalid image
        *          -2 invalid vertice list
        **/
        int drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            return _drawWireFrameQuads<true>(nb_quads, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }



        /**
        * Draw a list of quads on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness)
            {
            return _drawWireFrameQuads<false>(nb_quads, ind_vertices, vertices, color_t(_color), 1.0f, thickness);
            }


        /**
        * Draw a list of quads on the image. (HIGH QUALITY / SLOW VERSION).
        *
        * Same as above but specify the thickness (in pixels > 0.0) and use anti-aliasing.
        * Also set the color and opacity for blending instead of using the material color.
        *
        * *** MUCH SLOWER THAN THE VERSION WITHOUT AA/THICKNESS (AND EVEN SLOWER THAN DRAWING WITH SHADERS !) ***
        *
        **/
        int drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            return _drawWireFrameQuads<false>(nb_quads, ind_vertices, vertices, color, opacity, thickness);
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
            _culling_dir = 1;
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
            _culling_dir = 1;
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
            _drawSphere<true, true>(TGX_SHADER_FLAT, nb_sectors, nb_stacks, nullptr, 1.0f, color_t(_color), 1.0f);
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
            _drawSphere<true,false>(TGX_SHADER_FLAT, nb_sectors, nb_stacks, nullptr, thickness, color, opacity);
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
            const int nb_stacks = 2 + (int)sqrtf(l * quality); // Why this formula ? Well, why not...
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
            const int nb_stacks = 2 + (int)sqrtf(l * quality); // Why this formula ? Well, why not...
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
            const int nb_stacks = 2 + (int)sqrtf(l * quality); // Why this formula ? Well, why not...
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




       /***********************************************************
        * DRAWING STUFF
        ************************************************************/


        /** Method called by drawMesh() which does the actual drawing. */
        template<int RASTER_TYPE> void _drawMesh(const Mesh3D<color_t>* mesh);



        /** draw a single triangle */
        void _drawTriangle(const int RASTER_TYPE,
                           const fVec3 * P0, const fVec3 * P1, const fVec3 * P2,
                           const fVec3 * N0, const fVec3 * N1, const fVec3 * N2,
                           const fVec2 * T0, const fVec2 * T1, const fVec2 * T2,
                           const RGBf & Vcol0, const RGBf & Vcol1, const RGBf & Vcol2)
            {
            _uni.shader_type = RASTER_TYPE;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(*P0);
            const fVec4 Q1 = _r_modelViewM.mult1(*P1);
            const fVec4 Q2 = _r_modelViewM.mult1(*P2);
            
            // behind the scene, drop it
            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // skip triangle !

            RasterizerVec4 PC0, PC1, PC2;

            // test if clipping is needed
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));

            (*((fVec4*)&PC0)) = _projM * Q0;
            if (ORTHO) { PC0.w = 2.0f - PC0.z; } else { PC0.zdivide(); }
            bool needclip = (PC0.x < -clipboundXY) | (PC0.x > clipboundXY)
                          | (PC0.y < -clipboundXY) | (PC0.y > clipboundXY)
                          | (PC0.z < -1) | (PC0.z > 1);
          
            (*((fVec4*)&PC1)) = _projM * Q1;
            if (ORTHO) { PC1.w = 2.0f - PC1.z; } else { PC1.zdivide(); }
            needclip |= (PC1.x < -clipboundXY) | (PC1.x > clipboundXY)
                     | (PC1.y < -clipboundXY) | (PC1.y > clipboundXY)
                     | (PC1.z < -1) | (PC1.z > 1);

            (*((fVec4*)&PC2)) = _projM * Q2;
            if (ORTHO) { PC2.w = 2.0f - PC2.z; } else { PC2.zdivide(); }
            needclip |= (PC2.x < -clipboundXY) | (PC2.x > clipboundXY)
                     | (PC2.y < -clipboundXY) | (PC2.y > clipboundXY)
                     | (PC2.z < -1) | (PC2.z > 1);

            if (needclip) return; // we just drop triangle that need clipping (TODO : improve this !)


            // compute phong lightning
            if (TGX_SHADER_HAS_GOURAUD(RASTER_TYPE))
                { // gouraud shading
                const fVec3 NN0 = _r_modelViewM.mult0(*N0);
                const fVec3 NN1 = _r_modelViewM.mult0(*N1);
                const fVec3 NN2 = _r_modelViewM.mult0(*N2);

                // reverse normal only when culling is disabled (and we assume in this case that normals are  given for the CCW face).
                const float icu = (_culling_dir != 0) ? 1.0f : ((cu > 0) ? -1.0f : 1.0f);

                if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                    {
                    PC0.color = _phong<true>(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm));
                    PC1.color = _phong<true>(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm));
                    PC2.color = _phong<true>(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm));
                    }
                else
                    {
                    PC0.color = _phong(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm),Vcol0);
                    PC1.color = _phong(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm),Vcol1);
                    PC2.color = _phong(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm),Vcol2);
                    }
                }
            else
                { // flat shading
                const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
                faceN.normalize();
                if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                    {
                    _uni.facecolor = _phong<true>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                else
                    {
                    _uni.facecolor = _phong<false>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                }

            if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                { // store texture vectors if needed
                PC0.T = *T0;
                PC1.T = *T1;
                PC2.T = *T2;
                }

            // go rasterize !          
            rasterizeTriangle<LX, LY>(PC0, PC1, PC2, _ox, _oy, _uni, shader_select<ZBUFFER, ORTHO, color_t>);

            return;
            }



        /** draw a single quad : the 4 points are assumed to be coplanar */
        void _drawQuad(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2, const fVec3* P3,
            const fVec3* N0, const fVec3* N1, const fVec3* N2, const fVec3* N3,
            const fVec2* T0, const fVec2* T1, const fVec2* T2, const fVec2* T3,
            const RGBf & Vcol0, const RGBf & Vcol1, const RGBf & Vcol2,  const RGBf & Vcol3)
            {
            _uni.shader_type = RASTER_TYPE;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(*P0);
            const fVec4 Q1 = _r_modelViewM.mult1(*P1);
            const fVec4 Q2 = _r_modelViewM.mult1(*P2);

            // behind the scene, drop it.
            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

            const fVec4 Q3 = _r_modelViewM.mult1(*P3); // compute fourth point

            // behind the scene, drop it.
            if (Q3.z >= 0) return;

            RasterizerVec4 PC0, PC1, PC2, PC3;

            // test if clipping is needed
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));

            (*((fVec4*)&PC0)) = _projM * Q0;
            if (ORTHO) { PC0.w = 2.0f - PC0.z; } else { PC0.zdivide(); }
            bool needclip  = (PC0.x < -clipboundXY) | (PC0.x > clipboundXY)
                           | (PC0.y < -clipboundXY) | (PC0.y > clipboundXY)
                           | (PC0.z < -1) | (PC0.z > 1);

            (*((fVec4*)&PC1)) = _projM * Q1;
            if (ORTHO) { PC1.w = 2.0f - PC1.z; } else { PC1.zdivide(); }
            needclip |= (PC1.x < -clipboundXY) | (PC1.x > clipboundXY)
                     | (PC1.y < -clipboundXY) | (PC1.y > clipboundXY)
                     | (PC1.z < -1) | (PC1.z > 1);

            (*((fVec4*)&PC2)) = _projM * Q2;
            if (ORTHO) { PC2.w = 2.0f - PC2.z; } else { PC2.zdivide(); }
            needclip |= (PC2.x < -clipboundXY) | (PC2.x > clipboundXY)
                     | (PC2.y < -clipboundXY) | (PC2.y > clipboundXY)
                     | (PC2.z < -1) | (PC2.z > 1);


            (*((fVec4*)&PC3)) = _projM * Q3;
            if (ORTHO) { PC3.w = 2.0f - PC3.z; } else { PC3.zdivide(); }
            needclip |= (PC3.x < -clipboundXY) | (PC3.x > clipboundXY)
                     | (PC3.y < -clipboundXY) | (PC3.y > clipboundXY)
                     | (PC3.z < -1) | (PC3.z > 1);

            if (needclip) return; // just discard the whole quad (TODO : improve this !)

            // compute phong lightning
            if (TGX_SHADER_HAS_GOURAUD(RASTER_TYPE))
                { // gouraud shading
                const fVec3 NN0 = _r_modelViewM.mult0(*N0);
                const fVec3 NN1 = _r_modelViewM.mult0(*N1);
                const fVec3 NN2 = _r_modelViewM.mult0(*N2);
                const fVec3 NN3 = _r_modelViewM.mult0(*N3);

                // reverse normal only when culling is disabled (and we assume in this case that normals are  given for the CCW face).
                const float icu = (_culling_dir != 0) ? 1.0f : ((cu > 0) ? -1.0f : 1.0f);

                if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                    {
                    PC0.color = _phong<true>(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm));
                    PC1.color = _phong<true>(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm));
                    PC2.color = _phong<true>(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm));
                    PC3.color = _phong<true>(icu * dotProduct(NN3, _r_light_inorm), icu * dotProduct(NN3, _r_H_inorm));
                    }
                else
                    {
                    PC0.color = _phong(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm), Vcol0);
                    PC1.color = _phong(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm), Vcol1);
                    PC2.color = _phong(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm), Vcol2);
                    PC3.color = _phong(icu * dotProduct(NN3, _r_light_inorm), icu * dotProduct(NN3, _r_H_inorm), Vcol3);
                    }
                }
            else
                { // flat shading
                const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
                faceN.normalize();
                if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                    {
                    _uni.facecolor = _phong<true>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                else
                    {
                    _uni.facecolor = _phong<false>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                }

            if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                { // store texture vectors if needed
                PC0.T = *T0;
                PC1.T = *T1;
                PC2.T = *T2;
                PC3.T = *T3;
                }

            // go rasterize !
            rasterizeTriangle<LX, LY>(PC0, PC1, PC2, _ox, _oy, _uni, shader_select<ZBUFFER, ORTHO, color_t>);
            rasterizeTriangle<LX, LY>(PC0, PC2, PC3, _ox, _oy, _uni, shader_select<ZBUFFER, ORTHO, color_t>);
            
            return;
            }






        /***********************************************************
        * Drawing wireframe
        ************************************************************/


        template<bool DRAW_FAST> int _drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameLine(const fVec3& P1, const fVec3& P2, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, color_t color, float opacity, float thickness);

        template<bool DRAW_FAST> int _drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness);




        /***********************************************************
        * Simple geometric objects
        ************************************************************/
        



        template<bool USE_BLENDING> void _drawPixel(const fVec3& pos, color_t color, float opacity)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return;   // nothing to do
            const bool has_zbuffer = ((ZBUFFER) && ((_uni.zbuf != nullptr) && (_zbuffer_len >= _uni.im->lx()* _uni.im->ly())));                       
            fVec4 Q = _projM * _r_modelViewM.mult1(pos);
            if (ORTHO) { Q.w = 2.0f - Q.z; } else { Q.zdivide(); }
            if ((Q.w < -1) || (Q.w > 1)) return;
            Q.x = ((Q.x + 1) * LX) / 2 - _ox;
            Q.y = ((Q.y + 1) * LY) / 2 - _oy;
            const int x = (int)roundfp(Q.x);
            const int y = (int)roundfp(Q.y);
            if (!has_zbuffer)
                {
                if (USE_BLENDING) _uni.im->drawPixel<true>(x, y, color, opacity); else  _uni.im->drawPixel<true>(x, y, color);
                }
            else
                {
                drawPixelZbuf<true, USE_BLENDING>(x, y, color, opacity, Q.w);
                }
            }


        template<bool USE_BLENDING> void _drawPixels(int nb_pixels, fVec3* pos_list, int* colors_ind, color_t* colors, int* opacities_ind, float* opacities)
            {
            if ((pos_list == nullptr) || (colors_ind == nullptr) || (colors == nullptr)) return;
            if ((USE_BLENDING) && ((opacities_ind == nullptr) || (opacities == nullptr))) return;
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return;   // nothing to do
            const bool has_zbuffer = ((ZBUFFER) && ((_uni.zbuf != nullptr) && (_zbuffer_len >= _uni.im->lx()* _uni.im->ly())));                       
            for (int k = 0; k < nb_pixels; k++)
                {
                fVec4 Q = _projM * _r_modelViewM.mult1(pos_list[k]);
                if (ORTHO) { Q.w = 2.0f - Q.z; } else { Q.zdivide(); }
                if ((Q.w < -1) || (Q.w > 1)) continue;
                Q.x = ((Q.x + 1) * LX) / 2 - _ox;
                Q.y = ((Q.y + 1) * LY) / 2 - _oy;
                const int x = (int)roundfp(Q.x);
                const int y = (int)roundfp(Q.y);
                if (!has_zbuffer)
                    {
                    if (USE_BLENDING) _uni.im->drawPixel<true>(x, y, colors[colors_ind[k]], opacities[opacities_ind[k]]); else  _uni.im->drawPixel<true>(x, y, colors[colors_ind[k]], opacities[opacities_ind[k]]);
                    }
                else
                    {
                    drawPixelZbuf<true, USE_BLENDING>(x, y, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                    }
                }           
            }


        template<bool USE_BLENDING> void _drawDot(const fVec3& pos, int r, color_t color, float opacity)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return;   // nothing to do
            const bool has_zbuffer = ((ZBUFFER) && ((_uni.zbuf != nullptr) && (_zbuffer_len >= _uni.im->lx()* _uni.im->ly())));                       
            fVec4 Q = _projM * _r_modelViewM.mult1(pos);
            if (ORTHO) { Q.w = 2.0f - Q.z; } else { Q.zdivide(); }
            if ((Q.w < -1) || (Q.w > 1)) return;
            Q.x = ((Q.x + 1) * LX) / 2 - _ox;
            Q.y = ((Q.y + 1) * LY) / 2 - _oy;
            const int x = (int)roundfp(Q.x);
            const int y = (int)roundfp(Q.y);
            if (!has_zbuffer)
                {
                if (USE_BLENDING)
                    _uni.im->drawCircle(x, y, r, color, opacity);
                else
                    _uni.im->drawCircle(x, y, r, color);
                }
            else 
                {
                const int lx = _uni.im->lx();
                const int ly = _uni.im->ly();
                if ((x - r >= 0) && (x + r < lx) && (y - r >= 0) && (y + r < ly))
                    _drawCircleZbuf<false, USE_BLENDING>(x, y, r, color, opacity, Q.w);
                else
                    {
                    if ((x >= 0) && (x < lx) && (y >= 0) && (y < ly))
                        _drawCircleZbuf<true, USE_BLENDING>(x, y, r, color, opacity, Q.w);
                    }
                }                
            }


        template<bool USE_BLENDING> void _drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            if ((pos_list == nullptr) || (colors_ind == nullptr) || (colors == nullptr) || (radius_ind == nullptr) || (radius == nullptr)) return;
            if ((USE_BLENDING) && ((opacities_ind == nullptr) || (opacities == nullptr))) return;
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return;   // nothing to do
            const bool has_zbuffer = ((ZBUFFER) && ((_uni.zbuf != nullptr) && (_zbuffer_len >= _uni.im->lx() * _uni.im->ly())));
            for (int k = 0; k < nb_dots; k++)
                {
                fVec4 Q = _projM * _r_modelViewM.mult1(pos_list[k]);
                if (ORTHO) { Q.w = 2.0f - Q.z; } else { Q.zdivide(); }
                if ((Q.w < -1) || (Q.w > 1)) continue;
                Q.x = ((Q.x + 1) * LX) / 2 - _ox;
                Q.y = ((Q.y + 1) * LY) / 2 - _oy;
                const int x = (int)roundfp(Q.x);
                const int y = (int)roundfp(Q.y);
                if (!has_zbuffer)
                    {
                    if (USE_BLENDING) 
                        _uni.im->drawCircle(x, y, radius[radius_ind[k]], colors[colors_ind[k]], opacities[opacities_ind[k]]); 
                    else 
                        _uni.im->drawCircle(x, y, radius[radius_ind[k]], colors[colors_ind[k]]);
                    }
                else
                    {
                    const int lx = _uni.im->lx();
                    const int ly = _uni.im->ly();
                    const int r = radius[radius_ind[k]];
                    if ((x - r >= 0) && (x + r < lx) && (y - r >= 0) && (y + r < ly))
                        _drawCircleZbuf<false,USE_BLENDING>(x, y, r, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                    else
                        {
                        if ((x >= 0) && (x < lx) && (y >= 0) && (y < ly))
                            _drawCircleZbuf<true, USE_BLENDING>(x, y, r, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                        }
                    }
                }
            }


        template<bool CHECKRANGE, bool USE_BLENDING> void drawPixelZbuf(int x, int y, color_t color, float opacity, float z)
            {
            if (CHECKRANGE && ((x < 0) || (x >= _uni.im->lx()) || (y < 0) || (y >= _uni.im->ly()))) return;
            if (_uni.zbuf[x + _uni.im->lx() * y] < z)
                {
                _uni.zbuf[x + _uni.im->lx() * y] = z;
                if (USE_BLENDING) _uni.im->drawPixel<false>(x, y, color, opacity); else  _uni.im->drawPixel<false>(x, y, color);
                }
            }


        template<bool CHECKRANGE, bool USE_BLENDING> TGX_INLINE inline void drawHLineZbuf(int x, int y, int w, color_t color, float opacity, float z)
            {
            if (CHECKRANGE)	// optimized away at compile time
                {
                const int lx = _uni.im->lx();
                const int ly = _uni.im->ly();
                if ((y < 0) || (y >= ly) || (x >= lx)) return;
                if (x < 0) { w += x; x = 0; }
                if (x + w > lx) { w = lx - x; }
                }
            while(w--) drawPixelZbuf<CHECKRANGE, USE_BLENDING>(x++, y, color, opacity, z);
            }


        template<bool CHECKRANGE, bool USE_BLENDING> void _drawCircleZbuf(int xm, int ym, int r, color_t color, float opacity, float z)
            { 
            if ((CHECKRANGE) && (r > 2))
                { // circle is large enough to check first if there is something to draw.
                if ((xm + r < 0) || (xm - r >= _uni.im->lx()) || (ym + r < 0) || (ym - r >= _uni.im->ly())) return; // outside of image. 
                }

            auto fillcolor = color; 
            const bool OUTLINE = true;
            const bool FILL = true;

            switch (r)
                {
                case 0:
                    {
                    if (OUTLINE)
                        drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm, ym, color, opacity, z);
                    else if (FILL)
                        drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm, ym, fillcolor, opacity, z);
                    return;
                    }
                case 1:
                    {
                    if (FILL)
                        drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm, ym, fillcolor, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm + 1, ym, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm - 1, ym, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm, ym - 1, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm, ym + 1, color, opacity, z);
                    return;
                    }
                }
            int x = -r, y = 0, err = 2 - 2 * r;
            do {
                if (OUTLINE)
                    {
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm - x, ym + y, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm - y, ym - x, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm + x, ym - y, color, opacity, z);
                    drawPixelZbuf<CHECKRANGE, USE_BLENDING>(xm + y, ym + x, color, opacity, z);
                    }
                r = err;
                if (r <= y)
                    {
                    if (FILL)
                        {
                        drawHLineZbuf<CHECKRANGE, USE_BLENDING>(xm, ym + y, -x, fillcolor, opacity, z);
                        drawHLineZbuf<CHECKRANGE, USE_BLENDING>(xm + x + 1, ym - y, -x - 1, fillcolor, opacity, z);
                        }
                    err += ++y * 2 + 1;
                    }
                if (r > x || err > y)
                    {
                    err += ++x * 2 + 1;
                    if (FILL)
                        {
                        if (x)
                            {
                            drawHLineZbuf<CHECKRANGE, USE_BLENDING>(xm - y + 1, ym - x, y - 1, fillcolor, opacity, z);
                            drawHLineZbuf<CHECKRANGE, USE_BLENDING>(xm, ym + x, y, fillcolor, opacity, z);
                            }
                        }
                    }
                } 
            while (x < 0);
            }



        /**
        * For adaptative mesh: compute the diameter (in pixels) of the unit sphere S(0,1) once projected the screen
        **/
        float _unitSphereScreenDiameter()
            {
            const float ONEOVERSQRT2 = 0.70710678118f;
            fVec4 P0 = _r_modelViewM.mult1(fVec3(0, 0, 0));
            fVec4 P1 = _r_modelViewM.mult1(fVec3(1, 0, 0));
            float r = fVec3(P0 - P1).norm(); // radius after modelview transform
            fVec4 P2 = { P0.x - r * ONEOVERSQRT2, P0.y - r * ONEOVERSQRT2, P0.z, P0.w }; // take a point on the sphere that is at same z as origin and take equal contribution from x and y. 

            fVec4 Q0 = _projM * P0;
            fVec4 Q2 = _projM * P2;
            if (!ORTHO)
                {
                Q0.zdivide();
                Q2.zdivide();
                }
            return sqrtf(((Q2.x - Q0.x) * (Q2.x - Q0.x) * LX * LX) + ((Q2.y - Q0.y) * (Q2.y - Q0.y) * LY * LY)); // diameter on the screen
            }


        template<bool WIREFRAME, bool DRAWFAST> void _drawSphere(int shader, int nb_sectors, int nb_stacks, const Image<color_t>* texture, float thickness, color_t color, float opacity);



        /***********************************************************
        * CLIPPING
        ************************************************************/


        /** used by _discard() for testing a point position against the frustum planes */
        void _clip(int & fl, const fVec3 & P, const fMat4 & M, float bx, float Bx, float by, float By)
            {
            fVec4 S = M.mult1(P);
            if (!ORTHO)
                {
                S.zdivide();
                if (S.w <= 0) S.z = -2;
                }
            if (S.x >= bx) { fl &= (~(1)); }
            if (S.x <= Bx) { fl &= (~(2)); }
            if (S.y >= by) { fl &= (~(4)); }
            if (S.y <= By) { fl &= (~(8)); }
            if (S.z >= -1.0f) { fl &= (~(16)); }
            if (S.z <= +1.0f) { fl &= (~(32)); }
            }


        /* test if a box is outside the image and should be discarded. */
        bool _discard(const fBox3 & bb, const fMat4& M)
            {
            if ((bb.minX == 0) && (bb.maxX == 0) && (bb.minY == 0) && (bb.maxY == 0) && (bb.minZ == 0) && (bb.maxZ == 0))
                return false; // do not discard if the bounding box is uninitialized.

            const float ilx = 2.0f / LX;
            const float bx = (_ox - 1) * ilx - 1.0f;
            const float Bx = (_ox + _uni.im->width() + 1) * ilx - 1.0f;
            const float ily = 2.0f / LY;
            const float by = (_oy - 1) * ily - 1.0f;
            const float By = (_oy + _uni.im->height() + 1) * ily - 1.0f;

            int fl = 63; // every bit set
            _clip(fl, fVec3(bb.minX, bb.minY, bb.minZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.minY, bb.maxZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.maxY, bb.minZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.minX, bb.maxY, bb.maxZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.minY, bb.minZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.minY, bb.maxZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.maxY, bb.minZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(bb.maxX, bb.maxY, bb.maxZ), M, bx, Bx, by, By);
            if (fl == 0) return false;
            return true;
            }


        /** used by _clipTestNeeded() */
        bool _clip2(float clipboundXY, const fVec3 & P, const fMat4 & M)
            {
            fVec4 S = M.mult1(P);
            if (!ORTHO)
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
        * PHONG LIGHTNING
        ************************************************************/

        static const int _POWTABSIZE = 32;      // number of entries in the precomputed power table for specular exponent.
        int _currentpow;                        // exponent for the currently computed table (<0 if table not yet computed)
        float _powmax;                          // used to compute exponent
        float _fastpowtab[_POWTABSIZE];         // the precomputed power table.

        /** Pre-compute the power table for computing specular light component (if needed). */
        void _precomputeSpecularTable(int exponent)
            {
            if (_currentpow == exponent) return;
            _currentpow = exponent;
            float specularExponent = (float)exponent;            
            if (exponent > 0)
                {
                const float MAX_VAL_POW = 10.0f;  //  maximum value that the power can take

                _powmax = pow(MAX_VAL_POW, 1 / specularExponent);
                for (int k = 0; k < _POWTABSIZE; k++)
                    {
                    float v = 1.0f - (((float)k) / _POWTABSIZE);
                    _fastpowtab[k] = powf(_powmax*v, specularExponent);
                    }
                }
            else
                {
                _powmax = 1; 
                for (int k = 0; k < _POWTABSIZE; k++)
                    {
                    _fastpowtab[k] = 0.0f;
                    }
                }
            return;
            }

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

        int     _ox, _oy;           // image offset w.r.t. the viewport        

        fMat4   _projM;             // projection matrix

        int     _zbuffer_len;       // size of the zbuffer
        
        RasterizerParams<color_t, color_t>  _uni; // rasterizer param (contain the image pointer and the zbuffer pointer).

        float _culling_dir;         // culling direction postive/negative or 0 to disable back face culling.


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












/******************************************************************************************************************************************
*
*
* IMPLEMENTATION DETAILS.
*
*
*******************************************************************************************************************************************/



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::Renderer3D() : _currentpow(-1), _ox(0), _oy(0), _zbuffer_len(0), _uni(), _culling_dir(1)
            {
            _uni.im = nullptr;
            _uni.tex = nullptr; 
            _uni.shader_type = 0; 
            _uni.zbuf = 0; 
            _uni.facecolor = RGBf(1.0, 1.0, 1.0);
            _uni.use_bilinear_texturing = false;

            // let's set some default values
            fMat4 M;
            if (ORTHO)
                M.setOrtho(-16.0f, 16.0f, -12.0f, 12.0f, 1.0f, 1000.0f);
            else
                M.setPerspective(45.0f, 1.5f, 1.0f, 1000.0f);  // 45 degree, 4/3 image ratio and minz = 1 maxz = 1000.
            this->setProjectionMatrix(M);

            this->setLookAt({ 0,0,0 }, { 0,0,-1 }, { 0,1,0 }); // look toward the negative z axis (id matrix)

            this->setLight(fVec3(-1.0f, -1.0f, -1.0f), // white light coming from the right, above, in front.
                RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f)); // full white.

            M.setIdentity();
            this->setModelMatrix(M); // no transformation on the mesh.

            this->setMaterial({ 0.75f, 0.75f, 0.75f }, 0.15f, 0.7f, 0.5f, 8); // just in case: silver color and some default reflexion param...
            this->_precomputeSpecularTable(8);
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        int  Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::drawMesh(const int shader, const Mesh3D<color_t>* mesh, bool use_mesh_material, bool draw_chained_meshes)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly() ))) return -2; // zbuffer required but not available.

            while (mesh)
                {
                if (mesh->vertice)
                    {
                    if (use_mesh_material)
                        {   // use mesh material if requested
                        _r_ambiantColor = _ambiantColor * mesh->ambiant_strength;
                        _r_diffuseColor = _diffuseColor * mesh->diffuse_strength;
                        _r_specularColor = _specularColor * mesh->specular_strength;
                        _r_objectColor = mesh->color;
                        }
                    // precompute pow(.,specularExponent) table if needed
                    const int specularExpo = (use_mesh_material ? mesh->specular_exponent : _specularExponent);
                    _precomputeSpecularTable(specularExpo);
                    int raster_type = shader;
                    if (mesh->normal == nullptr) TGX_SHADER_REMOVE_GOURAUD(raster_type) // gouraud shading not available so we disable it
                    if ((mesh->texcoord == nullptr) || (mesh->texture == nullptr)) TGX_SHADER_REMOVE_TEXTURE(raster_type) // texturing not available so we disable it
                    if (TGX_SHADER_HAS_GOURAUD(raster_type))
                        {
                        if (TGX_SHADER_HAS_TEXTURE(raster_type))
                            _drawMesh<TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE>(mesh);
                        else
                            _drawMesh<TGX_SHADER_GOURAUD>(mesh);
                        }
                    else
                        {
                        if (TGX_SHADER_HAS_TEXTURE(raster_type))
                            _drawMesh<TGX_SHADER_FLAT | TGX_SHADER_TEXTURE>(mesh);
                        else
                            _drawMesh<TGX_SHADER_FLAT>(mesh);
                        }
                    }
                mesh = ((draw_chained_meshes) ? mesh->next : nullptr);
                }

            if (use_mesh_material)
                { // restore material pre-computed values
                _r_ambiantColor = _ambiantColor * _ambiantStrength;
                _r_diffuseColor = _diffuseColor * _diffuseStrength;
                _r_specularColor = _specularColor * _specularStrength;
                _r_objectColor = _color;
                }
            return 0;
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<int RASTER_TYPE>
        void Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawMesh(const Mesh3D<color_t>* mesh)
            {
            _uni.shader_type = RASTER_TYPE;

            static const bool TEXTURE = (bool)(TGX_SHADER_HAS_TEXTURE(RASTER_TYPE));
            static const bool GOURAUD = (bool)(TGX_SHADER_HAS_GOURAUD(RASTER_TYPE));
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));

            // check if the object is completely outside of the image for fast discard.
            if (_discard(mesh->bounding_box, _projM * _r_modelViewM)) return;

            // check if the clipping test should be performed for each triangle in the mesh.
            const bool cliptestneeded = _clipTestNeeded(clipboundXY, mesh->bounding_box, _projM * _r_modelViewM);

            const fVec3* const tab_vert = mesh->vertice;  // array of vertices
            const fVec3* const tab_norm = mesh->normal;   // array of normals
            const fVec2* const tab_tex = mesh->texcoord;  // array of texture
            const uint16_t* face = mesh->face;      // array of triangles

            // set the texture.
            _uni.tex = (const Image<color_t>*)mesh->texture;

            ExtVec4 QQA, QQB, QQC;
            ExtVec4* PC0 = &QQA;
            ExtVec4* PC1 = &QQB;
            ExtVec4* PC2 = &QQC;

            int nbt;
            while ((nbt = *(face++)) > 0)
                { // starting a chain with nbt triangles

                // load the first triangle
                const uint16_t v0 = *(face++);
                if (TEXTURE) PC0->indt = *(face++); else { if (tab_tex) face++; }
                if (GOURAUD) PC0->indn = *(face++); else { if (tab_norm) face++; }

                const uint16_t v1 = *(face++);
                if (TEXTURE) PC1->indt = *(face++); else { if (tab_tex) face++; }
                if (GOURAUD) PC1->indn = *(face++); else { if (tab_norm) face++; }

                const uint16_t v2 = *(face++);
                if (TEXTURE) PC2->indt = *(face++); else { if (tab_tex) face++; }
                if (GOURAUD) PC2->indn = *(face++); else { if (tab_norm) face++; }

                // compute vertices position because we are sure we will need them...
                PC2->P = _r_modelViewM.mult1(tab_vert[v2]);
                PC0->P = _r_modelViewM.mult1(tab_vert[v0]);
                PC1->P = _r_modelViewM.mult1(tab_vert[v1]);

                // ...but use lazy computation of other vertex attributes
                PC0->missedP = true;
                PC1->missedP = true;
                PC2->missedP = true;

                while (1)
                    {
                    // face culling
                    fVec3 faceN = crossProduct(PC1->P - PC0->P, PC2->P - PC0->P);
                    const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, PC0->P);
                    if (cu * _culling_dir > 0) goto rasterize_next_triangle; // skip triangle !
                    // triangle is not culled

                    *((fVec4*)PC2) = _projM * PC2->P;
                    if (ORTHO) { PC2->w = 2.0f - PC2->z; } else { PC2->zdivide(); }

                    if (PC0->missedP)
                        {
                        *((fVec4*)PC0) = _projM * PC0->P;
                        if (ORTHO) { PC0->w = 2.0f - PC0->z; } else { PC0->zdivide(); }
                        }
                    if (PC1->missedP)
                        {
                        *((fVec4*)PC1) = _projM * PC1->P;
                        if (ORTHO) { PC1->w = 2.0f - PC1->z; } else { PC1->zdivide(); }
                        }
                        
                    // for the time being, we just drop the triangles that need clipping
                    // *** TODO : implement correct clipping ***                        
                    if (cliptestneeded)
                        {
                        const bool needclip = (PC2->P.z >= 0)
                            | (PC2->x < -clipboundXY) | (PC2->x > clipboundXY)
                            | (PC2->y < -clipboundXY) | (PC2->y > clipboundXY)
                            | (PC2->z < -1) | (PC2->z > 1)                      
                            | (PC0->P.z >= 0)
                            | (PC0->x < -clipboundXY) | (PC0->x > clipboundXY)
                            | (PC0->y < -clipboundXY) | (PC0->y > clipboundXY)
                            | (PC0->z < -1) | (PC0->z > 1)
                            | (PC1->P.z >= 0)
                            | (PC1->x < -clipboundXY) | (PC1->x > clipboundXY)
                            | (PC1->y < -clipboundXY) | (PC1->y > clipboundXY)
                            | (PC1->z < -1) | (PC1->z > 1);
                        if (needclip) goto rasterize_next_triangle;
                        }

                    // ok, the triangle must be rasterized !
                    if (GOURAUD)
                        { // Gouraud shading : color on vertices

                        // reverse normal only when culling is disabled (and we assume in this case that normals are given for the CCW face).
                        const float icu = (_culling_dir != 0) ? 1.0f : ((cu > 0) ? -1.0f : 1.0f);
                        if (PC0->missedP)
                            {
                            PC0->N = _r_modelViewM.mult0(tab_norm[PC0->indn]);
                            PC0->color = _phong<TEXTURE>(icu * dotProduct(PC0->N, _r_light_inorm), icu * dotProduct(PC0->N, _r_H_inorm));
                            }
                        if (PC1->missedP)
                            {
                            PC1->N = _r_modelViewM.mult0(tab_norm[PC1->indn]);
                            PC1->color = _phong<TEXTURE>(icu * dotProduct(PC1->N, _r_light_inorm), icu * dotProduct(PC1->N, _r_H_inorm));
                            }
                        PC2->N = _r_modelViewM.mult0(tab_norm[PC2->indn]);
                        PC2->color = _phong<TEXTURE>(icu * dotProduct(PC2->N, _r_light_inorm), icu * dotProduct(PC2->N, _r_H_inorm));
                        }
                    else
                        { // flat shading : color on faces
                        const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
                        faceN.normalize();
                        _uni.facecolor = _phong<TEXTURE>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                        }

                    if (TEXTURE)
                        { // compute texture vectors if needed
                        if (PC0->missedP) { PC0->T = tab_tex[PC0->indt]; }
                        if (PC1->missedP) { PC1->T = tab_tex[PC1->indt]; }
                        PC2->T = tab_tex[PC2->indt];
                        }

                    // attributes are now all up to date
                    PC0->missedP = false;
                    PC1->missedP = false;
                    PC2->missedP = false;

                    // go rasterize !                   
                    rasterizeTriangle<LX, LY> ((RasterizerVec4)QQA, (RasterizerVec4)QQB, (RasterizerVec4)QQC, _ox, _oy, _uni, shader_select<ZBUFFER, ORTHO, color_t>);

                
                rasterize_next_triangle:

                    if (--nbt == 0) break; // exit loop at end of chain

                    // get the next triangle
                    const uint16_t nv2 = *(face++);
                    swap(((nv2 & 32768) ? PC0 : PC1), PC2);
                    if (TEXTURE) PC2->indt = *(face++); else { if (tab_tex) face++; }
                    if (GOURAUD) PC2->indn = *(face++);  else { if (tab_norm) face++; }
                    PC2->P = _r_modelViewM.mult1(tab_vert[nv2 & 32767]);
                    PC2->missedP = true;
                    }
                }
            }





        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::drawTriangles(int shader, int nb_triangles,
            const uint16_t* ind_vertices, const fVec3* vertices,
            const uint16_t* ind_normals, const fVec3* normals,
            const uint16_t* ind_texture, const fVec2* textures,
            const Image<color_t>* texture_image)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return -3; // invalid vertices
            if ((ind_normals == nullptr) || (normals == nullptr)) TGX_SHADER_REMOVE_GOURAUD(shader) // disable gouraud            
            if ((ind_texture == nullptr) || (textures == nullptr) || (texture_image == nullptr)) TGX_SHADER_REMOVE_TEXTURE(shader) // disable texture
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            nb_triangles *= 3;

            if (TGX_SHADER_HAS_TEXTURE(shader))
                {
                _uni.tex = (const Image<color_t>*)texture_image;
                if (TGX_SHADER_HAS_GOURAUD(shader))
                    {
                    for (int n = 0; n < nb_triangles; n += 3)
                        {
                        _drawTriangle(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2],
                            normals + ind_normals[n], normals + ind_normals[n + 1], normals + ind_normals[n + 2],
                            textures + ind_texture[n], textures + ind_texture[n + 1], textures + ind_texture[n + 2],
                            _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                else
                    {
                    for (int n = 0; n < nb_triangles; n += 3)
                        {
                        _drawTriangle(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2],
                            nullptr, nullptr, nullptr,
                            textures + ind_texture[n], textures + ind_texture[n + 1], textures + ind_texture[n + 2],
                            _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                }
            else
                {
                if (TGX_SHADER_HAS_GOURAUD(shader))
                    {
                    for (int n = 0; n < nb_triangles; n += 3)
                        {
                        _drawTriangle(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2],
                            normals + ind_normals[n], normals + ind_normals[n + 1], normals + ind_normals[n + 2],
                            nullptr, nullptr, nullptr,
                            _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                else
                    {
                    for (int n = 0; n < nb_triangles; n += 3)
                        {
                        _drawTriangle(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2],
                            nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr,
                            _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                }
            return 0;
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::drawQuads(int shader, int nb_quads,
            const uint16_t* ind_vertices, const fVec3* vertices,
            const uint16_t* ind_normals, const fVec3* normals,
            const uint16_t* ind_texture, const fVec2* textures,
            const Image<color_t>* texture_image)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_uni.zbuf == nullptr) || (_zbuffer_len < _uni.im->lx() * _uni.im->ly()))) return -2; // zbuffer required but not available.
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return -3; // invalid vertices
            if ((ind_normals == nullptr) || (normals == nullptr)) TGX_SHADER_REMOVE_GOURAUD(shader); // disable gouraud
            if ((ind_texture == nullptr) || (textures == nullptr) || (texture_image == nullptr)) TGX_SHADER_REMOVE_TEXTURE(shader) // disable texture
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            nb_quads *= 4;
            if (TGX_SHADER_HAS_TEXTURE(shader))
                {
                _uni.tex = (const Image<color_t>*)texture_image;
                if (TGX_SHADER_HAS_GOURAUD(shader))
                    {
                    for (int n = 0; n < nb_quads; n += 4)
                        {
                        _drawQuad(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2], vertices + ind_vertices[n + 3],
                            normals + ind_normals[n], normals + ind_normals[n + 1], normals + ind_normals[n + 2], normals + ind_normals[n + 3],
                            textures + ind_texture[n], textures + ind_texture[n + 1], textures + ind_texture[n + 2], textures + ind_texture[n + 3],
                            _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                else
                    {
                    for (int n = 0; n < nb_quads; n += 4)
                        {
                        _drawQuad(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2], vertices + ind_vertices[n + 3],
                            nullptr, nullptr, nullptr, nullptr,
                            textures + ind_texture[n], textures + ind_texture[n + 1], textures + ind_texture[n + 2], textures + ind_texture[n + 3],
                            _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                }
            else
                {
                if (TGX_SHADER_HAS_GOURAUD(shader))
                    {
                    for (int n = 0; n < nb_quads; n += 4)
                        {
                        _drawQuad(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2], vertices + ind_vertices[n + 3],
                            normals + ind_normals[n], normals + ind_normals[n + 1], normals + ind_normals[n + 2], normals + ind_normals[n + 3],
                            nullptr, nullptr, nullptr, nullptr,
                            _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                else
                    {
                    for (int n = 0; n < nb_quads; n += 4)
                        {
                        _drawQuad(shader, vertices + ind_vertices[n], vertices + ind_vertices[n + 1], vertices + ind_vertices[n + 2], vertices + ind_vertices[n + 3],
                            nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr,
                            _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
                        }
                    }
                }
            return 0;
            }





        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if (thickness <= 0) return 0;

            const tgx::fMat4 M(LX/2.0f, 0, 0, LX / 2.0f - _ox,
                               0, LY / 2.0f, 0, LY / 2.0f - _oy,
                               0, 0, 1, 0,
                               0, 0, 0, 0);

            while (mesh != nullptr)
                {
                // check if the object is completely outside of the image for fast discard.
                if (_discard(mesh->bounding_box, _projM * _r_modelViewM)) return 0;

                const fVec3* const tab_norm = mesh->normal;   // array of normals
                const fVec2* const tab_tex = mesh->texcoord;  // array of texture

                const fVec3* const tab_vert = mesh->vertice;  // array of vertices
                const uint16_t* face = mesh->face;      // array of triangles

                ExtVec4 QQA, QQB, QQC;
                ExtVec4* PC0 = &QQA;
                ExtVec4* PC1 = &QQB;
                ExtVec4* PC2 = &QQC;

                int nbt;
                while ((nbt = *(face++)) > 0)
                    { // starting a chain with nbt triangles

                    // load the first triangle
                    const uint16_t v0 = *(face++);
                    if (tab_tex) face++;
                    if (tab_norm) face++;

                    const uint16_t v1 = *(face++);
                    if (tab_tex) face++;
                    if (tab_norm) face++;

                    const uint16_t v2 = *(face++);
                    if (tab_tex) face++;
                    if (tab_norm) face++;

                    // compute vertices position because we are sure we will need them...
                    PC2->P = _r_modelViewM.mult1(tab_vert[v2]);
                    PC0->P = _r_modelViewM.mult1(tab_vert[v0]);
                    PC1->P = _r_modelViewM.mult1(tab_vert[v1]);

                    // ...but use lazy computation of other vertex attributes
                    PC0->missedP = true;
                    PC1->missedP = true;
                    PC2->missedP = true;

                    while (1)
                        {
                        // face culling
                        fVec3 faceN = crossProduct(PC1->P - PC0->P, PC2->P - PC0->P);
                        const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, PC0->P);
                        if (cu * _culling_dir > 0) goto rasterize_next_wireframetriangle; // skip triangle !


                        // triangle is not culled
                        *((fVec4*)PC2) = _projM * PC2->P;                        
                        if (ORTHO) { PC2->w = 2.0f - PC2->z; } else { PC2->zdivide(); }
                        *((fVec4*)PC2) = M.mult1(*((fVec4*)PC2));

                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = _projM * PC0->P;                            
                            if (ORTHO) { PC0->w = 2.0f - PC0->z; } else { PC0->zdivide(); }
                            *((fVec4*)PC0) = M.mult1(*((fVec4*)PC0));
                            }
                        if (PC1->missedP)
                            {
                            *((fVec4*)PC1) = _projM * PC1->P;
                            if (ORTHO) { PC1->w = 2.0f - PC1->z; } else { PC1->zdivide(); }
                            *((fVec4*)PC1) = M.mult1(*((fVec4*)PC1));
                            }

                        // attributes are now all up to date
                        PC0->missedP = false;
                        PC1->missedP = false;
                        PC2->missedP = false;

                        // clip test
                        if ((PC0->P.z >= 0) || (PC0->z < -1) || (PC0->z > 1) 
                         || (PC1->P.z >= 0) || (PC1->z < -1) || (PC1->z > 1) 
                         || (PC2->P.z >= 0) || (PC2->z < -1) || (PC2->z > 1))
                            goto rasterize_next_wireframetriangle;

                        // draw triangle                       
                        if (DRAW_FAST)
                            {
                            iVec2 PP0(*((fVec2*)PC0));
                            iVec2 PP1(*((fVec2*)PC1));
                            iVec2 PP2(*((fVec2*)PC2));
                            if (PP0 == PP1) 
                                {
                                _uni.im->drawLine(PP0, PP2, color);
                                }
                            else if (PP0 == PP2)
                                {
                                _uni.im->drawLine(PP0, PP1, color);
                                }
                            else if (PP1 == PP2)
                                {
                                _uni.im->drawLine(PP0, PP1, color);
                                }
                            else
                                {
                                _uni.im->drawLine(PP0, PP1, color);
                                _uni.im->drawLine(PP1, PP2, color);
                                _uni.im->drawLine(PP2, PP0, color);
                                }
                            }
                        else
                            {
                            _uni.im->drawWideLine(*((fVec2*)PC0), *((fVec2*)PC1), thickness, color, opacity);
                            _uni.im->drawWideLine(*((fVec2*)PC1), *((fVec2*)PC2), thickness, color, opacity);
                            _uni.im->drawWideLine(*((fVec2*)PC2), *((fVec2*)PC0), thickness, color, opacity);
                            }

                    rasterize_next_wireframetriangle:

                        if (--nbt == 0) break; // exit loop at end of chain

                        // get the next triangle
                        const uint16_t nv2 = *(face++);
                        swap(((nv2 & 32768) ? PC0 : PC1), PC2);
                        if (tab_tex) face++;
                        if (tab_norm) face++;
                        PC2->P = _r_modelViewM.mult1(tab_vert[nv2 & 32767]);
                        PC2->missedP = true;
                        }
                    }
                mesh = ((draw_chained_meshes) ? mesh->next : nullptr);
                }
            return 0;            
        }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameLine(const fVec3& P1, const fVec3& P2, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if (thickness <= 0) return 0;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);

            if ((Q0.z >= 0)||(Q1.z >= 0)) return;

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);

            fVec4 H1 = _projM * Q1;
            if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
            if ((H1.w < -1) || (H1.w > 1)) return;
            H1 = M.mult1(H1);

            if (DRAW_FAST)
                {
                iVec2 PP0(H0);
                iVec2 PP1(H1);
                _uni.im->drawLine(PP0, PP1, color);
                }
            else
                {
                _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                }

            return 0;
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return -2; // invalid vertices
            if (thickness <= 0) return 0;

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            nb_lines *= 2;
            for (int n = 0; n < nb_lines; n += 2)
                {
                // compute position in wiew space.
                const fVec4 Q0 = _r_modelViewM.mult1(vertices[ind_vertices[n]]);
                const fVec4 Q1 = _r_modelViewM.mult1(vertices[ind_vertices[n + 1]]);

                if ((Q0.z >= 0)||(Q1.z >= 0)) return;
                
                fVec4 H0 = _projM * Q0;
                if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);

                fVec4 H1 = _projM * Q1;
                if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
                if ((H1.w < -1) || (H1.w > 1)) continue;
                H1 = M.mult1(H1);

                if (DRAW_FAST)
                    {
                    iVec2 PP0(H0);
                    iVec2 PP1(H1);
                    _uni.im->drawLine(PP0, PP1, color);
                    }
                else
                    {
                    _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                    }

                }

            return 0;            
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if (thickness <= 0) return 0;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);
            const fVec4 Q2 = _r_modelViewM.mult1(P3);

            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return 0; // skip triangle !

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);

            fVec4 H1 = _projM * Q1;
            if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
            if ((H1.w < -1) || (H1.w > 1)) return;
            H1 = M.mult1(H1);

            fVec4 H2 = _projM * Q2;
            if (ORTHO) { H2.w = 2.0f - H2.z; } else { H2.zdivide(); }
            if ((H2.w < -1) || (H2.w > 1)) return;
            H2 = M.mult1(H2);

            // draw triangle                       
            if (DRAW_FAST)
                {
                iVec2 PP0(H0);
                iVec2 PP1(H1);
                iVec2 PP2(H2);
                if (PP0 == PP1)
                    {
                    _uni.im->drawLine(PP0, PP2, color);
                    }
                else if (PP0 == PP2)
                    {
                    _uni.im->drawLine(PP0, PP1, color);
                    }
                else if (PP1 == PP2)
                    {
                    _uni.im->drawLine(PP0, PP1, color);
                    }
                else
                    {
                    _uni.im->drawLine(PP0, PP1, color);
                    _uni.im->drawLine(PP1, PP2, color);
                    _uni.im->drawLine(PP2, PP0, color);
                    }
                }
            else
                {
                _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                _uni.im->drawWideLine(H1, H2, thickness, color, opacity);
                _uni.im->drawWideLine(H2, H0, thickness, color, opacity);
                }

            return 0;
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return -2; // invalid vertices
            if (thickness <= 0) return 0;

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            nb_triangles *= 3;
            for (int n = 0; n < nb_triangles; n += 3)
                {
                // compute position in wiew space.
                const fVec4 Q0 = _r_modelViewM.mult1(vertices[ind_vertices[n]]);
                const fVec4 Q1 = _r_modelViewM.mult1(vertices[ind_vertices[n + 1]]);
                const fVec4 Q2 = _r_modelViewM.mult1(vertices[ind_vertices[n + 2]]);

                if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

                // face culling 
                fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
                const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
                if (cu * _culling_dir > 0) continue; // we discard the triangle.

                fVec4 H0 = _projM * Q0;
                if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);

                fVec4 H1 = _projM * Q1;
                if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
                if ((H1.w < -1) || (H1.w > 1)) continue;
                H1 = M.mult1(H1);

                fVec4 H2 = _projM * Q2;
                if (ORTHO) { H2.w = 2.0f - H2.z; } else { H2.zdivide(); }
                if ((H2.w < -1) || (H2.w > 1)) continue;
                H2 = M.mult1(H2);

                // draw triangle                       
                if (DRAW_FAST)
                    {
                    iVec2 PP0(H0);
                    iVec2 PP1(H1);
                    iVec2 PP2(H2);
                    if (PP0 == PP1)
                        {
                        _uni.im->drawLine(PP0, PP2, color);
                        }
                    else if (PP0 == PP2)
                        {
                        _uni.im->drawLine(PP0, PP1, color);
                        }
                    else if (PP1 == PP2)
                        {
                        _uni.im->drawLine(PP0, PP1, color);
                        }
                    else
                        {
                        _uni.im->drawLine(PP0, PP1, color);
                        _uni.im->drawLine(PP1, PP2, color);
                        _uni.im->drawLine(PP2, PP0, color);
                        }
                    }
                else
                    {
                    _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                    _uni.im->drawWideLine(H1, H2, thickness, color, opacity);
                    _uni.im->drawWideLine(H2, H0, thickness, color, opacity);
                    }

                }

            return 0;

            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if (thickness <= 0) return 0;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);
            const fVec4 Q2 = _r_modelViewM.mult1(P3);

            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return 0; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

            const fVec4 Q3 = _r_modelViewM.mult1(P4); // compute fourth point

            if (Q3.z >= 0) return;

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);

            fVec4 H1 = _projM * Q1;
            if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
            if ((H1.w < -1) || (H1.w > 1)) return;
            H1 = M.mult1(H1);

            fVec4 H2 = _projM * Q2;
            if (ORTHO) { H2.w = 2.0f - H2.z; } else { H2.zdivide(); }
            if ((H2.w < -1) || (H2.w > 1)) return;
            H2 = M.mult1(H2);

            fVec4 H3 = _projM * Q3;
            if (ORTHO) { H3.w = 2.0f - H3.z; } else { H3.zdivide(); }
            if ((H3.w < -1) || (H3.w > 1)) return;
            H3 = M.mult1(H3);

            // draw quad                      
            if (DRAW_FAST)
                {
                iVec2 PP0(H0);
                iVec2 PP1(H1);
                iVec2 PP2(H2);
                iVec2 PP3(H3);
                if (PP0 != PP1) _uni.im->drawLine(PP0, PP1, color);
                if (PP1 != PP2) _uni.im->drawLine(PP1, PP2, color);
                if (PP2 != PP3) _uni.im->drawLine(PP2, PP3, color);
                if (PP3 != PP0) _uni.im->drawLine(PP3, PP0, color);
                }
            else
                {
                _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                _uni.im->drawWideLine(H1, H2, thickness, color, opacity);
                _uni.im->drawWideLine(H2, H3, thickness, color, opacity);
                _uni.im->drawWideLine(H3, H0, thickness, color, opacity);
                }

            return 0;
            }



        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool DRAW_FAST>
        int Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            if ((_uni.im == nullptr) || (!_uni.im->isValid())) return -1;   // no valid image
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return -2; // invalid vertices
            if (thickness <= 0) return 0;

            const tgx::fMat4 M(LX / 2.0f, 0, 0, LX / 2.0f - _ox,
                0, LY / 2.0f, 0, LY / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            nb_quads *= 4;
            for (int n = 0; n < nb_quads; n += 4)
                {
                // compute position in wiew space.
                const fVec4 Q0 = _r_modelViewM.mult1(vertices[ind_vertices[n]]);
                const fVec4 Q1 = _r_modelViewM.mult1(vertices[ind_vertices[n + 1]]);
                const fVec4 Q2 = _r_modelViewM.mult1(vertices[ind_vertices[n + 2]]);

                if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

                // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
                fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
                const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
                if (cu * _culling_dir > 0) continue; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

                const fVec4 Q3 = _r_modelViewM.mult1(vertices[ind_vertices[n + 3]]); // compute fourth point

                if (Q3.z >= 0) return;

                fVec4 H0 = _projM * Q0;
                if (ORTHO) { H0.w = 2.0f - H0.z; } else { H0.zdivide(); }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);

                fVec4 H1 = _projM * Q1;
                if (ORTHO) { H1.w = 2.0f - H1.z; } else { H1.zdivide(); }
                if ((H1.w < -1) || (H1.w > 1)) continue;
                H1 = M.mult1(H1);

                fVec4 H2 = _projM * Q2;
                if (ORTHO) { H2.w = 2.0f - H2.z; } else { H2.zdivide(); }
                if ((H2.w < -1) || (H2.w > 1)) continue;
                H2 = M.mult1(H2);

                fVec4 H3 = _projM * Q3;
                if (ORTHO) { H3.w = 2.0f - H3.z; } else { H3.zdivide(); }
                if ((H3.w < -1) || (H3.w > 1)) continue;
                H3 = M.mult1(H3);

                // draw quad                      
                if (DRAW_FAST)
                    {
                    iVec2 PP0(H0);
                    iVec2 PP1(H1);
                    iVec2 PP2(H2);
                    iVec2 PP3(H3);
                    if (PP0 != PP1) _uni.im->drawLine(PP0, PP1, color);
                    if (PP1 != PP2) _uni.im->drawLine(PP1, PP2, color);
                    if (PP2 != PP3) _uni.im->drawLine(PP2, PP3, color);
                    if (PP3 != PP0) _uni.im->drawLine(PP3, PP0, color);
                    }
                else
                    {
                    _uni.im->drawWideLine(H0, H1, thickness, color, opacity);
                    _uni.im->drawWideLine(H1, H2, thickness, color, opacity);
                    _uni.im->drawWideLine(H2, H3, thickness, color, opacity);
                    _uni.im->drawWideLine(H3, H0, thickness, color, opacity);
                    }

                }

            return 0;
            }




        template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO>
        template<bool WIREFRAME, bool DRAWFAST>
        void Renderer3D<color_t, LX, LY, ZBUFFER, ORTHO>::_drawSphere(int shader, int nb_sectors, int nb_stacks, const Image<color_t>* texture, float thickness, color_t color, float opacity)
            {
            if (texture == nullptr)
                {
                TGX_SHADER_REMOVE_TEXTURE(shader);
                }

            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            _culling_dir = 1; 

            const float MPI = 3.141592653589793238f;
            const int MAX_SECTORS = 256; 
            if (nb_sectors > MAX_SECTORS) nb_sectors = 256;
            if (nb_sectors < 3) nb_sectors = 3;
            if (nb_stacks < 3) nb_stacks = 3;

            // precomputed sin/cos for sectors as we will need them often
            float cosTheta[MAX_SECTORS];
            float sinTheta[MAX_SECTORS];
            
            const float d_sector = 2*MPI / nb_sectors;
            for(int i = 0; i < nb_sectors; i++)
                {
                cosTheta[i] = cosf(i * d_sector);
                sinTheta[i] = sinf(i * d_sector);
                }

            const float d_stack = MPI / nb_stacks;

            fVec3 P1, P2, P3, P4;

            // top part, top vertex at {0,1,0}
            
            float cosPhi = cosf(d_stack);
            float sinPhi = sinf(d_stack);

            P1 = { 0,1,0 };

            P2.x = sinPhi * cosTheta[nb_sectors-1];
            P2.y = cosPhi;
            P2.z = sinPhi * sinTheta[nb_sectors - 1];

            P3.y = cosPhi;

            const float dtx = 1.0f / nb_sectors;

            float v = 0.5f * cosPhi + 0.5f;
            float u = 0;

            for (int i = 0; i < nb_sectors; i++)
                {
                P3.x = sinPhi * cosTheta[i];
                P3.z = sinPhi * sinTheta[i];
                
                if (WIREFRAME)
                    {
                    if (DRAWFAST) drawWireFrameTriangle(P1, P3, P2); else drawWireFrameTriangle(P1, P3, P2, thickness, color, opacity);
                    }
                else
                    {
                    drawTriangle(shader, P1, P3, P2, P1, P3, P2, { 0,1 }, { u + dtx, v }, { u , v }, texture);
                    }

                u += dtx; 
                P2.x = P3.x;
                P2.z = P3.z;
                }
            
            // main part       
            for (int j = 2; j < nb_stacks; j++)
                {

                float new_cosPhi = cosf(d_stack * j);
                float new_sinPhi = sinf(d_stack * j);

                P1.x = sinPhi * cosTheta[nb_sectors - 1];
                P1.y = cosPhi;
                P1.z = sinPhi * sinTheta[nb_sectors - 1];

                P3.y = cosPhi;

                P2.x = new_sinPhi * cosTheta[nb_sectors - 1];
                P2.y = new_cosPhi;
                P2.z = new_sinPhi * sinTheta[nb_sectors - 1];

                P4.y = new_cosPhi;

                v = 0.5f*cosPhi + 0.5f;
                const float vv = 0.5f*new_cosPhi + 0.5f;

                u = 0; 

                for (int i = 0; i < nb_sectors; i++)
                    {
                    const float uu = u + dtx;
                    P3.x = sinPhi * cosTheta[i];
                    P3.z = sinPhi * sinTheta[i];
                    P4.x = new_sinPhi * cosTheta[i];
                    P4.z = new_sinPhi * sinTheta[i];

                    if (WIREFRAME)
                        {
                        if (DRAWFAST) drawWireFrameQuad(P1, P3, P4, P2); else drawWireFrameQuad(P1, P3, P4, P2, thickness, color, opacity);
                        }
                        else
                        {
                        drawQuad(shader, P1, P3, P4, P2, P1, P3, P4, P2, {u,v}, {uu,v}, {uu,vv}, {u,vv}, texture);
                        }

                    u += dtx; 
                    P1.x = P3.x;
                    P1.z = P3.z;
                    P2.x = P4.x;
                    P2.z = P4.z;
                    }

                cosPhi = new_cosPhi;
                sinPhi = new_sinPhi;
                }
                

            // bottom part, bottom vertex at {0,1,0}
            P1 = { 0,-1,0 };

            P2.x = sinPhi * cosTheta[nb_sectors - 1];
            P2.y = cosPhi;
            P2.z = sinPhi * sinTheta[nb_sectors - 1];

            P3.y = cosPhi;

            u = 0; 

            for (int i = 0; i < nb_sectors; i++)
                {
                P3.x = sinPhi * cosTheta[i];
                P3.z = sinPhi * sinTheta[i];

                if (WIREFRAME)
                    {
                    if (DRAWFAST) drawWireFrameTriangle(P1, P2, P3); else drawWireFrameTriangle(P1, P2, P3, thickness, color, opacity);
                    }
                else
                    {
                    drawTriangle(shader, P1, P2, P3, P1, P2, P3, { 0, 0 }, { u,v }, { u + dtx, v }, texture);
                    }

                u += dtx;
                P2.x = P3.x;
                P2.z = P3.z;
                }

            // restore culling direction
            _culling_dir = save_culling;
            return; 
            }











}


#endif

#endif


/** end of file */

