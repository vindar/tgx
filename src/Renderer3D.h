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

            // face culling
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // skip triangle !

            RasterizerVec4 PC0, PC1, PC2;

            // test if clipping is needed
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));

            (*((fVec4*)&PC0)) = _projM * Q0;
            if (ORTHO) { PC0.w = 2.0f - PC0.z; } else { PC0.zdivide(); }
            bool needclip = (Q0.z >= 0)
                          | (PC0.x < -clipboundXY) | (PC0.x > clipboundXY)
                          | (PC0.y < -clipboundXY) | (PC0.y > clipboundXY)
                          | (PC0.z < -1) | (PC0.z > 1);
            (*((fVec4*)&PC1)) = _projM * Q1;
            if (ORTHO) { PC1.w = 2.0f - PC1.z; } else { PC1.zdivide(); }
            needclip |= (Q1.z >= 0)
                     | (PC1.x < -clipboundXY) | (PC1.x > clipboundXY)
                     | (PC1.y < -clipboundXY) | (PC1.y > clipboundXY)
                     | (PC1.z < -1) | (PC1.z > 1);

            (*((fVec4*)&PC2)) = _projM * Q2;
            if (ORTHO) { PC2.w = 2.0f - PC2.z; } else { PC2.zdivide(); }
            needclip |= (Q2.z >= 0)
                     | (PC2.x < -clipboundXY) | (PC2.x > clipboundXY)
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

            // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

            const fVec4 Q3 = _r_modelViewM.mult1(*P3); // compute fourth point

            RasterizerVec4 PC0, PC1, PC2, PC3;

            // test if clipping is needed
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));

            (*((fVec4*)&PC0)) = _projM * Q0;
            if (ORTHO) { PC0.w = 2.0f - PC0.z; } else { PC0.zdivide(); }
            bool needclip  = (Q0.z >= 0)
                           | (PC0.x < -clipboundXY) | (PC0.x > clipboundXY)
                           | (PC0.y < -clipboundXY) | (PC0.y > clipboundXY)
                           | (PC0.z < -1) | (PC0.z > 1);

            (*((fVec4*)&PC1)) = _projM * Q1;
            if (ORTHO) { PC1.w = 2.0f - PC1.z; } else { PC1.zdivide(); }
            needclip |= (Q1.z >= 0)
                     | (PC1.x < -clipboundXY) | (PC1.x > clipboundXY)
                     | (PC1.y < -clipboundXY) | (PC1.y > clipboundXY)
                     | (PC1.z < -1) | (PC1.z > 1);

            (*((fVec4*)&PC2)) = _projM * Q2;
            if (ORTHO) { PC2.w = 2.0f - PC2.z; } else { PC2.zdivide(); }
            needclip |= (Q2.z >= 0)
                     | (PC2.x < -clipboundXY) | (PC2.x > clipboundXY)
                     | (PC2.y < -clipboundXY) | (PC2.y > clipboundXY)
                     | (PC2.z < -1) | (PC2.z > 1);


            (*((fVec4*)&PC3)) = _projM * Q3;
            if (ORTHO) { PC3.w = 2.0f - PC3.z; } else { PC3.zdivide(); }
            needclip |= (Q3.z >= 0)
                     | (PC3.x < -clipboundXY) | (PC3.x > clipboundXY)
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
                    PC0.color = _phong(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm),Vcol0);
                    PC1.color = _phong(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm),Vcol1);
                    PC2.color = _phong(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm),Vcol2);
                    PC3.color = _phong(icu * dotProduct(NN3, _r_light_inorm), icu * dotProduct(NN3, _r_H_inorm),Vcol3);
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

        static const int _POWTABSIZE = 16;  // number of entries in the precomputed power table for specular exponent.
        int _currentpow;                    // exponent for the currently computed table (<0 if table not yet computed)
        float _powfact;                     // used to compute exponent
        float _fastpowtab[_POWTABSIZE];     // the precomputed power table.

        /** Pre-compute the power table for computing specular light component (if needed). */
        void _precomputeSpecularTable(int exponent)
            {
            if (_currentpow == exponent) return;
            _currentpow = exponent;
            float specularExponent = (float)exponent;
            const float bbsp = (specularExponent < 8.0f) ? specularExponent : 8.0f;
            if (exponent > 0)
                {
                _powfact = ((specularExponent * _POWTABSIZE) / bbsp);
                for (int k = 0; k < _POWTABSIZE; k++)
                    {
                    float v = 1.0f - ((bbsp * k) / (specularExponent * _POWTABSIZE));
                    _fastpowtab[k] = powf(v, specularExponent);
                    }
                }
            else
                {
                _powfact = 0;
                for (int k = 0; k < _POWTABSIZE; k++)
                    {
                    _fastpowtab[k] = 0.0f;
                    }
                }
            }

        /** compute pow(x, exponent) using linear interpolation from the pre-computed table */
        TGX_INLINE float _powSpecular(float x) const
            {
            const float indf = (1.0f - x) * _powfact;
            const int indi = (int)indf;
            return (indi >= (_POWTABSIZE - 1)) ? 0.0f : (_fastpowtab[indi] + (indf - indi) * (_fastpowtab[indi + 1] - _fastpowtab[indi]));
            }


        /** compute a color according to Phong lightning model, use model color */
        template<bool TEXTURE> TGX_INLINE  RGBf _phong(float v_diffuse, float v_specular) const
            {
            RGBf col = _r_ambiantColor;
            col += _r_diffuseColor * max(v_diffuse, 0.0f);
            col += _r_specularColor * _powSpecular(max(v_specular, 0.0f)); // pow() this is too slow so we use a lookup table instead
            if (!(TEXTURE)) col *= _r_objectColor;
            col.clamp();
            return col;
            }


        /** compute a color according to Phong lightning model, use given color */
        TGX_INLINE  RGBf _phong(float v_diffuse, float v_specular, RGBf color) const
            {
            RGBf col = _r_ambiantColor;
            col += _r_diffuseColor * max(v_diffuse, 0.0f);
            col += _r_specularColor * _powSpecular(max(v_specular, 0.0f)); // pow() this is too slow so we use a lookup table instead
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

            this->setMaterial({ 0.75f, 0.75f, 0.75f }, 0.15f, 0.7f, 0.5f, 16); // just in case: silver color and some default reflexion param...
            this->_precomputeSpecularTable(16);
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
                    if (cliptestneeded)
                        {
                        // test if clipping is needed
                        *((fVec4*)PC2) = _projM * PC2->P;
                        if (ORTHO) { PC2->w = 2.0f - PC2->z; }
                        else { PC2->zdivide(); }
                        bool needclip = (PC2->P.z >= 0)
                            | (PC2->x < -clipboundXY) | (PC2->x > clipboundXY)
                            | (PC2->y < -clipboundXY) | (PC2->y > clipboundXY)
                            | (PC2->z < -1) | (PC2->z > 1);
                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = _projM * PC0->P;
                            if (ORTHO) { PC0->w = 2.0f - PC0->z; }
                            else { PC0->zdivide(); }
                            needclip |= (PC0->P.z >= 0)
                                | (PC0->x < -clipboundXY) | (PC0->x > clipboundXY)
                                | (PC0->y < -clipboundXY) | (PC0->y > clipboundXY)
                                | (PC0->z < -1) | (PC0->z > 1);
                            }
                        if (PC1->missedP)
                            {
                            *((fVec4*)PC1) = _projM * PC1->P;
                            if (ORTHO) { PC1->w = 2.0f - PC1->z; }
                            else { PC1->zdivide(); }
                            needclip |= (PC1->P.z >= 0)
                                | (PC1->x < -clipboundXY) | (PC1->x > clipboundXY)
                                | (PC1->y < -clipboundXY) | (PC1->y > clipboundXY)
                                | (PC1->z < -1) | (PC1->z > 1);
                            }
                        // for the time being, we just drop the triangles that need clipping
                        // *** TODO : implement correct clipping ***
                        if (needclip) goto rasterize_next_triangle;
                        }
                    else
                        {
                        // skip the clipping test
                        *((fVec4*)PC2) = _projM * PC2->P;
                        if (ORTHO) { PC2->w = 2.0f - PC2->z; }
                        else { PC2->zdivide(); }
                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = _projM * PC0->P;
                            if (ORTHO) { PC0->w = 2.0f - PC0->z; }
                            else { PC0->zdivide(); }
                            }
                        if (PC1->missedP)
                            {
                            *((fVec4*)PC1) = _projM * PC1->P;
                            if (ORTHO) { PC1->w = 2.0f - PC1->z; }
                            else { PC1->zdivide(); }
                            }
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


}


#endif

#endif


/** end of file */

