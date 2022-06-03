/** @file Shaders.h */
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.
#ifndef _TGX_SHADERSPARAMS_H_
#define _TGX_SHADERSPARAMS_H_


// only C++, no plain C
#ifdef __cplusplus


#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Box2.h"
#include "Color.h"

#include <stdint.h>

namespace tgx
{


    /** options for triangle rasterization */


    #define TGX_SHADER_SET_FLAGS(shader_type, flags) { shader_type = (flags); }

    #define TGX_SHADER_ADD_FLAGS(shader_type, flags) { shader_type |= (flags);}

    #define TGX_SHADER_REMOVE_FLAGS(shader_type, flags) { shader_type &= ~(flags); }

    #define TGX_SHADER_HAS_ONE_FLAG(shader_type, flags) (shader_type & (flags))

    #define TGX_SHADER_HAS_ALL_FLAGS(shader_type, flags) ((shader_type & (flags)) == flags)




    // Shaders for projection type : Perspective or ortho. 
    #define TGX_SHADER_PERSPECTIVE      (1 << 0)
    #define TGX_SHADER_ORTHO            (1 << 1)
    #define TGX_SHADER_MASK_PROJECTION  (TGX_SHADER_PERSPECTIVE | TGX_SHADER_ORTHO)

    // Shaders for depth buffer : Zbuffer or no-Zbuffer
    #define TGX_SHADER_NOZBUFFER        (1 << 2)
    #define TGX_SHADER_ZBUFFER          (1 << 3)
    #define TGX_SHADER_MASK_ZBUFFER     (TGX_SHADER_NOZBUFFER | TGX_SHADER_ZBUFFER)

    // Shaders for shading algorithm: flat, gouraud or phong
    #define TGX_SHADER_FLAT             (1 << 4)
    #define TGX_SHADER_GOURAUD          (1 << 5)
    #define TGX_SHADER_MASK_SHADING     (TGX_SHADER_FLAT | TGX_SHADER_GOURAUD)

    // Shaders for texturing mode: no-texture, texture-wrap, texture-clamp 
    #define TGX_SHADER_NOTEXTURE        (1 << 7)
    #define TGX_SHADER_TEXTURE          (1 << 8)
    #define TGX_SHADER_MASK_TEXTURE     (TGX_SHADER_NOTEXTURE | TGX_SHADER_TEXTURE)
    
    // Shaders for texture quality: nearest, bilinear
    #define TGX_SHADER_TEXTURE_NEAREST  (1 << 11)
    #define TGX_SHADER_TEXTURE_BILINEAR (1 << 12)
    #define TGX_SHADER_MASK_TEXTURE_QUALITY (TGX_SHADER_TEXTURE_BILINEAR | TGX_SHADER_TEXTURE_NEAREST)

    // Shaders for texture wrapping mode: wrap , clamp
    #define TGX_SHADER_TEXTURE_WRAP_POW2  (1 << 13)
    #define TGX_SHADER_TEXTURE_CLAMP (1 << 14)
    #define TGX_SHADER_MASK_TEXTURE_MODE (TGX_SHADER_TEXTURE_WRAP_POW2 | TGX_SHADER_TEXTURE_CLAMP)



    // all shaders set. 
    #define TGX_SHADER_MASK_ALL (TGX_SHADER_MASK_PROJECTION | TGX_SHADER_MASK_ZBUFFER | TGX_SHADER_MASK_SHADING | TGX_SHADER_MASK_TEXTURE | TGX_SHADER_MASK_TEXTURE_QUALITY | TGX_SHADER_MASK_TEXTURE_MODE)
    

    #define TGX_SHADER_HAS_PERSPECTIVE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_PERSPECTIVE))
    #define TGX_SHADER_HAS_ORTHO(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_ORTHO))

    #define TGX_SHADER_HAS_NOZBUFFER(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_NOZBUFFER))
    #define TGX_SHADER_HAS_ZBUFFER(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_ZBUFFER))

    #define TGX_SHADER_HAS_FLAT(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_FLAT))
    #define TGX_SHADER_HAS_GOURAUD(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_GOURAUD))

    #define TGX_SHADER_HAS_NOTEXTURE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_NOTEXTURE))
    #define TGX_SHADER_HAS_TEXTURE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_TEXTURE))
    
    #define TGX_SHADER_HAS_TEXTURE_NEAREST(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_TEXTURE_NEAREST))
    #define TGX_SHADER_HAS_TEXTURE_BILINEAR(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_TEXTURE_BILINEAR))

