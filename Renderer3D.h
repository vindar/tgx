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

#pragma once


#include "Misc.h"
#include "Color.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Box2.h"
#include "Mat4.h"
#include "Image.h"
#include "Mesh3D.h"



namespace tgx
{


    /**
    * Class that draws a 3D mesh onto an Image. 
    * 
    * Template parameters: 
    * 
    * - color_t : the color type for the image to draw onto.
    * 
    * - LX , LY : Viewport size (up to 2048x2048). The normalized coordinates in [-1,1]x[-1,1]
    *             are mapped to [0,LX-1]x[0,LY-1] just before rasterization. 
    *             It is possible to use a viewport larger than the image we draw onto and set
    *             an offset for the image inside the viewport to do 'tile rendering'. 
    * 
    * - ZBUFFER : (default true) Set this to use depth testing when drawing the mesh. In this case,
    *             a valid depth buffer MUST be supplied with setZbuffer() before drawing a mesh. 
    * 
    * - ORTHO   : (default false) Set this to use orthographic projection and disable the z-divide 
    *              after projection. 
    *
    * - BACKFACE_CULLING : (default true) Set this to enable culling of back facing triangle in the 
    *                       meshes. Make sure that all the triangles in the mesh have correct winding! 
    * 
    **/
    template<typename color_t, int LX, int LY, bool ZBUFFER, bool ORTHO, bool BACKFACE_CULLING = true>
    class Renderer3D
    {

        static const int MAXVIEWPORTDIMENSION = 2048;

        static_assert((LX > 0) && (LX <= MAXVIEWPORTDIMENSION), "Invalid viewport width.");
        static_assert((LY > 0) && (LY <= MAXVIEWPORTDIMENSION), "Invalid viewport height.");
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");

       public:

   
        /**
        * Ctor
        **/
        Renderer3D() :  _ox(0), _oy(0), _im(nullptr), _zbuffer(nullptr), _zbuffer_len(0)
            { 
            // let us set some default values             
            if (ORTHO)
                {
                this->projectionMatrix().setOrtho(-16.0f, 16.0f, -12.0f, 12.0f, 1.0f, 1000.0f); // ortho projection
                }
            else
                {
                this->projectionMatrix().setPerspective(45.0f, 1.5f, 1.0f, 1000.0f);  // 45 degree, 4/3 image ratio and minz = 1 maxz = 1000. 
                }

            this->viewMatrix().setLookAt({ 0,0,0 }, { 0,0,-1 }, { 0,1,0 }); // look toward the negative z axis (id matrix)

            this->setLight(fVec3( -1.0f, -1.0f, -1.0f ), // white light coming from the right, above, in front. 
                           RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f)); // full white. 

            this->modelMatrix().setIdentity(); // no transformation on the mesh. 

            this->useModelDefaultLightning(true); // use the mesh default color/illumination parameters. 

            this->setModelLightning({ 0.75f, 0.75f, 0.75f }, 0.15f, 0.7f, 0.5f, 16); // just in case: silver color and some default reflexion param...
            }




        /*****************************************************************************************
        *
        * General parameters
        *
        ******************************************************************************************/


        /**
        * Set the image that will be drawn onto.
        * The image can be smaller than the viewport.
        **/
        void setImage(Image<color_t>* im)
            {
            _im = im;
            }


        /**
        * Set the offset of the image relative to the viewport.
        *
        * If the image has size (sx,sy), then during rasterization only the sub part
        * [ox, ox + sx[x[oy, oy+sy[ of the viewport will be drawn onto the image
        *
        * By changing the offset and redrawing several times it it possible to use an image
        * smaller  than the viewport (and thus also save on zbuffer space).
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
        **/
        void setProjectionMatrix(const fMat4& M)
            {
            _projM = M;
            }


        /**
        * Get a reference to the projection matrix
        * Can be used to modify the matrix 'in place'.
        **/
        fMat4& projectionMatrix()
            {
            return _projM;
            }


