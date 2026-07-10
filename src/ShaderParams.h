/**   
 * @file ShaderParams.h
 * Triangle shader parameters.
 */
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

    /**
    * Shader flags used by the TGX 3D renderer.
    *
    * These flags have two related roles:
    * 1. As the `Renderer3D<LOADED_SHADERS>` template parameter, they select which drawing paths are compiled into the renderer;
    * 2. At runtime, one flag from each group is used to determine the exact drawing mechanism.
    *
    * The flags are organized in 4 main groups:
    * - [A] projection path: perspective or orthographic;
    * - [B] depth path: without or with Z-buffer testing;
    * - [C] shading path: unlit, flat or Gouraud;
    * - [D] texture path: no texture, perspective-correct texture or affine texture;
    * 
    * and 3 additional groups specific to texturing:
    * - [E] texture sampling quality: nearest or bilinear;
    * - [F] texture addressing mode: wrap or clamp;
    * - [G] texture color masking: disabled or enabled.
    *
    * A valid `Renderer3D<LOADED_SHADERS>` configuration must contain:
    * - at least one flag from groups [A], [B], [C] and [D]. 
    * - at least one flag from groups [E], [F] and [G] when the renderer is compiled with texturing enabled, i.e. when `LOADED_SHADERS` contains
    * `SHADER_TEXTURE` or `SHADER_TEXTURE_AFFINE`.
    *
    * @warning "Negative" flags such as `SHADER_NOZBUFFER`, `SHADER_NOTEXTURE` and
    * `SHADER_TEXTURE_NOMASK` are real rendering paths. They must be listed in
    * `LOADED_SHADERS` when the corresponding mode may be used !
    *
    * @remark Code is smallest and fastest when only one flag from each group is enabled, because the
    * corresponding runtime branches can then be eliminated at compile time.
    */
    enum Shader
        {
        SHADER_PERSPECTIVE = (1 << 0),          ///< [A] Perspective projection path.
        SHADER_ORTHO = (1 << 1),                ///< [A] Orthographic projection path.

        SHADER_NOZBUFFER = (1 << 2),            ///< [B] Rendering path without Z-buffer testing.
        SHADER_ZBUFFER = (1 << 3),              ///< [B] Rendering path with Z-buffer testing.

        SHADER_FLAT = (1 << 4),                 ///< [C] Flat shading: one lighted color is computed per face.
        SHADER_GOURAUD = (1 << 5),              ///< [C] Gouraud shading: lighting is computed at vertices and interpolated.
        SHADER_UNLIT = (1 << 6),                ///< [C] Unlit shading: material or texture color is used directly; lighting is ignored.

        SHADER_NOTEXTURE = (1 << 7),            ///< [D] Rendering path without texture mapping.
        SHADER_TEXTURE = (1 << 8),              ///< [D] Perspective-correct texture mapping.
        SHADER_TEXTURE_AFFINE = (1 << 9),       ///< [D] Affine texture mapping: faster screen-space interpolation, lower perspective quality.

        SHADER_TEXTURE_NEAREST = (1 << 11),     ///< [E] Nearest-neighbor texture sampling: fastest, pixelated when magnified.
        SHADER_TEXTURE_BILINEAR = (1 << 12),    ///< [E] Bilinear texture sampling: smoother, slower.

        SHADER_TEXTURE_WRAP_POW2 = (1 << 13),   ///< [F] Repeat/wrap texture coordinates; texture width and height must be powers of two.
        SHADER_TEXTURE_CLAMP = (1 << 14),       ///< [F] Clamp texture coordinates to the texture edge; arbitrary texture sizes are allowed.

        SHADER_TEXTURE_NOMASK = (1 << 15),      ///< [G] Disable texture color masking.
        SHADER_TEXTURE_MASK = (1 << 16)         ///< [G] Enable texture color masking: texels equal to the renderer mask color are discarded.
        };


    /** Enable bitwise `|` operator for enum */
    inline constexpr Shader operator|(Shader a1, Shader a2) { return ((Shader)((int)a1 | (int)a2)); }

    /** Enable bitwise `|=` operator for enum */
    inline Shader& operator|=(Shader& a1, Shader a2) { a1 = a1 | a2; return a1; }

    /** Enable bitwise `&` operator for enum */
    inline constexpr Shader operator&(Shader a1, Shader a2) { return ((Shader)((int)a1 & (int)a2)); }

    /** Enable bitwise `&=` operator for enum */
    inline Shader& operator&=(Shader& a1, Shader a2) { a1 = a1 & a2; return a1; }

    /** Enable bitwise `~` operator for enum */
    inline constexpr Shader operator~(Shader a) { return (Shader)(~((int)a)); }



    /** @name Shader group masks.
    * Mostly internal helpers, also useful for advanced code that needs to test or filter
    * complete shader groups.
    */
    ///@{
    #define TGX_SHADER_MASK_PROJECTION      (SHADER_PERSPECTIVE | SHADER_ORTHO)                         ///< [A] projection group mask.
    #define TGX_SHADER_MASK_ZBUFFER         (SHADER_NOZBUFFER | SHADER_ZBUFFER)                         ///< [B] Z-buffer group mask.
    #define TGX_SHADER_MASK_SHADING         (SHADER_FLAT | SHADER_GOURAUD | SHADER_UNLIT)               ///< [C] shading group mask.
    #define TGX_SHADER_MASK_TEXTURE         (SHADER_NOTEXTURE | SHADER_TEXTURE | SHADER_TEXTURE_AFFINE) ///< [D] texture path group mask.
    #define TGX_SHADER_MASK_TEXTURE_QUALITY (SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_NEAREST)          ///< [E] texture quality group mask.
    #define TGX_SHADER_MASK_TEXTURE_MODE    (SHADER_TEXTURE_WRAP_POW2 | SHADER_TEXTURE_CLAMP)           ///< [F] texture addressing group mask.
    #define TGX_SHADER_MASK_TEXTURE_MASK    (SHADER_TEXTURE_NOMASK | SHADER_TEXTURE_MASK)               ///< [G] texture masking group mask.
    #define TGX_SHADER_MASK_ALL             (TGX_SHADER_MASK_PROJECTION | TGX_SHADER_MASK_ZBUFFER | TGX_SHADER_MASK_SHADING | TGX_SHADER_MASK_TEXTURE | TGX_SHADER_MASK_TEXTURE_QUALITY | TGX_SHADER_MASK_TEXTURE_MODE | TGX_SHADER_MASK_TEXTURE_MASK) ///< all shader groups.
    ///@}

    #define TGX_SHADER_SET_FLAGS(shader_type, flags) { shader_type = (flags); }
    #define TGX_SHADER_ADD_FLAGS(shader_type, flags) { shader_type |= (flags);}
    #define TGX_SHADER_REMOVE_FLAGS(shader_type, flags) { shader_type &= ~(flags); }
    #define TGX_SHADER_HAS_ONE_FLAG(shader_type, flags) (shader_type & (flags))
    #define TGX_SHADER_HAS_ALL_FLAGS(shader_type, flags) ((shader_type & (flags)) == flags)
    
    #define TGX_SHADER_HAS_PERSPECTIVE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_PERSPECTIVE))
    #define TGX_SHADER_HAS_ORTHO(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_ORTHO))
    #define TGX_SHADER_HAS_NOZBUFFER(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_NOZBUFFER))
    #define TGX_SHADER_HAS_ZBUFFER(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_ZBUFFER))
    #define TGX_SHADER_HAS_FLAT(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_FLAT))
    #define TGX_SHADER_HAS_GOURAUD(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_GOURAUD))
    #define TGX_SHADER_HAS_UNLIT(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_UNLIT))

    /** True when a shader configuration can use either flat shading or unlit shading. */
    #define TGX_SHADER_CAN_USE_FLAT_OR_UNLIT(enabled_shader_type, shader_type) \
        (TGX_SHADER_HAS_UNLIT(enabled_shader_type) ? \
            (TGX_SHADER_HAS_UNLIT(shader_type) || (TGX_SHADER_HAS_FLAT(enabled_shader_type) && !TGX_SHADER_HAS_GOURAUD(shader_type))) : \
            TGX_SHADER_HAS_FLAT(enabled_shader_type))
    #define TGX_SHADER_HAS_NOTEXTURE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_NOTEXTURE))
    #define TGX_SHADER_HAS_TEXTURE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE))
    #define TGX_SHADER_HAS_TEXTURE_AFFINE(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_AFFINE))
    #define TGX_SHADER_HAS_TEXTURING_ENABLED(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , (SHADER_TEXTURE | SHADER_TEXTURE_AFFINE)))
    #define TGX_SHADER_HAS_TEXTURE_NEAREST(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_NEAREST))
    #define TGX_SHADER_HAS_TEXTURE_BILINEAR(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_BILINEAR))
    #define TGX_SHADER_HAS_TEXTURE_WRAP_POW2(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_WRAP_POW2))
    #define TGX_SHADER_HAS_TEXTURE_CLAMP(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_CLAMP))
    #define TGX_SHADER_HAS_TEXTURE_NOMASK(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_NOMASK))
    #define TGX_SHADER_HAS_TEXTURE_MASK(shader_type) (TGX_SHADER_HAS_ONE_FLAG(shader_type , SHADER_TEXTURE_MASK))

    #define TGX_SHADER_REMOVE_PERSPECTIVE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_PERSPECTIVE)
    #define TGX_SHADER_REMOVE_ORTHO(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_ORTHO)
    #define TGX_SHADER_REMOVE_NOZBUFFER(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_NOZBUFFER)
    #define TGX_SHADER_REMOVE_ZBUFFER(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_ZBUFFER)
    #define TGX_SHADER_REMOVE_FLAT(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_FLAT)
    #define TGX_SHADER_REMOVE_GOURAUD(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_GOURAUD)
    #define TGX_SHADER_REMOVE_UNLIT(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_UNLIT)
    #define TGX_SHADER_REMOVE_NOTEXTURE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_NOTEXTURE)
    #define TGX_SHADER_REMOVE_TEXTURE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE)
    #define TGX_SHADER_REMOVE_TEXTURE_AFFINE(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_AFFINE)
    #define TGX_SHADER_REMOVE_TEXTURING_ENABLED(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , (SHADER_TEXTURE | SHADER_TEXTURE_AFFINE))
    #define TGX_SHADER_REMOVE_TEXTURE_NEAREST(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_NEAREST)
    #define TGX_SHADER_REMOVE_TEXTURE_BILINEAR(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_BILINEAR)
    #define TGX_SHADER_REMOVE_TEXTURE_WRAP_POW2(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_WRAP_POW2)
    #define TGX_SHADER_REMOVE_TEXTURE_CLAMP(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_CLAMP)
    #define TGX_SHADER_REMOVE_TEXTURE_NOMASK(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_NOMASK)
    #define TGX_SHADER_REMOVE_TEXTURE_MASK(shader_type) TGX_SHADER_REMOVE_FLAGS(shader_type , SHADER_TEXTURE_MASK)

    #define TGX_SHADER_ADD_PERSPECTIVE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_PERSPECTIVE)
    #define TGX_SHADER_ADD_ORTHO(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_ORTHO)
    #define TGX_SHADER_ADD_NOZBUFFER(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_NOZBUFFER)
    #define TGX_SHADER_ADD_ZBUFFER(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_ZBUFFER)
    #define TGX_SHADER_ADD_FLAT(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_FLAT)
    #define TGX_SHADER_ADD_GOURAUD(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_GOURAUD)
    #define TGX_SHADER_ADD_UNLIT(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_UNLIT)
    #define TGX_SHADER_ADD_NOTEXTURE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_NOTEXTURE)
    #define TGX_SHADER_ADD_TEXTURE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE)
    #define TGX_SHADER_ADD_TEXTURE_AFFINE(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_AFFINE)
    #define TGX_SHADER_ADD_TEXTURE_NEAREST(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_NEAREST)
    #define TGX_SHADER_ADD_TEXTURE_BILINEAR(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_BILINEAR)
    #define TGX_SHADER_ADD_TEXTURE_WRAP_POW2(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_WRAP_POW2)
    #define TGX_SHADER_ADD_TEXTURE_CLAMP(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_CLAMP)
    #define TGX_SHADER_ADD_TEXTURE_NOMASK(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_NOMASK)
    #define TGX_SHADER_ADD_TEXTURE_MASK(shader_type) TGX_SHADER_ADD_FLAGS(shader_type , SHADER_TEXTURE_MASK)


    //forward declaration    
    template<typename color_t> class Image;


    /**
    * Vertex parameters passed to the shader (**for internal use**).
    *
    * Extension of the fVec4 class that holds the 'varying' parameters (in the OpenGL sense)
    * associated with a vertex and passed to the shader method.
    */
    struct RasterizerVec4 : public tgx::fVec4
        {
        tgx::RGBf   color;  ///< vertex color for Gouraud shading (or light intensity for Gouraud shading with texturing). 
        tgx::fVec2  T;      ///< texture coordinates if applicable 
        float       A;      ///< alpha value (if used). 
        };



    /**
    * Uniform parameters passed to the shader (**for internal use**).
    *
    * Structure that holds the 'uniform' parameters (in opengl sense) passed
    * to the triangle rasterizer and then the shader when doing 3D rendering.
    **/
    template<typename color_t_im, typename color_t_tex, typename ZBUFFER_t, typename BLEND_OP = void> struct RasterizerParams
        {
        RGBf                        facecolor;      ///< pointer to the face color (when using flat shading).  
        float                       opacity;        ///< opacity multiplier (currently used only with the 2D shader)        
        Image<color_t_im> *         im;             ///< pointer to the destination image to draw onto
        ZBUFFER_t *                 zbuf;           ///< pointer to the z buffer (when using depth testing).
        const Image<color_t_tex>*   tex;            ///< pointer to the texture (when using texturing).        
        color_t_tex                 mask_color;     ///< 'transparent color' when texture masking is enabled.
        int                         shader_type;    ///< shader type
        float                       wa;             ///< constants such that f(w) = wa * w + wb maps w (= -1/z) to float(0, 65535) for conversion to uint16_t
        float                       wb;             ///< constants such that f(w) = wa * w + wb maps w (= -1/z) to float(0, 65535) for conversion to uint16_t
        const BLEND_OP *            p_blend_op;     ///< pointer to the blending operator to use (only with the 2D shader)        
        };



}


#endif

#endif


/** end of file */


