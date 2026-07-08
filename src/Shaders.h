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
    void shader_test(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const tgx::RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const tgx::RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const tgx::RasterizerVec4& fP3,
        const tgx::RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        color_t col = (color_t)tgx::RGB32_Red; //data.facecolor;
        const int32_t stride = data.im->stride();
        color_t* buf = data.im->data() + oox + (ooy * stride);

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




TGX_INLINE inline int tgx_clamp_i32(const int x, const int lo, const int hi)
    {
    return (x < lo) ? lo : ((x > hi) ? hi : x);
    }


TGX_INLINE inline uint16_t tgx_rgb565_pack_raw_u16(const int r, const int g, const int b)
    {
    #if TGX_SHADER_RGB565_FAST_CLAMP
    const uint32_t rr = (uint32_t)tgx_clamp_i32(r, 0, 31);
    const uint32_t gg = (uint32_t)tgx_clamp_i32(g, 0, 63);
    const uint32_t bb = (uint32_t)tgx_clamp_i32(b, 0, 31);
    #if TGX_RGB565_ORDER_BGR
    return (uint16_t)((rr << 11) | (gg << 5) | bb);
    #else
    return (uint16_t)((bb << 11) | (gg << 5) | rr);
    #endif
    #else // Same spirit as RGB565 bitfields: keep only the useful low bits.
    #if TGX_RGB565_ORDER_BGR
    return (uint16_t)(((uint32_t)r << 11) | ((uint32_t)g << 5) | (uint32_t)b);
    #else
    return (uint16_t)(((uint32_t)b << 11) | ((uint32_t)g << 5) | (uint32_t)r);
    #endif
    #endif
    }


TGX_INLINE inline int tgx_rgb565_r5(const RGB565 c)
    {
    #if TGX_RGB565_ORDER_BGR
    return (int)((c.val >> 11) & 31u);
    #else
    return (int)(c.val & 31u);
    #endif
    }


TGX_INLINE inline int tgx_rgb565_g6(const RGB565 c)
    {
    return (int)((c.val >> 5) & 63u);
    }


TGX_INLINE inline int tgx_rgb565_b5(const RGB565 c)
    {
    #if TGX_RGB565_ORDER_BGR
    return (int)(c.val & 31u);
    #else
    return (int)((c.val >> 11) & 31u);
    #endif
    }


TGX_INLINE inline RGB565 tgx_rgb565_modulate256(const RGB565 c, const int mr, const int mg, const int mb)
    {
    const int r = (tgx_rgb565_r5(c) * mr) >> 8;
    const int g = (tgx_rgb565_g6(c) * mg) >> 8;
    const int b = (tgx_rgb565_b5(c) * mb) >> 8;
    return RGB565(tgx_rgb565_pack_raw_u16(r, g, b));
    }


TGX_INLINE inline RGB565 tgx_rgb565_bilinear_fast(const RGB565& C00, const RGB565& C10, const RGB565& C01, const RGB565& C11, const float ax, const float ay)
    {
    const int iax = (int)(ax * 256);
    const int iay = (int)(ay * 256);
    const int rax = 256 - iax;
    const int ray = 256 - iay;
    const int R = rax * (ray * tgx_rgb565_r5(C00) + iay * tgx_rgb565_r5(C01)) + iax * (ray * tgx_rgb565_r5(C10) + iay * tgx_rgb565_r5(C11));
    const int G = rax * (ray * tgx_rgb565_g6(C00) + iay * tgx_rgb565_g6(C01)) + iax * (ray * tgx_rgb565_g6(C10) + iay * tgx_rgb565_g6(C11));
    const int B = rax * (ray * tgx_rgb565_b5(C00) + iay * tgx_rgb565_b5(C01)) + iax * (ray * tgx_rgb565_b5(C10) + iay * tgx_rgb565_b5(C11));
    return RGB565(tgx_rgb565_pack_raw_u16(R >> 16, G >> 16, B >> 16));
    }


TGX_INLINE inline RGB565 tgx_rgb565_bilinear_modulate256(const RGB565& C00, const RGB565& C10, const RGB565& C01, const RGB565& C11, const float ax, const float ay,const int mr, const int mg, const int mb)
    {
    const int iax = (int)(ax * 256);
    const int iay = (int)(ay * 256);
    const int rax = 256 - iax;
    const int ray = 256 - iay;
    const int R = rax * (ray * tgx_rgb565_r5(C00) + iay * tgx_rgb565_r5(C01)) + iax * (ray * tgx_rgb565_r5(C10) + iay * tgx_rgb565_r5(C11));
    const int G = rax * (ray * tgx_rgb565_g6(C00) + iay * tgx_rgb565_g6(C01)) + iax * (ray * tgx_rgb565_g6(C10) + iay * tgx_rgb565_g6(C11));
    const int B = rax * (ray * tgx_rgb565_b5(C00) + iay * tgx_rgb565_b5(C01)) + iax * (ray * tgx_rgb565_b5(C10) + iay * tgx_rgb565_b5(C11));
    const int r = (R * mr) >> 24;
    const int g = (G * mg) >> 24;
    const int b = (B * mb) >> 24;
    return RGB565(tgx_rgb565_pack_raw_u16(r, g, b));
    }


TGX_INLINE inline RGB565 tgx_make_rgb565_from_raw(const int r, const int g, const int b)
    {
    return RGB565(tgx_rgb565_pack_raw_u16(r, g, b));
    }





    /**
    * UBER-SHADER for all 3D rendering variants.
    * forced-inline version 
    **/
    template<typename color_t, typename ZBUFFER_t,
             bool USE_ZBUFFER, bool USE_GOURAUD, bool USE_TEXTURE,
             bool USE_ORTHO, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP,
             bool USE_UNLIT, bool USE_TEXTURE_AFFINE, bool USE_MASK_COLOR>
    TGX_UBER_SHADER_INLINE inline void uber_shader_inline(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        static_assert(!(USE_GOURAUD && USE_UNLIT), "UNLIT and GOURAUD shader variants are mutually exclusive.");
        static_assert((!USE_UNLIT) || USE_TEXTURE, "The dedicated UNLIT shader variant is only useful for textured rendering.");
        constexpr bool USE_PERSPECTIVE_TEXTURE = USE_TEXTURE && !USE_ORTHO && !USE_TEXTURE_AFFINE;

        // --- Common setup for all shaders ---
        const int32_t stride = data.im->stride();
        color_t* buf = data.im->data() + oox + (ooy * stride);

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;
        const float faera = (float)aera;

        // --- Z-Buffer setup ---
        ZBUFFER_t* zbuf = nullptr;
        int32_t zstride = 0;
        float wa = 0.0f, wb = 0.0f;
        float fP1a_z = 0.0f, fP2a_z = 0.0f, fP3a_z = 0.0f;
        float fP1a_z_aera = 0.0f;
        float fP21a_z = 0.0f, fP31a_z = 0.0f;
        float dw_z = 0.0f;

        float invaera = 0.0f;
        if constexpr (USE_ZBUFFER || USE_TEXTURE)
            {
            invaera = fast_inv((float)aera);
            }
#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
        else if constexpr (USE_GOURAUD && !USE_TEXTURE && std::is_same<color_t, RGB565>::value)
            {
            invaera = fast_inv((float)aera);
            }
#endif

        if constexpr (USE_ZBUFFER)
            {
            zstride = data.im->lx();
            zbuf = data.zbuf + oox + (ooy * zstride);
            wa = data.wa;
            wb = data.wb;

            if constexpr (!USE_PERSPECTIVE_TEXTURE)
                {
                // Normal z path, needed for:
                //   - no texture
                //   - orthographic texture
                //   - affine texture
                //
                // Use base+delta form:
                //   cw_z = fP1a_z*aera + C2*(fP2a_z-fP1a_z) + C3*(fP3a_z-fP1a_z)
                // because C1 + C2 + C3 = aera.
                const float invaera_wa_factor = USE_ORTHO ? 1.0f : wa;

                fP1a_z = fP1.w * invaera * invaera_wa_factor;
                fP2a_z = fP2.w * invaera * invaera_wa_factor;
                fP3a_z = fP3.w * invaera * invaera_wa_factor;

                fP21a_z = fP2a_z - fP1a_z;
                fP31a_z = fP3a_z - fP1a_z;
                fP1a_z_aera = fP1a_z * faera;

                dw_z = (dx2 * fP21a_z) + (dx3 * fP31a_z);
                }
            }

        // --- Shading & Texturing setup ---
        color_t flat_color;
        color_t col1_g, col2_g, col3_g;
        int shiftC = 0, aeraShifted = 0;
        int fPR = 0, fPG = 0, fPB = 0; // Flat color components
        int fP1R = 0, fP1G = 0, fP1B = 0; // Gouraud color components
        int fP21R = 0, fP21G = 0, fP21B = 0;
        int fP31R = 0, fP31G = 0, fP31B = 0;

#if TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
        float fP21Rf = 0.0f, fP21Gf = 0.0f, fP21Bf = 0.0f;
        float fP31Rf = 0.0f, fP31Gf = 0.0f, fP31Bf = 0.0f;
        float dGR = 0.0f, dGG = 0.0f, dGB = 0.0f;
#endif

#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
        float g565P1R = 0.0f, g565P1G = 0.0f, g565P1B = 0.0f;
        float g565P21R = 0.0f, g565P21G = 0.0f, g565P21B = 0.0f;
        float g565P31R = 0.0f, g565P31G = 0.0f, g565P31B = 0.0f;
        float g565dR = 0.0f, g565dG = 0.0f, g565dB = 0.0f;
#endif

        float invaera_persp = 0.0f;
        float fP1a_p = 0.0f, fP2a_p = 0.0f, fP3a_p = 0.0f;
        float dw_p = 0.0f;

        const color_t* tex = nullptr;
        int32_t texsize_x_mm = 0, texsize_y_mm = 0, texstride = 0;
        float dtx = 0.0f, dty = 0.0f;
        fVec2 T1, T2, T3;

        float T1x_aera = 0.0f, T1y_aera = 0.0f;
        float T21x = 0.0f, T31x = 0.0f;
        float T21y = 0.0f, T31y = 0.0f;

        float fP1a_p_aera = 0.0f;
        float fP21a_p = 0.0f, fP31a_p = 0.0f;

        if constexpr (USE_GOURAUD)
            {
            if constexpr (USE_TEXTURE)
                {
                const RGBf& cf1 = fP1.color;
                const RGBf& cf2 = fP2.color;
                const RGBf& cf3 = fP3.color;

                fP1R = (int)(256 * cf1.R); fP1G = (int)(256 * cf1.G); fP1B = (int)(256 * cf1.B);
                fP21R = (int)(256 * (cf2.R - cf1.R)); fP21G = (int)(256 * (cf2.G - cf1.G)); fP21B = (int)(256 * (cf2.B - cf1.B));
                fP31R = (int)(256 * (cf3.R - cf1.R)); fP31G = (int)(256 * (cf3.G - cf1.G)); fP31B = (int)(256 * (cf3.B - cf1.B));

                shiftC = (aera > (1 << 22)) ? 10 : 0;
                aeraShifted = aera >> shiftC;

#if TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
                fP21Rf = (float)fP21R; fP21Gf = (float)fP21G; fP21Bf = (float)fP21B;
                fP31Rf = (float)fP31R; fP31Gf = (float)fP31G; fP31Bf = (float)fP31B;

                dGR = ((dx2 * fP21Rf) + (dx3 * fP31Rf)) * invaera;
                dGG = ((dx2 * fP21Gf) + (dx3 * fP31Gf)) * invaera;
                dGB = ((dx2 * fP21Bf) + (dx3 * fP31Bf)) * invaera;
#endif
                }
            else
                {
#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
                if constexpr (std::is_same<color_t, RGB565>::value)
                    {
                    const RGBf& cf1 = fP1.color;
                    const RGBf& cf2 = fP2.color;
                    const RGBf& cf3 = fP3.color;

                    g565P1R = 31.0f * cf1.R;
                    g565P1G = 63.0f * cf1.G;
                    g565P1B = 31.0f * cf1.B;

                    g565P21R = 31.0f * (cf2.R - cf1.R);
                    g565P21G = 63.0f * (cf2.G - cf1.G);
                    g565P21B = 31.0f * (cf2.B - cf1.B);

                    g565P31R = 31.0f * (cf3.R - cf1.R);
                    g565P31G = 63.0f * (cf3.G - cf1.G);
                    g565P31B = 31.0f * (cf3.B - cf1.B);

                    g565dR = ((dx2 * g565P21R) + (dx3 * g565P31R)) * invaera;
                    g565dG = ((dx2 * g565P21G) + (dx3 * g565P31G)) * invaera;
                    g565dB = ((dx2 * g565P21B) + (dx3 * g565P31B)) * invaera;
                    }
                else
#endif
                    {
                    col1_g = (color_t)fP1.color;
                    col2_g = (color_t)fP2.color;
                    col3_g = (color_t)fP3.color;
                    shiftC = (aera > (1 << 22)) ? 10 : 0;
                    aeraShifted = aera >> shiftC;
                    }
                }
            }
        else // Flat or unlit shading
            {
            if constexpr (!USE_TEXTURE)
                {
                flat_color = (color_t)data.facecolor;
                }
            else if constexpr (!USE_UNLIT)
                {
                const RGBf& cf = data.facecolor;
                fPR = (int)(256 * cf.R); fPG = (int)(256 * cf.G); fPB = (int)(256 * cf.B);
                }
            }

        if constexpr (USE_TEXTURE)
            {
            if constexpr (USE_MASK_COLOR)
                {
                flat_color = (color_t)data.facecolor; // this is the masked color.
                }

            tex = data.tex->data();
            const int32_t texsize_x = data.tex->width();
            const int32_t texsize_y = data.tex->height();
            texsize_x_mm = texsize_x - 1;
            texsize_y_mm = texsize_y - 1;
            texstride = data.tex->stride();

            T1 = fP1.T; T2 = fP2.T; T3 = fP3.T;            

            if constexpr (!USE_PERSPECTIVE_TEXTURE)
                {
                T1 *= invaera; T2 *= invaera; T3 *= invaera;
                }
            else // Perspective-correct texture
                {
                invaera_persp = invaera;
                fP1a_p = fP1.w * invaera_persp;
                fP2a_p = fP2.w * invaera_persp;
                fP3a_p = fP3.w * invaera_persp;
                T1 *= fP1a_p; T2 *= fP2a_p; T3 *= fP3a_p;
                }

            T1.x *= texsize_x; T2.x *= texsize_x; T3.x *= texsize_x;
            T1.y *= texsize_y; T2.y *= texsize_y; T3.y *= texsize_y;

            T21x = T2.x - T1.x;
            T31x = T3.x - T1.x;
            T21y = T2.y - T1.y;
            T31y = T3.y - T1.y;

            T1x_aera = T1.x * faera;
            T1y_aera = T1.y * faera;

            dtx = (T21x * dx2) + (T31x * dx3);
            dty = (T21y * dx2) + (T31y * dx3);

            if constexpr (USE_PERSPECTIVE_TEXTURE)
                {
                fP21a_p = fP2a_p - fP1a_p;
                fP31a_p = fP3a_p - fP1a_p;
                fP1a_p_aera = fP1a_p * faera;

                dw_p = (fP21a_p * dx2) + (fP31a_p * dx3);
                if constexpr (USE_ZBUFFER) { dw_z = dw_p * wa; }
                }
            }

        // --- Scanline iteration ---
        while ((uintptr_t)(buf) < end)
            {
            // --- Clipping and finding start x (bx) ---
            int32_t bx = 0;
            if (O1 < 0)
                {
                bx = (-O1 + dx1 - 1u) / dx1;
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1u) / dy2;
                    O1 += (by * dy1); O2 += (by * dy2); O3 += (by * dy3);
                    buf += by * stride;
                    if constexpr (USE_ZBUFFER) zbuf += by * zstride;
                    continue;
                    }
                bx = max(bx, (int32_t)((-O2 + dx2 - 1u) / dx2));
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1u) / dy3;
                    O1 += (by * dy1); O2 += (by * dy2); O3 += (by * dy3);
                    buf += by * stride;
                    if constexpr (USE_ZBUFFER) zbuf += by * zstride;
                    continue;
                    }
                bx = max(bx, (int32_t)((-O3 + dx3 - 1u) / dx3));
                }

            // --- Per-scanline setup ---
            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);

            float cw_z = 0.0f;
            float cw_p = 0.0f;
            float tx = 0.0f, ty = 0.0f;

