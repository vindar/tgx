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

	#define	TGX_SHADER_FLAT (0)				// flat shading
	#define TGX_SHADER_GOURAUD (1)			// gouraud shading, this flag overwrites flat shading
	#define TGX_SHADER_TEXTURE (2)			// use texture mapping,: can be combined with either TGX_SHADER_FLAT or TGX_SHADER_GOURAUD


	// macro to test if a shader has a given flag
	#define TGX_SHADER_HAS_FLAT(shader_type)		(~(shader_type & TGX_SHADER_GOURAUD))
	#define TGX_SHADER_HAS_GOURAUD(shader_type)		(shader_type & TGX_SHADER_GOURAUD)
	#define TGX_SHADER_HAS_TEXTURE(shader_type)		(shader_type & TGX_SHADER_TEXTURE)

	// macro to set,add and remove shader flags
	#define TGX_SHADER_SET(shader_type, flags) { shader_type = flags; }
	#define TGX_SHADER_ADD_FLAT(shader_type) { shader_type &= ~(TGX_SHADER_GOURAUD); }
	#define TGX_SHADER_ADD_GOURAUD(shader_type) { shader_type |= TGX_SHADER_GOURAUD; }
	#define TGX_SHADER_ADD_TEXTURE(shader_type) { shader_type |= TGX_SHADER_TEXTURE; }
	#define TGX_SHADER_ADD_BILINEAR(shader_type) { shader_type |= TGX_SHADER_BILINEAR; }
	#define TGX_SHADER_REMOVE_GOURAUD(shader_type) { shader_type &= ~(TGX_SHADER_GOURAUD); }
	#define TGX_SHADER_REMOVE_TEXTURE(shader_type) { shader_type &= ~(TGX_SHADER_TEXTURE); }


	//forward declaration
	template<typename color_t> class Image;


	/**
	* Vertex parameter
	*
	* Extension of the fVec4 class that holds the 'varying' parameters (in the OpenGL sense)
	* associated with a vertex and passed to the shader method
	**/
	struct RasterizerVec4 : public tgx::fVec4
		{
		tgx::RGBf color;     // vertex color for gouraud shading (or light intensity for gouraud shading with texturing). 
		tgx::fVec2 T;        // texture coordinates if applicable 
		float A;			 // alpha value (if used). 
		};



	/**
	* Uniform parameters
	*
	* Structure that holds the 'uniform' parameters (in opengl sense) passed
	* to the triangle rasterizer when doing 3D rendering
	**/
	template<typename color_t_im, typename color_t_tex> struct RasterizerParams
		{
		int shader_type;				// shader type
		Image<color_t_im> * im;			// pointer to the destination image to draw onto
		float* zbuf;					// pointer to the z buffer (when using depth testing).
		RGBf facecolor;					// pointer to the face color (when using flat shading).  
		float opacity;					// opacity multiplier (currently used only with the 2D shader)
		const Image<color_t_tex>* tex;	// pointer to the texture (when using texturing).
        bool use_bilinear_texturing;    // true to use bilinear point sampling (when using texturing).
		color_t_tex mask_color;			// 'transparent color' when masking is enabled (on for the 2D shader).
		};



}


#endif

#endif


/** end of file */