		/** 
         * Set the projection matrix as an orthographic matrix: 
		 * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml 
         *
         * This is equivalent to projectionMatrix().setOrtho()
         * This method is only available when ORTHO = true.
         **/
		void setOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
			{
            static_assert(ORTHO == true, "the setOrtho() method can only be used with template parameter ORTHO = true (use projectionMatrix().setOrtho() is you really want to...)");
            projectionMatrix().setOrtho(left, right, bottom, top, zNear, zFar);
			}


		/**
         * Set the projection matrix as a perspective matrix
		* https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glFrustum.xml
        * 
        * This is equivalent to projectionMatrix().setFrustum()
        * This method is only available when ORTHO = false. 
		**/
		void setFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
			{
            static_assert(ORTHO == false, "the setFrustum() method can only be used with template parameter ORTHO = false (use projectionMatrix().setFrustum() is you really want to...)");
            projectionMatrix().setFrustum(left, right, bottom, top, zNear, zFar);
			}


		/**
		*  Set the projection matrix as a perspective matrix
		* https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
        * 
        * This is equivalent to projectionMatrix().setPerspective()
        * This method is only available when ORTHO = false
		**/
		void setPerspective(float fovy, float aspect, float zNear, float zFar)
			{
            static_assert(ORTHO == false, "the setPerspective() method can only be used with template parameter ORTHO = false (use projectionMatrix().setPerspective() is you really want to...)");
            projectionMatrix().setPerspective(fovy, aspect, zNear, zFar);
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
            _zbuffer = zbuffer;
            _zbuffer_len = length;
            }


        /**
        * Clear the Zbuffer.
        *
        * This method should be called before drawing a new frame to erase
        * the previous zbuffer.
        *
        * The zbuffer is intentionally not clear between draw() calls to enable
        * the rendering of multiple mesh on the same scene.
        **/
        void clearZbuffer()
            {
            static_assert(ZBUFFER == true, "the clearZbuffer() method can only be used with template parameter ZBUFFER = true");
            if (_zbuffer) memset(_zbuffer, 0, _zbuffer_len*sizeof(float));
            }




        /*****************************************************************************************
        * 
        * Scene specific parameters
        * 
        ******************************************************************************************/

        /**
        * Set the view tranformation matrix.
        **/
        void setViewMatrix(const fMat4& M)
            {
            _viewM = M;
            }


        /**
        * Get a reference to the view tranformation matrix
        * (can be used to modify the matrix 'in place')
        **/
        fMat4& viewMatrix()
            {
            return _viewM;
            }


        /**
        * Set the view matrix so that the camera is looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        * 
        * This is equivalent to viewMatrix().setLookAt()
        **/
        void setLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
            {
            viewMatrix().setLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
            }
            

        /**
        * Set the matrix a camera looking at a given direction.
        * https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
        *
        * This is equivalent to viewMatrix().setLookAt()
        **/
        void setLookAt(const fVec3 eye, const fVec3 center, const fVec3 up)
            {
            setLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
            }


        /**
        * Set the light source direction (i.e. the direction the light points to). 
        * Direction is given in world cooridnates (hence transformed by the viewMatrix
        * at rendering time)
        **/
        void setLightDirection(const fVec3 & direction)
            {
            _light = direction; 
            }


        /**
        * Set the scene ambiant light color (in the Phong lightning model). 
        **/
        void setLightAmbiant(const RGBf & color)
            {
            _ambiantColor = color;
            }


        /**
        * Set the scene diffuse light color (in the Phong lightning model). 
        **/
        void setLightDiffuse(const RGBf & color)
            {
            _diffuseColor = color;
            }


