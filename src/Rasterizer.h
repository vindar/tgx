/**   
 * @file Rasterizer.h 
 * 3D triangle rasterizer function. 
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
#ifndef _TGX_RASTERIZER_H_
#define _TGX_RASTERIZER_H_


// only C++, no plain C
#ifdef __cplusplus


#include "ShaderParams.h"

namespace tgx
{

/**
* Sub-pixel precision bits. 
* 
* Value should range between 1 and 8. Larger value provide greater resolution and
* smoother animation but at the expense of the maximum viewport size:
* 
*  | subpixel bits | max viewport size LX*LY |
*  |---------------|-------------------------|
*  |      8        |      2048 x 2048        |
*  |      6        |      4096 x 4096        |
*  |      4        |      8192 x 8192        |
*  |      2        |     16384 x 16384       |
*/
#define TGX_RASTERIZE_SUBPIXEL_BITS (6)

#define TGX_RASTERIZE_SUBPIXEL256 (1 << TGX_RASTERIZE_SUBPIXEL_BITS)
#define TGX_RASTERIZE_SUBPIXEL128 (1 << (TGX_RASTERIZE_SUBPIXEL_BITS -1))
#define TGX_RASTERIZE_MULT256(X) ((X) << (TGX_RASTERIZE_SUBPIXEL_BITS))
#define TGX_RASTERIZE_MULT128(X) ((X) << (TGX_RASTERIZE_SUBPIXEL_BITS -1))
#define TGX_RASTERIZE_DIV256(X) ((X) >> (TGX_RASTERIZE_SUBPIXEL_BITS))


    /**
    * Fast triangle rasterizer for 3D graphics:
    *
    * **Features**
    *
    * - Pixel perfect rasterization with adjustable subpixels from 2 to 8 bits  (set with `TGX_RASTERIZE_SUBPIXEL_BITS`).  
    * - Top-left rule to prevent drawing pixels twice. 
    * - Tile rasterization: large viewport can be splitted in multiple sub-images.
    * - templated shader functions so to implement: z-buffer testing, shading, texturing...
    *
    * 
    * @tparam SHADER_FUNCTION     the shader function to call.          
    * @tparam RASTERIZER_PARAMS   the type of the object containing the 'uniform' data (ie data not specific to a vertex). 
    *                       
    * @param LX, LY     Viewport size. The image itself may be smaller than the viewport and an offset may be
    *                   specified so it is possible to draw the whole viewport  in 'tile" mode be calling this method 
    *                   several times with different offsets. The maximum viewport size depend on `TGX_RASTERIZE_SUBPIXEL_BITS`                    
    *                   
    * @param V0,V1,V2   Normalized coordinates of the vertices of the triangle (x,y,z,w)  where, 'a la opengl' the viewport 
    *                   is mapped to [-1, 1]^2. These vectors also optionally contain  the 'varying' parameters associated 
    *                   with each vertex, namely the texture coords and the color associated with  each vertex (when applicable) 
    *                   that are used by the shader function.
    *                       
    * @param offset_x, offset_y     Offset of this image inside the viewport. So the image corresponds to
    *                               to the box `[offset_x, offset_x + im.width[x[offset_x, offset_x + im.height[`
    *                               and only the intersection of this box with the viewport box `[0, LX[x[0, LY[`
    *                               is drawn onto the image.
    *
    * @param data       'Uniform' parameters (depending on the rasterization type).
    *
    * @param shader_fun the shader function. see Shaders.h for the implementation of classic shaders.
    *                   
    * 
    * @remark
    *   
    * - Maximum viewport size depending on `TGX_RASTERIZE_SUBPIXEL_BITS`:   
    *  ```                                 
    *  | subpixel bits | max viewport size LX*LY |
    *  |---------------|-------------------------|
    *  |      8        |      2048 x 2048        |
    *  |      6        |      4096 x 4096        |
    *  |      4        |      8192 x 8192        |
    *  |      2        |     16384 x 16384       |
    *  ```
    *  
    * - The `(x,y)` coordinates of the vertices `V0,V1,V2` do not need to be inside the viewport `[-1,1]^2` and yet the triangle will still be
    *   perfectly rasterized provided that they are not 'too far away'. This 'too far away' correspond to the maximum viewport size 
    *   according to the chosen sub-pixel precision (for instance, [-2,2]^2 will work any viewport at most 1024x1024 when using 8 bits 
    *   precision).
    *
    * - Color are passed in RGBf format irrespectively of the image color type to improve quality and simplify handling of different image types.
    */
    template<typename SHADER_FUNCTION, typename RASTERIZER_PARAMS> 
    void rasterizeTriangle(const int LX, const int LY, const RasterizerVec4 & V0, const RasterizerVec4 & V1, const RasterizerVec4 & V2, const int32_t offset_x, const int32_t offset_y, const RASTERIZER_PARAMS & data, SHADER_FUNCTION shader_fun)
        {
        
        // assuming that clipping was already perfomed and that V0, V1, V2 are in a reasonable "range" so no overflow will occur. 
        const float mx = (float)(TGX_RASTERIZE_MULT128(LX));
        const float my = (float)(TGX_RASTERIZE_MULT128(LY));
        const iVec2  P0((int32_t)floorf(V0.x * mx), (int32_t)floorf(V0.y * my));
        const iVec2 sP1((int32_t)floorf(V1.x * mx), (int32_t)floorf(V1.y * my));
        const iVec2 sP2((int32_t)floorf(V2.x * mx), (int32_t)floorf(V2.y * my));

        const int32_t umx = min(min(P0.x, sP1.x), sP2.x);
        const int32_t uMx = max(max(P0.x, sP1.x), sP2.x);
        const int32_t umy = min(min(P0.y, sP1.y), sP2.y);
        const int32_t uMy = max(max(P0.y, sP1.y), sP2.y);

        const bool c32 = ((uMx - umx < 32768) && (uMy - umy < 32768));
        int32_t a; 
        if (c32)
            { // 32 bits computation
            a  = (((sP2.x - P0.x) * (sP1.y - P0.y)) - ((sP2.y - P0.y) * (sP1.x - P0.x))); // aera
            if (a == 0) return; // do not draw flat triangles
            }
        else
            { // 64 bits computations
            int64_t a64 = (((int64_t)(sP2.x - P0.x)) * ((int64_t)(sP1.y - P0.y))) - (((int64_t)(sP2.y - P0.y)) * ((int64_t)(sP1.x - P0.x))); // aera
            if (a64 == 0) return; // do not draw flat triangles
            a = (a64 > 0) ? 1 : -1; // real aera value does not matter, onlythe sign
            }         
        
        int32_t xmin = (umx + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // use division and not bitshift  
        int32_t xmax = (uMx + TGX_RASTERIZE_MULT128(LX)) / TGX_RASTERIZE_SUBPIXEL256; // in case values are negative.
        int32_t ymin = (umy + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //
        int32_t ymax = (uMy + TGX_RASTERIZE_MULT128(LY)) / TGX_RASTERIZE_SUBPIXEL256; //

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

        const RasterizerVec4& fP1 = (a > 0) ? V1 : V2;
        const RasterizerVec4& fP2 = (a > 0) ? V2 : V1;
        const iVec2& P1 = (a > 0) ? sP1 : sP2;
        const iVec2& P2 = (a > 0) ? sP2 : sP1;

        const int32_t us = TGX_RASTERIZE_MULT256(ox) - TGX_RASTERIZE_MULT128(LX) + TGX_RASTERIZE_SUBPIXEL128;   // start pixel position
        const int32_t vs = TGX_RASTERIZE_MULT256(oy) - TGX_RASTERIZE_MULT128(LY) + TGX_RASTERIZE_SUBPIXEL128;   //

        ox -= offset_x;
        oy -= offset_y;

        int32_t dx1 = P1.y - P0.y;
        int32_t dy1 = P0.x - P1.x;
        int32_t dx2 = P2.y - P1.y;
        int32_t dy2 = P1.x - P2.x;
        int32_t dx3 = P0.y - P2.y;
        int32_t dy3 = P2.x - P0.x;

        int32_t O1, O2, O3;

        if (c32)
            { // 32 bits computation
            O1 = ((us - P0.x) * dx1) + ((vs - P0.y) * dy1);
            if ((dx1 < 0) || ((dx1 == 0) && (dy1 < 0))) O1--; // top left rule (beware, changes total aera).

            O2 = ((us - P1.x) * dx2) + ((vs - P1.y) * dy2);
            if ((dx2 < 0) || ((dx2 == 0) && (dy2 < 0))) O2--; // top left rule (beware, changes total aera).  

            O3 = ((us - P2.x) * dx3) + ((vs - P2.y) * dy3);
            if ((dx3 < 0) || ((dx3 == 0) && (dy3 < 0))) O3--; // top left rule (beware, changes total aera).  

            dx1 *= (TGX_RASTERIZE_SUBPIXEL256);
            dy1 *= (TGX_RASTERIZE_SUBPIXEL256);
            dx2 *= (TGX_RASTERIZE_SUBPIXEL256);
            dy2 *= (TGX_RASTERIZE_SUBPIXEL256);
            dx3 *= (TGX_RASTERIZE_SUBPIXEL256);
            dy3 *= (TGX_RASTERIZE_SUBPIXEL256);
            }
        else
            { // 64 bits computation
            int64_t dO1 = (((int64_t)(us - P0.x)) * ((int64_t)dx1)) + (((int64_t)(vs - P0.y)) * ((int64_t)dy1));
            if ((dx1 < 0) || ((dx1 == 0) && (dy1 < 0))) dO1--; // top left rule (beware, changes total aera).
            O1 = (dO1 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO1)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO1 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

            int64_t dO2 = (((int64_t)(us - P1.x)) * ((int64_t)dx2)) + (((int64_t)(vs - P1.y)) * ((int64_t)dy2));
            if ((dx2 < 0) || ((dx2 == 0) && (dy2 < 0))) dO2--; // top left rule (beware, changes total aera).  
            O2 = (dO2 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO2)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO2 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));

            int64_t dO3 = (((int64_t)(us - P2.x)) * ((int64_t)dx3)) + (((int64_t)(vs - P2.y)) * ((int64_t)dy3));
            if ((dx3 < 0) || ((dx3 == 0) && (dy3 < 0))) dO3--; // top left rule (beware, changes total aera).  
            O3 = (dO3 >= 0) ? ((int32_t)TGX_RASTERIZE_DIV256(dO3)) : -((int32_t)TGX_RASTERIZE_DIV256(-dO3 + (TGX_RASTERIZE_SUBPIXEL256 - 1)));
            }

        // beware that O1 + O2 + O3 = 0 is possible now but still we should not discard the triangle:
        // this case must be dealt with inside the shader...

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



}

#endif

#endif

/** end of file */