    #define TGX_SHADER_HAS_TEXTURE_WRAP_POW2(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_TEXTURE_WRAP_POW2))
    #define TGX_SHADER_HAS_TEXTURE_CLAMP(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , TGX_SHADER_TEXTURE_CLAMP))



    #define TGX_SHADER_REMOVE_PERSPECTIVE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_PERSPECTIVE)
    #define TGX_SHADER_REMOVE_ORTHO(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_ORTHO)

    #define TGX_SHADER_REMOVE_NOZBUFFER(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_NOZBUFFER)
    #define TGX_SHADER_REMOVE_ZBUFFER(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_ZBUFFER)

    #define TGX_SHADER_REMOVE_FLAT(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_FLAT)
    #define TGX_SHADER_REMOVE_GOURAUD(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_GOURAUD)

    #define TGX_SHADER_REMOVE_NOTEXTURE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_NOTEXTURE)
    #define TGX_SHADER_REMOVE_TEXTURE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_TEXTURE)
    
    #define TGX_SHADER_REMOVE_TEXTURE_NEAREST(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_TEXTURE_NEAREST)
    #define TGX_SHADER_REMOVE_TEXTURE_BILINEAR(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_TEXTURE_BILINEAR)

    #define TGX_SHADER_REMOVE_TEXTURE_WRAP_POW2(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_TEXTURE_WRAP_POW2)
    #define TGX_SHADER_REMOVE_TEXTURE_CLAMP(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , TGX_SHADER_TEXTURE_CLAMP)



    #define TGX_SHADER_ADD_PERSPECTIVE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_PERSPECTIVE)
    #define TGX_SHADER_ADD_ORTHO(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_ORTHO)

    #define TGX_SHADER_ADD_NOZBUFFER(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_NOZBUFFER)
    #define TGX_SHADER_ADD_ZBUFFER(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_ZBUFFER)

    #define TGX_SHADER_ADD_FLAT(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_FLAT)
    #define TGX_SHADER_ADD_GOURAUD(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_GOURAUD)

    #define TGX_SHADER_ADD_NOTEXTURE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_NOTEXTURE)
    #define TGX_SHADER_ADD_TEXTURE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_TEXTURE)
    
    #define TGX_SHADER_ADD_TEXTURE_NEAREST(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_TEXTURE_NEAREST)
    #define TGX_SHADER_ADD_TEXTURE_BILINEAR(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_TEXTURE_BILINEAR)

    #define TGX_SHADER_ADD_TEXTURE_WRAP_POW2(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_TEXTURE_WRAP_POW2)
    #define TGX_SHADER_ADD_TEXTURE_CLAMP(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , TGX_SHADER_TEXTURE_CLAMP)


    //forward declaration
    template<typename color_t> class Image;


    /**
    * Vertex parameters
    *
    * Extension of the fVec4 class that holds the 'varying' parameters (in the OpenGL sense)
    * associated with a vertex and passed to the shader method
    **/
    struct RasterizerVec4 : public tgx::fVec4
        {
        tgx::RGBf   color;  // vertex color for gouraud shading (or light intensity for gouraud shading with texturing). 
        tgx::fVec2  T;      // texture coordinates if applicable 
        float       A;      // alpha value (if used). 
        };



    /**
    * Uniform parameters
    *
    * Structure that holds the 'uniform' parameters (in opengl sense) passed
    * to the triangle rasterizer when doing 3D rendering
    **/
    template<typename color_t_im, typename color_t_tex, typename ZBUFFER_t, typename BLEND_OP = void> struct RasterizerParams
        {
        RGBf                        facecolor;      // pointer to the face color (when using flat shading).  
        float                       opacity;        // opacity multiplier (currently used only with the 2D shader)        
        Image<color_t_im> *         im;             // pointer to the destination image to draw onto
        ZBUFFER_t *                 zbuf;           // pointer to the z buffer (when using depth testing).
        const Image<color_t_tex>*   tex;            // pointer to the texture (when using texturing).        
        color_t_tex                 mask_color;     // 'transparent color' when masking is enabled (on for the 2D shader).
        int                         shader_type;    // shader type
        float                       wa;             // constants such that f(w) = wa * w + wb maps
        float                       wb;             // w (= -1/z) -> float(0, 65535) for conversion to uint16_t
        const BLEND_OP *            p_blend_op;     // pointer to the blending operator to use (only with the 2D shader)        
        };



}


#endif

#endif


/** end of file */