        /**
        * Set the scene specular light color (in the Phong lightning model). 
        **/
        void setLightSpecular(const RGBf & color)
            {
            _specularColor = color;
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
        * 
        * Model specific parameters 
        * 
        ******************************************************************************************/


        /**
        * Set the model tranformation matrix. 
        **/
        void setModelMatrix(const fMat4& M)
            {
            _modelM = M;
            }


        /**
        * Get a reference to the model tranformation matrix
        * (can be used to modify the matrix 'in place').
        **/
        fMat4 &  modelMatrix()
            {
            return _modelM;
            }


        /**
        * If true, the default color and reflexions values specifies in the 
        * mesh3D structure are used when rendering the model. Otherwise, the 
        * defaults values used are those set with the methods below. 
        **/
        void useModelDefaultLightning(bool use_default_values)
            {
            _useMeshDefault = use_default_values;
            }


        /**
        * Set the object color (used when texturing is disabled)
        * 
        * Only used when rendering if useModelDefaultLightning(false).
        **/
        void setModelColor(RGBf color)
            {
            _color = color;
            }


        /**
        * Set the object ambiant lightning reflection strength.
        *
        * Only used when rendering if useModelDefaultLightning(false).
        **/
        void setModelAmbiantStrength(float strenght = 0.1f)
            {
            _ambiantStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces. 
            }


        /**
        * Set the object diffuse lightning reflection strength.
        *
        * Only used when rendering if useModelDefaultLightning(false).
        **/
        void setModelDiffuseStrength(float strenght = 0.6f)
            {
            _diffuseStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces. 
            }


        /**
        * Set the object specular lightning reflection strength.
        *
        * Only used when rendering if useModelDefaultLightning(false).
        **/
        void setModelSpecularStrength(float strenght = 0.5f)
            {
            _specularStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces. 
            }


        /**
        * Set the object specular exponent
        * Between 0 (no specular lightning) and 100 (very localized/glossy). 
        *
        * Only used when rendering if useModelDefaultLightning(false).
        **/
        void setModelSpecularExponent(int exponent = 16)
            {
            _specularExponent = clamp(exponent, 1, 100);
            }


        /**
        * Set the object color and light reflexion properties all at once. 
        *
        * These parameters are used when rendering only if useModelDefaultLightning(false).
        **/
        void setModelLightning(RGBf color, float ambiantStrength, float diffuseStrength, float specularStrength, int specularExponent)
            {
            this->setModelColor(color);
            this->setModelAmbiantStrength(ambiantStrength);
            this->setModelDiffuseStrength(diffuseStrength);
            this->setModelSpecularStrength(specularStrength);
            this->setModelSpecularExponent(specularExponent);
            }




        /**
        * Draw a mesh onto the image.
        * 
        * - shader  Type of shader to use. Combination of SHADER_GOURAUD, SHADER_FLAT, SHADER_TEXTURE
        * 
        *            - SHADER_FLAT : use flat shading (uniform color on each triangle). 
        * 
        *            - SHADER_GOURAUD : use gouraud shading (linear interpolation of triangles vertices 
        *              colors. This flag overwrites SHADER_FLAT but requires the mesh to have a normal 
        *              array otherwise the renderer will fall back to flat shading.
        * 
        *            - SHADER_TEXTURE : use texture mapping. This flag complements SHADER_FLAT/SHADER_GOURAUD
        *              but require the mesh to a a valid texture array and a valid texture image otherwise
        *              the renderer will fall back to uniform color shading.
        * 
        * - mesh    The mesh to draw. THe meshes / vertices array / textures can be in RAM or in FLASH 
        *           (faster when in RAM).  
        * 
        * - draw_chained_meshes  If true, the meshes linked to this mesh (via the ->next member) are also drawn.
        *                        
        * The method returns 0 if drawing was correctly performed and a negative number of an error occured:
        *   -1 : invalid image
        *   -2 : invalid zbuffer (only when template parameter ZBUFFER=true)
        *   -3 : the mesh (or one of the linked mesh) was invalid
        * 
        * REMARK:
        * 
        * (1) Depth testing, face culling and zdivide are automatically enabled/disabled depending on the class
        *     templates parameters ZBUFFER, BACKFACE_CULLING and ORTHO. 
        * 
        * (2) When texturing is disabled (or not available), the object color/reflexion is either the default color
        *     defined in the mesh or the color set by setModelLightning() depending on useModelDefaultLightning().
        * 
        **/
        int draw(const int shader, const Mesh3D<color_t> * mesh, bool draw_chained_meshes = true)
            {            
            if ((_im == nullptr) || (!_im->isValid())) return -1;   // no valid image
            if ((ZBUFFER) && ((_zbuffer == nullptr) || (_zbuffer_len < LX * LY))) return -2; // zbuffer required but not available. 
            
            while (mesh)
                {
                int raster_type = shader; 
                if (mesh->vertice == nullptr) return -3;  // invalid mesh
                if (mesh->normal == nullptr) raster_type &= ~((int)SHADER_GOURAUD); // gouraud shading not available so we disable it
                if ((mesh->texcoord == nullptr)||(mesh->texture == nullptr)) raster_type &= ~((int)SHADER_TEXTURE); // texturing not available so we disable it                                                     
                if (raster_type & SHADER_GOURAUD)
                    {
                    if (raster_type & SHADER_TEXTURE) 
                        _draw<SHADER_GOURAUD | SHADER_TEXTURE>(mesh);
                    else  
                        _draw<SHADER_GOURAUD>(mesh);
                    }                
                else
                    {
                    if (raster_type & SHADER_TEXTURE)
                        _draw<SHADER_FLAT | SHADER_TEXTURE>(mesh);
                    else
                        _draw<SHADER_FLAT>(mesh);
                    }                

                mesh = ((draw_chained_meshes) ? mesh->next :  nullptr);
                }   
            return 0;
            }



  
    private:








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

        
        /* test if a box is outside the viewing frusum and should be discarded. */
        bool _discard(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax, const fMat4& M, float bx, float Bx, float by, float By)
            {
            if ((xmin == 0) && (xmax == 0) && (ymin == 0) && (ymax == 0) && (zmin == 0) && (zmax == 0))
                return false; // do not discard if the obunding box is uninitialized. 
            int fl = 63; // every bit set
            _clip(fl, fVec3(xmin, ymin, zmin), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmin, ymin, zmax), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmin, ymax, zmin), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmin, ymax, zmax), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmax, ymin, zmin), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmax, ymin, zmax), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmax, ymax, zmin), M, bx, Bx, by, By);
            if (fl == 0) return false;
            _clip(fl, fVec3(xmax, ymax, zmax), M, bx, Bx, by, By);
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
        bool _clipTestNeeded(float clipboundXY, float xmin, float xmax, float ymin, float ymax, float zmin, float zmax, const fMat4& M)
            {
            return (_clip2(clipboundXY, fVec3(xmin, ymin, zmin), M)
                 || _clip2(clipboundXY, fVec3(xmin, ymin, zmax), M)
                 || _clip2(clipboundXY, fVec3(xmin, ymax, zmin), M)
                 || _clip2(clipboundXY, fVec3(xmin, ymax, zmax), M)
                 || _clip2(clipboundXY, fVec3(xmax, ymin, zmin), M)
                 || _clip2(clipboundXY, fVec3(xmax, ymin, zmax), M)
                 || _clip2(clipboundXY, fVec3(xmax, ymax, zmin), M)
                 || _clip2(clipboundXY, fVec3(xmax, ymax, zmax), M));
            }




        struct ExtVec4 : public Image<color_t>::RasterizerVec4
            {
            fVec4 P;       // after model-view matrix multiplication
            fVec4 N;       // normal vector after model-view matrix multiplication
            bool missedP;  // true if the attributes should be computed
            int indn;      // index for normal vector in array 
            int indt;      // index for texture vector in array
            };



    /** fast computation of powers via linear interpolation with a precomputed table */
    #define FASTPOW(VAR , X)  float VAR; \
                            { const float indf = (1.0f - (X)) * powfact; \
                              const int indi = (int)indf; \
                              VAR = (indi >= (LA - 1)) ? 0.0f : (fastpowtab[indi] + (indf - indi) * (fastpowtab[indi + 1] - fastpowtab[indi])); }


        /** 
        * Draw a mesh. this method acts as the "vertex shader" function.
        **/        
        template<int RASTER_TYPE> void _draw(const Mesh3D<color_t> * mesh)
            {
            static const int TEXTURE = (RASTER_TYPE & SHADER_TEXTURE);
            static const int GOURAUD = (RASTER_TYPE & SHADER_GOURAUD);

            const int ox = _ox;
            const int oy = _oy;
            
            // projection matrix
            fMat4 projM = _projM;
            projM.multScale({ 1,-1, 1 }); // invert the  y-axis because screen indexing starts from upper left and not bottom left corner. 

            // model-view matrix
            const fMat4 modelViewM = _viewM * _modelM;

            
            // check if the object is completely outside of the drawn image for fast discard. 
            const float ilx = 2.0f/LX;
            const float bx = (ox - 1)*ilx - 1.0f;
            const float Bx = (ox + _im->width() + 1)*ilx - 1.0f;
            const float ily = 2.0f/LY;
            const float by = (oy - 1)*ily - 1.0f;
            const float By = (oy + _im->height() + 1)*ily - 1.0f;
            if (_discard(mesh->bounding_box.xmin, mesh->bounding_box.xmax,
                         mesh->bounding_box.ymin, mesh->bounding_box.ymax,
                         mesh->bounding_box.zmin, mesh->bounding_box.zmax, projM * modelViewM, bx, Bx, by, By)) { return; }
            
            
            // check if the clipping test should be perfomed for each triangle in the mesh. 
            static const float clipboundXY = (2048 / ((LX > LY) ? LX : LY));            
            const bool cliptestneeded = _clipTestNeeded(clipboundXY, 
                                                        mesh->bounding_box.xmin, mesh->bounding_box.xmax,
                                                        mesh->bounding_box.ymin, mesh->bounding_box.ymax,
                                                        mesh->bounding_box.zmin, mesh->bounding_box.zmax, projM * modelViewM);
                                                        
            // compute normalized light vector in view space
            fVec3 light = _viewM.mult0(_light);
            light = -light; // invert light direction
            light.normalize();

            // inverse of norm of unit vector after matrix mul.  
            const float inorm = 1.0f / modelViewM.mult0(fVec3{ 0,0,1 }).norm();

            const fVec3* tab_vert = mesh->vertice;  // array of vertices
            const fVec3* tab_norm = mesh->normal;   // array of normals
            const fVec2* tab_tex = mesh->texcoord;  // array of texture
            const uint16_t* face = mesh->face;      // array of triangles

            // default rasterization parameters
            typename Image<color_t>::RasterizerParams  uni = { _zbuffer ,  RGBf(1.0f,1.0f,1.0f) , (const tgx::Image<color_t>*)mesh->texture };

            // compute object colors
            const RGBf  ambiantColor = _ambiantColor * (_useMeshDefault ? mesh->ambiant_strength :  _ambiantStrength);
            const RGBf  diffuseColor = _diffuseColor * (_useMeshDefault ? mesh->diffuse_strength : _diffuseStrength);
            const RGBf  specularColor = _specularColor * (_useMeshDefault ? mesh->specular_strength : _specularStrength);
            const RGBf  objectColor = (_useMeshDefault ? mesh->color : _color);
            const float specularExponent = (float)(_useMeshDefault ? mesh->specular_exponent : _specularExponent);
       
            // precompute pow(.,specularExponent) table
            // todo: keep pre-computed table in memory for next object ? would it really help
            // when there are lots of object with the same specular exponent ? 
            static const int LA = 12;
            const float bbsp = (specularExponent < 8.0f) ? specularExponent : 8.0f;
            const float powfact = ((specularExponent * LA) / bbsp);
            float fastpowtab[LA];
            for (int k = 0; k < LA; k++)
                {
                fastpowtab[k] = 0.5f;
                float v = 1.0f - ((bbsp * k) / (specularExponent * LA));
                fastpowtab[k] = powf(v, specularExponent);
                }

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
                PC2->P = modelViewM.mult1(tab_vert[v2]);
                PC0->P = modelViewM.mult1(tab_vert[v0]);
                PC1->P = modelViewM.mult1(tab_vert[v1]);

                // ...but use lazy computation of other vertex attributes
                PC0->missedP = true;
                PC1->missedP = true;
                PC2->missedP = true;

                while (1)
                    {
                    // face culling
                    fVec3 faceN = crossProduct(PC1->P - PC0->P, PC2->P - PC0->P);
                    const float cu = (ORTHO) ? dotProduct(faceN, fVec3(0.0f,0.0f,-1.0f)) : dotProduct(faceN, PC0->P);
                    if ((BACKFACE_CULLING) && (cu > 0)) goto rasterize_next_triangle; // skip triangle !
                    // triangle is not culled 
                  
                    if (cliptestneeded)
                        {
                        // test if clipping is needed                
                        *((fVec4*)PC2) = projM * PC2->P;
                        if (ORTHO) { PC2->w = 2.0f - PC2->z;  } else { PC2->zdivide(); }                        
                        bool needclip = (PC2->P.z >= 0)
                                      | (PC2->x < -clipboundXY) | (PC2->x > clipboundXY)
                                      | (PC2->y < -clipboundXY) | (PC2->y > clipboundXY)
                                      | (PC2->z < -1) | (PC2->z > 1);                        
                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = projM * PC0->P;
                            if (ORTHO) { PC0->w = 2.0f - PC0->z;  } else { PC0->zdivide(); }
                            needclip |= (PC0->P.z >= 0)
                                     | (PC0->x < -clipboundXY) | (PC0->x > clipboundXY)
                                     | (PC0->y < -clipboundXY) | (PC0->y > clipboundXY)
                                     | (PC0->z < -1) | (PC0->z > 1);
                            }
                        if (PC1->missedP)
                            {                            
                            *((fVec4*)PC1) = projM * PC1->P;
                            if (ORTHO) { PC1->w = 2.0f - PC1->z;  } else { PC1->zdivide(); }
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
                        *((fVec4*)PC2) = projM * PC2->P;
                        if (ORTHO) { PC2->w = 2.0f - PC2->z; } else { PC2->zdivide(); }
                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = projM * PC0->P;
                            if (ORTHO) { PC0->w = 2.0f - PC0->z;  } else { PC0->zdivide(); }
                            }
                        if (PC1->missedP)
                            {                            
                            *((fVec4*)PC1) = projM * PC1->P;
                            if (ORTHO) { PC1->w = 2.0f - PC1->z;  } else { PC1->zdivide(); }
                            }
                        }
                                       
                    // ok, the triangle must be rasterized !
                    if (GOURAUD)
                        { // Gouraud shading : color on vertices
                        if (PC0->missedP)
                            {
                            // ambiant component
                            RGBf& col = PC0->color;
                            col = ambiantColor;
                            // diffuse component
                            PC0->N = modelViewM.mult0(tab_norm[PC0->indn]);
                            const float a = max(dotProduct(PC0->N, light) * inorm, 0.0f);
                            col += diffuseColor * a;
                            // specular component: compute normalized halfway vector                        
                            if (_specularExponent > 0)
                                {
                                fVec3 H(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)... 
                                H += light;
                                H.normalize();
                                FASTPOW(b, (dotProduct(PC0->N, H) * inorm));  // pow() this is too slow so we use a lookup table instead
                                col += specularColor * b;
                                }
                            if (!(TEXTURE)) col *= objectColor;
                            col.clamp();
                            }
                        if (PC1->missedP)
                            {
                            // ambiant component
                            RGBf& col = PC1->color;
                            col = ambiantColor;
                            // diffuse component
                            PC1->N = modelViewM.mult0(tab_norm[PC1->indn]);
                            const float a = max(dotProduct(PC1->N, light) * inorm, 0.0f);
                            col += diffuseColor * a;
                            // specular component: compute normalized halfway vector                        
                            if (_specularExponent > 0)
                                {
                                fVec3 H(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)... 
                                H += light;
                                H.normalize();
                                FASTPOW(b, (dotProduct(PC1->N, H) * inorm)); // pow() this is too slow so we use a lookup table instead
                                col += specularColor * b;
                                }
                            if (!(TEXTURE)) col *= objectColor;
                            col.clamp();
                            }
                        // ambiant component
                        RGBf& col = PC2->color;
                        col = ambiantColor;
                        // diffuse component
                        PC2->N = modelViewM.mult0(tab_norm[PC2->indn]);
                        const float a = max(dotProduct(PC2->N, light) * inorm, 0.0f);
                        col += diffuseColor * a;
                        // specular component: compute normalized halfway vector                        
                        if (_specularExponent > 0)
                            {
                            fVec3 H(0, 0, 1); // cheating: should use the normalized current vertex position (but this is faster with almost the same result)... 
                            H += light;
                            H.normalize();                            
                            FASTPOW(b, (dotProduct(PC2->N, H) * inorm)); // pow() this is too slow so we use a lookup table instead
                            col += specularColor * b;
                            }
                        if (!(TEXTURE)) col *= objectColor;
                        col.clamp();
                        }
                    else
                        { // flat shading : color on faces
                        // ambiant component
                        RGBf& col = uni.facecolor;
                        col = ambiantColor;
                        // diffuse component
                        faceN.normalize();
                        const float a = max(dotProduct(faceN, light), 0.0f);  // here we know that |light| = 1                        
                        col += diffuseColor * a;
                        // specular component: compute normalized halfway vector                                  
                        if (_specularExponent > 0)
                            {
                            fVec3 H(0, 0, 1);
                            H += light;
                            H.normalize();
                            FASTPOW(b, (dotProduct(faceN, H))); // pow() this is too slow so we use a lookup table instead
                            col += specularColor * b;
                            }
                        if (!(TEXTURE)) col *= objectColor;
                        col.clamp();
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
                    (*_im).template rasterizeTriangle<LX,LY, ZBUFFER,ORTHO>(RASTER_TYPE,
                        (typename Image<color_t>::RasterizerVec4)QQA,
                        (typename Image<color_t>::RasterizerVec4)QQB,
                        (typename Image<color_t>::RasterizerVec4)QQC,
                        ox, oy, uni);

            rasterize_next_triangle:

                    if (--nbt == 0) break; // exit loop at end of chain

                    // get the next triangle
                    const uint16_t nv2 = *(face++);
                    swap(((nv2 & 32768) ? PC0 : PC1), PC2);
                    if (TEXTURE) PC2->indt = *(face++); else { if (tab_tex) face++; }
                    if (GOURAUD) PC2->indn = *(face++);  else { if (tab_norm) face++; }
                    PC2->P = modelViewM.mult1(tab_vert[nv2 & 32767]);
                    PC2->missedP = true;
                    }
                }
            }


    #undef FASTPOW




        // *** general parameters ***
       
        int     _ox, _oy;           // image offset w.r.t. the viewport
        Image<color_t>* _im;        // image to drawn onto

        fMat4   _projM;             // projection matrix

        float*  _zbuffer;           // the zbuffer
        int     _zbuffer_len;       // size of the zbuffer

        // *** scene parameters ***

        fMat4   _viewM;             // view transform matrix

        fVec3   _light;             // light direction
        RGBf    _ambiantColor;      // light ambiant color
        RGBf    _diffuseColor;      // light diffuse color
        RGBf    _specularColor;     // light specular color

        // *** model specific parameters ***

        fMat4   _modelM;            // model transform matrix

        bool    _useMeshDefault;    // true to use the mesh default color / reflection parameters instead on the ones below
        RGBf    _color;             // model color (use when texturing is disabled)
        float   _ambiantStrength;   // ambient light reflection strength
        float   _diffuseStrength;   // diffuse light reflection strength
        float   _specularStrength;  // specular light reflection strength
        int     _specularExponent;  // specular exponent

    };




}


/** end of file */