#if TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
            float gR = 0.0f, gG = 0.0f, gB = 0.0f;

            if constexpr (USE_GOURAUD && USE_TEXTURE)
                {
                gR = (float)fP1R + (((float)C2 * fP21Rf) + ((float)C3 * fP31Rf)) * invaera;
                gG = (float)fP1G + (((float)C2 * fP21Gf) + ((float)C3 * fP31Gf)) * invaera;
                gB = (float)fP1B + (((float)C2 * fP21Bf) + ((float)C3 * fP31Bf)) * invaera;
                }
#endif

#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
            float g565R = 0.0f, g565G = 0.0f, g565B = 0.0f;

            if constexpr (USE_GOURAUD && !USE_TEXTURE && std::is_same<color_t, RGB565>::value)
                {
                g565R = g565P1R + (((float)C2 * g565P21R) + ((float)C3 * g565P31R)) * invaera;
                g565G = g565P1G + (((float)C2 * g565P21G) + ((float)C3 * g565P31G)) * invaera;
                g565B = g565P1B + (((float)C2 * g565P21B) + ((float)C3 * g565P31B)) * invaera;
                }
#endif

            if constexpr (USE_TEXTURE)
                {
                tx = T1x_aera + (T21x * C2) + (T31x * C3);
                ty = T1y_aera + (T21y * C2) + (T31y * C3);

                if constexpr (USE_PERSPECTIVE_TEXTURE)
                    {
                    cw_p = fP1a_p_aera + (fP21a_p * C2) + (fP31a_p * C3);
                    if constexpr (USE_ZBUFFER)
                        { // Perspective textured path: cw_z = wa * cw_p + wb.
                        cw_z = (cw_p * wa) + wb;
                        }
                    }
                }

            if constexpr (USE_ZBUFFER)
                {
                if constexpr (!USE_PERSPECTIVE_TEXTURE)
                    {
                    cw_z = fP1a_z_aera + (C2 * fP21a_z) + (C3 * fP31a_z);
                    if constexpr (!USE_ORTHO) { cw_z += wb; }
                    }
                }

            // --- Pixel loop ---
