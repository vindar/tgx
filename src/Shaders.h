/**   
 * @file Shaders.h 
 * Triangle shader functions. 
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
#ifndef _TGX_SHADERS_H_
#define _TGX_SHADERS_H_


// only C++, no plain C
#ifdef __cplusplus


#include "ShaderParams.h"

namespace tgx
{

    /** for texture clamping */
    inline TGX_INLINE int shaderclip(int v, int maxv)
        {
        return ((v < 0) ? 0 : ((v > maxv) ? maxv : v));
        }



    /**
    * For test purposes... 
    **/
    template<typename color_t, typename ZBUFFER_t>
    void shader_test(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t& dx1, const int32_t& dy1, int32_t O1, const tgx::RasterizerVec4& fP1,
        const int32_t& dx2, const int32_t& dy2, int32_t O2, const tgx::RasterizerVec4& fP2,
        const int32_t& dx3, const int32_t& dy3, int32_t O3, const tgx::RasterizerVec4& fP3,
        const tgx::RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t col = (color_t)tgx::RGB32_Red; //data.facecolor; 
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();
        for (int y = 0; y < ly; y++)
            {
            for (int x = 0; x < lx; x++)
                {
                const int32_t o1 = O1 + dx1 * x + dy1 * y;
                const int32_t o2 = O2 + dx2 * x + dy2 * y;
                const int32_t o3 = O3 + dx3 * x + dy3 * y;
                if ((o1 >= 0) && (o2 >= 0) && (o3 >= 0))
                    {
                    buf[x + stride * y].blend256(col, 128);                
                    }
                }
            }
        }


    /**
    * FLAT SHADING (NO ZBUFFER)
    **/
    template<typename color_t, typename ZBUFFER_t>
    void shader_Flat(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t col = (color_t)data.facecolor;
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                buf[bx] = col;
                //buf[bx].blend256(col, 128) // for testing. 
                C2 += dx2;
                C3 += dx3;
                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * GOURAUD SHADING (NO Z BUFFER)
    **/
    template<typename color_t, typename ZBUFFER_t>
    void shader_Gouraud(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        const color_t col1 = (color_t)fP1.color;
        const color_t col2 = (color_t)fP2.color;
        const color_t col3 = (color_t)fP3.color;

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                buf[bx] = interpolateColorsTriangle(col2, C2, col3, C3, col1, aera);
                C2 += dx2;
                C3 += dx3;
                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * TEXTURE + FLAT SHADING (NO ZBUFFER)
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Flat_Texture(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;


        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf = (RGBf)data.facecolor;
        const int fPR = (int)(256 * cf.R);
        const int fPG = (int)(256 * cf.G);
        const int fPB = (int)(256 * cf.B);

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by z * aera
        T1 *= fP1a;
        T2 *= fP2a;
        T3 *= fP3a;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                const float icw = fast_inv(cw);
                                
                color_t col;
                if (TEXTURE_BILINEAR)
                    {
                    const float xx = tx * icw;
                    const float yy = ty * icw;                    
                    const int ttx = (int)floorf(xx);
                    const int tty = (int)floorf(yy);
                    const float ax = xx - ttx;
                    const float ay = yy - tty;                    
                    const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                    const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                    const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm))*texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                    const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm))*texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                    col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                    }
                else
                    {
                    const int ttx = (TEXTURE_WRAP ? ((int)((tx * icw))) & (texsize_x_mm) : shaderclip((int)(tx * icw), texsize_x_mm));
                    const int tty = (TEXTURE_WRAP ? ((int)((ty * icw))) & (texsize_y_mm) : shaderclip((int)(ty * icw), texsize_y_mm));
                    col = tex[ttx + (tty)*texstride];
                    }                  
                                
                col.mult256(fPR, fPG, fPB);
                buf[bx] = col;

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * TEXTURE + GOURAUD SHADING (NO ZBUFFER)
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Gouraud_Texture(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
       
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf1 = (RGBf)fP1.color;
        const RGBf& cf2 = (RGBf)fP2.color;
        const RGBf& cf3 = (RGBf)fP3.color;
        const int fP1R = (int)(256 * cf1.R);
        const int fP1G = (int)(256 * cf1.G);
        const int fP1B = (int)(256 * cf1.B);
        const int fP21R = (int)(256 * (cf2.R - cf1.R));
        const int fP21G = (int)(256 * (cf2.G - cf1.G));
        const int fP21B = (int)(256 * (cf2.B - cf1.B));
        const int fP31R = (int)(256 * (cf3.R - cf1.R));
        const int fP31G = (int)(256 * (cf3.G - cf1.G));
        const int fP31B = (int)(256 * (cf3.B - cf1.B));

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by z * aera
        T1 *= fP1a;
        T2 *= fP2a;
        T3 *= fP3a;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;


        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                const float icw = fast_inv(cw);
                
                color_t col;
                if (TEXTURE_BILINEAR)
                    {
                    const float xx = tx * icw;
                    const float yy = ty * icw;                    
                    const int ttx = (int)floorf(xx);
                    const int tty = (int)floorf(yy);
                    const float ax = xx - ttx;
                    const float ay = yy - tty;  
                    const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                    const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                    const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                    const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                    col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                    }
                else
                    {
                    const int ttx = (TEXTURE_WRAP ? ((int)((tx * icw))) & (texsize_x_mm) : shaderclip((int)(tx * icw), texsize_x_mm));
                    const int tty = (TEXTURE_WRAP ? ((int)((ty * icw))) & (texsize_y_mm) : shaderclip((int)(ty * icw), texsize_y_mm));
                    col = tex[ttx + (tty)*texstride];
                    }
                    
                const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);

                col.mult256(r, g, b);
                buf[bx] = col;

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * ZBUFFER + FLAT SHADING
    **/
    template<typename color_t, typename ZBUFFER_t>
    void shader_Flat_Zbuffer(const int32_t offset, const int32_t& lx, const int32_t& ly,
        const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        const color_t col = (color_t)data.facecolor;
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t * zbuf = data.zbuf + offset;

        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float wa = data.wa;
        const float wb = data.wb;
        const float invaera = fast_inv((float)aera);
        const float invaera_wa = invaera * wa;
        const float fP1a = fP1.w * invaera_wa;
        const float fP2a = fP2.w * invaera_wa;
        const float fP3a = fP3.w * invaera_wa;
        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            const int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a)) + wb;

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (ZBUFFER_t)(cw);
                if (W < aa)
                    {
                    W = aa;
                    buf[bx] = col;
                    }
                C2 += dx2;
                C3 += dx3;
                cw += dw;
                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }



    /**
    * ZBUFFER + GOURAUD SHADING
    **/
    template<typename color_t, typename ZBUFFER_t>
    void shader_Gouraud_Zbuffer(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t* zbuf = data.zbuf + offset;


        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const color_t col1 = (color_t)fP1.color;
        const color_t col2 = (color_t)fP2.color;
        const color_t col3 = (color_t)fP3.color;

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float wa = data.wa;
        const float wb = data.wb;
        const float invaera = fast_inv((float)aera);
        const float invaera_wa = invaera * wa;
        const float fP1a = fP1.w * invaera_wa;
        const float fP2a = fP2.w * invaera_wa;
        const float fP3a = fP3.w * invaera_wa;
        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            const int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a)) + wb;

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (ZBUFFER_t)(cw);
                if (W < aa)
                    {
                    W = aa;
                    buf[bx] = interpolateColorsTriangle(col2, C2, col3, C3, col1, aera);
                    }
                C2 += dx2;
                C3 += dx3;
                cw += dw;
                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }


    /**
    * ZBUFFER + TEXTURE + FLAT SHADING
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Flat_Texture_Zbuffer(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {      
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t* zbuf = data.zbuf + offset;

        const float wa = data.wa;
        const float wb = data.wb;
        (void)wa; // silence possible unused warnings
        (void)wb; // 


        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf = (RGBf)data.facecolor;
        const int fPR = (int)(256 * cf.R);
        const int fPG = (int)(256 * cf.G);
        const int fPB = (int)(256 * cf.B);

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by z * aera
        T1 *= fP1a;
        T2 *= fP2a;
        T3 *= fP3a;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(cw * wa + wb)) : ((ZBUFFER_t)cw);
                if (W < aa)
                    {
                    W = aa;
                    const float icw = fast_inv(cw);
                    color_t col;
                    if (TEXTURE_BILINEAR)
                        {
                        const float xx = tx * icw;
                        const float yy = ty * icw;                    
                        const int ttx = (int)floorf(xx);
                        const int tty = (int)floorf(yy);
                        const float ax = xx - ttx;
                        const float ay = yy - tty;       
                        const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                        const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                        const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                        const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                        col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                        }
                    else
                        {
                        const int ttx = (TEXTURE_WRAP ? ((int)((tx * icw))) & (texsize_x_mm) : shaderclip((int)(tx * icw), texsize_x_mm));
                        const int tty = (TEXTURE_WRAP ? ((int)((ty * icw))) & (texsize_y_mm) : shaderclip((int)(ty * icw), texsize_y_mm));                                          
                        col = tex[ttx + (tty)*texstride];                           
                        }  
                    
                    col.mult256(fPR, fPG, fPB);
                    buf[bx] = col;
                    }

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }



    /**
    * ZBUFFER + TEXTURE + GOURAUD SHADING
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Gouraud_Texture_Zbuffer(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {       
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t* zbuf = data.zbuf + offset;

        const float wa = data.wa;
        const float wb = data.wb;
        (void)wa; // silence possible unused warnings
        (void)wb; // 

        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf1 = (RGBf)fP1.color;
        const RGBf& cf2 = (RGBf)fP2.color;
        const RGBf& cf3 = (RGBf)fP3.color;
        const int fP1R = (int)(256 * cf1.R);
        const int fP1G = (int)(256 * cf1.G);
        const int fP1B = (int)(256 * cf1.B);
        const int fP21R = (int)(256 * (cf2.R - cf1.R));
        const int fP21G = (int)(256 * (cf2.G - cf1.G));
        const int fP21B = (int)(256 * (cf2.B - cf1.B));
        const int fP31R = (int)(256 * (cf3.R - cf1.R));
        const int fP31G = (int)(256 * (cf3.G - cf1.G));
        const int fP31B = (int)(256 * (cf3.B - cf1.B));

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by z * aera
        T1 *= fP1a;
        T2 *= fP2a;
        T3 *= fP3a;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }
            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(cw * wa + wb)) : ((ZBUFFER_t)cw);
                if (W < aa)
                    {
                    W = aa;
                    const float icw = fast_inv(cw);

                    color_t col;
                    if (TEXTURE_BILINEAR)
                        {
                        const float xx = tx * icw;
                        const float yy = ty * icw;                    
                        const int ttx = (int)floorf(xx);
                        const int tty = (int)floorf(yy);
                        const float ax = xx - ttx;
                        const float ay = yy - tty;                    
                        const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                        const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                        const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                        const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                        col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                        }
                    else
                        {
                        const int ttx = (TEXTURE_WRAP ? ((int)((tx * icw))) & (texsize_x_mm) : shaderclip((int)(tx * icw), texsize_x_mm));
                        const int tty = (TEXTURE_WRAP ? ((int)((ty * icw))) & (texsize_y_mm) : shaderclip((int)(ty * icw), texsize_y_mm));
                        col = tex[ttx + (tty)*texstride];
                        }  

                    const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                    const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                    const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);
                    col.mult256(r, g, b);
                    buf[bx] = col;
                    }

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }



    /**
    * TEXTURE + FLAT SHADING (NO ZBUFFER) + ORTHOGRAPHIC
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Flat_Texture_Ortho(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {       
        color_t* buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);

        const RGBf& cf = (RGBf)data.facecolor;
        const int fPR = (int)(256 * cf.R);
        const int fPG = (int)(256 * cf.G);
        const int fPB = (int)(256 * cf.B);

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }
            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {

                color_t col;
                if (TEXTURE_BILINEAR)
                    {
                    const float xx = tx;
                    const float yy = ty;                    
                    const int ttx = (int)floorf(xx);
                    const int tty = (int)floorf(yy);
                    const float ax = xx - ttx;
                    const float ay = yy - tty;                    
                    const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                    const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                    const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                    const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                    col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                    }
                else
                    {
                    const int ttx = (TEXTURE_WRAP ? ((int)((tx))) & (texsize_x_mm) : shaderclip((int)(tx), texsize_x_mm));
                    const int tty = (TEXTURE_WRAP ? ((int)((ty))) & (texsize_y_mm) : shaderclip((int)(ty), texsize_y_mm));
                    col = tex[ttx + (tty)*texstride];
                    }  
                        
                col.mult256(fPR, fPG, fPB);
                buf[bx] = col;

                C2 += dx2;
                C3 += dx3;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * TEXTURE + GOURAUD SHADING (NO ZBUFFER) + ORTHOGRAPHIC
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Gouraud_Texture_Ortho(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {        
        color_t* buf = data.im->data() + offset;        
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);

        const RGBf& cf1 = (RGBf)fP1.color;
        const RGBf& cf2 = (RGBf)fP2.color;
        const RGBf& cf3 = (RGBf)fP3.color;
        const int fP1R = (int)(256 * cf1.R);
        const int fP1G = (int)(256 * cf1.G);
        const int fP1B = (int)(256 * cf1.B);
        const int fP21R = (int)(256 * (cf2.R - cf1.R));
        const int fP21G = (int)(256 * (cf2.G - cf1.G));
        const int fP21B = (int)(256 * (cf2.B - cf1.B));
        const int fP31R = (int)(256 * (cf3.R - cf1.R));
        const int fP31G = (int)(256 * (cf3.G - cf1.G));
        const int fP31B = (int)(256 * (cf3.B - cf1.B));

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }
                
            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                            
                color_t col;
                if (TEXTURE_BILINEAR)
                    {
                    const float xx = tx;
                    const float yy = ty;                    
                    const int ttx = (int)floorf(xx);
                    const int tty = (int)floorf(yy);
                    const float ax = xx - ttx;
                    const float ay = yy - tty;                    
                    const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                    const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                    const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                    const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                    col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                    }
                else
                    {
                    const int ttx = (TEXTURE_WRAP ? ((int)((tx))) & (texsize_x_mm) : shaderclip((int)(tx), texsize_x_mm));
                    const int tty = (TEXTURE_WRAP ? ((int)((ty))) & (texsize_y_mm) : shaderclip((int)(ty), texsize_y_mm));                  
                    col = tex[ttx + (tty)*texstride];
                    }
                           
                const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);
                col.mult256(r, g, b);
                buf[bx] = col;

                C2 += dx2;
                C3 += dx3;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * ZBUFFER + TEXTURE + FLAT SHADING + ORTHOGRAPHIC
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Flat_Texture_Zbuffer_Ortho(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t* zbuf = data.zbuf + offset;

        const float wa = data.wa;
        const float wb = data.wb;
        (void)wa; // silence possible unused warnings
        (void)wb; // 

        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf = (RGBf)data.facecolor;
        const int fPR = (int)(256 * cf.R);
        const int fPG = (int)(256 * cf.G);
        const int fPB = (int)(256 * cf.B);

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(cw * wa + wb)) : ((ZBUFFER_t)cw);
                if (W < aa)
                    {
                    W = aa; 

                    color_t col;
                    if (TEXTURE_BILINEAR)
                        {
                        const float xx = tx;
                        const float yy = ty;                    
                        const int ttx = (int)floorf(xx);
                        const int tty = (int)floorf(yy);
                        const float ax = xx - ttx;
                        const float ay = yy - tty;    
                        const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                        const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                        const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                        const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                        col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                        }
                    else
                        {
                        const int ttx = (TEXTURE_WRAP ? ((int)((tx))) & (texsize_x_mm) : shaderclip((int)(tx), texsize_x_mm));
                        const int tty = (TEXTURE_WRAP ? ((int)((ty))) & (texsize_y_mm) : shaderclip((int)(ty), texsize_y_mm));
                        col = tex[ttx + (tty)*texstride];
                        }                            
                                                        
                    col.mult256(fPR, fPG, fPB);
                    buf[bx] = col;
                    }

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }



    /**
    * ZBUFFER + TEXTURE + GOURAUD SHADING + ORTHOGRAPHIC
    **/
    template<typename color_t, typename ZBUFFER_t, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP>
    void shader_Gouraud_Texture_Zbuffer_Ortho(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t* buf = data.im->data() + offset;
        ZBUFFER_t* zbuf = data.zbuf + offset;

        const float wa = data.wa;
        const float wb = data.wb;
        (void)wa; // silence possible unused warnings
        (void)wb; // 

        const int32_t stride = data.im->stride();
        const int32_t zstride = data.im->lx();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);
        const float fP1a = fP1.w * invaera;
        const float fP2a = fP2.w * invaera;
        const float fP3a = fP3.w * invaera;

        const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

        const RGBf& cf1 = (RGBf)fP1.color;
        const RGBf& cf2 = (RGBf)fP2.color;
        const RGBf& cf3 = (RGBf)fP3.color;
        const int fP1R = (int)(256 * cf1.R);
        const int fP1G = (int)(256 * cf1.G);
        const int fP1B = (int)(256 * cf1.B);
        const int fP21R = (int)(256 * (cf2.R - cf1.R));
        const int fP21G = (int)(256 * (cf2.G - cf1.G));
        const int fP21B = (int)(256 * (cf2.B - cf1.B));
        const int fP31R = (int)(256 * (cf3.R - cf1.R));
        const int fP31G = (int)(256 * (cf3.G - cf1.G));
        const int fP31B = (int)(256 * (cf3.B - cf1.B));

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t* tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = texsize_x - 1;
        const int32_t texsize_y_mm = texsize_y - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    zbuf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));

            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                ZBUFFER_t& W = zbuf[bx];
                const ZBUFFER_t aa = (std::is_same<ZBUFFER_t, uint16_t>::value) ? ((ZBUFFER_t)(cw * wa + wb)) : ((ZBUFFER_t)cw);
                if (W < aa)
                    {
                    W = aa;

                    color_t col;
                    if (TEXTURE_BILINEAR)
                        {
                        const float xx = tx;
                        const float yy = ty;                    
                        const int ttx = (int)floorf(xx);
                        const int tty = (int)floorf(yy);                    
                        const float ax = xx - ttx;
                        const float ay = yy - tty;
                        const int minx = (TEXTURE_WRAP ? (ttx & (texsize_x_mm)) : shaderclip(ttx, texsize_x_mm));
                        const int maxx = (TEXTURE_WRAP ? ((ttx + 1) & (texsize_x_mm)) : shaderclip(ttx + 1, texsize_x_mm));
                        const int miny = (TEXTURE_WRAP ? ((tty & (texsize_y_mm)) * texstride) : shaderclip(tty, texsize_y_mm) * texstride);
                        const int maxy = (TEXTURE_WRAP ? (((tty + 1) & (texsize_y_mm)) * texstride) : shaderclip(tty + 1, texsize_y_mm) * texstride);
                        col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);                            
                        }
                    else
                        {
                        const int ttx = (TEXTURE_WRAP ? ((int)((tx))) & (texsize_x_mm) : shaderclip((int)(tx), texsize_x_mm));
                        const int tty = (TEXTURE_WRAP ? ((int)((ty))) & (texsize_y_mm) : shaderclip((int)(ty), texsize_y_mm));
                        col = tex[ttx + (tty)*texstride];
                        } 
                                
                    const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                    const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                    const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);

                    col.mult256(r, g, b);
                    buf[bx] = col;

                    }

                C2 += dx2;
                C3 += dx3;
                cw += dw;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            zbuf += zstride;
            }
        }




    /**
    * META-Shader THAT DISPATCH TO THE CORRECT Shader ABOVE (IF ENABLED).
    **/
    template<int SHADER_FLAGS_ENABLED, typename color_t, typename ZBUFFER_t> void shader_select(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t> & data)
        {       
        int raster_type = data.shader_type;       
        if (TGX_SHADER_HAS_ZBUFFER(SHADER_FLAGS_ENABLED) && (TGX_SHADER_HAS_ZBUFFER(raster_type)))
            { // USING ZBUFFER
            if (TGX_SHADER_HAS_ORTHO(SHADER_FLAGS_ENABLED) && (TGX_SHADER_HAS_ORTHO(raster_type)))
                { // USING ORTHOGRAPHIC PROJECTION
                if (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))                    
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Zbuffer_Ortho<color_t,ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, true, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, true,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Zbuffer_Ortho<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        shader_Gouraud_Zbuffer<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);  // same as perspective projection
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        shader_Flat_Zbuffer<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);  // same as perspective projection
                    }
                }
            else if (TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED))
                { // USING PERSPECTIVE PROJECTION
                if (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Zbuffer<color_t, ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Zbuffer<color_t, ZBUFFER_t, true, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Zbuffer<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Zbuffer<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Zbuffer<color_t, ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Zbuffer<color_t, ZBUFFER_t, true,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Zbuffer<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Zbuffer<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        shader_Gouraud_Zbuffer<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        shader_Flat_Zbuffer<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            }
        else if (TGX_SHADER_HAS_NOZBUFFER(SHADER_FLAGS_ENABLED))
            { // NOT USING Z-BUFFER
            if (TGX_SHADER_HAS_ORTHO(SHADER_FLAGS_ENABLED) && (TGX_SHADER_HAS_ORTHO(raster_type)))
                { // USING ORTHOGRAPHIC PROJECTION
                if (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Ortho<color_t, ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Ortho<color_t, ZBUFFER_t, true,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture_Ortho<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture_Ortho<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Ortho<color_t, ZBUFFER_t, true,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Ortho<color_t, ZBUFFER_t, true,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture_Ortho<color_t, ZBUFFER_t, false,false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture_Ortho<color_t, ZBUFFER_t, false,true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        shader_Gouraud<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data); // same as perspective projection
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        shader_Flat<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data); // same as perspective projection
                    }
                }
            else if (TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED))
                { // USING PERSPECTIVE PROJECTION
                if (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture<color_t, ZBUFFER_t, true, false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture<color_t, ZBUFFER_t, true, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Gouraud_Texture<color_t, ZBUFFER_t, false, false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Gouraud_Texture<color_t, ZBUFFER_t, false, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        {
                        if (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture<color_t, ZBUFFER_t, true, false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture<color_t, ZBUFFER_t, true, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            {
                            if (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type))
                                shader_Flat_Texture<color_t, ZBUFFER_t, false, false>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                shader_Flat_Texture<color_t, ZBUFFER_t, false, true>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    {
                    if (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type))
                        shader_Gouraud<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        shader_Flat<color_t, ZBUFFER_t>(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            }       
        }





    /**
    * 2D shader (gradient)
    **/
    template<bool USE_BLENDING, typename color_t_im>
    void shader_2D_gradient(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_im, float> & data)
        {
        color_t_im * buf = data.im->data() + offset;
        const int32_t stride = data.im->stride();

        // use RGB32 (could use RGB64 be it would be slower). 
        const RGB32 col1 = RGB64(fP1.color.R, fP1.color.G, fP1.color.B, fP1.A);
        const RGB32 col2 = RGB64(fP2.color.R, fP2.color.G, fP2.color.B, fP2.A);
        const RGB32 col3 = RGB64(fP3.color.R, fP3.color.G, fP3.color.B, fP3.A);

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }

            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                if (USE_BLENDING)
                    {
                    RGB32 c(buf[bx]); // could use RGB64 instead but would be slower
                    c.blend(interpolateColorsTriangle(col2, C2, col3, C3, col1, aera), data.opacity);
                    buf[bx] = color_t_im(c);
                    }
                else
                    {
                    buf[bx] = color_t_im(interpolateColorsTriangle(col2, C2, col3, C3, col1, aera));
                    }
                C2 += dx2;
                C3 += dx3;
                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }



    /**
    * 2D shader (texture)
    **/
    template<bool USE_BLENDING, bool USE_MASKING, bool USE_GRADIENT, typename color_t_im, typename color_t_tex>
    void shader_2D_texture(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_tex, float> & data)
        {

        color_t_im * buf = data.im->data() + offset;        
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);

        const color_t_tex mask_color = data.mask_color;

        const RGBf& cf1 = (RGBf)fP1.color;
        const RGBf& cf2 = (RGBf)fP2.color;
        const RGBf& cf3 = (RGBf)fP3.color;
        const int fP1R = (int)(256 * cf1.R);
        const int fP1G = (int)(256 * cf1.G);
        const int fP1B = (int)(256 * cf1.B);
        const int fP1A = (int)(256 * fP1.A);
        const int fP21R = (int)(256 * (cf2.R - cf1.R));
        const int fP21G = (int)(256 * (cf2.G - cf1.G));
        const int fP21B = (int)(256 * (cf2.B - cf1.B));
        const int fP21A = (int)(256 * (fP2.A - fP1.A));
        const int fP31R = (int)(256 * (cf3.R - cf1.R));
        const int fP31G = (int)(256 * (cf3.G - cf1.G));
        const int fP31B = (int)(256 * (cf3.B - cf1.B));
        const int fP31A = (int)(256 * (fP3.A - fP1.A));

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;

        const color_t_tex * tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = data.tex->width() - 1;
        const int32_t texsize_y_mm = data.tex->height() - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));
         
        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }
                
            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3)) - 0.5f;
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3)) - 0.5f;

            while ((bx < lx) && ((C2 | C3) >= 0))
                {                         
                const float xx = tx;
                const float yy = ty;                    
                const int ttx = (int)floorf(xx);
                const int tty = (int)floorf(yy);
                const float ax = xx - ttx;
                const float ay = yy - tty;                    

                const int minx = shaderclip(ttx, texsize_x_mm);
                const int maxx = shaderclip(ttx + 1, texsize_x_mm);
                const int miny = shaderclip(tty,  texsize_y_mm) * texstride;
                const int maxy = shaderclip(tty + 1,  texsize_y_mm) * texstride;

                if (USE_MASKING)
                    { // 
                    auto col00 = tex[minx + miny];
                    tgx::RGB32 acol00 = (col00 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col00);

                    auto col10 = tex[maxx + miny];
                    tgx::RGB32 acol10 = (col10 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col10);

                    auto col01 = tex[minx + maxy];
                    tgx::RGB32 acol01 = (col01 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col01);

                    auto col11 = tex[maxx + maxy];
                    tgx::RGB32 acol11 = (col11 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col11);

                    tgx::RGB32 col = interpolateColorsBilinear(acol00, acol10, acol01, acol11, ax, ay);

                    if (USE_GRADIENT)
                        {
                        const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                        const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                        const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);
                        const int a = fP1A + ((C2 * fP21A + C3 * fP31A) / aera);
                        col.mult256(r, g, b, a);
                        }
                    if (USE_BLENDING)
                        {
                        tgx::RGB32 c = tgx::RGB32(buf[bx]);
                        c.blend(col, data.opacity);
                        buf[bx] = color_t_im(c);
                        }
                    else
                        {
                        buf[bx] = color_t_im(col);
                        }
                    }
                else
                    {
                    color_t_tex col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);
                    if (USE_GRADIENT)
                        {
                        const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
                        const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
                        const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);
                        const int a = fP1A + ((C2 * fP21A + C3 * fP31A) / aera);
                        col.mult256(r, g, b, a);
                        }
                    if (USE_BLENDING)
                        {
                        color_t_tex c = color_t_tex(buf[bx]);
                        c.blend(col, data.opacity);
                        buf[bx] = color_t_im(c);
                        }
                    else
                        {
                        buf[bx] = color_t_im(col);
                        }
                    }

                C2 += dx2;
                C3 += dx3;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }




    /**
    * 2D shader (texture with custom blending operator)
    **/
    template<typename BLEND_OP, typename color_t_im, typename color_t_tex>
    void shader_2D_texture_blend_op(const int32_t& offset, const int32_t& lx, const int32_t& ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_tex, float, BLEND_OP> & data)
        {

        color_t_im * buf = data.im->data() + offset;        
        const int32_t stride = data.im->stride();

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);

        // the texture coord
        fVec2 T1 = fP1.T;
        fVec2 T2 = fP2.T;
        fVec2 T3 = fP3.T;


        const color_t_tex * tex = data.tex->data();
        const int32_t texsize_x = data.tex->width();
        const int32_t texsize_y = data.tex->height();
        const int32_t texsize_x_mm = data.tex->width() - 1;
        const int32_t texsize_y_mm = data.tex->height() - 1;
        const int32_t texstride = data.tex->stride();

        // divide the texture coord by aera
        T1 *= invaera;
        T2 *= invaera;
        T3 *= invaera;
        T1.x *= texsize_x;
        T2.x *= texsize_x;
        T3.x *= texsize_x;
        T1.y *= texsize_y;
        T2.y *= texsize_y;
        T3.y *= texsize_y;

        const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
        const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));
         
        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0                 
                bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O2 + dx2 - 1) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                bx = max(bx, ((-O3 + dx3 - 1) / dx3));
                }
                
            int32_t C1 = O1 + (dx1 * bx) + E;
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);

            float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3)) - 0.5f;
            float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3)) - 0.5f;

            while ((bx < lx) && ((C2 | C3) >= 0))
                {                         
                const float xx = tx;
                const float yy = ty;                    
                const int ttx = (int)floorf(xx);
                const int tty = (int)floorf(yy);
                const float ax = xx - ttx;
                const float ay = yy - tty;                    

                const int minx = shaderclip(ttx, texsize_x_mm);
                const int maxx = shaderclip(ttx + 1, texsize_x_mm);
                const int miny = shaderclip(tty,  texsize_y_mm) * texstride;
                const int maxy = shaderclip(tty + 1,  texsize_y_mm) * texstride;
                
                color_t_tex col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);

                buf[bx] = (color_t_im)((*data.p_blend_op)(col, buf[bx]));

                C2 += dx2;
                C3 += dx3;

                tx += dtx;
                ty += dty;

                bx++;
                }

            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            }
        }


}

#endif

#endif

/** end of file */

