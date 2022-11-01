/** @file Renderer3D.inl */
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
#ifndef _TGX_RENDERER3D_INL_
#define _TGX_RENDERER3D_INL_


/************************************************************************************
*
* Implementation file for the template class Renderer3D<color_t, LOADED_SHADERS>
* 
*************************************************************************************/
namespace tgx
{



    /********************************************************
    * CONSTRUCTOR
    *********************************************************/


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::Renderer3D(const iVec2& viewportSize, Image<color_t> * im, ZBUFFER_t * zbuffer) : _currentpow(-1), _uni(), _culling_dir(1)
            {                        
            _shaders = 0;             
            _ortho = TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS) ? false : true; // default projection is perspective if not disabled)
            
            _uni.tex = nullptr; 
            _uni.shader_type = 0; 
            _uni.zbuf = nullptr; 
            _uni.facecolor = RGBf(1.0f, 1.0f, 1.0f);

            setViewportSize(viewportSize);
            setImage(im);
            setOffset(0, 0); // no offset

            // let's set some default values
            fMat4 M;
            const float ar = _validDraw() ? (((float)_lx) / _ly) : 1.5f;
            if (_ortho)
                M.setOrtho(-10*ar, 10*ar, -10.0f, 10.0f, 1.0f, 100.0f);
            else
                M.setPerspective(45.0f, ar, 1.0f, 100.0f);
                
            this->setProjectionMatrix(M);

            this->setLookAt({ 0,0,0 }, { 0,0,-1 }, { 0,1,0 }); // look toward the negative z axis (id matrix)

