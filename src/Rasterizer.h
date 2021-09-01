/** @file Rasterizer.h */
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
#ifndef _TGX_RASTERIZER_H_
#define _TGX_RASTERIZER_H_


// only C++, no plain C
#ifdef __cplusplus


#include "ShaderParams.h"

namespace tgx
{

#define TGX_RASTERIZE_SUBPIXEL_BITS (8) // <- change this to adjust sub-pixel precision (between 1 and 8) UNTESTED !
#define TGX_RASTERIZE_SUBPIXEL256 (1 << TGX_RASTERIZE_SUBPIXEL_BITS)
#define TGX_RASTERIZE_SUBPIXEL128 (1 << (TGX_RASTERIZE_SUBPIXEL_BITS -1))
#define TGX_RASTERIZE_MULT256(X) ((X) << (TGX_RASTERIZE_SUBPIXEL_BITS))
#define TGX_RASTERIZE_MULT128(X) ((X) << (TGX_RASTERIZE_SUBPIXEL_BITS -1))
#define TGX_RASTERIZE_DIV256(X) ((X) >> (TGX_RASTERIZE_SUBPIXEL_BITS))


	/**
	* Main method for rasterizing a triangle onto the image for 3D graphics:
	*
	* FEATURES:
	*
	* - Pixel perfect rasterization with adjustable subpixel precisions (8 bit default).
	*
	* - Tile rasterization: large viewport can be splitted in multiple sub-images.
	*
	* - Zbuffer depth testing
	*
	* - Perspective/Orthographic projection rasterization.
	*
	* - Flat shading / gouraud shading.
	*
	* - Perspective correct texture mapping
	*
	* TEMPLATE PARAMETERS :
	*
	* - (LX, LY)     the viewport size (at most 2048x2048 when using 8bit subpixels).
	*                The image itself may be smaller than the viewport and an offset may be
	*                specified so it is possible to draw the whole viewport  in 'tile" mode
	*                be calling this method several time with different offsets.
	*
	* - ZBUFFER : if true, use zbuffer depth test. The buffer must be passed in data.zbuf
	*             and must be an array of size at least LX*LY floats.
	*
	* - ORTHO : if true, use orthographic rasterization (which differs from projective
	*           rasterization when doing texture mapping).
	*
	* PARAMETERS:
	*
	* - raster_type  specify the type of shader to use during rasterization. Combination of
	*                TGX_SHADER_FLAT, TGX_SHADER_GOURAUD, TGX_SHADER_TEXTURE
	*
	*                - TGX_SHADER_FLAT: Use flat shading (uniform color on the triangle).
	*                  the triangle color is passed in data.facecolor.
	*
	*                - TGX_SHADER_GOURAUD: Overwrites the TGX_SHADER_FLAT flag and enables gouraud
	*                  shading which correspond to linear interpolation of the triangle vertices
	*                  colors. the color of each vertex is passed in V.color
	*
	*				 - TGX_SHADER_TEXTURE: Enable perspective correct texture mapping on the
	*                  triangle. the texture image is passed in data.tex. THE DIMENSION OF
	*                  THE TEXTURE MUST BE POWERS OF TWO ! the texture coordinate for each
	*                  vertex are passed in V.T members and the 'illumination' for each vertex
	*                  is passed in V.color.
	*
	* - V0,V1,V2     Normalized coordinates of the vertices of the triangle (x,y,z,w)  where,
	*                'a la opengl' the viewport is mapped to [-1, 1]^2.
	*                The projective w coordinate should contains 1/z when using perspective
	*                projection (ORTHO=false) and should contain 2 - z when using orthographic
	*                projection (ORTHO = true). This value is used for depth testing when
	*                ZBUFFER = true. These vectors also optionally contain  the 'varying'
	*                parameters associated with each vertex, namely the  texture coords.
	*                and the color associated with each vertex (when applicable).
	*
	*			     NOTE: the (x,y) coordinates of the vertices V0,V1,V2 do not need to
	*                be inside the viewport [-1.0,1.0]^2 and the triangle will still be
	*                perfectly rasterized provided that they are not 'too far away'. This
	*                'too far away' depends on the viewport size and the number of subpixel
	*                bits. For example:
	*
	*                - [-2.0,2.0]^2 will work for any viewport at most 1024x1024.
	*
	*                - For a viewport of size 320x240 and 8bit subpixels, it suffices for the
	*                  coordinates stay inside [-6.0, 6.0].
	*
	* - offset_x, offset_y  Offset of this image inside the viewport. So the image corresponds to
	*                       to the box  [offset_x, offset_x + im.width[x[offset_x, offset_x + im.height[
	*                       and only the intersection of this box with the viewport box [0, LX[x[0, LY[
	*                       is drawn onto the image.
	*
	* - data : contain the 'uniform' parameters depending on the rasterization type.
	*
	* REMARKS: color are passed in RGBf format irrespectively of the image color type to improve
	*          quality and simplify handling of different image types.
	**/
	