#if TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
            color_t* pix = buf + bx;
            ZBUFFER_t* zpix = nullptr;
            if constexpr (USE_ZBUFFER)
                {
                zpix = zbuf + bx;
                }
#endif
            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                bool z_pass = true;
                if constexpr (USE_ZBUFFER)
                    {
#if TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
                    ZBUFFER_t& W = *zpix;
#else
                    ZBUFFER_t& W = zbuf[bx];
#endif
                    ZBUFFER_t current_z;

                    if constexpr (std::is_same<ZBUFFER_t, uint16_t>::value)
                        {
                         current_z = (USE_ORTHO) ? ((ZBUFFER_t)(cw_z * wa + wb)) : ((ZBUFFER_t)cw_z);
                        }
                    else
                        {
                         current_z = (ZBUFFER_t)cw_z;
                        }

                    if (W < current_z)
                        {
                        W = current_z;
                        }
                    else
                        {
                        z_pass = false;
                        }
                    }

                if (z_pass)
                    {
                    color_t final_color;

                    if constexpr (USE_TEXTURE)
                        {
                        float xx, yy;
                        if constexpr (USE_PERSPECTIVE_TEXTURE)
                            {
                            const float icw = fast_inv_approx(cw_p);
                            xx = tx * icw;
                            yy = ty * icw;
                            }
                        else
                            {
                            xx = tx;
                            yy = ty;
                            }

                        int modR = 256;
                        int modG = 256;
                        int modB = 256;

                        if constexpr (USE_GOURAUD)
                            {
#if TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
                            modR = (int)gR;
                            modG = (int)gG;
                            modB = (int)gB;
#else
                            const int32_t C2s = C2 >> shiftC;
                            const int32_t C3s = C3 >> shiftC;
                            modR = fP1R + ((C2s * fP21R + C3s * fP31R) / aeraShifted);
                            modG = fP1G + ((C2s * fP21G + C3s * fP31G) / aeraShifted);
                            modB = fP1B + ((C2s * fP21B + C3s * fP31B) / aeraShifted);
#endif
                            }
                        else if constexpr (!USE_UNLIT) // Flat
                            {
                            modR = fPR;
                            modG = fPG;
                            modB = fPB;
                            }

#if TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE
                        if constexpr (std::is_same<color_t, RGB565>::value)
                            {
                            if constexpr (TEXTURE_BILINEAR)
                                {
                                const int ttx = lfloorf(xx);
                                const int tty = lfloorf(yy);
                                const float ax = xx - ttx;
                                const float ay = yy - tty;

                                const int minx = TEXTURE_WRAP ? (ttx & texsize_x_mm) : shaderclip(ttx, texsize_x_mm);
                                const int maxx = TEXTURE_WRAP ? ((ttx + 1) & texsize_x_mm) : shaderclip(ttx + 1, texsize_x_mm);
                                const int miny = (TEXTURE_WRAP ? (tty & texsize_y_mm) : shaderclip(tty, texsize_y_mm)) * texstride;
                                const int maxy = (TEXTURE_WRAP ? ((tty + 1) & texsize_y_mm) : shaderclip(tty + 1, texsize_y_mm)) * texstride;

                                if constexpr (USE_UNLIT)
                                    {
#if TGX_SHADER_RGB565_FAST_BILINEAR
                                    final_color = tgx_rgb565_bilinear_fast(
                                        tex[minx + miny],
                                        tex[maxx + miny],
                                        tex[minx + maxy],
                                        tex[maxx + maxy],
                                        ax,
                                        ay);
#else
                                    final_color = interpolateColorsBilinear(
                                        tex[minx + miny],
                                        tex[maxx + miny],
                                        tex[minx + maxy],
                                        tex[maxx + maxy],
                                        ax,
                                        ay);
#endif
                                    }
                                else
                                    {
                                    final_color = tgx_rgb565_bilinear_modulate256(
                                        tex[minx + miny],
                                        tex[maxx + miny],
                                        tex[minx + maxy],
                                        tex[maxx + maxy],
                                        ax,
                                        ay,
                                        modR,
                                        modG,
                                        modB);
                                    }
                                }
                            else // Nearest neighbor
                                {
                                const int ttx = TEXTURE_WRAP ? ((int)(xx)) & texsize_x_mm : shaderclip((int)(xx), texsize_x_mm);
                                const int tty = TEXTURE_WRAP ? ((int)(yy)) & texsize_y_mm : shaderclip((int)(yy), texsize_y_mm);

                                if constexpr (USE_UNLIT)
                                    {
                                    final_color = tex[ttx + tty * texstride];
                                    }
                                else
                                    {
                                    final_color = tgx_rgb565_modulate256(tex[ttx + tty * texstride], modR, modG, modB);
                                    }
                                }
                            }
                        else
#endif
                            {
                            if constexpr (TEXTURE_BILINEAR)
                                {
                                const int ttx = lfloorf(xx);
                                const int tty = lfloorf(yy);
                                const float ax = xx - ttx;
                                const float ay = yy - tty;

                                const int minx = TEXTURE_WRAP ? (ttx & texsize_x_mm) : shaderclip(ttx, texsize_x_mm);
                                const int maxx = TEXTURE_WRAP ? ((ttx + 1) & texsize_x_mm) : shaderclip(ttx + 1, texsize_x_mm);
                                const int miny = (TEXTURE_WRAP ? (tty & texsize_y_mm) : shaderclip(tty, texsize_y_mm)) * texstride;
                                const int maxy = (TEXTURE_WRAP ? ((tty + 1) & texsize_y_mm) : shaderclip(tty + 1, texsize_y_mm)) * texstride;

#if TGX_SHADER_RGB565_FAST_BILINEAR
                                if constexpr (std::is_same<color_t, RGB565>::value)
                                    {
                                    final_color = tgx_rgb565_bilinear_fast(
                                        tex[minx + miny],
                                        tex[maxx + miny],
                                        tex[minx + maxy],
                                        tex[maxx + maxy],
                                        ax,
                                        ay);
                                    }
                                else
#endif
                                    {
                                    final_color = interpolateColorsBilinear(
                                        tex[minx + miny],
                                        tex[maxx + miny],
                                        tex[minx + maxy],
                                        tex[maxx + maxy],
                                        ax,
                                        ay);
                                    }
                                }
                            else // Nearest neighbor
                                {
                                const int ttx = TEXTURE_WRAP ? ((int)(xx)) & texsize_x_mm : shaderclip((int)(xx), texsize_x_mm);
                                const int tty = TEXTURE_WRAP ? ((int)(yy)) & texsize_y_mm : shaderclip((int)(yy), texsize_y_mm);
                                final_color = tex[ttx + tty * texstride];
                                }

                            if constexpr (USE_GOURAUD)
                                {
                                final_color.mult256(modR, modG, modB);
                                }
                            else if constexpr (!USE_UNLIT) // Flat
                                {
                                final_color.mult256(modR, modG, modB);
                                }
                            }
                        }
                    else // No texture
                        {
                        if constexpr (USE_GOURAUD)
                            {
#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
                            if constexpr (std::is_same<color_t, RGB565>::value)
                                {
                                final_color = tgx_make_rgb565_from_raw((int)g565R, (int)g565G, (int)g565B);
                                }
                            else
#endif
                                {
                                final_color = interpolateColorsTriangle(col2_g, C2 >> shiftC, col3_g, C3 >> shiftC, col1_g, aeraShifted);
                                }
                            }
                        else // Flat
                            {
                            final_color = flat_color;
                            }
                        }

#if TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
                    *pix = final_color;
#else
                    buf[bx] = final_color;
#endif
                    }

                // --- Increment for next pixel ---
                C2 += dx2;
                C3 += dx3;