            this->setLight(fVec3(-1.0f, -1.0f, -1.0f), // white light coming from the right, above, in front.
                RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f)); // full white.

            M.setIdentity();
            this->setModelMatrix(M); // no transformation on the mesh.

            this->setMaterial({ 0.75f, 0.75f, 0.75f }, 0.15f, 0.7f, 0.5f, 8); // just in case: silver color and some default reflexion param...
            this->_precomputeSpecularTable(8);
            
            setShaders(SHADER_FLAT);
            setTextureWrappingMode(SHADER_TEXTURE_CLAMP); // slow but safer (no need to be power of 2)
            setTextureQuality(SHADER_TEXTURE_NEAREST); // dirty but fast
            setZbuffer(zbuffer);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_precomputeSpecularTable2(int exponent)
            {
            _currentpow = exponent;
            float specularExponent = (float)exponent;
            if (exponent > 0)
                {
                const float MAX_VAL_POW = 10.0f;  //  maximum value that the power can take
                _powmax = pow(MAX_VAL_POW, 1 / specularExponent);
                for (int k = 0; k < _POWTABSIZE; k++)
                    {
                    float v = 1.0f - (((float)k) / _POWTABSIZE);
                    _fastpowtab[k] = powf(_powmax * v, specularExponent);
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






    /********************************************************
     * GLOBAL METHODS
     ********************************************************/



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setViewportSize(int lx, int ly)
            {
            _lx = clamp(lx, 0, MAXVIEWPORTDIMENSION);
            _ly = clamp(ly, 0, MAXVIEWPORTDIMENSION);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setViewportSize(const iVec2& viewport_dim)
            {
            setViewportSize(viewport_dim.x, viewport_dim.y);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setImage(Image<color_t>* im)
            {
            _uni.im = im;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setOffset(int ox, int oy)
            {
            _ox = clamp(ox, 0, MAXVIEWPORTDIMENSION);
            _oy = clamp(oy, 0, MAXVIEWPORTDIMENSION);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setOffset(const iVec2& offset)
            {
            this->setOffset(offset.x, offset.y);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setProjectionMatrix(const fMat4& M)
            {
            _projM = M;
            _projM.invertYaxis();
            _recompute_wa_wb();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        fMat4 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::getProjectionMatrix() const
            {
            fMat4 M = _projM;
            M.invertYaxis();
            return M;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::useOrthographicProjection()
            {
            static_assert(TGX_SHADER_HAS_ORTHO(ENABLED_SHADERS), "shader TGX_SHADER_ORTHO must be enabled to use useOrthographicProjection()");
            _ortho = true;
            _rectifyShaderOrtho();
            _recompute_wa_wb();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::usePerspectiveProjection()
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use usePerspectiveProjection()");
            _ortho = false;
            _rectifyShaderOrtho();
            _recompute_wa_wb();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setOrtho(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_ORTHO(ENABLED_SHADERS), "shader TGX_SHADER_ORTHO must be enabled to use setOrtho()");
            _projM.setOrtho(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
            useOrthographicProjection();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setFrustum(float left, float right, float bottom, float top, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use setFrustrum()");
            _projM.setFrustum(left, right, bottom, top, zNear, zFar);
            _projM.invertYaxis();
            usePerspectiveProjection();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setPerspective(float fovy, float aspect, float zNear, float zFar)
            {
            static_assert(TGX_SHADER_HAS_PERSPECTIVE(ENABLED_SHADERS), "shader TGX_SHADER_PERSPECTIVE must be enabled to use setPerspective()");
            _projM.setPerspective(fovy, aspect, zNear, zFar);
            _projM.invertYaxis();
            usePerspectiveProjection();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setCulling(int w)
            {
            _culling_dir = (w > 0) ? 1.0f : ((w < 0) ? -1.0f : 0.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setZbuffer(ZBUFFER_t* zbuffer)
            {
            static_assert(TGX_SHADER_HAS_ZBUFFER(ENABLED_SHADERS), "shader TGX_SHADER_ZBUFFER must be enabled to use setZbuffer()");
            _uni.zbuf = zbuffer;
            _rectifyShaderZbuffer();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::clearZbuffer()
            {
            static_assert(TGX_SHADER_HAS_ZBUFFER(ENABLED_SHADERS), "shader TGX_SHADER_ZBUFFER must be enabled to use clearZbuffer()");
            if ((_uni.zbuf) && (_uni.im != nullptr) && (_uni.im->isValid()))
                {
                memset(_uni.zbuf, 0, _uni.im->lx() * _uni.im->ly() * sizeof(ZBUFFER_t));
                }
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setShaders(Shader shaders)
            {
            _rectifyShaderShading(shaders);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setTextureWrappingMode(Shader wrap_mode)
            {
            if (TGX_SHADER_HAS_TEXTURE_CLAMP(wrap_mode))
                {
                if (TGX_SHADER_HAS_TEXTURE_CLAMP(ENABLED_SHADERS))
                    _texture_wrap_mode = SHADER_TEXTURE_CLAMP;
                else
                    _texture_wrap_mode = SHADER_TEXTURE_WRAP_POW2; // fallback
                } else
                {
                if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(ENABLED_SHADERS))
                    _texture_wrap_mode = SHADER_TEXTURE_WRAP_POW2;
                else
                    _texture_wrap_mode = SHADER_TEXTURE_CLAMP; // fallback
                }
                _rectifyShaderTextureWrapping();
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setTextureQuality(Shader quality)
            {
            if (TGX_SHADER_HAS_TEXTURE_BILINEAR(quality))
                {
                if (TGX_SHADER_HAS_TEXTURE_BILINEAR(ENABLED_SHADERS))
                    _texture_quality = SHADER_TEXTURE_BILINEAR;
                else
                    _texture_quality = SHADER_TEXTURE_NEAREST; // fallback
                } else
                {
                if (TGX_SHADER_HAS_TEXTURE_NEAREST(ENABLED_SHADERS))
                    _texture_quality = SHADER_TEXTURE_NEAREST;
                else
                    _texture_quality = SHADER_TEXTURE_BILINEAR; // fallback
                }
                _rectifyShaderTextureQuality();
            }



    /********************************************************
     * SCENE RELATED METHODS
     ********************************************************/


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setViewMatrix(const fMat4& M)
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


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        fMat4 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::getViewMatrix() const
            {
            return _viewM;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ)
            {
            fMat4 M;
            M.setLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
            setViewMatrix(M);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLookAt(const fVec3 eye, const fVec3 center, const fVec3 up)
            {
            setLookAt(eye.x, eye.y, eye.z, center.x, center.y, center.z, up.x, up.y, up.z);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        fVec4 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::worldToNDC(fVec3 P)
            {
            fVec4 Q = _projM * _viewM.mult1(P);
            if (!_ortho) Q.zdivide();
            return Q;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        iVec2 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::worldToImage(fVec3 P)
            {
            fVec4 Q = _projM * _viewM.mult1(P);
            if (!_ortho) Q.zdivide();
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
            return iVec2((int)roundfp(Q.x), (int)roundfp(Q.y));
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLightDirection(const fVec3 & direction)
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


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLightAmbiant(const RGBf & color)
            {
            _ambiantColor = color;
            // recompute
            _r_ambiantColor = _ambiantColor * _ambiantStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLightDiffuse(const RGBf & color)
            {
            _diffuseColor = color;
            // recompute
            _r_diffuseColor = _diffuseColor * _diffuseStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLightSpecular(const RGBf & color)
            {
            _specularColor = color;
            // recompute
            _r_specularColor = _specularColor * _specularStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setLight(const fVec3 direction, const RGBf & ambiantColor, const RGBf & diffuseColor, const RGBf & specularColor)
            {
            this->setLightDirection(direction);
            this->setLightAmbiant(ambiantColor);
            this->setLightDiffuse(diffuseColor);
            this->setLightSpecular(specularColor);
            }






    /********************************************************
     * MODEL RELATED METHODS
     ********************************************************/


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setModelMatrix(const fMat4& M)
            {
            _modelM = M;
            // recompute
            _r_modelViewM = _viewM * _modelM;
            _r_inorm = _r_modelViewM.mult0(fVec3{ 0,0,1 }).invnorm();
            _r_light_inorm = _r_light * _r_inorm;
            _r_H_inorm = _r_H * _r_inorm;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        fMat4  Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::getModelMatrix() const
            {
            return _modelM;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setModelPosScaleRot(const fVec3& center, const fVec3& scale, float rot_angle, const fVec3& rot_dir)
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


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        fVec4 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::modelToNDC(fVec3 P)
            {
            fVec4 Q = _projM * _r_modelViewM.mult1(P);
            if (!_ortho) Q.zdivide();
            return Q;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        iVec2 Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::modelToImage(fVec3 P)
            {
            fVec4 Q = _projM * _r_modelViewM.mult1(P);
            if (!_ortho) Q.zdivide();
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
            return iVec2(roundfp(Q.x), roundfp(Q.y));
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterialColor(RGBf color)
            {
            _color = color;
            // recompute
            _r_objectColor = _color;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterialAmbiantStrength(float strenght)
            {
            _ambiantStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_ambiantColor = _ambiantColor * _ambiantStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterialDiffuseStrength(float strenght)
            {
            _diffuseStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_diffuseColor = _diffuseColor * _diffuseStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterialSpecularStrength(float strenght)
            {
            _specularStrength = clamp(strenght, 0.0f, 10.0f); // allow values larger than 1 to simulate emissive surfaces.
            // recompute
            _r_specularColor = _specularColor * _specularStrength;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterialSpecularExponent(int exponent)
            {
            _specularExponent = clamp(exponent, 0, 100);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::setMaterial(RGBf color, float ambiantStrength, float diffuseStrength, float specularStrength, int specularExponent)
            {
            this->setMaterialColor(color);
            this->setMaterialAmbiantStrength(ambiantStrength);
            this->setMaterialDiffuseStrength(diffuseStrength);
            this->setMaterialSpecularStrength(specularStrength);
            this->setMaterialSpecularExponent(specularExponent);
            }












        /********************************************************
        * TRIANGLE CLIPPING
        *********************************************************/


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_triangleClip1in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4)
            {
            const float f12 = _cpfactor(CP, cp1, cp2);
            const float f13 = _cpfactor(CP, cp1, cp3);
            fVec4& U1 = *((fVec4*)&P1);
            fVec4& U2 = *((fVec4*)&P2);
            fVec4& U3 = *((fVec4*)&P3);
            fVec4& nU1 = *((fVec4*)&nP1);
            fVec4& nU2 = *((fVec4*)&nP2);
            fVec4& nU3 = *((fVec4*)&nP3);

            nU1 = U1;
            nU2 = U1 + f12 * (U2 - U1);
            nU3 = U1 + f13 * (U3 - U1);

            nP1.color = P1.color;
            nP2.color = interpolate(P1.color, P2.color, f12);
            nP3.color = interpolate(P1.color, P3.color, f13);

            if (TGX_SHADER_HAS_TEXTURE(shader))
                {
                nP1.T = P1.T;
                nP2.T = P1.T + f12 * (P2.T - P1.T);
                nP3.T = P1.T + f13 * (P3.T - P1.T);
                }
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_triangleClip2in(int shader, tgx::fVec4 CP,
            float cp1, float cp2, float cp3,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4)
            {
            const float f23 = _cpfactor(CP, cp2, cp3);
            const float f13 = _cpfactor(CP, cp1, cp3);
            fVec4& U1 = *((fVec4*)&P1);
            fVec4& U2 = *((fVec4*)&P2);
            fVec4& U3 = *((fVec4*)&P3);
            fVec4& nU1 = *((fVec4*)&nP1);
            fVec4& nU2 = *((fVec4*)&nP2);
            fVec4& nU3 = *((fVec4*)&nP3);
            fVec4& nU4 = *((fVec4*)&nP4);

            nU1 = U1;
            nU2 = U2;
            nU3 = U2 + f23 * (U3 - U2);
            nU4 = U1 + f13 * (U3 - U1);

            nP1.color = P1.color;
            nP2.color = P2.color;
            nP3.color = interpolate(P2.color, P3.color, f23);
            nP4.color = interpolate(P1.color, P3.color, f13);

            if (TGX_SHADER_HAS_TEXTURE(shader))
                {
                nP1.T = P1.T;
                nP2.T = P2.T;
                nP3.T = P2.T + f23 * (P3.T - P2.T);
                nP4.T = P1.T + f13 * (P3.T - P1.T);
                }
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        int Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_triangleClip(int shader, tgx::fVec4 CP, float off,
            const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3,
            RasterizerVec4& nP1, RasterizerVec4& nP2, RasterizerVec4& nP3, RasterizerVec4& nP4)
            {
            const float cp1 = _cpdist(CP, off, P1);
            const float cp2 = _cpdist(CP, off, P2);
            const float cp3 = _cpdist(CP, off, P3);
            if ((cp1 >= 0) && (cp2 >= 0) && (cp3 >= 0))
                { // triangle fully inside so it can be drawn directly
                return 0;
                }
            // at least one vertex if outside the clip plane
            if (cp1 >= 0)
                {
                if (cp2 >= 0)
                    {
                    _triangleClip2in(shader, CP, cp1, cp2, cp3, P1, P2, P3, nP1, nP2, nP3, nP4);
                    return 2;
                    }
                else if (cp3 >= 0)
                    {
                    _triangleClip2in(shader, CP, cp3, cp1, cp2, P3, P1, P2, nP1, nP2, nP3, nP4);
                    return 2;
                    }
                _triangleClip1in(shader, CP, cp1, cp2, cp3, P1, P2, P3, nP1, nP2, nP3, nP4);
                return 1;
                }
            if (cp2 >= 0)
                {
                if (cp3 >= 0)
                    {
                    _triangleClip2in(shader, CP, cp2, cp3, cp1, P2, P3, P1, nP1, nP2, nP3, nP4);
                    return 2;
                    }
                _triangleClip1in(shader, CP, cp2, cp3, cp1, P2, P3, P1, nP1, nP2, nP3, nP4);
                return 1;
                }

            if (cp3 >= 0)
                {
                _triangleClip1in(shader, CP, cp3, cp1, cp2, P3, P1, P2, nP1, nP2, nP3, nP4);
                return 1;
                }
            // nothing to draw !
            return -1;
            }



    /********************************************************
    * DRAWING TRIANGLE / QUADS / MESHES
    *********************************************************/



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawTriangleClipped(const int RASTER_TYPE,
            const fVec4* Q0, const fVec4* Q1, const fVec4* Q2,
            const fVec3* N0, const fVec3* N1, const fVec3* N2,
            const fVec2* T0, const fVec2* T1, const fVec2* T2,
            const RGBf& col0, const RGBf& col1, const RGBf& col2)
            {

            _uni.shader_type = RASTER_TYPE; // just in case

            // face culling (unneeded but we must compute cu anyway). 
            fVec3 faceN = crossProduct(*Q1 - *Q0, *Q2 - *Q0);
            const float cu = (_ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, *Q0);
            if (cu * _culling_dir > 0) return; // skip triangle !
            
            RasterizerVec4 PC0,PC1,PC2;

            *((fVec4*)&PC0) = _projM * (*Q0);
            *((fVec4*)&PC1) = _projM * (*Q1);
            *((fVec4*)&PC2) = _projM * (*Q2);


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
                    PC0.color = _phong(icu * dotProduct(NN0, _r_light_inorm), icu * dotProduct(NN0, _r_H_inorm), col0);
                    PC1.color = _phong(icu * dotProduct(NN1, _r_light_inorm), icu * dotProduct(NN1, _r_H_inorm), col1);
                    PC2.color = _phong(icu * dotProduct(NN2, _r_light_inorm), icu * dotProduct(NN2, _r_H_inorm), col2);
                    }
                }
            else
                { // flat shading
                const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
                faceN.normalize_fast();
                if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                    {
                    _uni.facecolor = _phong<true>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                else
                    {
                    _uni.facecolor = _phong<false>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                    }
                PC0.color = _uni.facecolor; // unneeded but
                PC1.color = _uni.facecolor; // does no harm
                PC2.color = _uni.facecolor; // and remove a warning
                }

            if (TGX_SHADER_HAS_TEXTURE(RASTER_TYPE))
                { // store texture vectors if needed
                PC0.T = *T0;
                PC1.T = *T1;
                PC2.T = *T2;
                }

            // ok, now PC0, PC1 and PC2 contain the points in NDC coord with associated color and texture indices, let's go (recursively) !
            _drawTriangleClippedSub(RASTER_TYPE, 0, PC0, PC1, PC2); 
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawTriangleClippedSub(const int RASTER_TYPE, const int plane, const RasterizerVec4& P1, const RasterizerVec4& P2, const RasterizerVec4& P3)
            {
                    
            const float CLIPBOUND_XY = _clipbound_xy();

            fVec4 CP; 
            float off;
            RasterizerVec4 V1, V2, V3, V4;     
            const bool ortho = _ortho;
            switch (plane)
                {                
                case 0: // left plane
                    if (ortho)
                        { // X + 1 >= 0
                        CP = fVec4(1, 0, 0, 0);
                        off = 1;
                        }
                    else
                        { // X  + CLIPBOUND_XY W >= 0
                        CP = fVec4(1, 0, 0, CLIPBOUND_XY);
                        off = 0;
                        }
                    break;
                
                case 1: // right plane
                    if (ortho)
                        { // -X + 1 >= 0
                        CP = fVec4(-1, 0, 0, 0);
                        off = 1;
                        }
                    else
                        { // -X + CLIPBOUND_XY W >= 0
                        CP = fVec4(-1, 0, 0, CLIPBOUND_XY);
                        off = 0;
                        }
                    break;
                
                case 2: // bottom plane
                    if (ortho)
                        { // Y + 1 >= 0
                        CP = fVec4(0, 1, 0, 0);
                        off = 1;
                        }
                    else
                        { // Y + CLIPBOUND_XY W >= 0
                        CP = fVec4(0, 1, 0, CLIPBOUND_XY);
                        off = 0;
                        }
                    break;
                
                case 3: // top plane
                    if (ortho)
                        { // -Y + 1 >= 0
                        CP = fVec4(0, -1, 0, 0);
                        off = 1;
                        }
                    else
                        { // -Y + CLIPBOUND_XY W >= 0
                        CP = fVec4(0, -1, 0, CLIPBOUND_XY);
                        off = 0;
                        }
                    break;
                
                case 4: // near plane 
                    if (ortho)
                        { // Z + 1 >= 0
                        CP = fVec4(0, 0, 1, 0);
                        off = 1;
                        }
                    else
                        { // Z + W >= 0
                        CP = fVec4(0, 0, 1, 1);
                        off = 0;
                        }
                    break;
                /*
                case 5: // far plane
                    if (ortho)
                        { // -Z + 1 >= 0
                        CP = fVec4(0, 0, -1, 0);
                        off = 1;
                        }
                    else
                        { // -Z + W >= 0
                        CP = fVec4(0, 0, -1, 1);
                        off = 0;
                        }
                    break;
                */

                default:
                    {  
                    V1 = P1; 
                    V2 = P2; 
                    V3 = P3;
                    // Triangle is now correctly clipped so it can be drawn directly !
                    if (ortho)
                        {
                        V1.w = 1.0f - V1.z;
                        V2.w = 1.0f - V2.z;
                        V3.w = 1.0f - V3.z;
                        }
                    else
                        {
                        V1.zdivide();
                        V2.zdivide();
                        V3.zdivide();
                        }
                    rasterizeTriangle(_lx, _ly, V1, V2, V3, _ox, _oy, _uni, shader_select<ENABLED_SHADERS, color_t, ZBUFFER_t>);
                    return;
                    }
                }
            const int nbt = _triangleClip(RASTER_TYPE, CP, off, P1, P2, P3, V1, V2, V3, V4);
            switch (nbt)
                {
                case 0:
                    _drawTriangleClippedSub(RASTER_TYPE, plane + 1, P1, P2, P3);
                    break;
                case 2: 
                    _drawTriangleClippedSub(RASTER_TYPE, plane + 1, V1, V3, V4);
                case 1:
                    _drawTriangleClippedSub(RASTER_TYPE, plane + 1, V1, V2, V3);
                }
            return;
            }





        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawTriangle(const int RASTER_TYPE,
                           const fVec3 * P0, const fVec3 * P1, const fVec3 * P2,
                           const fVec3 * N0, const fVec3 * N1, const fVec3 * N2,
                           const fVec2 * T0, const fVec2 * T1, const fVec2 * T2,
                           const RGBf & Vcol0, const RGBf & Vcol1, const RGBf & Vcol2)
            {
            const bool ortho = _ortho;
            _uni.shader_type = RASTER_TYPE;

            // compute position in view space.
            const fVec4 Q0 = _r_modelViewM.mult1(*P0);
            const fVec4 Q1 = _r_modelViewM.mult1(*P1);
            const fVec4 Q2 = _r_modelViewM.mult1(*P2);
            
            // face culling
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // skip triangle !

            RasterizerVec4 PC0, PC1, PC2;

            // test if clipping is needed
            (*((fVec4*)&PC0)) = _projM * Q0;
            (*((fVec4*)&PC1)) = _projM * Q1;
            (*((fVec4*)&PC2)) = _projM * Q2;

            if (ortho) 
                { 
                PC0.w = 1.0f - PC0.z; 
                PC1.w = 1.0f - PC1.z;
                PC2.w = 1.0f - PC2.z;
                } 
            else 
                { 
                PC0.zdivide(); 
                PC1.zdivide();
                PC2.zdivide();
                }

            const float CLIPBOUND_XY = _clipbound_xy();

            bool needclip = (PC0.x < -CLIPBOUND_XY) | (PC0.x > CLIPBOUND_XY)
                          | (PC0.y < -CLIPBOUND_XY) | (PC0.y > CLIPBOUND_XY)
                          | (PC0.z < -1) | (PC0.z > 1) | (PC0.w <= 0)                    
                          | (PC1.x < -CLIPBOUND_XY) | (PC1.x > CLIPBOUND_XY)
                          | (PC1.y < -CLIPBOUND_XY) | (PC1.y > CLIPBOUND_XY)
                          | (PC1.z < -1) | (PC1.z > 1) | (PC1.w <= 0)           
                          | (PC2.x < -CLIPBOUND_XY) | (PC2.x > CLIPBOUND_XY)
                          | (PC2.y < -CLIPBOUND_XY) | (PC2.y > CLIPBOUND_XY)
                          | (PC2.z < -1) | (PC2.z > 1) | (PC2.w <= 0);

            if (needclip)
                {// test if we can discard the triangle immediately
                if (!_discardTriangle(PC0, PC1, PC2))
                    { // no, so we use the slow version where we perform clipping.
                    _drawTriangleClipped(RASTER_TYPE, 
                                         &Q0, &Q1, &Q2, // vertices are sent after modelview mult.
                                         N0, N1, N2,
                                         T0, T1, T2,
                                         Vcol0, Vcol1, Vcol2);
                    }
                return;
                }

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
                faceN.normalize_fast();
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
            rasterizeTriangle(_lx, _ly, PC0, PC1, PC2, _ox, _oy, _uni, shader_select<ENABLED_SHADERS, color_t, ZBUFFER_t>);

            return;
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawQuad(const int RASTER_TYPE,
            const fVec3* P0, const fVec3* P1, const fVec3* P2, const fVec3* P3,
            const fVec3* N0, const fVec3* N1, const fVec3* N2, const fVec3* N3,
            const fVec2* T0, const fVec2* T1, const fVec2* T2, const fVec2* T3,
            const RGBf & Vcol0, const RGBf & Vcol1, const RGBf & Vcol2,  const RGBf & Vcol3)
            {
            const bool ortho = _ortho; 

            _uni.shader_type = RASTER_TYPE;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(*P0);
            const fVec4 Q1 = _r_modelViewM.mult1(*P1);
            const fVec4 Q2 = _r_modelViewM.mult1(*P2);
         
            // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

            const fVec4 Q3 = _r_modelViewM.mult1(*P3); // compute fourth point

            RasterizerVec4 PC0, PC1, PC2, PC3;

            // test if clipping is needed
            (*((fVec4*)&PC0)) = _projM * Q0;
            (*((fVec4*)&PC1)) = _projM * Q1;
            (*((fVec4*)&PC2)) = _projM * Q2;
            (*((fVec4*)&PC3)) = _projM * Q3;

            if (ortho)
                { 
                PC0.w = 1.0f - PC0.z; 
                PC1.w = 1.0f - PC1.z;
                PC2.w = 1.0f - PC2.z;
                PC3.w = 1.0f - PC3.z;
                } 
            else 
                { 
                PC0.zdivide(); 
                PC1.zdivide();
                PC2.zdivide();
                PC3.zdivide();
                }

            const float CLIPBOUND_XY = _clipbound_xy();

            bool needclip = (PC0.x < -CLIPBOUND_XY) | (PC0.x > CLIPBOUND_XY)
                     | (PC0.y < -CLIPBOUND_XY) | (PC0.y > CLIPBOUND_XY)
                     | (PC0.z < -1) | (PC0.z > 1) | (PC0.w <= 0)
                     | (PC1.x < -CLIPBOUND_XY) | (PC1.x > CLIPBOUND_XY)
                     | (PC1.y < -CLIPBOUND_XY) | (PC1.y > CLIPBOUND_XY)
                     | (PC1.z < -1) | (PC1.z > 1) | (PC1.w <= 0)
                     | (PC2.x < -CLIPBOUND_XY) | (PC2.x > CLIPBOUND_XY)
                     | (PC2.y < -CLIPBOUND_XY) | (PC2.y > CLIPBOUND_XY)
                     | (PC2.z < -1) | (PC2.z > 1) | (PC2.w <= 0)
                     | (PC3.x < -CLIPBOUND_XY) | (PC3.x > CLIPBOUND_XY)
                     | (PC3.y < -CLIPBOUND_XY) | (PC3.y > CLIPBOUND_XY)
                     | (PC3.z < -1) | (PC3.z > 1) | (PC3.w <= 0);

            if (needclip)
                {// test if we can discard some triangles of the quad immediately
                if (!_discardTriangle(PC0, PC1, PC2))
                    {// slow version where we perform clipping.
                    _drawTriangleClipped(RASTER_TYPE, 
                                         &Q0, &Q1, &Q2, // vertices are sent after modelview mult.
                                         N0, N1, N2,
                                         T0, T1, T2,
                                         Vcol0, Vcol1, Vcol2);
                    }
                if (!_discardTriangle(PC0, PC2, PC3))
                    {// slow version where we perform clipping.
                    _drawTriangleClipped(RASTER_TYPE, 
                                         &Q0, &Q2, &Q3, // vertices are sent after modelview mult.
                                         N0, N2, N3,
                                         T0, T2, T3,
                                         Vcol0, Vcol2, Vcol3);
                    }                
                return;
                }

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
                faceN.normalize_fast();
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
            rasterizeTriangle(_lx, _ly, PC0, PC1, PC2, _ox, _oy, _uni, shader_select<ENABLED_SHADERS, color_t, ZBUFFER_t>);
            rasterizeTriangle(_lx, _ly, PC0, PC2, PC3, _ox, _oy, _uni, shader_select<ENABLED_SHADERS, color_t, ZBUFFER_t>);
            
            return;
            }





        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawMesh(const Mesh3D<color_t>* mesh, bool use_mesh_material, bool draw_chained_meshes)
            {
            if (!_validDraw()) return;

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
                    int raster_type = _shaders;
                    if (mesh->normal == nullptr) { TGX_SHADER_REMOVE_GOURAUD(raster_type) } // gouraud shading not available so we disable it
                    if ((mesh->texcoord == nullptr) || (mesh->texture == nullptr)) TGX_SHADER_REMOVE_TEXTURE(raster_type) // texturing not available so we disable it
                    _drawMesh(raster_type, mesh);
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
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>  TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawMesh(const int RASTER_TYPE, const Mesh3D<color_t>* mesh)
            {
            _uni.shader_type = RASTER_TYPE;
            const bool ortho = _ortho;

            const bool TEXTURE = (bool)(TGX_SHADER_HAS_TEXTURE(RASTER_TYPE));
            const bool GOURAUD = (bool)(TGX_SHADER_HAS_GOURAUD(RASTER_TYPE));

            // check if the object is completely outside of the image for fast discard.
            if (_discardBox(mesh->bounding_box, _projM * _r_modelViewM)) return;
            
            const float CLIPBOUND_XY = _clipbound_xy();

            // check if the clipping test should be performed for each triangle in the mesh.
            const bool cliptestneeded = _clipTestNeeded(CLIPBOUND_XY, mesh->bounding_box, _projM * _r_modelViewM);

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
                    const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, PC0->P);
                    if (cu * _culling_dir > 0) goto rasterize_next_triangle; // skip triangle !
                    // triangle is not culled

                    *((fVec4*)PC2) = _projM * PC2->P;
                    if (ortho) { PC2->w = 1.0f - PC2->z; } else { PC2->zdivide(); }

                    if (PC0->missedP)
                        {
                        *((fVec4*)PC0) = _projM * PC0->P;
                        if (ortho) { PC0->w = 1.0f - PC0->z; } else { PC0->zdivide(); }
                        }
                    if (PC1->missedP)
                        {
                        *((fVec4*)PC1) = _projM * PC1->P;
                        if (ortho) { PC1->w = 1.0f - PC1->z; } else { PC1->zdivide(); }
                        }
                        
                    // test if triangle must be clipped                                         
                    if (cliptestneeded)
                        {
                        const float CLIPBOUND_XY = _clipbound_xy();
                        const bool needclip = (PC2->P.z >= 0)
                            | (PC2->x < -CLIPBOUND_XY) | (PC2->x > CLIPBOUND_XY)
                            | (PC2->y < -CLIPBOUND_XY) | (PC2->y > CLIPBOUND_XY)
                            | (PC2->z < -1) | (PC2->z > 1)                      
                            | (PC0->P.z >= 0)
                            | (PC0->x < -CLIPBOUND_XY) | (PC0->x > CLIPBOUND_XY)
                            | (PC0->y < -CLIPBOUND_XY) | (PC0->y > CLIPBOUND_XY)
                            | (PC0->z < -1) | (PC0->z > 1)
                            | (PC1->P.z >= 0)
                            | (PC1->x < -CLIPBOUND_XY) | (PC1->x > CLIPBOUND_XY)
                            | (PC1->y < -CLIPBOUND_XY) | (PC1->y > CLIPBOUND_XY)
                            | (PC1->z < -1) | (PC1->z > 1);
                        if (needclip)
                            { // need cliiping, test is we can just discard the triangle if not shown on screen
                            if (!_discardTriangle(*((fVec4*)PC0), *((fVec4*)PC1), *((fVec4*)PC2)))
                                { // no, use the slow drawing method with clipping
                                _drawTriangleClipped(RASTER_TYPE,
                                                &(PC0->P), &(PC1->P), &(PC2->P),
                                                ((GOURAUD) ? tab_norm + PC0->indn : nullptr), ((GOURAUD) ? tab_norm + PC1->indn : nullptr), ((GOURAUD) ? tab_norm + PC2->indn : nullptr),                                
                                                ((TEXTURE) ? tab_tex + PC0->indt : nullptr), ((TEXTURE) ? tab_tex + PC1->indt : nullptr), ((TEXTURE) ? tab_tex + PC2->indt : nullptr),
                                                _uni.facecolor, _uni.facecolor, _uni.facecolor);
                                }
                            goto rasterize_next_triangle;
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
                            if (TEXTURE)
                                PC0->color = _phong<true>(icu * dotProduct(PC0->N, _r_light_inorm), icu * dotProduct(PC0->N, _r_H_inorm));
                            else
                                PC0->color = _phong<false>(icu * dotProduct(PC0->N, _r_light_inorm), icu * dotProduct(PC0->N, _r_H_inorm));
                            }
                        if (PC1->missedP)
                            {
                            PC1->N = _r_modelViewM.mult0(tab_norm[PC1->indn]);
                            if (TEXTURE)
                                PC1->color = _phong<true>(icu * dotProduct(PC1->N, _r_light_inorm), icu * dotProduct(PC1->N, _r_H_inorm));
                            else
                                PC1->color = _phong<false>(icu * dotProduct(PC1->N, _r_light_inorm), icu * dotProduct(PC1->N, _r_H_inorm));
                            }
                        PC2->N = _r_modelViewM.mult0(tab_norm[PC2->indn]);
                        if (TEXTURE)
                            PC2->color = _phong<true>(icu * dotProduct(PC2->N, _r_light_inorm), icu * dotProduct(PC2->N, _r_H_inorm));
                        else
                            PC2->color = _phong<false>(icu * dotProduct(PC2->N, _r_light_inorm), icu * dotProduct(PC2->N, _r_H_inorm));

                        }
                    else
                        { // flat shading : color on faces
                        const float icu = ((cu > 0) ? -1.0f : 1.0f); // -1 if we need to reverse the face normal.
                        faceN.normalize_fast();
                        if (TEXTURE)
                            _uni.facecolor = _phong<true>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
                        else
                            _uni.facecolor = _phong<false>(icu * dotProduct(faceN, _r_light), icu * dotProduct(faceN, _r_H));
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
                    rasterizeTriangle(_lx, _ly, (RasterizerVec4)QQA, (RasterizerVec4)QQB, (RasterizerVec4)QQC, _ox, _oy, _uni, shader_select<ENABLED_SHADERS, color_t, ZBUFFER_t>);
              
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


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3,
                const fVec3* N1, const fVec3* N2, const fVec3* N3,
                const fVec2* T1, const fVec2* T2, const fVec2* T3,
                const Image<color_t>* texture)
                {
                if (!_validDraw()) return;
                int shader = _shaders;
                if ((N1 == nullptr) || (N2 == nullptr) || (N3 == nullptr)) { TGX_SHADER_REMOVE_GOURAUD(shader) }
                if ((T1 == nullptr) || (T2 == nullptr) || (T3 == nullptr) || (texture == nullptr)) { TGX_SHADER_REMOVE_TEXTURE(shader) }
                _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
                _uni.tex = (const Image<color_t>*)texture;
                _drawTriangle(shader, &P1, &P2, &P3, N1, N2, N3, T1, T2, T3, _r_objectColor, _r_objectColor, _r_objectColor);
                }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawTriangleWithVertexColor(const fVec3& P1, const fVec3& P2, const fVec3& P3,
                const RGBf& col1, const RGBf& col2, const RGBf& col3,
                const fVec3* N1, const fVec3* N2, const fVec3* N3)
                {
                if (!_validDraw()) return;
                int shader = _shaders;
                TGX_SHADER_REMOVE_TEXTURE(shader)
                if ((N1 == nullptr) || (N2 == nullptr) || (N3 == nullptr)) { TGX_SHADER_REMOVE_GOURAUD(shader) }
                _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed            
                if (TGX_SHADER_HAS_GOURAUD(shader))
                    {
                    _drawTriangle(shader, &P1, &P2, &P3, N1, N2, N3, nullptr, nullptr, nullptr, col1, col2, col3);
                    }
                else
                    {
                    fVec3 N = crossProduct(P2 - P1, P3 - P1);// compute the triangle face normal
                    N.normalize_fast(); // normalize it
                    _drawTriangle(shader, &P1, &P2, &P3, &N, &N, &N, nullptr, nullptr, nullptr, col1, col2, col3); // call gouraud shader with the same normal for all 3 vertices
                    }
                }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawTriangles(int nb_triangles,
            const uint16_t* ind_vertices, const fVec3* vertices,
            const uint16_t* ind_normals, const fVec3* normals,
            const uint16_t* ind_texture, const fVec2* textures,
            const Image<color_t>* texture_image)
            {
            if (!_validDraw()) return;
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return; // invalid vertices

            int shader = _shaders;
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
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4,
            const fVec3* N1, const fVec3* N2, const fVec3* N3, const fVec3* N4,
            const fVec2* T1, const fVec2* T2, const fVec2* T3, const fVec2* T4,
            const Image<color_t>* texture)
            {
            if (!_validDraw()) return;
            int shader = _shaders;
            if ((N1 == nullptr) || (N2 == nullptr) || (N3 == nullptr) || (N4 == nullptr)) { TGX_SHADER_REMOVE_GOURAUD(shader) }
            if ((T1 == nullptr) || (T2 == nullptr) || (T3 == nullptr) || (T4 == nullptr) || (texture == nullptr)) { TGX_SHADER_REMOVE_TEXTURE(shader) }
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed
            _uni.tex = (const Image<color_t>*)texture;
            _drawQuad(shader, &P1, &P2, &P3, &P4, N1, N2, N3, N4, T1, T2, T3, T4, _r_objectColor, _r_objectColor, _r_objectColor, _r_objectColor);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawQuadWithVertexColor(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4,
            const RGBf& col1, const RGBf& col2, const RGBf& col3, const RGBf& col4,
            const fVec3* N1, const fVec3* N2, const fVec3* N3, const fVec3* N4)
            {
            if (!_validDraw()) return;
            int shader = _shaders;
            TGX_SHADER_REMOVE_TEXTURE(shader)
                if ((N1 == nullptr) || (N2 == nullptr) || (N3 == nullptr) || (N4 == nullptr)) { TGX_SHADER_REMOVE_GOURAUD(shader) }
            _precomputeSpecularTable(_specularExponent); // precomputed pow(.specularexpo) if needed            
            if (TGX_SHADER_HAS_GOURAUD(shader))
                {
                _drawQuad(shader, &P1, &P2, &P3, &P4, N1, N2, N3, N4, nullptr, nullptr, nullptr, nullptr, col1, col2, col3, col4);
                }
            else
                {
                fVec3 N = crossProduct(P2 - P1, P3 - P1);// compute the triangle face normal
                N.normalize_fast(); // normalize it
                _drawQuad(shader, &P1, &P2, &P3, &P4, &N, &N, &N, &N, nullptr, nullptr, nullptr, nullptr, col1, col2, col3, col4); // call gouraud shader with the same normal for all 3 vertices
                }
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawQuads(int nb_quads,
            const uint16_t* ind_vertices, const fVec3* vertices,
            const uint16_t* ind_normals, const fVec3* normals,
            const uint16_t* ind_texture, const fVec2* textures,
            const Image<color_t>* texture_image)
            {
            if (!_validDraw()) return;
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return; // invalid vertices

            int shader = _shaders;
            if ((ind_normals == nullptr) || (normals == nullptr)) TGX_SHADER_REMOVE_GOURAUD(shader) // disable gouraud            
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
            }




    /********************************************************
    * WIREFRAME DRAWING
    *********************************************************/




        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes)
            {
            _drawWireFrameMesh<true>(mesh, draw_chained_meshes, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, float thickness, color_t color, float opacity)
            {
            _drawWireFrameMesh<false>(mesh, draw_chained_meshes, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameLine(const fVec3& P1, const fVec3& P2)
            {
            _drawWireFrameLine<true>(P1, P2, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameLine(const fVec3& P1, const fVec3& P2, float thickness, color_t color, float opacity)
            {
            _drawWireFrameLine<false>(P1, P2, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameLines<true>(nb_lines, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameLines<false>(nb_lines, ind_vertices, vertices, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3)
            {
            _drawWireFrameTriangle<true>(P1, P2, P3, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, float thickness, color_t color, float opacity)
            {
            _drawWireFrameTriangle<false>(P1, P2, P3, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameTriangles<true>(nb_triangles, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameTriangles<false>(nb_triangles, ind_vertices, vertices, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4)
            {
            _drawWireFrameQuad<true>(P1, P2, P3, P4, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, float thickness, color_t color, float opacity)
            {
            _drawWireFrameQuad<false>(P1, P2, P3, P4, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices)
            {
            _drawWireFrameQuads<true>(nb_quads, ind_vertices, vertices, color_t(_color), 1.0f, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, float thickness, color_t color, float opacity)
            {
            _drawWireFrameQuads<false>(nb_quads, ind_vertices, vertices, color, opacity, thickness);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameMesh(const Mesh3D<color_t>* mesh, bool draw_chained_meshes, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            const tgx::fMat4 M(_lx/2.0f, 0, 0, _lx / 2.0f - _ox,
                               0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
                               0, 0, 1, 0,
                               0, 0, 0, 0);

            while (mesh != nullptr)
                {
                // check if the object is completely outside of the image for fast discard.
                if (_discardBox(mesh->bounding_box, _projM * _r_modelViewM)) return;

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
                        const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, PC0->P);
                        if (cu * _culling_dir > 0) goto rasterize_next_wireframetriangle; // skip triangle !


                        // triangle is not culled
                        *((fVec4*)PC2) = _projM * PC2->P;                        
                        if (ortho) { PC2->w = 1.0f - PC2->z; } else { PC2->zdivide(); }
                        *((fVec4*)PC2) = M.mult1(*((fVec4*)PC2));

                        if (PC0->missedP)
                            {
                            *((fVec4*)PC0) = _projM * PC0->P;                            
                            if (ortho) { PC0->w = 1.0f - PC0->z; } else { PC0->zdivide(); }
                            *((fVec4*)PC0) = M.mult1(*((fVec4*)PC0));
                            }
                        if (PC1->missedP)
                            {
                            *((fVec4*)PC1) = _projM * PC1->P;
                            if (ortho) { PC1->w = 1.0f - PC1->z; } else { PC1->zdivide(); }
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
                            _uni.im->drawThickLineAA(*((fVec2*)PC0), *((fVec2*)PC1), thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                            _uni.im->drawThickLineAA(*((fVec2*)PC1), *((fVec2*)PC2), thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                            _uni.im->drawThickLineAA(*((fVec2*)PC2), *((fVec2*)PC0), thickness, END_ROUNDED, END_ROUNDED, color, opacity);
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
        }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameLine(const fVec3& P1, const fVec3& P2, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);

            if ((Q0.z >= 0)||(Q1.z >= 0)) return;

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            fVec4 H1 = _projM * Q1;

            if (ortho) 
                { 
                H0.w = 1.0f - H0.z; 
                H1.w = 1.0f - H1.z;
                } 
            else 
                { 
                H0.zdivide(); 
                H1.zdivide();
                }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);
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
                _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                }
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameLines(int nb_lines, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            
            if (!_validDraw()) return;
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return; // invalid vertices
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
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
                fVec4 H1 = _projM * Q1;
                if (ortho) 
                    { 
                    H0.w = 1.0f - H0.z; 
                    H1.w = 1.0f - H1.z;
                    } 
                else 
                    { 
                    H0.zdivide(); 
                    H1.zdivide();
                    }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);
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
                    _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    }

                }            
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameTriangle(const fVec3& P1, const fVec3& P2, const fVec3& P3, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);
            const fVec4 Q2 = _r_modelViewM.mult1(P3);

            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // skip triangle !

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            fVec4 H1 = _projM * Q1;
            fVec4 H2 = _projM * Q2;

            if (ortho) 
                { 
                H0.w = 1.0f - H0.z; 
                H1.w = 1.0f - H1.z;
                H2.w = 1.0f - H2.z;
                }
            else 
                { 
                H0.zdivide(); 
                H1.zdivide();
                H2.zdivide();
                }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);
            if ((H1.w < -1) || (H1.w > 1)) return;
            H1 = M.mult1(H1);
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
                _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                _uni.im->drawThickLineAA(H1, H2, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                _uni.im->drawThickLineAA(H2, H0, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                }
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameTriangles(int nb_triangles, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return; // invalid vertices
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
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
                const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
                if (cu * _culling_dir > 0) continue; // we discard the triangle.

                fVec4 H0 = _projM * Q0;
                fVec4 H1 = _projM * Q1;
                fVec4 H2 = _projM * Q2;
                if (ortho) 
                    { 
                    H0.w = 1.0f - H0.z; 
                    H1.w = 1.0f - H1.z;
                    H2.w = 1.0f - H2.z;
                    } 
                else 
                    { 
                    H0.zdivide(); 
                    H1.zdivide();
                    H2.zdivide();
                    }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);
                if ((H1.w < -1) || (H1.w > 1)) continue;
                H1 = M.mult1(H1);
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
                    _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    _uni.im->drawThickLineAA(H1, H2, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    _uni.im->drawThickLineAA(H2, H0, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    }

                }
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameQuad(const fVec3& P1, const fVec3& P2, const fVec3& P3, const fVec3& P4, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            // compute position in wiew space.
            const fVec4 Q0 = _r_modelViewM.mult1(P1);
            const fVec4 Q1 = _r_modelViewM.mult1(P2);
            const fVec4 Q2 = _r_modelViewM.mult1(P3);

            if ((Q0.z >= 0)||(Q1.z >= 0)||(Q2.z >= 0)) return;

            // face culling (use triangle (0 1 2), doesn't matter since 0 1 2 3 are coplanar.
            fVec3 faceN = crossProduct(Q1 - Q0, Q2 - Q0);
            const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
            if (cu * _culling_dir > 0) return; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

            const fVec4 Q3 = _r_modelViewM.mult1(P4); // compute fourth point

            if (Q3.z >= 0) return;

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
                0, 0, 1, 0,
                0, 0, 0, 0);

            fVec4 H0 = _projM * Q0;
            fVec4 H1 = _projM * Q1;
            fVec4 H2 = _projM * Q2;
            fVec4 H3 = _projM * Q3;

            if (ortho) 
                { 
                H0.w = 1.0f - H0.z; 
                H1.w = 1.0f - H1.z;
                H2.w = 1.0f - H2.z;
                H3.w = 1.0f - H3.z;
                }
            else 
                { 
                H0.zdivide(); 
                H1.zdivide();
                H2.zdivide();
                H3.zdivide();
                }
            if ((H0.w < -1) || (H0.w > 1)) return;
            H0 = M.mult1(H0);
            if ((H1.w < -1) || (H1.w > 1)) return;
            H1 = M.mult1(H1);
            if ((H2.w < -1) || (H2.w > 1)) return;
            H2 = M.mult1(H2);
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
                _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                _uni.im->drawThickLineAA(H1, H2, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                _uni.im->drawThickLineAA(H2, H3, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                _uni.im->drawThickLineAA(H3, H0, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                }
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool DRAW_FAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawWireFrameQuads(int nb_quads, const uint16_t* ind_vertices, const fVec3* vertices, color_t color, float opacity, float thickness)
            {
            if (!_validDraw()) return;
            if ((ind_vertices == nullptr) || (vertices == nullptr)) return; // invalid vertices
            if (thickness <= 0) return;

            const bool ortho = _ortho;

            const tgx::fMat4 M(_lx / 2.0f, 0, 0, _lx / 2.0f - _ox,
                0, _ly / 2.0f, 0, _ly / 2.0f - _oy,
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
                const float cu = (ortho) ? dotProduct(faceN, fVec3(0.0f, 0.0f, -1.0f)) : dotProduct(faceN, Q0);
                if (cu * _culling_dir > 0) continue; // Q3 is coplanar with Q0, Q1, Q2 so we discard the whole quad.

                const fVec4 Q3 = _r_modelViewM.mult1(vertices[ind_vertices[n + 3]]); // compute fourth point

                if (Q3.z >= 0) return;

                fVec4 H0 = _projM * Q0;
                if (ortho) { H0.w = 1.0f - H0.z; } else { H0.zdivide(); }
                if ((H0.w < -1) || (H0.w > 1)) continue;
                H0 = M.mult1(H0);

                fVec4 H1 = _projM * Q1;
                if (ortho) { H1.w = 1.0f - H1.z; } else { H1.zdivide(); }
                if ((H1.w < -1) || (H1.w > 1)) continue;
                H1 = M.mult1(H1);

                fVec4 H2 = _projM * Q2;
                if (ortho) { H2.w = 1.0f - H2.z; } else { H2.zdivide(); }
                if ((H2.w < -1) || (H2.w > 1)) continue;
                H2 = M.mult1(H2);

                fVec4 H3 = _projM * Q3;
                if (ortho) { H3.w = 1.0f - H3.z; } else { H3.zdivide(); }
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
                    _uni.im->drawThickLineAA(H0, H1, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    _uni.im->drawThickLineAA(H1, H2, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    _uni.im->drawThickLineAA(H2, H3, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    _uni.im->drawThickLineAA(H3, H0, thickness, END_ROUNDED, END_ROUNDED, color, opacity);
                    }

                }
            }



    /********************************************************
    * DRAWING BASIC OBJECTS : PIXELS, DOTS, SPHERE, CUBE, SKYBOX...
    *********************************************************/




        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool USE_BLENDING> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawPixel(const fVec3& pos, color_t color, float opacity)
            {
            if (!_validDraw()) return;
            const bool has_zbuffer = TGX_SHADER_HAS_ZBUFFER(_shaders);
            fVec4 Q = _projM * _r_modelViewM.mult1(pos);
            if (_ortho) { Q.w = 1.0f - Q.z; } else { Q.zdivide(); }
            if ((Q.w < -1) || (Q.w > 1)) return;
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
            const int x = (int)roundfp(Q.x);
            const int y = (int)roundfp(Q.y);
            if (!has_zbuffer)
                {
                if (USE_BLENDING) _uni.im->template drawPixel<true>(x, y, color, opacity); else  _uni.im->template drawPixel<true>(x, y, color);
                }
            else
                {
                drawPixelZbuf<true, USE_BLENDING>(x, y, color, opacity, Q.w);
                }
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool USE_COLORS, bool USE_BLENDING> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            if (!_validDraw()) return;
            if (pos_list == nullptr) return;            
            if ((USE_COLORS) && ((colors_ind == nullptr) || (colors == nullptr))) return;
            if ((USE_BLENDING) && ((!USE_COLORS) || (opacities_ind == nullptr) || (opacities == nullptr))) return;
            const bool has_zbuffer = TGX_SHADER_HAS_ZBUFFER(_shaders);
            const bool ortho = _ortho;

            auto M = _projM * _r_modelViewM;
            for (int k = 0; k < nb_pixels; k++)
                {
                fVec3 P = pos_list[k];
                fVec4 Q = M.mult1(P);
                if (ortho) { Q.w = 1.0f - Q.z; } else { Q.zdivide(); }
                if ((Q.w < -1) || (Q.w > 1)) continue;
                Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
                Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
                const int x = (int)roundfp(Q.x);
                const int y = (int)roundfp(Q.y);
                if (!has_zbuffer)
                    {
                    if (USE_BLENDING)
                        _uni.im->template drawPixel<true>(x, y, colors[colors_ind[k]], opacities[opacities_ind[k]]);
                    else if (USE_COLORS)
                        _uni.im->template drawPixel<true>(x, y, colors[colors_ind[k]]);
                    else
                        _uni.im->template drawPixel<true>(x, y, _color);
                    }
                else
                    {
                    
                    if (USE_BLENDING)
                        drawPixelZbuf<true, true>(x, y, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                    else if (USE_COLORS)
                        drawPixelZbuf<true, true>(x, y, colors[colors_ind[k]], 1.0f, Q.w);
                    else
                        drawPixelZbuf<true, true>(x, y, _color, 1.0f, Q.w);                        
                    }
                }           
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawPixel(const fVec3& pos)
            {
            _drawPixel<false>(pos, _color, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawPixel(const fVec3& pos, color_t color, float opacity)
            {
            _drawPixel<true>(pos, color, opacity);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawPixels(int nb_pixels, const fVec3* pos_list)
            {
            _drawPixels<false, false>(nb_pixels, pos_list, nullptr, nullptr, nullptr, nullptr);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawPixels(int nb_pixels, const fVec3* pos_list, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            _drawPixels<true, true>(nb_pixels, pos_list, colors_ind, colors, opacities_ind, opacities);
            }




        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool USE_BLENDING> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawDot(const fVec3& pos, int r, color_t color, float opacity)
            {
            if (!_validDraw()) return;
            const bool has_zbuffer = TGX_SHADER_HAS_ZBUFFER(_shaders);
            fVec4 Q = _projM * _r_modelViewM.mult1(pos);
            if (_ortho) { Q.w = 1.0f - Q.z; } else { Q.zdivide(); }
            if ((Q.w < -1) || (Q.w > 1)) return;
            Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
            Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
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


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool USE_RADIUS, bool USE_COLORS, bool USE_BLENDING> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            if (!_validDraw()) return;
            if ((pos_list == nullptr) || (radius == nullptr)) return;
            if ((USE_RADIUS) && (radius_ind == nullptr)) return;
            if ((USE_COLORS) && ((colors_ind == nullptr) || (colors == nullptr))) return;
            if ((USE_BLENDING) && ((!USE_COLORS) || (opacities_ind == nullptr) || (opacities == nullptr))) return;
            const bool has_zbuffer = TGX_SHADER_HAS_ZBUFFER(_shaders);
            const bool ortho = _ortho;
            const int rr = radius[0];
            auto M = _projM * _r_modelViewM;
            for (int k = 0; k < nb_dots; k++)
                {
                fVec4 Q = M.mult1(pos_list[k]);
                if (ortho) { Q.w = 1.0f - Q.z; } else { Q.zdivide(); }
                if ((Q.w < -1) || (Q.w > 1)) continue;
                Q.x = ((Q.x + 1) * _lx) / 2 - _ox;
                Q.y = ((Q.y + 1) * _ly) / 2 - _oy;
                const int x = (int)roundfp(Q.x);
                const int y = (int)roundfp(Q.y);
                const int r = (USE_RADIUS) ? radius[radius_ind[k]] : rr;
                if (!has_zbuffer)
                    {
                    if (USE_BLENDING)
                        _uni.im->drawCircle(x, y, radius[r], colors[colors_ind[k]], opacities[opacities_ind[k]]);
                    else if (USE_COLORS)
                        _uni.im->drawCircle(x, y, radius[r], colors[colors_ind[k]]);
                    else
                        _uni.im->drawCircle(x, y, radius[r], _color);
                    }
                else
                    {
                    const int lx = _uni.im->lx();
                    const int ly = _uni.im->ly();
                    
                    if ((x - r >= 0) && (x + r < lx) && (y - r >= 0) && (y + r < ly))
                        {
                        if (USE_BLENDING)
                            _drawCircleZbuf<false, USE_BLENDING>(x, y, r, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                        else if (USE_COLORS)
                            _drawCircleZbuf<false, USE_BLENDING>(x, y, r, colors[colors_ind[k]], 1.0f, Q.w);
                        else
                            _drawCircleZbuf<false, USE_BLENDING>(x, y, r, _color, 1.0f, Q.w);
                        }
                    else
                        {
                        if ((x >= 0) && (x < lx) && (y >= 0) && (y < ly))
                            {
                            if (USE_BLENDING)
                                _drawCircleZbuf<true, USE_BLENDING>(x, y, r, colors[colors_ind[k]], opacities[opacities_ind[k]], Q.w);
                            else if (USE_COLORS)
                                _drawCircleZbuf<true, USE_BLENDING>(x, y, r, colors[colors_ind[k]], 1.0f, Q.w);
                            else
                                _drawCircleZbuf<true, USE_BLENDING>(x, y, r, _color, 1.0f, Q.w);
                            }
                        }
                    }
                }
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> 
        template<bool CHECKRANGE, bool USE_BLENDING> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawCircleZbuf(int xm, int ym, int r, color_t color, float opacity, float z)
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawDot(const fVec3& pos, int r)
            {
            _drawDot<false>(pos, r, _color, 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawDot(const fVec3& pos, int r, color_t color, float opacity)
            {
            _drawDot<true>(pos, r, color, opacity);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawDots(int nb_dots, const fVec3* pos_list, const int radius)
            {
            _drawDots<false, false, false>(nb_dots, pos_list, nullptr, &radius, nullptr, nullptr, nullptr, nullptr);
            }






        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawDots(int nb_dots, const fVec3* pos_list, const int* radius_ind, const int* radius, const int* colors_ind, const color_t* colors, const int* opacities_ind, const float* opacities)
            {
            _drawDots<true, true, true>(nb_dots, pos_list, radius_ind, radius, colors_ind, colors, opacities_ind, opacities);
            }





        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        float Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_unitSphereScreenDiameter()
            {
            const float ONEOVERSQRT2 = 0.70710678118f;
            fVec4 P0 = _r_modelViewM.mult1(fVec3(0, 0, 0));
            fVec4 P1 = _r_modelViewM.mult1(fVec3(1, 0, 0));
            float r = fVec3(P0 - P1).norm_fast(); // radius after modelview transform
            fVec4 P2 = { P0.x - r * ONEOVERSQRT2, P0.y - r * ONEOVERSQRT2, P0.z, P0.w }; // take a point on the sphere that is at same z as origin and take equal contribution from x and y. 

            fVec4 Q0 = _projM * P0;
            fVec4 Q2 = _projM * P2;
            if (!_ortho)
                {
                Q0.zdivide();
                Q2.zdivide();
                }
            return tgx::fast_sqrt(((Q2.x - Q0.x) * (Q2.x - Q0.x) * _lx * _lx) + ((Q2.y - Q0.y) * (Q2.y - Q0.y) * _ly * _ly)); // diameter on the screen
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawCube()
            {
            // set culling direction = -1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES, UNIT_CUBE_FACES_NORMALS, UNIT_CUBE_NORMALS);
            // restore culling direction
            _culling_dir = save_culling;
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawCube(
            const fVec2 v_front_ABCD[4] , const Image<color_t>* texture_front,
            const fVec2 v_back_EFGH[4]  , const Image<color_t>* texture_back,
            const fVec2 v_top_HADE[4]   , const Image<color_t>* texture_top,
            const fVec2 v_bottom_BGFC[4], const Image<color_t>* texture_bottom,
            const fVec2 v_left_HGBA[4]  , const Image<color_t>* texture_left,
            const fVec2 v_right_DCFE[4] , const Image<color_t>* texture_right
            )
            {

            if (!(TGX_SHADER_HAS_TEXTURE(_shaders)))
                {
                drawCube();
                return;
                }

            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;

            
            const uint16_t list_v[4] = { 0,1,2,3 };
            if (texture_front) { drawQuads(1, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_front_ABCD, texture_front); } else { drawQuads(1, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES); }
            if (texture_back) { drawQuads(1, UNIT_CUBE_FACES + 4, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_back_EFGH, texture_back); } else { drawQuads(1, UNIT_CUBE_FACES + 4, UNIT_CUBE_VERTICES); }
            if (texture_top) { drawQuads(1, UNIT_CUBE_FACES + 8, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_top_HADE, texture_top); } else { drawQuads(1, UNIT_CUBE_FACES + 8, UNIT_CUBE_VERTICES); }
            if (texture_bottom) { drawQuads(1, UNIT_CUBE_FACES + 12, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_bottom_BGFC, texture_bottom); } else { drawQuads(1, UNIT_CUBE_FACES + 12, UNIT_CUBE_VERTICES); }
            if (texture_left) { drawQuads(1, UNIT_CUBE_FACES + 16, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_left_HGBA, texture_left); } else { drawQuads(1, UNIT_CUBE_FACES + 16, UNIT_CUBE_VERTICES); }
            if (texture_right) { drawQuads(1, UNIT_CUBE_FACES + 20, UNIT_CUBE_VERTICES, nullptr, nullptr, list_v, v_right_DCFE, texture_right); } else { drawQuads(1, UNIT_CUBE_FACES + 20, UNIT_CUBE_VERTICES); }
            

            // restore culling direction
            _culling_dir = save_culling;            
            }



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawCube(
            const Image<color_t>* texture_front,
            const Image<color_t>* texture_back,
            const Image<color_t>* texture_top,
            const Image<color_t>* texture_bottom,
            const Image<color_t>* texture_left,
            const Image<color_t>* texture_right
            )
            {
            float epsx = 0, epsy = 0; 
            if (texture_front) 
                {
                epsx = fast_inv((float)(texture_front->lx() - 1)); 
                epsy = fast_inv((float)(texture_front->ly() - 1));
                }
            tgx::fVec2 t_front[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            if (texture_back) 
                {
                epsx = fast_inv((float)(texture_back->lx() - 1)); 
                epsy = fast_inv((float)(texture_back->ly() - 1));
                }
            tgx::fVec2 t_back[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            if (texture_top) 
                {
                epsx = fast_inv((float)(texture_top->lx() - 1)); 
                epsy = fast_inv((float)(texture_top->ly() - 1));
                }
            tgx::fVec2 t_top[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            if (texture_bottom) 
                {
                epsx = fast_inv((float)(texture_bottom->lx() - 1)); 
                epsy = fast_inv((float)(texture_bottom->ly() - 1));
                }
            tgx::fVec2 t_bottom[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            if (texture_left) 
                {
                epsx = fast_inv((float)(texture_left->lx() - 1)); 
                epsy = fast_inv((float)(texture_left->ly() - 1));
                }
            tgx::fVec2 t_left[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            if (texture_right) 
                {
                epsx = fast_inv((float)(texture_right->lx() - 1)); 
                epsy = fast_inv((float)(texture_right->ly() - 1));
                }
            tgx::fVec2 t_right[4] = { tgx::fVec2(epsx,epsy), tgx::fVec2(epsx,1 - epsy), tgx::fVec2(1 - epsx,1 - epsy), tgx::fVec2(1 - epsx,epsy) };

            drawCube(
                t_front, texture_front,
                t_back, texture_back,
                t_top, texture_top,
                t_bottom, texture_bottom,
                t_left, texture_left,
                t_right, texture_right);
            }




        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawSphere(int nb_sectors, int nb_stacks)
            {
            drawSphere(nb_sectors, nb_stacks, nullptr);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture)
            {
            _drawSphere<false, false>(nb_sectors, nb_stacks, texture, 1.0f, color_t(_color), 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawAdaptativeSphere(float quality)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawSphere(nb_stacks * 2 - 2, nb_stacks, nullptr);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawAdaptativeSphere(const Image<color_t>* texture, float quality)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawSphere(nb_stacks * 2 - 2, nb_stacks, texture);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        template<bool WIREFRAME, bool DRAWFAST> TGX_NOINLINE
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_drawSphere(int nb_sectors, int nb_stacks, const Image<color_t>* texture, float thickness, color_t color, float opacity)
            {
            
            const int save_shaders = _shaders; 
            
            if (texture == nullptr)
                {
                TGX_SHADER_REMOVE_TEXTURE(_shaders);
                }

            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;

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
                    fVec2 T1(0, 1);
                    fVec2 T2(u + dtx, v);
                    fVec2 T3(u, v);
                    drawTriangle(P1, P3, P2, &P1, &P3, &P2, &T1, &T2, &T3, texture);
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
                        fVec2 T1(u, v);
                        fVec2 T2(uu, v);
                        fVec2 T3(uu, vv);
                        fVec2 T4(u, vv);
                        drawQuad(P1, P3, P4, P2, &P1, &P3, &P4, &P2, &T1, &T2, &T3, &T4, texture);
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
                    fVec2 T1(0, 0);
                    fVec2 T2(u, v);
                    fVec2 T3(u + dtx, v);
                    drawTriangle(P1, P2, P3, &P1, &P2, &P3, &T1, &T2, &T3, texture);
                    }

                u += dtx;
                P2.x = P3.x;
                P2.z = P3.z;
                }

            // restore culling direction
            _culling_dir = save_culling;
            
            // restore shader
            _shaders = save_shaders;
            return; 
            }




        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameCube()
            {
            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawWireFrameQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES);
            // restore culling direction
            _culling_dir = save_culling;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameCube(float thickness, color_t color, float opacity)
            {
            // set culling direction = 1 and save previous value
            float save_culling = _culling_dir;
            if (_culling_dir != 0) _culling_dir = 1;
            drawWireFrameQuads(6, UNIT_CUBE_FACES, UNIT_CUBE_VERTICES, thickness, color, opacity);
            // restore culling direction
            _culling_dir = save_culling;
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameSphere(int nb_sectors, int nb_stacks)
            {
            _drawSphere<true, true>(nb_sectors, nb_stacks, nullptr, 1.0f, color_t(_color), 1.0f);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameSphere(int nb_sectors, int nb_stacks, float thickness, color_t color, float opacity)
            {
            _drawSphere<true,false>(nb_sectors, nb_stacks, nullptr, thickness, color, opacity);
            }


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameAdaptativeSphere(float quality)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawWireFrameSphere(nb_stacks*2 - 2, nb_stacks);
            }   


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::drawWireFrameAdaptativeSphere(float quality, float thickness, color_t color, float opacity)
            {
            const float l = _unitSphereScreenDiameter(); // compute the diameter in pixel of the projected sphere on the screen
            const int nb_stacks = 2 + (int)tgx::fast_sqrt(l * quality); // Why this formula ? Well, why not...
            drawWireFrameSphere(nb_stacks * 2 - 2, nb_stacks, thickness, color, opacity);
            }









    /*****************************************************************************************
    ******************************************************************************************
    * Implementation of private methods
    ******************************************************************************************
    ******************************************************************************************/

                


        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_recompute_wa_wb()
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_rectifyShaderOrtho()
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_rectifyShaderZbuffer()
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_rectifyShaderShading(Shader new_shaders)
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
                setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_CLAMP(new_shaders))
                {
                setTextureWrappingMode(SHADER_TEXTURE_CLAMP);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_NEAREST(new_shaders))
                {
                setTextureQuality(SHADER_TEXTURE_NEAREST);
                tex = true;
                }
            if (TGX_SHADER_HAS_TEXTURE_BILINEAR(new_shaders))
                {
                setTextureQuality(SHADER_TEXTURE_BILINEAR);
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_rectifyShaderTextureWrapping()
            {
            if (_texture_wrap_mode == SHADER_TEXTURE_WRAP_POW2)
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



        template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t>
        void Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t>::_rectifyShaderTextureQuality()
            {
            if (_texture_quality == SHADER_TEXTURE_BILINEAR)
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







}



#endif

/** end of file */