	template<int LX, int LY, typename SHADER_FUNCTION, typename RASTERIZER_PARAMS> 
	void rasterizeTriangle(const RasterizerVec4 & V0, const RasterizerVec4 & V1, const RasterizerVec4 & V2, const int32_t offset_x, const int32_t offset_y, const RASTERIZER_PARAMS & data, SHADER_FUNCTION shader_fun)
		{

		// assuming that clipping was already perfomed and that V0, V1, V2 are in a reasonable "range" so no overflow will occur. 
		const float mx = (float)(TGX_RASTERIZE_MULT128(LX));
		const float my = (float)(TGX_RASTERIZE_MULT128(LY));
		const iVec2  P0((int32_t)floorf(V0.x * mx), (int32_t)floorf(V0.y * my));

		const iVec2 sP1((int32_t)floorf(V1.x * mx), (int32_t)floorf(V1.y * my));
		const iVec2 sP2((int32_t)floorf(V2.x * mx), (int32_t)floorf(V2.y * my));

		int32_t xmin = (min(min(P0.x, sP1.x), sP2.x) + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // use division and not bitshift  
		int32_t xmax = (max(max(P0.x, sP1.x), sP2.x) + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // in case values are negative.
		int32_t ymin = (min(min(P0.y, sP1.y), sP2.y) + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //
		int32_t ymax = (max(max(P0.y, sP1.y), sP2.y) + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //

		// intersect the sub-image with the triangle bounding box. 			
		int32_t sx = data.im->lx();
		int32_t sy = data.im->ly();
		int32_t ox = offset_x;
		int32_t oy = offset_y;
		if (ox < xmin) { sx -= (xmin - ox); ox = xmin; }
		if (ox + sx > xmax) { sx = xmax - ox + 1; }
		if (sx <= 0) return;
		if (oy < ymin) { sy -= (ymin - oy); oy = ymin; }
		if (oy + sy > ymax) { sy = ymax - oy + 1; }
		if (sy <= 0) return;

		const int64_t a = (((int64_t)(sP2.x - P0.x)) * ((int64_t)(sP1.y - P0.y))) - (((int64_t)(sP2.y - P0.y)) * ((int64_t)(sP1.x - P0.x))); // aera

		if (a == 0) return; // do not draw flat triangles

		const RasterizerVec4& fP1 = (a > 0) ? V1 : V2;
		const RasterizerVec4& fP2 = (a > 0) ? V2 : V1;
		const iVec2& P1 = (a > 0) ? sP1 : sP2;
		const iVec2& P2 = (a > 0) ? sP2 : sP1;

		const int32_t us = TGX_RASTERIZE_MULT256(ox) - TGX_RASTERIZE_MULT128(LX) + TGX_RASTERIZE_SUBPIXEL128;   // start pixel position
		const int32_t vs = TGX_RASTERIZE_MULT256(oy) - TGX_RASTERIZE_MULT128(LY) + TGX_RASTERIZE_SUBPIXEL128;   //

		ox -= offset_x;
		oy -= offset_y;

		const int32_t dx1 = P1.y - P0.y;
		const int32_t dy1 = P0.x - P1.x;
		int64_t dO1 = (((int64_t)(us - P0.x)) * ((int64_t)dx1)) + (((int64_t)(vs - P0.y)) * ((int64_t)dy1));
		if ((dx1 < 0) || ((dx1 == 0) && (dy1 < 0))) dO1--; // top left rule (beware, changes total aera).
		int32_t O1 = (dO1 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO1)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO1 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		const int32_t dx2 = P2.y - P1.y;
		const int32_t dy2 = P1.x - P2.x;
		int64_t dO2 = (((int64_t)(us - P1.x)) * ((int64_t)dx2)) + (((int64_t)(vs - P1.y)) * ((int64_t)dy2));
		if ((dx2 < 0) || ((dx2 == 0) && (dy2 < 0))) dO2--; // top left rule (beware, changes total aera).  
		int32_t O2 = (dO2 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO2)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO2 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		const int32_t dx3 = P0.y - P2.y;
		const int32_t dy3 = P2.x - P0.x;
		int64_t dO3 = (((int64_t)(us - P2.x)) * ((int64_t)dx3)) + (((int64_t)(vs - P2.y)) * ((int64_t)dy3));
		if ((dx3 < 0) || ((dx3 == 0) && (dy3 < 0))) dO3--; // top left rule (beware, changes total aera).  
		int32_t O3 = (dO3 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO3)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO3 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		if (sx == 1)
			{
			while (((O1 | O2 | O3) < 0) && (sy > 0))
				{
				sy--;
				oy++;
				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				}
			if (sy == 0) return;
			}
		else if (sy == 1)
			{
			while (((O1 | O2 | O3) < 0) && (sx > 0))
				{
				sx--;
				ox++;
				O1 += dx1;
				O2 += dx2;
				O3 += dx3;
				}
			if (sx == 0) return;
			}

		if (dx1 > 0)
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx1, dy1, O1, fP2,
				dx2, dy2, O2, V0,
				dx3, dy3, O3, fP1,
				data);
			}
		else if (dx2 > 0)
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx2, dy2, O2, V0,
				dx3, dy3, O3, fP1,
				dx1, dy1, O1, fP2,
				data);
			}
		else
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx3, dy3, O3, fP1,
				dx1, dy1, O1, fP2,
				dx2, dy2, O2, V0,
				data);
			}
		return;
		}






	/** UNTEMPLATED VERSION : SLOWER... */
	template<typename SHADER_FUNCTION, typename RASTERIZER_PARAMS> 
	void rasterizeTriangle(const int LX, const int LY, const RasterizerVec4 & V0, const RasterizerVec4 & V1, const RasterizerVec4 & V2, const int32_t offset_x, const int32_t offset_y, const RASTERIZER_PARAMS & data, SHADER_FUNCTION shader_fun)
		{
		// assuming that clipping was already perfomed and that V0, V1, V2 are in a reasonable "range" so no overflow will occur. 
		const float mx = (float)(TGX_RASTERIZE_MULT128(LX));
		const float my = (float)(TGX_RASTERIZE_MULT128(LY));
		const iVec2  P0((int32_t)floorf(V0.x * mx), (int32_t)floorf(V0.y * my));

		const iVec2 sP1((int32_t)floorf(V1.x * mx), (int32_t)floorf(V1.y * my));
		const iVec2 sP2((int32_t)floorf(V2.x * mx), (int32_t)floorf(V2.y * my));

		int32_t xmin = (min(min(P0.x, sP1.x), sP2.x) + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // use division and not bitshift  
		int32_t xmax = (max(max(P0.x, sP1.x), sP2.x) + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // in case values are negative.
		int32_t ymin = (min(min(P0.y, sP1.y), sP2.y) + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //
		int32_t ymax = (max(max(P0.y, sP1.y), sP2.y) + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //

		// intersect the sub-image with the triangle bounding box. 			
		int32_t sx = data.im->lx();
		int32_t sy = data.im->ly();
		int32_t ox = offset_x;
		int32_t oy = offset_y;
		if (ox < xmin) { sx -= (xmin - ox); ox = xmin; }
		if (ox + sx > xmax) { sx = xmax - ox + 1; }
		if (sx <= 0) return;
		if (oy < ymin) { sy -= (ymin - oy); oy = ymin; }
		if (oy + sy > ymax) { sy = ymax - oy + 1; }
		if (sy <= 0) return;

		const int64_t a = (((int64_t)(sP2.x - P0.x)) * ((int64_t)(sP1.y - P0.y))) - (((int64_t)(sP2.y - P0.y)) * ((int64_t)(sP1.x - P0.x))); // aera

		if (a == 0) return; // do not draw flat triangles

		const RasterizerVec4& fP1 = (a > 0) ? V1 : V2;
		const RasterizerVec4& fP2 = (a > 0) ? V2 : V1;
		const iVec2& P1 = (a > 0) ? sP1 : sP2;
		const iVec2& P2 = (a > 0) ? sP2 : sP1;

		const int32_t us = TGX_RASTERIZE_MULT256(ox) - TGX_RASTERIZE_MULT128(LX) + TGX_RASTERIZE_SUBPIXEL128;   // start pixel position
		const int32_t vs = TGX_RASTERIZE_MULT256(oy) - TGX_RASTERIZE_MULT128(LY) + TGX_RASTERIZE_SUBPIXEL128;   //

		ox -= offset_x;
		oy -= offset_y;

		const int32_t dx1 = P1.y - P0.y;
		const int32_t dy1 = P0.x - P1.x;
		int64_t dO1 = (((int64_t)(us - P0.x)) * ((int64_t)dx1)) + (((int64_t)(vs - P0.y)) * ((int64_t)dy1));
		if ((dx1 < 0) || ((dx1 == 0) && (dy1 < 0))) dO1--; // top left rule (beware, changes total aera).
		int32_t O1 = (dO1 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO1)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO1 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		const int32_t dx2 = P2.y - P1.y;
		const int32_t dy2 = P1.x - P2.x;
		int64_t dO2 = (((int64_t)(us - P1.x)) * ((int64_t)dx2)) + (((int64_t)(vs - P1.y)) * ((int64_t)dy2));
		if ((dx2 < 0) || ((dx2 == 0) && (dy2 < 0))) dO2--; // top left rule (beware, changes total aera).  
		int32_t O2 = (dO2 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO2)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO2 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		const int32_t dx3 = P0.y - P2.y;
		const int32_t dy3 = P2.x - P0.x;
		int64_t dO3 = (((int64_t)(us - P2.x)) * ((int64_t)dx3)) + (((int64_t)(vs - P2.y)) * ((int64_t)dy3));
		if ((dx3 < 0) || ((dx3 == 0) && (dy3 < 0))) dO3--; // top left rule (beware, changes total aera).  
		int32_t O3 = (dO3 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO3)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO3 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

		if (sx == 1)
			{
			while (((O1 | O2 | O3) < 0) && (sy > 0))
				{
				sy--;
				oy++;
				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				}
			if (sy == 0) return;
			}
		else if (sy == 1)
			{
			while (((O1 | O2 | O3) < 0) && (sx > 0))
				{
				sx--;
				ox++;
				O1 += dx1;
				O2 += dx2;
				O3 += dx3;
				}
			if (sx == 0) return;
			}

		if (dx1 > 0)
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx1, dy1, O1, fP2,
				dx2, dy2, O2, V0,
				dx3, dy3, O3, fP1,
				data);
			}
		else if (dx2 > 0)
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx2, dy2, O2, V0,
				dx3, dy3, O3, fP1,
				dx1, dy1, O1, fP2,
				data);
			}
		else
			{
			shader_fun(ox + (data.im->stride() * oy), sx, sy,
				dx3, dy3, O3, fP1,
				dx1, dy1, O1, fP2,
				dx2, dy2, O2, V0,
				data);
			}
		return;
		}




#undef TGX_RASTERIZE_SUBPIXEL_BITS
#undef TGX_RASTERIZE_SUBPIXEL256
#undef TGX_RASTERIZE_SUBPIXEL128
#undef TGX_RASTERIZE_MULT256
#undef TGX_RASTERIZE_MULT128
#undef TGX_RASTERIZE_DIV256



}

#endif

#endif

/** end of file */