#if TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
                if constexpr (USE_GOURAUD && USE_TEXTURE)
                    {
                    gR += dGR;
                    gG += dGG;
                    gB += dGB;
                    }
#endif

#if TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
                if constexpr (USE_GOURAUD && !USE_TEXTURE && std::is_same<color_t, RGB565>::value)
                    {
                    g565R += g565dR;
                    g565G += g565dG;
                    g565B += g565dB;
                    }
#endif

                bx++;

#if TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
                pix++;
#endif

                if constexpr (USE_ZBUFFER) cw_z += dw_z;

#if TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
                if constexpr (USE_ZBUFFER) zpix++;
#endif

                if constexpr (USE_TEXTURE)
                    {
                    tx += dtx;
                    ty += dty;
                    if constexpr (USE_PERSPECTIVE_TEXTURE) cw_p += dw_p;
                    }
                }

            // --- Increment for next scanline ---
            O1 += dy1;
            O2 += dy2;
            O3 += dy3;
            buf += stride;
            if constexpr (USE_ZBUFFER) zbuf += zstride;
            }
        }




    /**
    * UBER-SHADER for all 3D rendering variants.
    * Uses compile-time flags to generate optimized code for each specific case.
    **/
    template<typename color_t, typename ZBUFFER_t,
             bool USE_ZBUFFER, bool USE_GOURAUD, bool USE_TEXTURE,
             bool USE_ORTHO, bool TEXTURE_BILINEAR, bool TEXTURE_WRAP,
             bool USE_UNLIT, bool USE_TEXTURE_AFFINE, bool USE_MASK_COLOR>
    void uber_shader(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t>& data)
        {
        uber_shader_inline<color_t, ZBUFFER_t, USE_ZBUFFER, USE_GOURAUD, USE_TEXTURE, USE_ORTHO, TEXTURE_BILINEAR, TEXTURE_WRAP, USE_UNLIT, USE_TEXTURE_AFFINE, USE_MASK_COLOR>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
        }



    /**
    * META-Shader THAT DISPATCH TO THE CORRECT Shader ABOVE (IF ENABLED).
    **/
    template<int SHADER_FLAGS_ENABLED, typename color_t, typename ZBUFFER_t>
    inline TGX_SHADER_SELECT_INLINE void shader_select(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t, color_t, ZBUFFER_t> & data)
        {
        int raster_type = data.shader_type;
        if ((!TGX_SHADER_HAS_NOZBUFFER(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_ZBUFFER(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_ZBUFFER(raster_type)))
            { // USING ZBUFFER
            if ((!TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_ORTHO(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_ORTHO(raster_type)))
                { // USING ORTHOGRAPHIC PROJECTION
                if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type)))
                    { // Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_AFFINE(raster_type)))
                    { // Texture affine
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, true, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, true, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    { // No Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        uber_shader<color_t, ZBUFFER_t, true, true, false, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if ((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) || TGX_SHADER_CAN_USE_FLAT_OR_UNLIT(SHADER_FLAGS_ENABLED, raster_type))
                        uber_shader<color_t, ZBUFFER_t, true, false, false, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            else if (TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED))
                { // USING PERSPECTIVE PROJECTION
                if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type)))
                    { // Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_AFFINE(raster_type)))
                    { // Texture affine
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, true, true, false, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, true, false, true, false, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    { // No Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        uber_shader<color_t, ZBUFFER_t, true, true, false, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if ((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) || TGX_SHADER_CAN_USE_FLAT_OR_UNLIT(SHADER_FLAGS_ENABLED, raster_type))
                        uber_shader<color_t, ZBUFFER_t, true, false, false, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            }
        else if (TGX_SHADER_HAS_NOZBUFFER(SHADER_FLAGS_ENABLED))
            { // NOT USING Z-BUFFER
            if ((!TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_ORTHO(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_ORTHO(raster_type)))
                { // USING ORTHOGRAPHIC PROJECTION
                if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type)))
                    { // Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_AFFINE(raster_type)))
                    { // Texture affine
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, true, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, true, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    { // No Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        uber_shader<color_t, ZBUFFER_t, false, true, false, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if ((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) || TGX_SHADER_CAN_USE_FLAT_OR_UNLIT(SHADER_FLAGS_ENABLED, raster_type))
                        uber_shader<color_t, ZBUFFER_t, false, false, false, true, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            else if (TGX_SHADER_HAS_PERSPECTIVE(SHADER_FLAGS_ENABLED))
                { // USING PERSPECTIVE PROJECTION
                if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE(raster_type)))
                    { // Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, true, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, true, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, false, false, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (((!TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_TEXTURE(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_TEXTURE_AFFINE(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_AFFINE(raster_type)))
                    { // Texture affine
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        { // Gouraud
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, true, true, false, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_UNLIT(raster_type)))
                        { // Unlit
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, true, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, true, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    else if (TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED))
                        { // Flat
                        if ((!TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_BILINEAR(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_BILINEAR(raster_type)))
                            { // Bilinear
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, true, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        else if (TGX_SHADER_HAS_TEXTURE_NEAREST(SHADER_FLAGS_ENABLED))
                            { // Nearest
                            if ((!TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_CLAMP(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_CLAMP(raster_type)))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, false, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            else if (TGX_SHADER_HAS_TEXTURE_WRAP_POW2(SHADER_FLAGS_ENABLED))
                                {
                                if ((!TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED)) || (TGX_SHADER_HAS_TEXTURE_NOMASK(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_TEXTURE_NOMASK(raster_type)))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, false, true, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                else if (TGX_SHADER_HAS_TEXTURE_MASK(SHADER_FLAGS_ENABLED))
                                    uber_shader<color_t, ZBUFFER_t, false, false, true, false, false, true, false, true, true>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                                }
                            }
                        }
                    }
                else if (TGX_SHADER_HAS_NOTEXTURE(SHADER_FLAGS_ENABLED))
                    { // No Texture
                    if (((!TGX_SHADER_HAS_FLAT(SHADER_FLAGS_ENABLED)) && (!TGX_SHADER_HAS_UNLIT(SHADER_FLAGS_ENABLED))) || (TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED) && TGX_SHADER_HAS_GOURAUD(raster_type)))
                        uber_shader<color_t, ZBUFFER_t, false, true, false, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    else if ((!TGX_SHADER_HAS_GOURAUD(SHADER_FLAGS_ENABLED)) || TGX_SHADER_CAN_USE_FLAT_OR_UNLIT(SHADER_FLAGS_ENABLED, raster_type))
                        uber_shader<color_t, ZBUFFER_t, false, false, false, false, false, false, false, false, false>(oox, ooy, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
                    }
                }
            }
        }





    /**
    * 2D shader (gradient)
    **/
    template<bool USE_BLENDING, typename color_t_im>
    void shader_2D_gradient(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_im, float> & data)
        {
        const int32_t stride = data.im->stride();
        color_t_im * buf = data.im->data() + oox + (ooy * stride);

        // use RGB32 (could use RGB64 be it would be slower).
        const RGB32 col1 = RGB64(fP1.color.R, fP1.color.G, fP1.color.B, fP1.A);
        const RGB32 col2 = RGB64(fP2.color.R, fP2.color.G, fP2.color.B, fP2.A);
        const RGB32 col3 = RGB64(fP3.color.R, fP3.color.G, fP3.color.B, fP3.A);

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;
        const int shiftC = (aera > (1 << 22)) ? 10 : 0; // prevent overflow during color interpolation

        while ((uintptr_t)(buf) < end)
            { // iterate over scanlines
            int32_t bx = 0; // start offset
            if (O1 < 0)
                {
                // we know that dx1 > 0
                bx = (-O1 + dx1 - 1u) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1u) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx2 = (-O2 + dx2 - 1u) / dx2;
                bx = max(bx, bx2);
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1u) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx3 = (-O3 + dx3 - 1u) / dx3;
                bx = max(bx, bx3);
                }

            int32_t C2 = O2 + (dx2 * bx);
            int32_t C3 = O3 + (dx3 * bx);
            while ((bx < lx) && ((C2 | C3) >= 0))
                {
                if (USE_BLENDING)
                    {
                    RGB32 c(buf[bx]); // could use RGB64 instead but would be slower
                    c.blend(interpolateColorsTriangle(col2, C2 >> shiftC, col3, C3 >> shiftC, col1, aera >> shiftC), data.opacity);
                    buf[bx] = color_t_im(c);
                    }
                else
                    {
                    buf[bx] = color_t_im(interpolateColorsTriangle(col2, C2 >> shiftC, col3, C3 >> shiftC, col1, aera >> shiftC));
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
    void shader_2D_texture(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_tex, float> & data)
        {

        const int32_t stride = data.im->stride();
        color_t_im* buf = data.im->data() + oox + (ooy * stride);

        const uintptr_t end = (uintptr_t)(buf + (ly * stride));
        const int32_t pa = O1 + O2 + O3;
        const int32_t E = ((pa == 0) ? 1 : 0);
        const int32_t aera = pa + E;

        const float invaera = fast_inv((float)aera);

        const color_t_tex mask_color = data.mask_color;

        const RGBf& cf1 = fP1.color;
        const RGBf& cf2 = fP2.color;
        const RGBf& cf3 = fP3.color;

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
                bx = (-O1 + dx1 - 1u) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1u) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx2 = (-O2 + dx2 - 1u) / dx2;
                bx = max(bx, bx2);
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1u) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx3 = (-O3 + dx3 - 1u) / dx3;
                bx = max(bx, bx3);
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
                const int ttx = lfloorf(xx);
                const int tty = lfloorf(yy);
                const float ax = xx - ttx;
                const float ay = yy - tty;

                const int minx = shaderclip(ttx, texsize_x_mm);
                const int maxx = shaderclip(ttx + 1, texsize_x_mm);
                const int miny = shaderclip(tty,  texsize_y_mm) * texstride;
                const int maxy = shaderclip(tty + 1,  texsize_y_mm) * texstride;

                if (USE_MASKING)
                    { //
                    auto col00 = tex[minx + miny];
                    auto col10 = tex[maxx + miny];
                    auto col01 = tex[minx + maxy];
                    auto col11 = tex[maxx + maxy];

                    if ((col00 != mask_color) && (col10 != mask_color) && (col01 != mask_color) && (col11 != mask_color))
                        {
                        color_t_tex col = interpolateColorsBilinear(col00, col10, col01, col11, ax, ay);
                        if (USE_GRADIENT)
                            {
                            const int sC2 = C2;
                            const int sC3 = C3;
                            const int sC1 = aera - C3 - C2;
                            const float m = 256.0f / aera;
                            const int r = (int)((sC1 * cf1.R + sC2 * cf2.R + sC3 * cf3.R) * m);
                            const int g = (int)((sC1 * cf1.G + sC2 * cf2.G + sC3 * cf3.G) * m);
                            const int b = (int)((sC1 * cf1.B + sC2 * cf2.B + sC3 * cf3.B) * m);
                            const int a = (int)((sC1 * fP1.A + sC2 * fP2.A + sC3 * fP3.A) * m);
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
                    else
                        {
                        tgx::RGB32 acol00 = (col00 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col00);
                        tgx::RGB32 acol10 = (col10 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col10);
                        tgx::RGB32 acol01 = (col01 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col01);
                        tgx::RGB32 acol11 = (col11 == mask_color) ? tgx::RGB32((uint32_t)0) : tgx::RGB32(col11);

                        tgx::RGB32 col = interpolateColorsBilinear(acol00, acol10, acol01, acol11, ax, ay);

                        if (USE_GRADIENT)
                            {
                            const int sC2 = C2;
                            const int sC3 = C3;
                            const int sC1 = aera - C3 - C2;
                            const float m = 256.0f / aera;
                            const int r = (int)((sC1 * cf1.R + sC2 * cf2.R + sC3 * cf3.R) * m);
                            const int g = (int)((sC1 * cf1.G + sC2 * cf2.G + sC3 * cf3.G) * m);
                            const int b = (int)((sC1 * cf1.B + sC2 * cf2.B + sC3 * cf3.B) * m);
                            const int a = (int)((sC1 * fP1.A + sC2 * fP2.A + sC3 * fP3.A) * m);
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
                            if (col.A == 255)
                                {
                                buf[bx] = color_t_im(col);
                                }
                            else if (col.A > 0)
                                {
                                tgx::RGB32 c = tgx::RGB32(buf[bx]);
                                c.blend(col);
                                buf[bx] = color_t_im(c);
                                }
                            }
                        }
                    }
                else
                    {
                    color_t_tex col = interpolateColorsBilinear(tex[minx + miny], tex[maxx + miny], tex[minx + maxy], tex[maxx + maxy], ax, ay);
                    if (USE_GRADIENT)
                        {
                        const int sC2 = C2;
                        const int sC3 = C3;
                        const int sC1 = aera - C3 - C2;
                        const float m = 256.0f / aera;
                        const int r = (int)((sC1 * cf1.R + sC2 * cf2.R + sC3 * cf3.R) * m);
                        const int g = (int)((sC1 * cf1.G + sC2 * cf2.G + sC3 * cf3.G) * m);
                        const int b = (int)((sC1 * cf1.B + sC2 * cf2.B + sC3 * cf3.B) * m);
                        const int a = (int)((sC1 * fP1.A + sC2 * fP2.A + sC3 * fP3.A) * m);
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
    void shader_2D_texture_blend_op(const int32_t oox, const int32_t ooy, const int32_t lx, const int32_t ly,
        const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4& fP1,
        const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4& fP2,
        const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4& fP3,
        const RasterizerParams<color_t_im, color_t_tex, float, BLEND_OP> & data)
        {

        const int32_t stride = data.im->stride();
        color_t_im * buf = data.im->data() + oox + (ooy * stride);

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
                bx = (-O1 + dx1 - 1u) / dx1; // first index where it becomes positive
                }
            if (O2 < 0)
                {
                if (dx2 <= 0)
                    {
                    if (dy2 <= 0) return;
                    const int32_t by = (-O2 + dy2 - 1u) / dy2;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx2 = (-O2 + dx2 - 1u) / dx2;
                bx = max(bx, bx2);
                }
            if (O3 < 0)
                {
                if (dx3 <= 0)
                    {
                    if (dy3 <= 0) return;
                    const int32_t by = (-O3 + dy3 - 1u) / dy3;
                    O1 += (by * dy1);
                    O2 += (by * dy2);
                    O3 += (by * dy3);
                    const int32_t offs = by * stride;
                    buf += offs;
                    continue;
                    }
                const int32_t bx3 = (-O3 + dx3 - 1u) / dx3;
                bx = max(bx, bx3);
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
                const int ttx = lfloorf(xx);
                const int tty = lfloorf(yy);
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
