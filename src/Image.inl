/** @file Image.inl */
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
#ifndef _TGX_IMAGE_INL_
#define _TGX_IMAGE_INL_


/************************************************************************************
*
* Implementation file for the template class Image<color_t>
* 
*************************************************************************************/
namespace tgx
{


    /************************************************************************************
    *
    *  Creation of images and sub-images
    *
    *************************************************************************************/


    template<typename color_t>
    Image<color_t>::Image(const Image<color_t> & im, iBox2 subbox, bool clamp)
        {
        if (!im.isValid()) { setInvalid();  return; }
        if (clamp)
            {
            subbox &= im.imageBox();
            }
        else
            {
            if (!(im.imageBox().contains(subbox))) { setInvalid(); return; }
            }
        if (subbox.isEmpty()) { setInvalid(); return; }
        _lx = subbox.lx();
        _ly = subbox.ly();
        _stride = im._stride;
        _buffer = im._buffer + TGX_CAST32(subbox.minX) + TGX_CAST32(im._stride) * TGX_CAST32(subbox.minY);
        }



    /************************************************************************************
    * 
    *  Blitting / copying / resizing images
    * 
    *************************************************************************************/


    /** set len consecutive pixels given color starting at pdest */
    template<typename color_t>
    inline void Image<color_t>::_fast_memset(color_t* p_dest, color_t color, int32_t len)
        {       
        if(std::is_same <color_t, RGB565>::value) // optimized away at compile time
            { // optimized code for RGB565.         
            if (len <= 0) return;
            uint16_t* pdest = (uint16_t*)p_dest;                // recasting
            const uint16_t col = (uint16_t)((RGB565)color);     // conversion to RGB565 does nothing but prevent compiler error when color_t is not RGB565
            // ! We assume here that pdest is already aligned mod 2 (it should be) ! 
            if (((intptr_t)pdest) & 3)
                {
                *(pdest++) = col;
                len--;
                }
            // now we are aligned mod 4
            const uint32_t c32 = col;
            const uint32_t cc = (c32 | (c32 << 16));
            uint32_t* pdest2 = (uint32_t*)pdest;
            int len32 = (len >> 5);         
            while (len32 > 0)
                { // write 32 color pixels at once
                *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
                *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
                *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
                *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
                len32--;
                }               
            int len2 = ((len & 31) >> 1);       
            while (len2 > 0)
                { // write 2 color pixels at once 
                *(pdest2++) = cc;
                len2--;
                }
            
            if (len & 1)
                { // write the last pixel if needed. 
                *((uint16_t*)pdest2) = col;
                }               
            }
        else 
            { // generic code for other color types
            while (len > 0) 
                { 
                *(p_dest++) = color;
                len--;
                }
            }
        }



    
    template<typename color_t>
    bool Image<color_t>::_blitClip(const Image& sprite, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy)
        {
        if ((!sprite.isValid()) || (!isValid())) { return false; } // nothing to draw on or from
        if (sprite_x < 0) { dest_x -= sprite_x; sx += sprite_x; sprite_x = 0; }
        if (sprite_y < 0) { dest_y -= sprite_y; sy += sprite_y; sprite_y = 0; }
        if (dest_x < 0) { sprite_x -= dest_x;   sx += dest_x; dest_x = 0; }
        if (dest_y < 0) { sprite_y -= dest_y;   sy += dest_y; dest_y = 0; }
        if ((dest_x >= _lx) || (dest_y >= _ly) || (sprite_x >= sprite._lx) || (sprite_x >= sprite._ly)) return false;
        sx -= max(0, (dest_x + sx - _lx));
        sy -= max(0, (dest_y + sy - _ly));
        sx -= max(0, (sprite_x + sx - sprite._lx));
        sy -= max(0, (sprite_y + sy - sprite._ly));
        if ((sx <= 0) || (sy <= 0)) return false;
        return true;
        }






    template<typename color_t>
    void Image<color_t>::_blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy)
        {
        if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        _blitRegion(_buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy);
        }

    template<typename color_t>
    void Image<color_t>::_blitRegionUp(color_t * pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
        {
        // TODO, make faster with specialization (writing 32bit at once etc...) 
        for (int j = 0; j < sy; j++)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = 0; i < sx; i++) { pdest2[i] = psrc2[i]; }
            }
        }

    template<typename color_t>
    void Image<color_t>::_blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
        {
        // TODO, make faster with specialization (writing 32bit at once etc...)
        for (int j = sy - 1; j >= 0; j--)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = sx - 1; i >= 0; i--) { pdest2[i] = psrc2[i]; }
            }   
        }





    template<typename color_t>
    void Image<color_t>::_blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
        {
        if (opacity < 0.0f) opacity = 0.0f; else if (opacity > 1.0f) opacity = 1.0f;
        if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        _blitRegion(_buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy, opacity);
        }

    template<typename color_t>
    void Image<color_t>::_blitRegionUp(color_t * pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
        {
        const int op256 = (int)(opacity * 256);
        for (int j = 0; j < sy; j++)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = 0; i < sx; i++) 
                { 
                pdest2[i].blend256(psrc2[i], op256);
                }
            }
        }

    template<typename color_t>
    void Image<color_t>::_blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
        {
        const int op256 = (int)(opacity * 256);
        for (int j = sy - 1; j >= 0; j--)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = sx - 1; i >= 0; i--) 
                {
                pdest2[i].blend256(psrc2[i], op256);
                }
            }   
        }






    template<typename color_t>
    void Image<color_t>::_blitMasked(const Image& sprite, color_t transparent_color, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
        {
        if (opacity < 0.0f) opacity = 0.0f; else if (opacity > 1.0f) opacity = 1.0f;
        if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        _maskRegion(transparent_color, _buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy, opacity);
        }

    template<typename color_t>
    void Image<color_t>::_maskRegionUp(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
        {
        const int op256 = (int)(opacity * 256);
        for (int j = 0; j < sy; j++)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = 0; i < sx; i++) 
                { 
                color_t c = psrc2[i];
                if (c != transparent_color) pdest2[i].blend256(c, op256);
                }
            }
        }

    template<typename color_t>
    void Image<color_t>::_maskRegionDown(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
        {
        const int op256 = (int)(opacity * 256);
        for (int j = sy - 1; j >= 0; j--)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = sx - 1; i >= 0; i--) 
                {
                color_t c = psrc2[i];
                if (c != transparent_color) pdest2[i].blend256(c, op256);
                }
            }   
        }




    

    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blit(const Image<color_t_src>& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {
        if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        _blitRegion(_buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy, blend_op);
        }

    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blitRegionUp(color_t* pdest, int dest_stride, color_t_src* psrc, int src_stride, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {
        for (int j = 0; j < sy; j++)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = 0; i < sx; i++) 
                { 
                pdest2[i] = (color_t)blend_op(psrc2[i], pdest2[i]);
                }
            }
        }

    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blitRegionDown(color_t* pdest, int dest_stride, color_t_src* psrc, int src_stride, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {
        for (int j = sy - 1; j >= 0; j--)
            {
            color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
            color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
            for (int i = sx - 1; i >= 0; i--) 
                {
                pdest2[i] = (color_t)blend_op(psrc2[i], pdest2[i]);
                }
            }   
        }








    template<typename color_t>
    Image<color_t> Image<color_t>::copyReduceHalf(const Image<color_t>& src_image)
        {
        if ((!isValid())||(!src_image.isValid())) { return Image<color_t>(); }
        if (src_image._lx == 1)
            { 
            if (src_image._ly == 1)
                { // stupid case 
                _buffer[0] = src_image._buffer[0];
                return Image<color_t>(*this, iBox2(0, 0, 0, 0), false);
                }
            if (_ly < (src_image._ly >> 1)) { return Image<color_t>(); }
            const color_t * p_src = src_image._buffer;
            color_t * p_dest = _buffer;
            int ny = (src_image._ly >> 1); 
            while (ny-- > 0)
                {
                (*p_dest) = meanColor(p_src[0], p_src[src_image._stride]);
                p_dest += _stride;
                p_src += (src_image._stride*2);
                }
            return Image<color_t>(*this, iBox2(0, 0, 0, (src_image._ly >> 1) - 1), false);
            }
        if (src_image._ly == 1)
            {
            if (_lx < (src_image._lx >> 1)) { return Image<color_t>(); }
            const color_t * p_src = src_image._buffer;
            color_t * p_dest = _buffer;
            int nx = (src_image._lx >> 1);
            while (nx-- > 0)
                {
                (*p_dest) = meanColor(p_src[0], p_src[1]);
                p_dest++;
                p_src += 2;
                }
            return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, 0), false);
            }
        // source image dimension is strictly larger than 1 in each directions.
        if ((_lx < (src_image._lx >> 1)) || (_ly < (src_image._ly >> 1))) { return Image<color_t>(); }
        const int32_t ny = (int32_t)(src_image._ly >> 1);
        for(int32_t j=0; j < ny; j++)
            {
            const color_t * p_src = src_image._buffer + j * TGX_CAST32(2*src_image._stride);
            color_t * p_dest = _buffer + j * TGX_CAST32(_stride);
            int nx = (src_image._lx >> 1);
            while(nx-- > 0)
                {
                (*p_dest) = meanColor(p_src[0], p_src[1], p_src[src_image._stride], p_src[src_image._stride + 1]);
                p_dest++;
                p_src += 2;
                }
            }
        return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, (src_image._ly >> 1) - 1), false);
        }


    template<typename color_t>
    template<typename color_t_src, int CACHE_SIZE, bool USE_BLENDING, bool USE_MASK, bool USE_CUSTOM_OPERATOR, typename BLEND_OPERATOR>
    void Image<color_t>::_blitScaledRotated(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity, const BLEND_OPERATOR& blend_op)
        {
        if ((!isValid()) || (!src_im.isValid())) return;

        // number of slices to draw
        // (we slice it to improve cache access when reading texwxture from flash)
        const int nb_slices = (angle_degrees == 0) ? 1 : ((src_im.stride() * src_im.ly() * sizeof(color_t_src)) / CACHE_SIZE + 1);

        const float tlx = (float)src_im.lx();
        const float tly = (float)src_im.ly();       

        const float a = 0.01745329251f; // 2PI/360
        const float co = cosf(a * angle_degrees);
        const float so = sinf(a * angle_degrees);

        const fVec2 P1 = scale * (fVec2(0.0f, 0.0f) - anchor_src);
        const fVec2 Q1 = fVec2(P1.x * co - P1.y * so, P1.y * co + P1.x * so) + anchor_dst;

        const fVec2 P2 = scale * (fVec2(tlx, 0.0f) - anchor_src);
        const fVec2 Q2 = fVec2(P2.x * co - P2.y * so, P2.y * co + P2.x * so) + anchor_dst;

        const fVec2 P3 = scale * (fVec2(tlx, tly) - anchor_src);
        const fVec2 Q3 = fVec2(P3.x * co - P3.y * so, P3.y * co + P3.x * so) + anchor_dst;

        const fVec2 P4 = scale * (fVec2(0.0f, tly) - anchor_src);
        const fVec2 Q4 = fVec2(P4.x * co - P4.y * so, P4.y * co + P4.x * so) + anchor_dst;

        for (int n = 0; n < nb_slices; n++)
            {
            float y1 = (tly * n) / nb_slices;
            float y2 = (tly * (n+1)) / nb_slices;

            const float ma = ((float)n) / ((float)nb_slices);
            const float ima = 1.0f - ma;
            const float mb = ((float)(n+1)) / ((float)nb_slices);
            const float imb = 1.0f - mb;

            const fVec2 U1 = (Q1 * ima) + (Q4 * ma);
            const fVec2 U2 = (Q2 * ima) + (Q3 * ma);
            const fVec2 U3 = (Q2 * imb) + (Q3 * mb);
            const fVec2 U4 = (Q1 * imb) + (Q4 * mb);

            if (USE_MASK)
                {
                drawTexturedQuadMasked(src_im, transparent_color, fVec2(0.0f, y1), fVec2(tlx, y1), fVec2(tlx, y2), fVec2(0.0f, y2), U1, U2, U3, U4, opacity);
                }
            else if (USE_BLENDING)
                {
                if (USE_CUSTOM_OPERATOR)
                    drawTexturedQuad(src_im, fVec2(0.0f, y1), fVec2(tlx, y1), fVec2(tlx, y2), fVec2(0.0f, y2), U1, U2, U3, U4, blend_op);
                else
                    drawTexturedQuad(src_im, fVec2(0.0f, y1), fVec2(tlx, y1), fVec2(tlx, y2), fVec2(0.0f, y2), U1, U2, U3, U4, opacity);
                }
            else
                {
                drawTexturedQuad(src_im, fVec2(0.0f, y1), fVec2(tlx, y1), fVec2(tlx, y2), fVec2(0.0f, y2), U1, U2, U3, U4);
                }
            }
        }




    template<typename color_t>
    template<typename src_color_t> 
    void Image<color_t>::copyFrom(const Image<src_color_t>& src_im)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        const float ilx = (float)lx();
        const float ily = (float)ly();
        const float tlx = (float)src_im.lx();
        const float tly = (float)src_im.ly();
        drawTexturedQuad(src_im, fVec2(0.0f, 0.0f), fVec2(tlx, 0.0f), fVec2(tlx, tly), fVec2(0.0f, tly), fVec2(0.0f, 0.0f), fVec2(ilx, 0.0f), fVec2(ilx, ily), fVec2(0.0f, ily));
        }


    template<typename color_t>
    template<typename src_color_t> 
    void Image<color_t>::copyFrom(const Image<src_color_t>& src_im, float opacity)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        const float ilx = (float)lx();
        const float ily = (float)ly();
        const float tlx = (float)src_im.lx();
        const float tly = (float)src_im.ly();
        drawTexturedQuad(src_im, fVec2(0.0f, 0.0f), fVec2(tlx, 0.0f), fVec2(tlx, tly), fVec2(0.0f, tly), fVec2(0.0f, 0.0f), fVec2(ilx, 0.0f), fVec2(ilx, ily), fVec2(0.0f, ily), opacity);
        }



    template<typename color_t>
    template<typename src_color_t, typename BLEND_OPERATOR> 
    void Image<color_t>::copyFrom(const Image<src_color_t>& src_im, const BLEND_OPERATOR& blend_op)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        const float ilx = (float)lx();
        const float ily = (float)ly();
        const float tlx = (float)src_im.lx();
        const float tly = (float)src_im.ly();
        drawTexturedQuad(src_im, fVec2(0.0f, 0.0f), fVec2(tlx, 0.0f), fVec2(tlx, tly), fVec2(0.0f, tly), fVec2(0.0f, 0.0f), fVec2(ilx, 0.0f), fVec2(ilx, ily), fVec2(0.0f, ily), blend_op);
        }




    /*********************************************************************
    * Flood filling
    **********************************************************************/


    /**
    * scanline seed fill algorithm taken from Graphic Gems 1 (1995), chap IV.10 p271.
    **/
    template<typename color_t>
    template<bool UNICOLOR_COMP, int STACK_SIZE_BYTES> int Image<color_t>::_scanfill(int x, int y, color_t border_color, color_t new_color)
        {

        #define TGX_SCANFILL_PUSH(X1,X2,Y,DY)   { \
                                                if (stp == STACK_LEN) return -1; \
                                                if ((Y + DY >= 0)&(Y + DY < _ly)) \
                                                    { \
                                                    Qx1[stp] = (uint16_t)X1; \
                                                    Qx2[stp] = (uint16_t)X2; \
                                                    Qy[stp] = (uint16_t)((Y << 1) | ((DY > 0) ? 1 : 0)); \
                                                    stp++; \
                                                    if (stp > max_st) { max_st = stp; } \
                                                    } \
                                                }


        #define TGX_SCANFILL_POP(X1,X2,Y,DY)    { \
                                                stp--; \
                                                X1 = (int)Qx1[stp]; \
                                                X2 = (int)Qx2[stp]; \
                                                DY = (int)((Qy[stp] & 1) ? 1 : -1); \
                                                Y = (int)(Qy[stp] >> 1) + DY; \
                                                }

        #define TGX_SCANFILL_STACKSIZE         (stp)          

        #define TGX_SCANFILL_INSIDE(COLOR)      (UNICOLOR_COMP ? (COLOR == orig_color) : ((COLOR != border_color) && (COLOR != new_color)))

        const int STACK_LEN = STACK_SIZE_BYTES / 6;
        uint16_t Qx1[STACK_LEN];
        uint16_t Qx2[STACK_LEN];
        uint16_t Qy[STACK_LEN];
        int stp = 0;
        int max_st = 0;

        if ((!isValid()) || (x < 0) || (x >= _lx) || (y < 0) || (y >= _ly)) return 0;
        const color_t orig_color = readPixel<false>(x, y);
        if ((UNICOLOR_COMP) && (orig_color == new_color)) return 0; // nothing to do
        if (!TGX_SCANFILL_INSIDE(orig_color)) return 0; // nothing to do 

        TGX_SCANFILL_PUSH(x, x, y, 1);
        TGX_SCANFILL_PUSH(x, x, y + 1, -1);

        while (TGX_SCANFILL_STACKSIZE > 0)
            {
            int x1, x2, dy;
            TGX_SCANFILL_POP(x1, x2, y, dy); // segment previously filled was [x1,x2] x {y - dy}
            x = x1;
            while ((x >= 0) && (TGX_SCANFILL_INSIDE(readPixel<false>(x, y))))
                {
                drawPixel<false>(x--, y, new_color);
                }
            int start = x + 1;
            if (x >= x1) goto TGX_SCANFILL_SKIP;
            if (start < x1) TGX_SCANFILL_PUSH(start, x1 - 1, y, -dy); // leak on left
            x = x1 + 1;
            do
                {
                while ((x < _lx) && (TGX_SCANFILL_INSIDE(readPixel<false>(x, y))))
                    {
                    drawPixel<false>(x++, y, new_color);
                    }
                TGX_SCANFILL_PUSH(start, x - 1, y, dy);
                if (x > x2 + 1) TGX_SCANFILL_PUSH(x2 + 1, x - 1, y, -dy); // leak on right
            TGX_SCANFILL_SKIP:
                x++;
                while ((x <= x2) && (!(TGX_SCANFILL_INSIDE(readPixel<false>(x, y))))) { x++; }
                start = x;
                } 
            while (x <= x2);
            }
        return (6 * max_st);

        #undef TGX_SCANFILL_PUSH
        #undef TGX_SCANFILL_POP
        #undef TGX_SCANFILL_STACKSIZE
        #undef TGX_SCANFILL_INSIDE
        }




    /************************************************************************************
    * 
    *  Drawing primitives
    * 
    *************************************************************************************/

    /*****************************************************
    * Direct pixel access
    ******************************************************/


    template<typename color_t>
    template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun, tgx::iBox2 B)
        {
        B &= imageBox();
        if (B.isEmpty()) return;
        for (int j = B.minY; j <= B.maxY; j++)
            {
            for (int i = B.minX; i <= B.maxX; i++)
                {
                if (!cb_fun(tgx::iVec2(i, j), operator()(i,j))) return;
                }
            }
        }



    /*****************************************************
    * Lines
    ******************************************************/


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawLine(int x0, int y0, int x1, int y1, color_t color)
        {
        if (y0 == y1)
            {
            if (x1 >= x0) 
                drawFastHLine<CHECKRANGE>({ x0, y0 }, x1 - x0 + 1, color);
            else 
                drawFastHLine<CHECKRANGE>({ x1, y0 }, x0 - x1 + 1, color);
            return;
            }
        else if (x0 == x1)
            {
            if (y1 >= y0) 
                drawFastVLine<CHECKRANGE>({ x0, y0 }, y1 - y0 + 1, color);
            else
                drawFastVLine<CHECKRANGE>({ x0, y1 }, y0 - y1 + 1, color);
            return;
            }
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep)
            {
            swap(x0, y0);
            swap(x1, y1);
            }
        if (x0 > x1)
            {
            swap(x0, x1);
            swap(y0, y1);
            }
        int dx = x1 - x0;
        int dy = abs(y1 - y0);
        int err = dx / 2;
        int ystep = (y0 < y1) ? 1 : -1;
        int xbegin = x0;
        if (steep)
            {
            for (; x0 <= x1; x0++)
                {
                err -= dy;
                if (err < 0)
                    {
                    int len = x0 - xbegin;
                    if (len)
                        drawFastVLine<CHECKRANGE>({ y0, xbegin }, len + 1, color);
                    else
                        drawPixel<CHECKRANGE>(y0, x0, color);
                    xbegin = x0 + 1;
                    y0 += ystep;
                    err += dx;
                    }
                }
            if (x0 > xbegin + 1)
                drawFastVLine<CHECKRANGE>({ y0, xbegin }, x0 - xbegin, color);
            }
        else
            {
            for (; x0 <= x1; x0++)
                {
                err -= dy;
                if (err < 0)
                    {
                    int len = x0 - xbegin;
                    if (len)
                        drawFastHLine<CHECKRANGE>({ xbegin, y0 }, len + 1, color);
                    else
                        drawPixel<CHECKRANGE>(x0, y0, color);
                    xbegin = x0 + 1;
                    y0 += ystep;
                    err += dx;
                    }
                }
            if (x0 > xbegin + 1)
                drawFastHLine<CHECKRANGE>({ xbegin, y0 }, x0 - xbegin, color);
            }
        }


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity)
        {
        if (y0 == y1)
            {
            if (x1 >= x0) 
                drawFastHLine<CHECKRANGE>({ x0, y0 }, x1 - x0 + 1, color, opacity);
            else 
                drawFastHLine<CHECKRANGE>({ x1, y0 }, x0 - x1 + 1, color, opacity);
            return;
            }
        else if (x0 == x1)
            {
            if (y1 >= y0) 
                drawFastVLine<CHECKRANGE>({ x0, y0 } , y1 - y0 + 1, color, opacity);
            else
                drawFastVLine<CHECKRANGE>({ x0, y1 }, y0 - y1 + 1, color, opacity);
            return;
            }
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep)
            {
            swap(x0, y0);
            swap(x1, y1);
            }
        if (x0 > x1)
            {
            swap(x0, x1);
            swap(y0, y1);
            }
        int dx = x1 - x0;
        int dy = abs(y1 - y0);
        int err = dx / 2;
        int ystep = (y0 < y1) ? 1 : -1;
        int xbegin = x0;
        if (steep)
            {
            for (; x0 <= x1; x0++)
                {
                err -= dy;
                if (err < 0)
                    {
                    int len = x0 - xbegin;
                    if (len)
                        drawFastVLine<CHECKRANGE>({ y0, xbegin }, len + 1, color, opacity);
                    else
                        drawPixel<CHECKRANGE>(y0, x0, color, opacity);
                    xbegin = x0 + 1;
                    y0 += ystep;
                    err += dx;
                    }
                }
            if (x0 > xbegin + 1)
                drawFastVLine<CHECKRANGE>({ y0, xbegin }, x0 - xbegin, color, opacity);
            }
        else
            {
            for (; x0 <= x1; x0++)
                {
                err -= dy;
                if (err < 0)
                    {
                    int len = x0 - xbegin;
                    if (len)
                        drawFastHLine<CHECKRANGE>({ xbegin, y0 }, len + 1, color, opacity);
                    else
                        drawPixel<CHECKRANGE>(x0, y0, color, opacity);
                    xbegin = x0 + 1;
                    y0 += ystep;
                    err += dx;
                    }
                }
            if (x0 > xbegin + 1)
                drawFastHLine<CHECKRANGE>({ xbegin, y0 }, x0 - xbegin, color, opacity);
            }
        }




    /** Draw a Bresenham segment */
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_drawSeg(bool checkrange, int x0, int y0, int x1, int y1, bool draw_last, color_t color, float opacity)
        {
        const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2; /* error value e_xy */
        while ((x0 != x1) || (y0 != y1))
            {
            _drawPixel<BLEND>(checkrange, x0, y0, color, opacity);
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
            }
        if (draw_last) _drawPixel<BLEND>(checkrange, x1, y1, color, opacity);
        }



    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_drawPolyLine(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
    {
        if (!isValid()) return;
        for (int i = 0; i < nbpoints - 1; i++)
        {
            _drawSeg<BLEND>(true, tabPoints[i].x, tabPoints[i].y, tabPoints[i + 1].x, tabPoints[i + 1].y, ((draw_last_point) && (i == nbpoints - 2)), color, opacity);
        }
    }



    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
    {
        if (!isValid()) return;
        for (int i = 0; i < nbpoints - 1; i++)
        {
            _drawSeg<BLEND>(true, tabPoints[i].x, tabPoints[i].y, tabPoints[i + 1].x, tabPoints[i + 1].y, false, color, opacity);
        }
        _drawSeg<BLEND>(true, tabPoints[nbpoints - 1].x, tabPoints[nbpoints - 1].y, tabPoints[0].x, tabPoints[0].y, false, color, opacity);
    }


    /*****************************************************
    * Bezier / Splines
    ******************************************************/


    /**
    * draw a limited rational Bezier segment, squared weight. Used by _plotQuadRationalBezier().
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotQuadRationalBezierSeg(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const color_t color, const float opacity)
    {
        if ((x0 == x2) && (y0 == y2)) { return; }
        int sx = x2 - x1, sy = y2 - y1;
        float dx = (float)(x0 - x2), dy = (float)(y0 - y2), xx = (float)(x0 - x1), yy = (float)(y0 - y1);
        float xy = xx * sy + yy * sx, cur = xx * sy - yy * sx, err;
        if ((cur == 0) || (w <= 0))
        {
            _drawSeg<BLEND>(checkrange, x0, y0, x2, y2, false, color, opacity);
            return;
        }
        bool sw = false;
        if (sx * sx + sy * sy > xx * xx + yy * yy)
        {
            x2 = x0; x0 -= (int)dx; y2 = y0; y0 -= (int)dy; cur = -cur;
            sw = true;
        }
        xx = 2.0f * (4.0f * w * sx * xx + dx * dx);
        yy = 2.0f * (4.0f * w * sy * yy + dy * dy);
        sx = x0 < x2 ? 1 : -1;
        sy = y0 < y2 ? 1 : -1;
        xy = -2.0f * sx * sy * (2.0f * w * xy + dx * dy);
        if (cur * sx * sy < 0.0f) { xx = -xx; yy = -yy; xy = -xy; cur = -cur; }
        dx = 4.0f * w * (x1 - x0) * sy * cur + xx / 2.0f + xy;
        dy = 4.0f * w * (y0 - y1) * sx * cur + yy / 2.0f + xy;
        if (w < 0.5f && (dy > xy || dx < xy))
        {
            cur = (w + 1.0f) / 2.0f; w = sqrtf(w); xy = 1.0f / (w + 1.0f);
            sx = (int)floorf((x0 + 2.0f * w * x1 + x2) * xy / 2.0f + 0.5f);
            sy = (int)floorf((y0 + 2.0f * w * y1 + y2) * xy / 2.0f + 0.5f);
            dx = floorf((w * x1 + x0) * xy + 0.5f); dy = floorf((y1 * w + y0) * xy + 0.5f);
            if (sw)
            {
                _plotQuadRationalBezierSeg<BLEND>(checkrange, sx, sy, (int)dx, (int)dy, x0, y0, cur, color, opacity);
                dx = floorf((w * x1 + x2) * xy + 0.5f); dy = floorf((y1 * w + y2) * xy + 0.5f);
                _plotQuadRationalBezierSeg<BLEND>(checkrange, x2, y2, (int)dx, (int)dy, sx, sy, cur, color, opacity);
            }
            else
            {
                _plotQuadRationalBezierSeg<BLEND>(checkrange, x0, y0, (int)dx, (int)dy, sx, sy, cur, color, opacity);
                dx = floorf((w * x1 + x2) * xy + 0.5f); dy = floorf((y1 * w + y2) * xy + 0.5f);
                _plotQuadRationalBezierSeg<BLEND>(checkrange, sx, sy, (int)dx, (int)dy, x2, y2, cur, color, opacity);
            }
            return;
        }
        err = dx + dy - xy;
        if (sw)
        {
            x1 = 2 * err > dy; y1 = 2 * (err + yy) < -dy;
            if (2 * err < dx || y1) { y0 += sy; dy += xy; err += dx += xx; }
            if (2 * err > dx || x1) { x0 += sx; dx += xy; err += dy += yy; }
        }
        while (dy <= xy && dx >= xy)
        {
            if (x0 == x2 && y0 == y2)
            {
                if (sw) _drawPixel<BLEND>(checkrange, x0, y0, color, opacity); // write last if swaped
                return;
            }
            _drawPixel<BLEND>(checkrange, x0, y0, color, opacity);
            x1 = 2 * err > dy; y1 = 2 * (err + yy) < -dy;
            if (2 * err < dx || y1) { y0 += sy; dy += xy; err += dx += xx; }
            if (2 * err > dx || x1) { x0 += sx; dx += xy; err += dy += yy; }
        }
        _drawSeg<BLEND>(checkrange, x0, y0, x2, y2, sw, color, opacity);
    }


    /**
    * Plot any quadratic rational Bezier curve.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotQuadRationalBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const bool draw_P2, const color_t color, const float opacity)
    {
        if (checkrange)
        { // check if we can discard the whole curve
            iBox2 mbr(iVec2{ x0, y0 });
            mbr |= iVec2{ x1, y1 };
            mbr |= iVec2{ x2, y2 };
            if ((mbr & iBox2(0, _lx - 1, 0, _ly - 1)).isEmpty()) return;
        }
        if (draw_P2) { _drawPixel<BLEND>(checkrange, x2, y2, color, opacity); }
        if ((x0 == x2) && (y0 == y2)) { return; }
        int x = x0 - 2 * x1 + x2, y = y0 - 2 * y1 + y2;
        float xx = (float)(x0 - x1), yy = (float)(y0 - y1), ww, t, q;
        if (xx * (x2 - x1) > 0)
        {
            if (yy * (y2 - y1) > 0)
                if (fabs(xx * y) > fabs(yy * x))
                {
                    x0 = x2; x2 = (int)(xx + x1); y0 = y2; y2 = (int)(yy + y1);
                }
            if (x0 == x2 || w == 1.0f) t = (x0 - x1) / (float)x;
            else
            {
                q = sqrtf(4.0f * w * w * (x0 - x1) * (x2 - x1) + (x2 - x0) * (x2 - x0));
                if (x1 < x0) q = -q;
                t = (2.0f * w * (x0 - x1) - x0 + x2 + q) / (2.0f * (1.0f - w) * (x2 - x0));
            }
            q = 1.0f / (2.0f * t * (1.0f - t) * (w - 1.0f) + 1.0f);
            xx = (t * t * (x0 - 2.0f * w * x1 + x2) + 2.0f * t * (w * x1 - x0) + x0) * q;
            yy = (t * t * (y0 - 2.0f * w * y1 + y2) + 2.0f * t * (w * y1 - y0) + y0) * q;
            ww = t * (w - 1.0f) + 1.0f; ww *= ww * q;
            w = ((1.0f - t) * (w - 1.0f) + 1.0f) * sqrtf(q);
            x = (int)floorf(xx + 0.5f); y = (int)floorf(yy + 0.5f);
            yy = (xx - x0) * (y1 - y0) / (x1 - x0) + y0;
            _plotQuadRationalBezierSeg<BLEND>(checkrange, x0, y0, x, (int)floorf(yy + 0.5f), x, y, ww, color, opacity);
            yy = (xx - x2) * (y1 - y2) / (x1 - x2) + y2;
            y1 = (int)floorf(yy + 0.5f); x0 = x1 = x; y0 = y;
        }
        if ((y0 - y1) * (y2 - y1) > 0)
        {
            if (y0 == y2 || w == 1.0f) t = (y0 - y1) / (y0 - 2.0f * y1 + y2);
            else
            {
                q = sqrtf(4.0f * w * w * (y0 - y1) * (y2 - y1) + (y2 - y0) * (y2 - y0));
                if (y1 < y0) q = -q;
                t = (2.0f * w * (y0 - y1) - y0 + y2 + q) / (2.0f * (1.0f - w) * (y2 - y0));
            }
            q = 1.0f / (2.0f * t * (1.0f - t) * (w - 1.0f) + 1.0f);
            xx = (t * t * (x0 - 2.0f * w * x1 + x2) + 2.0f * t * (w * x1 - x0) + x0) * q;
            yy = (t * t * (y0 - 2.0f * w * y1 + y2) + 2.0f * t * (w * y1 - y0) + y0) * q;
            ww = t * (w - 1.0f) + 1.0f; ww *= ww * q;
            w = ((1.0f - t) * (w - 1.0f) + 1.0f) * sqrtf(q);
            x = (int)floorf(xx + 0.5f); y = (int)floorf(yy + 0.5f);
            xx = (x1 - x0) * (yy - y0) / (y1 - y0) + x0;
            _plotQuadRationalBezierSeg<BLEND>(checkrange, x0, y0, (int)floorf(xx + 0.5f), y, x, y, ww, color, opacity);
            xx = (x1 - x2) * (yy - y2) / (y1 - y2) + x2;
            x1 = (int)floorf(xx + 0.5f); x0 = x; y0 = y1 = y;
        }
        _plotQuadRationalBezierSeg<BLEND>(checkrange, x0, y0, x1, y1, x2, y2, w * w, color, opacity);
    }


    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity)
    {
        if (!isValid()) return;
        if (wc < 0) wc = 0;
        const bool checkrange = ((P1.x < 0) || (P2.x < 0) || (PC.x < 0) ||
            (P1.y < 0) || (P2.y < 0) || (PC.y < 0) ||
            (P1.x >= _lx) || (P2.x >= _lx) || (PC.x >= _lx) ||
            (P1.y >= _ly) || (P2.y >= _ly) || (PC.y >= _ly));
        _plotQuadRationalBezier<BLEND>(checkrange, P1.x, P1.y, PC.x, PC.y, P2.x, P2.y, wc, drawP2, color, opacity);
    }



    /**
    * Plot a limited cubic Bezier segment.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotCubicBezierSeg(const bool checkrange, int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, const color_t color, const float opacity)
    {
        if ((x0 == x3) && (y0 == y3)) return;
        int sax3 = x3, say3 = y3;
        int f, fx, fy, leg = 1;
        int sx = x0 < x3 ? 1 : -1, sy = y0 < y3 ? 1 : -1;
        float xc = -fabs(x0 + x1 - x2 - x3), xa = xc - 4 * sx * (x1 - x2), xb = sx * (x0 - x1 - x2 + x3);
        float yc = -fabs(y0 + y1 - y2 - y3), ya = yc - 4 * sy * (y1 - y2), yb = sy * (y0 - y1 - y2 + y3);
        float ab, ac, bc, cb, xx, xy, yy, dx, dy, ex, * pxy, EP = 0.01f;
        if (xa == 0 && ya == 0)
        {
            sx = (int)floorf((3 * x1 - x0 + 1) / 2); sy = (int)floorf((3 * y1 - y0 + 1) / 2);
            _plotQuadRationalBezierSeg<BLEND>(checkrange, x0, y0, sx, sy, x3, y3, 1.0f, color, opacity);
            return;
        }
        x1 = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + 1;
        x2 = (x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3) + 1;
        do {
            ab = xa * yb - xb * ya; ac = xa * yc - xc * ya; bc = xb * yc - xc * yb;
            ex = ab * (ab + ac - 3 * bc) + ac * ac;
            f = (ex > 0) ? 1 : (int)sqrtf(1 + 1024 / x1);
            ab *= f; ac *= f; bc *= f; ex *= f * f;
            xy = 9 * (ab + ac + bc) / 8; cb = 8 * (xa - ya);
            dx = 27 * (8 * ab * (yb * yb - ya * yc) + ex * (ya + 2 * yb + yc)) / 64 - ya * ya * (xy - ya);
            dy = 27 * (8 * ab * (xb * xb - xa * xc) - ex * (xa + 2 * xb + xc)) / 64 - xa * xa * (xy + xa);
            xx = 3 * (3 * ab * (3 * yb * yb - ya * ya - 2 * ya * yc) - ya * (3 * ac * (ya + yb) + ya * cb)) / 4;
            yy = 3 * (3 * ab * (3 * xb * xb - xa * xa - 2 * xa * xc) - xa * (3 * ac * (xa + xb) + xa * cb)) / 4;
            xy = xa * ya * (6 * ab + 6 * ac - 3 * bc + cb); ac = ya * ya; cb = xa * xa;
            xy = 3 * (xy + 9 * f * (cb * yb * yc - xb * xc * ac) - 18 * xb * yb * ab) / 8;
            if (ex < 0) { dx = -dx; dy = -dy; xx = -xx; yy = -yy; xy = -xy; ac = -ac; cb = -cb; }
            ab = 6 * ya * ac; ac = -6 * xa * ac; bc = 6 * ya * cb; cb = -6 * xa * cb;
            dx += xy; ex = dx + dy; dy += xy; /* error of 1st step */
            for (pxy = &xy, fx = fy = f; x0 != x3 && y0 != y3; )
            {
                if ((x0 != sax3) || (y0 != say3)) _drawPixel<BLEND>(checkrange, x0, y0, color, opacity);
                do
                {
                    if (dx > *pxy || dy < *pxy) goto exit_cubic_bezier_seg;
                    y1 = 2 * ex - dy;
                    if (2 * ex >= dx) { fx--; ex += dx += xx; dy += xy += ac; yy += bc; xx += ab; }
                    if (y1 <= 0) { fy--; ex += dy += yy; dx += xy += bc; xx += ac; yy += cb; }
                } while (fx > 0 && fy > 0);
                if (2 * fx <= f) { x0 += sx; fx += f; }
                if (2 * fy <= f) { y0 += sy; fy += f; }
                if (pxy == &xy && dx < 0 && dy > 0) pxy = &EP;
            }
        exit_cubic_bezier_seg:
            xx = (float)x0; x0 = x3; x3 = (int)xx; sx = -sx; xb = -xb;
            yy = (float)y0; y0 = y3; y3 = (int)yy; sy = -sy; yb = -yb; x1 = x2;
        } while (leg--);
        if ((x0 == sax3) && (y0 == say3)) { _drawSeg<BLEND>(checkrange, x3, y3, x0, y0, false, color, opacity); }
        else if ((x3 == sax3) && (y3 == say3)) { _drawSeg<BLEND>(checkrange, x0, y0, x3, y3, false, color, opacity); }
        else _drawSeg<BLEND>(checkrange, x0, y0, x3, y3, true, color, opacity);
    }



    /**
    * Plot any cubic Bezier curve.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotCubicBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool draw_P2, const color_t color, const float opacity)
    {
        if (checkrange)
        { // check if we can discard the whole curve
            iBox2 mbr(iVec2{ x0, y0 });
            mbr |= iVec2{ x1, y1 };
            mbr |= iVec2{ x2, y2 };
            mbr |= iVec2{ x3, y3 };
            if ((mbr & iBox2(0, _lx - 1, 0, _ly - 1)).isEmpty()) return;
        }
        if (draw_P2) { _drawPixel<BLEND>(checkrange, x3, y3, color, opacity); }
        if ((x0 == x3) && (y0 == y3)) return;
        int n = 0, i = 0;
        int xc = x0 + x1 - x2 - x3, xa = xc - 4 * (x1 - x2);
        int xb = x0 - x1 - x2 + x3, xd = xb + 4 * (x1 + x2);
        int yc = y0 + y1 - y2 - y3, ya = yc - 4 * (y1 - y2);
        int yb = y0 - y1 - y2 + y3, yd = yb + 4 * (y1 + y2);
        float fx0 = (float)x0, fx1, fx2, fx3, fy0 = (float)y0, fy1, fy2, fy3;
        float t1 = (float)(xb * xb - xa * xc), t2, t[6];
        if (xa == 0) { if (fabs(xc) < 2 * fabs(xb)) t[n++] = xc / (2.0f * xb); }
        else if (t1 > 0.0f)
        {
            t2 = sqrtf(t1);
            t1 = (xb - t2) / xa; if (fabs(t1) < 1.0f) t[n++] = t1;
            t1 = (xb + t2) / xa; if (fabs(t1) < 1.0f) t[n++] = t1;
        }
        t1 = (float)(yb * yb - ya * yc);
        if (ya == 0) { if (fabs(yc) < 2 * fabs(yb)) t[n++] = yc / (2.0f * yb); }
        else if (t1 > 0.0f)
        {
            t2 = sqrtf(t1);
            t1 = (yb - t2) / ya; if (fabs(t1) < 1.0f) t[n++] = t1;
            t1 = (yb + t2) / ya; if (fabs(t1) < 1.0f) t[n++] = t1;
        }
        for (i = 1; i < n; i++) if ((t1 = t[i - 1]) > t[i]) { t[i - 1] = t[i]; t[i] = t1; i = 0; }
        t1 = -1.0f; t[n] = 1.0f;
        for (i = 0; i <= n; i++)
        {
            t2 = t[i];
            fx1 = (t1 * (t1 * xb - 2 * xc) - t2 * (t1 * (t1 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
            fy1 = (t1 * (t1 * yb - 2 * yc) - t2 * (t1 * (t1 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
            fx2 = (t2 * (t2 * xb - 2 * xc) - t1 * (t2 * (t2 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
            fy2 = (t2 * (t2 * yb - 2 * yc) - t1 * (t2 * (t2 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
            fx0 -= fx3 = (t2 * (t2 * (3 * xb - t2 * xa) - 3 * xc) + xd) / 8;
            fy0 -= fy3 = (t2 * (t2 * (3 * yb - t2 * ya) - 3 * yc) + yd) / 8;
            x3 = (int)floorf(fx3 + 0.5f); y3 = (int)floorf(fy3 + 0.5f);
            if (fx0 != 0.0f) { fx1 *= fx0 = (x0 - x3) / fx0; fx2 *= fx0; }
            if (fy0 != 0.0f) { fy1 *= fy0 = (y0 - y3) / fy0; fy2 *= fy0; }
            if (x0 != x3 || y0 != y3)
            {
                _plotCubicBezierSeg<BLEND>(checkrange, x0, y0, x0 + fx1, y0 + fy1, x0 + fx2, y0 + fy2, x3, y3, color, opacity);
            }
            x0 = x3; y0 = y3; fx0 = fx3; fy0 = fy3; t1 = t2;
        }
    }


    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity)
    {
        if (!isValid()) return;
        const bool checkrange = ((P1.x < 0) || (P2.x < 0) || (PA.x < 0) || (PB.x < 0) ||
            (P1.y < 0) || (P2.y < 0) || (PA.y < 0) || (PB.y < 0) ||
            (P1.x >= _lx) || (P2.x >= _lx) || (PA.x >= _lx) || (PB.x >= _lx) ||
            (P1.y >= _ly) || (P2.y >= _ly) || (PA.y >= _ly) || (PB.y >= _ly));
        _plotCubicBezier<BLEND>(checkrange, P1.x, P1.y, PA.x, PA.y, PB.x, PB.y, P2.x, P2.y, drawP2, color, opacity);
    }


    /**
    * plot quadratic spline, destroys input arrays x,y.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotQuadSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity)
    {
        if (draw_last) { _drawPixel<BLEND>(true, x[n], y[n], color, opacity); }
        const int M_MAX = 6;
        float mi = 1, m[M_MAX];
        int i, x0, y0, x1, y1, x2 = x[n], y2 = y[n];
        x[1] = x0 = 8 * x[1] - 2 * x[0];
        y[1] = y0 = 8 * y[1] - 2 * y[0];
        for (i = 2; i < n; i++)
        {
            if (i - 2 < M_MAX) m[i - 2] = mi = 1.0f / (6.0f - mi);
            x[i] = x0 = (int)floorf(8 * x[i] - x0 * mi + 0.5f);
            y[i] = y0 = (int)floorf(8 * y[i] - y0 * mi + 0.5f);
        }
        x1 = (int)(floorf((x0 - 2 * x2) / (5.0f - mi) + 0.5f));
        y1 = (int)(floorf((y0 - 2 * y2) / (5.0f - mi) + 0.5f));
        for (i = n - 2; i > 0; i--)
        {
            if (i <= M_MAX) mi = m[i - 1];
            x0 = (int)floorf((x[i] - x1) * mi + 0.5f);
            y0 = (int)floorf((y[i] - y1) * mi + 0.5f);
            _drawQuadBezier<BLEND>({ (x0 + x1) / 2, (y0 + y1) / 2 }, { x2, y2 }, { x1, y1 }, 1.0f, false, color, opacity);
            x2 = (x0 + x1) / 2; x1 = x0;
            y2 = (y0 + y1) / 2; y1 = y0;
        }
        _drawQuadBezier<BLEND>({ x[0], y[0] }, { x2, y2 }, { x1, y1 }, 1.0f, false, color, opacity);
    }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS, bool BLEND> void Image<color_t>::_drawQuadSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity)
    {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
        {
        case 0:
        {
            return;
        }
        case 1:
        {
            if (draw_last_point) _drawPixel<BLEND>(true, tabPoints[0].x, tabPoints[0].y, color, opacity);
            return;
        }
        case 2:
        {
            _drawSeg<BLEND>(true, tabPoints[0].x, tabPoints[0].y, tabPoints[1].x, tabPoints[1].y, draw_last_point, color, opacity);
            return;
        }
        default:
        {
            int xx[SPLINE_MAX_POINTS];
            int yy[SPLINE_MAX_POINTS];
            for (int n = 0; n < nbpoints; n++)
            {
                xx[n] = tabPoints[n].x;
                yy[n] = tabPoints[n].y;
            }
            _plotQuadSpline<BLEND>(nbpoints - 1, xx, yy, draw_last_point, color, opacity);
        }
        }
    }


    /**
    * Uses the method described in "On the solution of circulant linear systems (Chen 1985)
    * to solve the linear system with circulant matrix:
    *
    *  |61    1|   |Q1|   |8P1|
    *  |161    |   |Q2|   |8P2|
    *  | ...   |   |. |   |.  |
    *  |  ...  | x |. | = |.  |
    *  |   ... |   |. |   |.  |
    *  |    161|   |. |   |.  |
    *  |1    16|   |Qn|   |8Pn|
    *
    * Same as Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html) page 85 but
    * for closed Bezier curve...
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotClosedSpline(int n, int x[], int y[], const color_t color, const float opacity)
    {
        const float a = 0.1715728752538099f; // 3 - 2 * sqrt(2)
        float ux = 0, uy = 0;
        float p = 1;
        for (int i = 1; i <= n; i++)
        {
            x[n - i] *= 8;
            y[n - i] *= 8;
            ux += (x[n - i] * p);
            uy += (y[n - i] * p);
            p *= (-a);
        }
        const float eta = a / (1 - p);

        float xx, yy;
        xx = a * (x[0] - (eta * ux));
        yy = a * (y[0] - (eta * uy));
        x[0] = (int)floorf(xx + 0.5f);
        y[0] = (int)floorf(yy + 0.5f);
        for (int i = 1; i < n; i++)
        {
            xx = a * (x[i] - xx);
            yy = a * (y[i] - yy);
            x[i] = (int)floorf(xx + 0.5f);
            y[i] = (int)floorf(yy + 0.5f);
        }

        ux = 0, uy = 0;
        p = 1;
        for (int i = 0; i < n; i++)
        {
            ux += (x[i] * p);
            uy += (y[i] * p);
            p *= (-a);
        }

        xx = x[n - 1] - (eta * ux);
        yy = y[n - 1] - (eta * uy);
        x[n - 1] = (int)floorf(xx + 0.5f);
        y[n - 1] = (int)floorf(yy + 0.5f);

        for (int i = n - 2; i >= 0; i--)
        {
            xx = x[i] - a * xx;
            yy = y[i] - a * yy;
            x[i] = (int)floorf(xx + 0.5f);
            y[i] = (int)floorf(yy + 0.5f);
        }
        _drawQuadBezier<BLEND>({ (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 }, { (x[0] + x[1]) / 2, (y[0] + y[1]) / 2 }, { x[0], y[0] }, 1.0f, false, color, opacity);
        for (int i = 1; i < n - 1; i++)
        {
            _drawQuadBezier<BLEND>({ (x[i - 1] + x[i]) / 2, (y[i - 1] + y[i]) / 2 }, { (x[i] + x[i + 1]) / 2, (y[i] + y[i + 1]) / 2 }, { x[i], y[i] }, 1.0f, false, color, opacity);
        }
        _drawQuadBezier<BLEND>({ (x[n - 2] + x[n - 1]) / 2, (y[n - 2] + y[n - 1]) / 2 }, { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 }, { x[n - 1], y[n - 1] }, 1.0f, false, color, opacity);
    }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS, bool BLEND> void Image<color_t>::_drawClosedSpline(int nbpoints, const iVec2* tabPoints, color_t color, float opacity)
    {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
        {
        case 0:
        {
            return;
        }
        case 1:
        {
            _drawPixel<BLEND>(true, tabPoints[0].x, tabPoints[0].y, color, opacity);
            return;
        }
        case 2:
        {
            _drawSeg<BLEND>(true, tabPoints[0].x, tabPoints[0].y, tabPoints[1].x, tabPoints[1].y, true, color, opacity);
            return;
        }
        default:
        {
            int xx[SPLINE_MAX_POINTS];
            int yy[SPLINE_MAX_POINTS];
            for (int n = 0; n < nbpoints; n++)
            {
                xx[n] = tabPoints[n].x;
                yy[n] = tabPoints[n].y;
            }
            _plotClosedSpline<BLEND>(nbpoints, xx, yy, color, opacity);
        }
        }
    }


    /**
    * plot cubic spline, destroys input arrays x,y.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    template<bool BLEND> void Image<color_t>::_plotCubicSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity)
    {
        if (draw_last) { _drawPixel<BLEND>(true, x[n], y[n], color, opacity); }
        const int M_MAX = 6;
        float mi = 0.25f, m[M_MAX];
        int x3 = x[n - 1], y3 = y[n - 1], x4 = x[n], y4 = y[n];
        int i, x0, y0, x1, y1, x2, y2;
        x[1] = x0 = 12 * x[1] - 3 * x[0];
        y[1] = y0 = 12 * y[1] - 3 * y[0];
        for (i = 2; i < n; i++)
        {
            if (i - 2 < M_MAX) m[i - 2] = mi = 0.25f / (2.0f - mi);
            x[i] = x0 = (int)floorf(12 * x[i] - 2 * x0 * mi + 0.5f);
            y[i] = y0 = (int)floorf(12 * y[i] - 2 * y0 * mi + 0.5f);
        }
        x2 = (int)(floorf((x0 - 3 * x4) / (7 - 4 * mi) + 0.5f));
        y2 = (int)(floorf((y0 - 3 * y4) / (7 - 4 * mi) + 0.5f));
        _drawCubicBezier<BLEND>({ x3, y3 }, { x4, y4 }, { (x2 + x4) / 2, (y2 + y4) / 2 }, { x4, y4 }, false, color, opacity);
        if (n - 3 < M_MAX) mi = m[n - 3];
        x1 = (int)floorf((x[n - 2] - 2 * x2) * mi + 0.5f);
        y1 = (int)floorf((y[n - 2] - 2 * y2) * mi + 0.5f);
        for (i = n - 3; i > 0; i--)
        {
            if (i <= M_MAX) mi = m[i - 1];
            x0 = (int)floorf((x[i] - 2 * x1) * mi + 0.5f);
            y0 = (int)floorf((y[i] - 2 * y1) * mi + 0.5f);
            x4 = (int)floorf((x0 + 4 * x1 + x2 + 3) / 6.0f);
            y4 = (int)floorf((y0 + 4 * y1 + y2 + 3) / 6.0f);
            _drawCubicBezier<BLEND>({ x4, y4 }, { x3, y3 }, { (int)floorf((2 * x1 + x2) / 3 + 0.5f), (int)floorf((2 * y1 + y2) / 3 + 0.5f) }, { (int)floorf((x1 + 2 * x2) / 3 + 0.5f), (int)floorf((y1 + 2 * y2) / 3 + 0.5f) }, false, color, opacity);
            x3 = x4; y3 = y4; x2 = x1; y2 = y1; x1 = x0; y1 = y0;
        }
        x0 = x[0]; x4 = (int)floorf((3 * x0 + 7 * x1 + 2 * x2 + 6) / 12.0f);
        y0 = y[0]; y4 = (int)floorf((3 * y0 + 7 * y1 + 2 * y2 + 6) / 12.0f);
        _drawCubicBezier<BLEND>({ x4, y4 }, { x3, y3 }, { (int)floorf((2 * x1 + x2) / 3 + 0.5f), (int)floorf((2 * y1 + y2) / 3 + 0.5f) }, { (int)floorf((x1 + 2 * x2) / 3 + 0.5f), (int)floorf((y1 + 2 * y2) / 3 + 0.5f) }, false, color, opacity);
        _drawCubicBezier<BLEND>({ x0, y0 }, { x4, y4 }, { x0, y0 }, { (x0 + x1) / 2, (y0 + y1) / 2 }, false, color, opacity);
    }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS, bool BLEND> void Image<color_t>::_drawCubicSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity)
    {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
        {
        case 0:
        {
            return;
        }
        case 1:
        {
            if (draw_last_point) _drawPixel<BLEND>(true, tabPoints[0].x, tabPoints[0].y, color, opacity);
            return;
        }
        case 2:
        {
            _drawSeg<BLEND>(true, tabPoints[0].x, tabPoints[0].y, tabPoints[1].x, tabPoints[1].y, draw_last_point, color, opacity);
            return;
        }
        case 3:
        {
            _drawQuadSpline<SPLINE_MAX_POINTS, BLEND>(nbpoints, tabPoints, draw_last_point, color, opacity);
            return;
        }
        default:
        {
            int xx[SPLINE_MAX_POINTS];
            int yy[SPLINE_MAX_POINTS];
            for (int n = 0; n < nbpoints; n++)
            {
                xx[n] = tabPoints[n].x;
                yy[n] = tabPoints[n].y;
            }
            _plotCubicSpline<BLEND>(nbpoints - 1, xx, yy, draw_last_point, color, opacity);
        }
        }
    }





    /*****************************************************
    * Triangles
    ******************************************************/


        /**Adapted from from adafruit gfx / Kurte ILI93412_t3n. */
        template<typename color_t>
        template<bool CHECKRANGE, bool DRAW_INTERIOR, bool DRAW_OUTLINE, bool BLEND>
        void Image<color_t>::_drawTriangle_sub(int x0, int y0, int x1, int y1, int x2, int y2, color_t interior_color, color_t outline_color, float opacity)
            {
            int a, b, y, last;
            if (y0 > y1)
                {
                swap(y0, y1);
                swap(x0, x1);
                }
            if (y1 > y2)
                {
                swap(y2, y1);
                swap(x2, x1);
                }
            if (y0 > y1)
                {
                swap(y0, y1);
                swap(x0, x1);
                }

            if (y0 == y2)
                { // triangle is just a single horizontal line
                a = b = x0;
                if (x1 < a) a = x1; else if (x1 > b) b = x1;
                if (x2 < a) a = x2; else if (x2 > b) b = x2;
                if ((!DRAW_OUTLINE) && (DRAW_INTERIOR))
                    {
                    if (BLEND)
                        drawFastHLine<CHECKRANGE>({ a, y0 }, b - a + 1, interior_color, opacity);
                    else
                        drawFastHLine<CHECKRANGE>({ a, y0 }, b - a + 1, interior_color);
                    }
                if (DRAW_OUTLINE)
                    {
                    if (BLEND)
                        drawFastHLine<CHECKRANGE>({ a, y0 }, b - a + 1, outline_color, opacity);
                    else
                        drawFastHLine<CHECKRANGE>({ a, y0 }, b - a + 1, outline_color);
                    }
                return;
                }
            int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
                dx12 = x2 - x1, dy12 = y2 - y1, sa = 0, sb = 0;
            if (y1 == y2) last = y1; else last = y1 - 1;

            if ((DRAW_OUTLINE) && (DRAW_INTERIOR) && (outline_color == interior_color))
                {
                for (y = y0; y <= last; y++)
                    {
                    a = x0 + sa / dy01;
                    b = x0 + sb / dy02;
                    sa += dx01;
                    sb += dx02;
                    if (a > b) swap(a, b);
                    if (BLEND) drawFastHLine<CHECKRANGE>({ a, y }, b - a + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>({ a, y }, b - a + 1, interior_color);
                    }
                sa = dx12 * (y - y1);
                sb = dx02 * (y - y0);
                for (; y <= y2; y++)
                    {
                    a = x1 + sa / dy12;
                    b = x0 + sb / dy02;
                    sa += dx12;
                    sb += dx02;
                    if (a > b) swap(a, b);
                    if (BLEND) drawFastHLine<CHECKRANGE>({ a, y }, b - a + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>({ a, y }, b - a + 1, interior_color);
                    }
                }
            else
                {
                int prv_a = x0 + ((dy01 != 0) ? sa / dy01 : 0);
                int prv_b = x0 + ((dy02 != 0) ? sb / dy02 : 0);
                for (y = y0; y <= last; y++)
                    {
                    a = x0 + sa / dy01;
                    b = x0 + sb / dy02;
                    sa += dx01;
                    sb += dx02;
                    if (a > b) swap(a, b);

                    const auto hha = a - prv_a;
                    int ia, ib;
                    if (hha == 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); }
                        ia = a + 1;
                        }
                    else if (hha > 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ prv_a + 1, y }, hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ prv_a + 1, y }, hha, outline_color); }
                        ia = a + 1;
                        }
                    else
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ a, y }, -hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ a, y }, -hha, outline_color); }
                        ia = prv_a;
                        }

                    const auto hhb = b - prv_b;
                    if (hhb == 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); }
                        ib = prv_b - 1;
                        }
                    else if (hhb > 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ prv_b + 1, y }, hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ prv_b + 1, y }, hhb, outline_color); }
                        ib = prv_b;
                        }
                    else
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ b, y }, -hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ b, y }, -hhb, outline_color); }
                        ib = b - 1;
                        }

                    if ((DRAW_OUTLINE) && ((y == y2) || (y == y0)))
                        {
                        if (BLEND) drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, outline_color);
                        }
                    if ((DRAW_INTERIOR) && (y < y2) && (y > y0))
                        {
                        if (BLEND) drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, interior_color);
                        }
                    prv_a = a;
                    prv_b = b;
                    }
                sa = dx12 * (y - y1);
                sb = dx02 * (y - y0);
                for (; y <= y2; y++)
                    {
                    a = x1 + sa / dy12;
                    b = x0 + sb / dy02;
                    sa += dx12;
                    sb += dx02;
                    if (a > b) swap(a, b);

                    const auto hha = a - prv_a;
                    int ia, ib;
                    if (hha == 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); }
                        ia = a + 1;
                        }
                    else if (hha > 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ prv_a + 1, y }, hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ prv_a + 1, y }, hha, outline_color); }
                        ia = a + 1;
                        }
                    else
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ a, y }, -hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ a, y }, -hha, outline_color); }
                        ia = prv_a;
                        }
                    const auto hhb = b - prv_b;
                    if (hhb == 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); }
                        ib = prv_b - 1;
                        }
                    else if (hhb > 0)
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ prv_b + 1, y }, hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ prv_b + 1, y }, hhb, outline_color); }
                        ib = prv_b;
                        }
                    else
                        {
                        if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>({ b, y }, -hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ b, y }, -hhb, outline_color); }
                        ib = b - 1;
                        }
                    if ((DRAW_OUTLINE) && ((y == y2) || (y == y0)))
                        {
                        if (BLEND) drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, outline_color, opacity); else drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, outline_color);
                        }
                    if ((DRAW_INTERIOR) && (y < y2) && (y > y0))
                        {
                        if (BLEND) drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>({ ia, y }, ib - ia + 1, interior_color);
                        }
                    prv_a = a;
                    prv_b = b;
                    }
                }
            }



    /*****************************************************
    * Rectangles
    ******************************************************/


    /** Fill a rectangle region with a single color */
    template<typename color_t> 
    void Image<color_t>::fillRect(iBox2 B, color_t color)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;
        const int sx = B.lx();
        int sy = B.ly();
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(B.minY) * TGX_CAST32(_stride);
        if (sx == _stride) 
            { // fast, set everything at once
            const int32_t len = TGX_CAST32(sy) * TGX_CAST32(_stride);
            _fast_memset(p, color, len);
            }
        else
            { // set each line separately
            while (sy-- > 0)
                {
                _fast_memset(p, color, sx);
                p += _stride;
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::fillRect(iBox2 B, color_t color, float opacity)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;
        const int sx = B.lx();
        int sy = B.ly();
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(B.minY) * TGX_CAST32(_stride);
        if (sx == _stride) 
            { // fast, set everything at once
            int32_t len = TGX_CAST32(sy) * TGX_CAST32(_stride);
            while (len-- > 0) { (*(p++)).blend(color, opacity); }
            }
        else
            { // set each line separately
            while (sy-- > 0)
                {
                int len = sx;
                while (len-- > 0) { (*(p++)).blend(color, opacity); }
                p += (_stride - sx);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::fillRectHGradient(iBox2 B, color_t color1, color_t color2)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;        
        const int w = B.lx();
        const uint16_t d = (uint16_t)((w > 1) ? (w - 1) : 1);
        RGB64 c64_a(color1);    // color conversion to RGB64
        RGB64 c64_b(color2);    //
        const int16_t dr = (c64_b.R - c64_a.R) / d;
        const int16_t dg = (c64_b.G - c64_a.G) / d;
        const int16_t db = (c64_b.B - c64_a.B) / d;
        const int16_t da = (c64_b.A - c64_a.A) / d;
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
        for (int h = B.ly(); h > 0; h--)
            {
            RGB64 c(c64_a);
            for (int i = 0; i < w; i++)
                {
                p[i] = color_t(c);  // convert back
                c.R += dr;
                c.G += dg;
                c.B += db;
                c.A += da;
                }
            p += _stride;
            }
        }


    template<typename color_t>
    void Image<color_t>::fillRectHGradient(iBox2 B, color_t color1, color_t color2, float opacity)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;        
        const int w = B.lx();
        const uint16_t d = (uint16_t)((w > 1) ? (w - 1) : 1);
        RGB64 c64_a(color1);    // color conversion to RGB64
        RGB64 c64_b(color2);    //
        const int16_t dr = (c64_b.R - c64_a.R) / d;
        const int16_t dg = (c64_b.G - c64_a.G) / d;
        const int16_t db = (c64_b.B - c64_a.B) / d;
        const int16_t da = (c64_b.A - c64_a.A) / d;
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
        for (int h = B.ly(); h > 0; h--)
            {
            RGB64 c(c64_a);
            for (int i = 0; i < w; i++)
                {
                p[i].blend(color_t(c), opacity);
                c.R += dr;
                c.G += dg;
                c.B += db;
                c.A += da;
                }
            p += _stride;
            }
        }


    template<typename color_t>
    void Image<color_t>::fillRectVGradient(iBox2 B, color_t color1, color_t color2)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;
        const int h = B.ly(); 
        const uint16_t d = (uint16_t)((h > 1) ? (h - 1) : 1);
        RGB64 c64_a(color1);    // color conversion to RGB64
        RGB64 c64_b(color2);    //
        const int16_t dr = (c64_b.R - c64_a.R) / d;
        const int16_t dg = (c64_b.G - c64_a.G) / d;
        const int16_t db = (c64_b.B - c64_a.B) / d;
        const int16_t da = (c64_b.A - c64_a.A) / d;
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
        for (int j = h; j > 0; j--)
            {
            _fast_memset(p, color_t(c64_a), B.lx());
            c64_a.R += dr;
            c64_a.G += dg;
            c64_a.B += db;
            c64_a.A += da;
            p += _stride;
            }
        }


    template<typename color_t>
    void Image<color_t>::fillRectVGradient(iBox2 B, color_t color1, color_t color2, float opacity)
        {
        if (!isValid()) return;
        B &= imageBox();
        if (B.isEmpty()) return;
        const int h = B.ly(); 
        const uint16_t d = (uint16_t)((h > 1) ? (h - 1) : 1);
        RGB64 c64_a(color1);    // color conversion to RGB64
        RGB64 c64_b(color2);    //
        const int16_t dr = (c64_b.R - c64_a.R) / d;
        const int16_t dg = (c64_b.G - c64_a.G) / d;
        const int16_t db = (c64_b.B - c64_a.B) / d;
        const int16_t da = (c64_b.A - c64_a.A) / d;
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
        for (int j = h; j > 0; j--)
            {
            int l = B.lx();
            while (l-- > 0) { (*(p++)).blend(color_t(c64_a), opacity); }
            c64_a.R += dr;
            c64_a.G += dg;
            c64_a.B += db;
            c64_a.A += da;
            p += _stride - B.lx();
            }
        }



    /*****************************************************
    * Rounded rectangles
    ******************************************************/



    /** draw a rounded rectangle outline */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawRoundRect(int x, int y, int w, int h, int r, color_t color)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        drawFastHLine<CHECKRANGE>({ x + r, y }, w - 2 * r, color);
        drawFastHLine<CHECKRANGE>({ x + r, y + h - 1 }, w - 2 * r, color);
        drawFastVLine<CHECKRANGE>({ x, y + r }, h - 2 * r, color);
        drawFastVLine<CHECKRANGE>({ x + w - 1 , y + r }, h - 2 * r, color);
        _drawCircleHelper<CHECKRANGE>(x + r, y + r, r, 1, color);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 2, color);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + h - r - 1, r, 4, color);
        _drawCircleHelper<CHECKRANGE>(x + r, y + h - r - 1, r, 8, color);
        }


    /** draw a rounded rectangle outline */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        drawFastHLine<CHECKRANGE>({ x + r, y }, w - 2 * r, color, opacity);
        drawFastHLine<CHECKRANGE>({ x + r, y + h - 1 }, w - 2 * r, color, opacity);
        drawFastVLine<CHECKRANGE>({ x, y + r }, h - 2 * r, color, opacity);
        drawFastVLine<CHECKRANGE>({ x + w - 1, y + r }, h - 2 * r, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + r, y + r, r, 1, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 2, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + h - r - 1, r, 4, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + r, y + h - r - 1, r, 8, color, opacity);
        }


    /** fill a rounded rectangle  */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_fillRoundRect(int x, int y, int w, int h, int r, color_t color)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        fillRect(iVec2{ x + r, y }, iVec2{ w - 2 * r, h }, color);
        _fillCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
        _fillCircleHelper<CHECKRANGE>(x + r, y + r, r, 2, h - 2 * r - 1, color);
        }


    /** fill a rounded rectangle  */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        fillRect(iVec2{ x + r, y }, iVec2{ w - 2 * r, h }, color, opacity);
        _fillCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color, opacity);
        _fillCircleHelper<CHECKRANGE>(x + r, y + r, r, 2, h - 2 * r - 1, color, opacity);
        }





    /*****************************************************
    * Circles
    ******************************************************/


    /** taken from adafruit gfx */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawCircleHelper(int x0, int y0, int r, int cornername, color_t color)
        {
        int f = 1 - r;
        int ddF_x = 1;
        int ddF_y = -2 * r;
        int x = 0;
        int y = r;
        while (x < y)
            {
            if (f >= 0)
                {
                y--;
                ddF_y += 2;
                f += ddF_y;
                }
            x++;
            ddF_x += 2;
            f += ddF_x;
            if (cornername & 0x4)
                {
                drawPixel<CHECKRANGE>(x0 + x, y0 + y, color);
                drawPixel<CHECKRANGE>(x0 + y, y0 + x, color);
                }
            if (cornername & 0x2)
                {
                drawPixel<CHECKRANGE>(x0 + x, y0 - y, color);
                drawPixel<CHECKRANGE>(x0 + y, y0 - x, color);
                }
            if (cornername & 0x8)
                {
                drawPixel<CHECKRANGE>(x0 - y, y0 + x, color);
                drawPixel<CHECKRANGE>(x0 - x, y0 + y, color);
                }
            if (cornername & 0x1)
                {
                drawPixel<CHECKRANGE>(x0 - y, y0 - x, color);
                drawPixel<CHECKRANGE>(x0 - x, y0 - y, color);
                }
            }
        }

    /** taken from adafruit gfx */
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawCircleHelper(int x0, int y0, int r, int cornername, color_t color, float opacity)
        {
        int f = 1 - r;
        int ddF_x = 1;
        int ddF_y = -2 * r;
        int x = 0;
        int y = r;
        while (x < y - 1)
            {
            if (f >= 0)
                {
                y--;
                ddF_y += 2;
                f += ddF_y;
                }
            x++;
            ddF_x += 2;
            f += ddF_x;
            if (cornername & 0x4)
                {
                drawPixel<CHECKRANGE>(x0 + x, y0 + y, color, opacity);
                if (x!= y) drawPixel<CHECKRANGE>(x0 + y, y0 + x, color, opacity);
                }
            if (cornername & 0x2)
                {
                drawPixel<CHECKRANGE>(x0 + x, y0 - y, color, opacity);
                if (x != y) drawPixel<CHECKRANGE>(x0 + y, y0 - x, color, opacity);
                }
            if (cornername & 0x8)
                {
                drawPixel<CHECKRANGE>(x0 - y, y0 + x, color, opacity);
                if (x != y) drawPixel<CHECKRANGE>(x0 - x, y0 + y, color, opacity);
                }
            if (cornername & 0x1)
                {
                drawPixel<CHECKRANGE>(x0 - y, y0 - x, color, opacity);
                if (x != y) drawPixel<CHECKRANGE>(x0 - x, y0 - y, color, opacity);
                }
            }
        }



    /** taken from Adafruiit GFX*/
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color) 
        {
        int f = 1 - r;
        int ddF_x = 1;
        int ddF_y = -2 * r;
        int x = 0;
        int y = r;
        int px = x;
        int py = y;
        delta++; // Avoid some +1's in the loop
        while (x < y) 
            {
            if (f >= 0) 
                {
                y--;
                ddF_y += 2;
                f += ddF_y;
                }
            x++;
            ddF_x += 2;
            f += ddF_x;
            // These checks avoid double-drawing certain lines, important
            // for the SSD1306 library which has an INVERT drawing mode.
            if (x < (y + 1)) 
                {
                if (corners & 1) drawFastVLine<CHECKRANGE>({ x0 + x, y0 - y }, 2 * y + delta, color);
                if (corners & 2) drawFastVLine<CHECKRANGE>({ x0 - x, y0 - y }, 2 * y + delta, color);
                }
            if (y != py) 
                {
                if (corners & 1) drawFastVLine<CHECKRANGE>({ x0 + py, y0 - px }, 2 * px + delta, color);
                if (corners & 2) drawFastVLine<CHECKRANGE>({ x0 - py, y0 - px }, 2 * px + delta, color);
                py = y;
                }
            px = x;
            }
        }


    /** taken from Adafruit GFX*/
    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color, float opacity)
        {
        int f = 1 - r;
        int ddF_x = 1;
        int ddF_y = -2 * r;
        int x = 0;
        int y = r;
        int px = x;
        int py = y;
        delta++; // Avoid some +1's in the loop
        while (x < y)
            {
            if (f >= 0)
                {
                y--;
                ddF_y += 2;
                f += ddF_y;
                }
            x++;
            ddF_x += 2;
            f += ddF_x;
            // These checks avoid double-drawing certain lines, important
            // for the SSD1306 library which has an INVERT drawing mode.
            if (x < (y + 1))
                {
                if (corners & 1) drawFastVLine<CHECKRANGE>({ x0 + x, y0 - y }, 2 * y + delta, color, opacity);
                if (corners & 2) drawFastVLine<CHECKRANGE>({ x0 - x, y0 - y }, 2 * y + delta, color, opacity);
                }
            if (y != py)
                {
                if (corners & 1) drawFastVLine<CHECKRANGE>({ x0 + py, y0 - px }, 2 * px + delta, color, opacity);
                if (corners & 2) drawFastVLine<CHECKRANGE>({ x0 - py, y0 - px }, 2 * px + delta, color, opacity);
                py = y;
                }
            px = x;
            }
        }



    template<typename color_t>
    template<bool OUTLINE, bool FILL, bool CHECKRANGE> void Image<color_t>::_drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor)
        {
        if ((r <= 0) || (!isValid())) return;
        if ((CHECKRANGE) && (r > 2))
            { // circle is large enough to check first if there is something to draw.
            if ((xm + r < 0) || (xm - r >= _lx) || (ym + r < 0) || (ym - r >= _ly)) return; // outside of image. 
            // TODO : check if the circle completely fills the image, in this case use FillScreen()
            }
        switch (r)
            {
            case 0:
                {
                if (OUTLINE)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, color);
                    }
                else if (FILL)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, fillcolor);
                    }
                return;
                }
            case 1:
                {
                if (FILL)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, fillcolor);
                    }
                drawPixel<CHECKRANGE>(xm + 1, ym, color);
                drawPixel<CHECKRANGE>(xm - 1, ym, color);
                drawPixel<CHECKRANGE>(xm, ym - 1, color);
                drawPixel<CHECKRANGE>(xm, ym + 1, color);
                return;
                }
            }
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(xm - x, ym + y, color);
                drawPixel<CHECKRANGE>(xm - y, ym - x, color);
                drawPixel<CHECKRANGE>(xm + x, ym - y, color);
                drawPixel<CHECKRANGE>(xm + y, ym + x, color);
                }
            r = err;
            if (r <= y)
                {
                if (FILL)
                    {
                    drawFastHLine<CHECKRANGE>({ xm, ym + y }, -x, fillcolor);
                    drawFastHLine<CHECKRANGE>({ xm + x + 1, ym - y }, -x - 1, fillcolor);
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
                        drawFastHLine<CHECKRANGE>({ xm - y + 1, ym - x }, y - 1, fillcolor);
                        drawFastHLine<CHECKRANGE>({ xm, ym + x }, y, fillcolor);
                        }
                    }
                }
            } while (x < 0);
        }


    template<typename color_t>
    template<bool OUTLINE, bool FILL, bool CHECKRANGE> void Image<color_t>::_drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor, float opacity)
        {
        if ((r <= 0) || (!isValid())) return;
        if ((CHECKRANGE) && (r > 2))
            { // circle is large enough to check first if there is something to draw.
            if ((xm + r < 0) || (xm - r >= _lx) || (ym + r < 0) || (ym - r >= _ly)) return; // outside of image. 
            // TODO : check if the circle completely fills the image, in this case use FillScreen()
            }
        switch (r)
            {
            case 0:
                {
                if (OUTLINE)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, color, opacity);
                    }
                else if (FILL)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, fillcolor, opacity);
                    }
                return;
                }
            case 1:
                {
                if (FILL)
                    {
                    drawPixel<CHECKRANGE>(xm, ym, fillcolor, opacity);
                    }
                drawPixel<CHECKRANGE>(xm + 1, ym, color, opacity);
                drawPixel<CHECKRANGE>(xm - 1, ym, color, opacity);
                drawPixel<CHECKRANGE>(xm, ym - 1, color, opacity);
                drawPixel<CHECKRANGE>(xm, ym + 1, color, opacity);
                return;
                }
            }
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(xm - x, ym + y, color, opacity);
                drawPixel<CHECKRANGE>(xm - y, ym - x, color, opacity);
                drawPixel<CHECKRANGE>(xm + x, ym - y, color, opacity);
                drawPixel<CHECKRANGE>(xm + y, ym + x, color, opacity);
                }
            r = err;
            if (r <= y)
                {
                if (FILL)
                    {
                    drawFastHLine<CHECKRANGE>({ xm, ym + y }, -x, fillcolor, opacity);
                    drawFastHLine<CHECKRANGE>({ xm + x + 1, ym - y }, -x - 1, fillcolor, opacity);
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
                        drawFastHLine<CHECKRANGE>({ xm - y + 1, ym - x }, y - 1, fillcolor, opacity);
                        drawFastHLine<CHECKRANGE>({ xm, ym + x }, y, fillcolor, opacity);
                        }
                    }
                }
            } while (x < 0);
        }


    /*****************************************************
    * Ellipses
    ******************************************************/


    /** Adapted from Bodmer e_tft library. */
    template<typename color_t>
    template<bool OUTLINE, bool FILL, bool CHECKRANGE>
    void Image<color_t>::_drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color)
        {
        if (!isValid()) return;
        if (rx < 2) return;
        if (ry < 2) return;
        int32_t x, y;
        int32_t rx2 = rx * rx;
        int32_t ry2 = ry * ry;
        int32_t fx2 = 4 * rx2;
        int32_t fy2 = 4 * ry2;
        int32_t s;
        int yt = ry;
        for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) 
            {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color);
                drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color);
                if (x != 0)
                    {
                    drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color);
                    drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color);
                    }
                }
            if (s >= 0) 
                {
                s += fx2 * (1 - y);
                y--;
                if (FILL)
                    {
                    if (ry2 * x <= rx2 * y)
                        {
                        drawFastHLine<CHECKRANGE>({ x0 - x, y0 - y }, x + x + 1, interior_color);
                        drawFastHLine<CHECKRANGE>({ x0 - x, y0 + y }, x + x + 1, interior_color);
                        yt = y;
                        }
                    }
                }
            s += ry2 * ((4 * x) + 6);
            }
        
        for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) 
            {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color);
                drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color);
                if (y != 0)
                    {
                    drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color);
                    drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color);
                    }
                }
            if (FILL)
                {
                if (y != yt)
                    {
                    if (y != 0)
                        {
                        drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 - y }, x + x - 1, interior_color);
                        }
                    drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 + y }, x + x - 1, interior_color);
                    }
                }

            if (s >= 0)
                {
                s += fy2 * (1 - x);
                x--;
                }
            s += rx2 * ((4 * y) + 6);
            }
        }


    /** Adapted from Bodmer e_tft library. */
    template<typename color_t>
    template<bool OUTLINE, bool FILL, bool CHECKRANGE>
    void Image<color_t>::_drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity)
        {
        if (!isValid()) return;
        if (rx < 2) return;
        if (ry < 2) return;
        int32_t x, y;
        int32_t rx2 = rx * rx;
        int32_t ry2 = ry * ry;
        int32_t fx2 = 4 * rx2;
        int32_t fy2 = 4 * ry2;
        int32_t s;
        int yt = ry;
        for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) 
            {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color, opacity);
                drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color, opacity);
                if (x != 0)
                    {
                    drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color, opacity);
                    drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color, opacity);
                    }
                }
            if (s >= 0) 
                {
                s += fx2 * (1 - y);
                y--;
                if (FILL)
                    {
                    if (ry2 * x <= rx2 * y)
                        {
                        drawFastHLine<CHECKRANGE>({ x0 - x, y0 - y }, x + x + 1, interior_color, opacity);
                        drawFastHLine<CHECKRANGE>({ x0 - x, y0 + y }, x + x + 1, interior_color, opacity);
                        yt = y;
                        }
                    }
                }
            s += ry2 * ((4 * x) + 6);
            }
        
        for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) 
            {
            if (OUTLINE)
                {
                drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color, opacity);
                drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color, opacity);
                if (y != 0)
                    {
                    drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color, opacity);
                    drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color, opacity);
                    }
                }
            if (FILL)
                {
                if (y != yt)
                    {
                    if (y != 0)
                        {
                        drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 - y }, x + x - 1, interior_color, opacity);
                        }
                    drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 + y }, x + x - 1, interior_color, opacity);
                    }
                }

            if (s >= 0)
                {
                s += fy2 * (1 - x);
                x--;
                }
            s += rx2 * ((4 * y) + 6);
            }
        }




    /*****************************************************
    * High quality routines
    ******************************************************/


    /** Adapted from Bodmer e_tft library. */
    template<typename color_t>
    void Image<color_t>::_drawWideLine(float ax, float ay, float bx, float by, float wd, color_t color, float opacity)
        {
        const float LoAlphaTheshold = 64.0f / 255.0f;
        const float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

        if ((abs(ax - bx) < 0.01f) && (abs(ay - by) < 0.01f)) bx += 0.01f;  // Avoid divide by zero

        wd = wd / 2.0f; // wd is now end radius of line

        iBox2 B((int)floorf(fminf(ax, bx) - wd), (int)ceilf(fmaxf(ax, bx) + wd), (int)floorf(fminf(ay, by) - wd), (int)ceilf(fmaxf(ay, by) + wd));
        B &= imageBox();
        if (B.isEmpty()) return;
        // Find line bounding box
        int x0 = B.minX;
        int x1 = B.maxX;
        int y0 = B.minY;
        int y1 = B.maxY;

        // Establish slope direction
        int xs = x0, yp = y1, yinc = -1;
        if ((ax > bx && ay > by) || (ax < bx && ay < by)) { yp = y0; yinc = 1; }

        float alpha = 1.0f; wd += 0.5f;
        int ri = (int)wd;

        float   wd2 = fmaxf(wd - 1.0f, 0.0f);
        wd2 = wd2 * wd2;
        float pax, pay, bax = bx - ax, bay = by - ay;

        // Scan bounding box, calculate pixel intensity from distance to line
        for (int y = y0; y <= y1; y++)
            {
            bool endX = false;                       // Flag to skip pixels
            pay = yp - ay;
            for (int xp = xs; xp <= x1; xp++)
                {
                if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
                pax = xp - ax;
                alpha = wd - _wideLineDistance(pax, pay, bax, bay, wd2);
                if (alpha <= LoAlphaTheshold) continue;
                // Track left line boundary
                if (!endX) { endX = true; if ((y > (y0 + ri)) && (xp > xs)) xs = xp; }
                if (alpha > HiAlphaTheshold) { drawPixel(xp, yp, color, opacity); continue; }
                //Blend colour with background and plot
                drawPixel(xp, yp, color, alpha * opacity);
                }
            yp += yinc;
            }
        }



    /** Adapted from Bodmer e_tft library. */
    template<typename color_t>
    void Image<color_t>::_drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity)
        {
        const float LoAlphaTheshold = 64.0f / 255.0f;
        const float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

        if ((abs(ax - bx) < 0.01f) && (abs(ay - by) < 0.01f)) bx += 0.01f;  // Avoid divide by zero

        aw = aw / 2.0f;
        bw = bw / 2.0f;

        iBox2 B((int)floorf(fminf(ax - aw, bx - bw)), (int)ceilf(fmaxf(ax + aw, bx + bw)), (int)floorf(fminf(ay - aw, by - bw)), (int)ceilf(fmaxf(ay + aw, by + bw)));
        B &= imageBox();
        if (B.isEmpty()) return;
        // Find line bounding box
        int x0 = B.minX;
        int x1 = B.maxX;
        int y0 = B.minY;
        int y1 = B.maxY;

        // Establish slope direction
        int xs = x0, yp = y1, yinc = -1;
        if (((ax - aw) > (bx - bw) && (ay > by)) || ((ax - aw) < (bx - bw) && ay < by)) { yp = y0; yinc = 1; }

        bw = aw - bw; // Radius delta
        float alpha = 1.0f; aw += 0.5f;
        int ri = (int)aw;
        float pax, pay, bax = bx - ax, bay = by - ay;

        // Scan bounding box, calculate pixel intensity from distance to line
        for (int y = y0; y <= y1; y++)
            {
            bool endX = false;                       // Flag to skip pixels
            pay = yp - ay;
            for (int32_t xp = xs; xp <= x1; xp++)
                {
                if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
                pax = xp - ax;
                alpha = aw - _wedgeLineDistance(pax, pay, bax, bay, bw);
                if (alpha <= LoAlphaTheshold) continue;
                // Track left line segment boundary
                if (!endX) { endX = true; if ((y > (y0 + ri)) && (xp > xs)) xs = xp; }
                if (alpha > HiAlphaTheshold) { drawPixel(xp, yp, color, opacity);  continue; }
                //Blend color with background and plot
                drawPixel(xp, yp, color, alpha * opacity);
                }
            yp += yinc;
            }
        }



    template<typename color_t>
    template<typename color_alt, bool USE_BLENDING>
    void Image<color_t>::_drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity)
        {
        if (!isValid()) return;
        const iVec2 imdim = dim();
        tgx::RasterizerVec4 V1, V2, V3;

        const fVec2 U1 = _coord_viewport(P1, imdim);
        V1.x = U1.x;
        V1.y = U1.y;
        V1.color = RGBf(colorP1);
        V1.A = colorP1.opacity();

        const fVec2 U2 = _coord_viewport(P2, imdim);
        V2.x = U2.x;
        V2.y = U2.y;
        V2.color = RGBf(colorP2);
        V2.A = colorP2.opacity();

        const fVec2 U3 = _coord_viewport(P3, imdim);
        V3.x = U3.x;
        V3.y = U3.y;
        V3.color = RGBf(colorP3);
        V3.A = colorP3.opacity();

        tgx::RasterizerParams<color_t, color_t, float> rparam;
        rparam.im = this;
        rparam.tex = nullptr;
        rparam.opacity = opacity;

        if (USE_BLENDING)
            {
            tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_gradient<true,color_t>);
            }
        else
            {
            tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_gradient<false, color_t>);
            }

        }




    template<typename color_t>
    template<typename color_t_tex, bool GRADIENT, bool USE_BLENDING, bool MASKED>
    void Image<color_t>::_drawTexturedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        const iVec2 texdim = src_im.dim();
        const iVec2 imdim = dim();
        tgx::RasterizerVec4 V1, V2, V3;

        const fVec2 U1 = _coord_viewport(dstP1, imdim);
        V1.x = U1.x;
        V1.y = U1.y;
        V1.T = _coord_texture(srcP1, texdim);
        V1.color = RGBf(C1);
        V1.A = C1.opacity();

        const fVec2 U2 = _coord_viewport(dstP2, imdim);
        V2.x = U2.x;
        V2.y = U2.y;
        V2.T = _coord_texture(srcP2, texdim);
        V2.color = RGBf(C2);
        V2.A = C2.opacity();

        const fVec2 U3 = _coord_viewport(dstP3, imdim);
        V3.x = U3.x;
        V3.y = U3.y;
        V3.T = _coord_texture(srcP3, texdim);
        V3.color = RGBf(C3);
        V3.A = C3.opacity();

        tgx::RasterizerParams<color_t, color_t_tex, float> rparam;
        rparam.im = this;
        rparam.tex = &src_im;
        rparam.opacity = opacity;
        rparam.mask_color = transparent_color;

        if (MASKED)
            {
            if (!GRADIENT)
                {
                tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<true, true, false, color_t, color_t_tex>);
                }
            else
                {
                tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<true, true, true, color_t, color_t_tex>);
                }
            }
        else
            {
            if (USE_BLENDING)
                {
                if (!GRADIENT)
                    {
                    tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<true, false, false, color_t, color_t_tex>);
                    }
                else
                    {
                    tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<true, false, true, color_t, color_t_tex>);
                    }
                }
            else
                {
                if (!GRADIENT)
                    {
                    tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<false, false, false, color_t, color_t_tex>);
                    }
                else
                    {
                    tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture<false, false, true, color_t, color_t_tex>);
                    }
                }
            }
        }




    template<typename color_t>
    template<typename color_t_tex, typename BLEND_OPERATOR>
    void Image<color_t>::drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, const BLEND_OPERATOR& blend_op)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        const iVec2 texdim = src_im.dim();
        const iVec2 imdim = dim();
        tgx::RasterizerVec4 V1, V2, V3;

        const fVec2 U1 = _coord_viewport(dstP1, imdim);
        V1.x = U1.x;
        V1.y = U1.y;
        V1.T = _coord_texture(srcP1, texdim);
        V1.color = RGBf(1.0f, 1.0f, 1.0f);
        V1.A = 1.0f;

        const fVec2 U2 = _coord_viewport(dstP2, imdim);
        V2.x = U2.x;
        V2.y = U2.y;
        V2.T = _coord_texture(srcP2, texdim);
        V2.color = RGBf(1.0f, 1.0f, 1.0f);
        V2.A = 1.0f;

        const fVec2 U3 = _coord_viewport(dstP3, imdim);
        V3.x = U3.x;
        V3.y = U3.y;
        V3.T = _coord_texture(srcP3, texdim);
        V3.color = RGBf(1.0f, 1.0f, 1.0f);
        V3.A = 1.0f;

        tgx::RasterizerParams<color_t, color_t_tex, float, BLEND_OPERATOR> rparam;
        rparam.im = this;
        rparam.tex = &src_im;
        rparam.p_blend_op = &blend_op;

        tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_texture_blend_op<BLEND_OPERATOR, color_t, color_t_tex>);
        }







    /************************************************************************************
    * 
    *  Drawing Text
    * 
    *************************************************************************************/


    template<typename color_t>
    uint32_t Image<color_t>::_fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required)
        {
        uint32_t val;
        uint8_t* s = (uint8_t*)&p[index >> 3];
    #ifdef UNALIGNED_IS_SAFE        // is this defined anywhere ? 
        val = *(uint32_t*)s; // read 4 bytes - unaligned is ok
        val = __builtin_bswap32(val); // change to big-endian order
    #else
        val = s[0] << 24;
        val |= (s[1] << 16);
        val |= (s[2] << 8);
        val |= s[3];
    #endif
        val <<= (index & 7); // shift out used bits
        if (32 - (index & 7) < required) 
            { // need to get more bits
            val |= (s[4] >> (8 - (index & 7)));
            }
        val >>= (32 - required); // right align the bits
        return val;
        }


    template<typename color_t>
    uint32_t Image<color_t>::_fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required)
        {
        uint32_t val = _fetchbits_unsigned(p, index, required);
        if (val & (1 << (required - 1))) 
            {
            return (int32_t)val - (1 << required);
            }
        return (int32_t)val;
        }


    template<typename color_t>
    bool Image<color_t>::_clipit(int& x, int& y, int& sx, int& sy, int & b_left, int & b_up)
        {
        b_left = 0;
        b_up = 0; 
        if ((sx < 1) || (sy < 1) || (y >= _ly) || (y + sy <= 0) || (x >= _lx) || (x + sx <= 0))
            { // completely outside of image
            return false;
            }
        if (y < 0)
            {
            b_up = -y;
            sy += y;
            y = 0;
            }
        if (y + sy > _ly)
            {
            sy = _ly - y;
            }
        if (x < 0)
            {
            b_left = -x;
            sx += x;
            x = 0;
            }
        if (x + sx > _lx)
            {
            sx = _lx - x;
            }
        return true;
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const GFXfont& font, int * xadvance)
        {
        uint8_t n = (uint8_t)c;
        if ((n < font.first) || (n > font.last)) return iBox2(pos.x,pos.x,pos.y,pos.y); // nothing to draw. 
        auto& g = font.glyph[n - font.first];
        int x = pos.x + g.xOffset;
        int y = pos.y + g.yOffset;
        int sx = g.width;
        int sy = g.height;
        if (xadvance) *xadvance = g.xAdvance;
        return iBox2(x, x + sx - 1, y, y + sy - 1);
        }


    template<typename color_t>
    template<bool BLEND> iVec2 Image<color_t>::_drawCharGFX(char c, iVec2 pos, color_t col, const GFXfont& font, float opacity)
        {
        uint8_t n = (uint8_t)c;
        if ((n < font.first) || (n > font.last)) return pos; // nothing to draw. 
        auto& g = font.glyph[n - font.first];
        if ((!isValid()) || (font.bitmap == nullptr)) return pos;
        int x = pos.x + g.xOffset;
        int y = pos.y + g.yOffset;
        int sx = g.width;
        int sy = g.height;
        const int rsx = sx; // save the real bitmap width; 
        int b_left, b_up;
        if (!_clipit(x, y, sx, sy, b_left, b_up)) return iVec2(pos.x + g.xAdvance, pos.y);
        _drawCharBitmap_1BPP<BLEND>(font.bitmap + g.bitmapOffset, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
        return iVec2(pos.x + g.xAdvance, pos.y);
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureText(const char* text, iVec2 pos, const GFXfont& font, bool start_newline_at_0)
        {
        const int startx = start_newline_at_0 ? 0 : pos.x;
        iBox2 B;
        B.empty();
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += font.yAdvance;
                }
            else
                {
                int xa = 0;
                B |= measureChar(c, pos, font, &xa);
                pos.x += xa;
                }
            }
        return B; 
        }


    template<typename color_t>
    template<bool BLEND>
    iVec2 Image<color_t>::_drawTextGFX(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0, float opacity)
        {
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += font.yAdvance;
                }
            else
                {
                pos = _drawCharGFX<BLEND>(c, pos, col, font, opacity);
                }
            }
        return pos;
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, int* xadvance)
        {
        uint8_t n = (uint8_t)c;
        if ((n >= font.index1_first) && (n <= font.index1_last))
            {
            n -= font.index1_first;
            }
        else if ((n >= font.index2_first) && (n <= font.index2_last))
            {
            n = (n - font.index2_first) + (font.index1_last - font.index1_first + 1);
            }
        else
            { // no char to draw
            return iBox2(); // nothing to draw. 
            }
        uint8_t* data = (uint8_t*)font.data + _fetchbits_unsigned(font.index, (n * font.bits_index), font.bits_index);
        int32_t off = 0;
        uint32_t encoding = _fetchbits_unsigned(data, off, 3);
        if (encoding != 0) return  pos; // wrong/unsupported format
        off += 3;
        const int sx = (int)_fetchbits_unsigned(data, off, font.bits_width);
        off += font.bits_width;
        const int sy = (int)_fetchbits_unsigned(data, off, font.bits_height);
        off += font.bits_height;
        const int xoffset = (int)_fetchbits_signed(data, off, font.bits_xoffset);
        off += font.bits_xoffset;
        const int yoffset = (int)_fetchbits_signed(data, off, font.bits_yoffset);
        off += font.bits_yoffset;
        if (xadvance)
            {
            *xadvance = (int)_fetchbits_unsigned(data, off, font.bits_delta);
            }
        const int x = pos.x + xoffset;
        const int y = pos.y - sy - yoffset;
        return iBox2(x, x + sx - 1, y, y + sy - 1);
        }



    template<typename color_t>
    template<bool BLEND> iVec2 Image<color_t>::_drawCharILI(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, float opacity)
        {
        if (!isValid()) return pos;
        uint8_t n = (uint8_t)c;
        if ((n >= font.index1_first) && (n <= font.index1_last))
            {
            n -= font.index1_first;
            }
        else if ((n >= font.index2_first) && (n <= font.index2_last))
            {
            n = (n - font.index2_first) + (font.index1_last - font.index1_first + 1);
            }
        else
            { // no char to draw
            return pos;
            }
        uint8_t * data = (uint8_t *)font.data + _fetchbits_unsigned(font.index, (n*font.bits_index), font.bits_index);
        int32_t off = 0; 
        uint32_t encoding = _fetchbits_unsigned(data, off, 3);
        if (encoding != 0) return  pos; // wrong/unsupported format
        off += 3;
        int sx = (int)_fetchbits_unsigned(data, off, font.bits_width);
        off += font.bits_width;         
        int sy = (int)_fetchbits_unsigned(data, off, font.bits_height);
        off += font.bits_height;            
        const int xoffset = (int)_fetchbits_signed(data, off, font.bits_xoffset);
        off += font.bits_xoffset;
        const int yoffset = (int)_fetchbits_signed(data, off, font.bits_yoffset);
        off += font.bits_yoffset;
        const int delta = (int)_fetchbits_unsigned(data, off, font.bits_delta);
        off += font.bits_delta;
        int x = pos.x + xoffset;
        int y = pos.y - sy - yoffset;
        const int rsx = sx; // save the real bitmap width; 
        int b_left, b_up;
        if (!_clipit(x, y, sx, sy, b_left, b_up)) return iVec2(pos.x + delta, pos.y);
        if (font.version == 1)
            { // non-antialiased font
            _drawCharILI9341_t3<BLEND>(data, off, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
            }
        else if (font.version == 23)
            { // antialised font
            data += (off >> 3) + ((off & 7) ? 1 : 0); // bitmap begins at the next byte boundary
            switch (font.reserved)
                {
                case 0: 
                    _drawCharBitmap_1BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
                    break;
                case 1:
                    _drawCharBitmap_2BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
                    break;
                case 2:
                    _drawCharBitmap_4BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
                    break;
                case 3:
                    _drawCharBitmap_8BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
                    break;
                }
            }
        return iVec2(pos.x + delta, pos.y);
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureText(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, bool start_newline_at_0)
        {
        const int startx = start_newline_at_0 ? 0 : pos.x;
        iBox2 B;
        B.empty();
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += font.line_space;
                }
            else
                {
                int xa = 0;
                B |= measureChar(c, pos, font, &xa);
                pos.x += xa;
                }
            }
        return B;
        }


    template<typename color_t>
    template<bool BLEND>
    iVec2 Image<color_t>::_drawTextILI(const char* text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0, float opacity)
        {
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += font.line_space;
                }
            else
                {
                pos = _drawCharILI<BLEND>(c, pos, col, font, opacity);
                }
            }
        return pos;
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
        {
        uint32_t rl = 0; // number of line repeat remaining. 
        while (b_up > 0)
            { // need to skip lines
            if (_fetchbit(bitmap, off++))
                { // this is a repeating line
                int n = (int)_fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
                if (n <= b_up) 
                    {  
                    b_up -= n;
                    off += (rsx + 3);
                    }
                else
                    {
                    rl = (n - b_up); // number of repeat remaining (at least 1)
                    off += 3; // beginning of the line pixels
                    b_up = 0;
                    break;
                    }
                }
            else
                { // skipping a single line
                b_up--; 
                off += rsx; 
                }
            }

        while (sy-- > 0)
            { // iterate over lines to draw
            if (rl == 0)
                { // need to read the line header. 
                if (_fetchbit(bitmap, off++))
                    { // repeating line
                    rl = _fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
                    off += 3; // go the the beginning of the line pixels
                    }
                else
                    {
                    rl = 1; // repeat only once, already at beginning of line pixels 
                    }
                }
            // off is now pointing to the begining of the line pixels and we can draw it. 
            _drawcharline<BLEND>(bitmap, off + b_left, _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), sx, col, opacity);
            if ((--rl) == 0)
                { // done repeating so we move to the next line in the bitmap
                off += rsx;
                }
            y++; // next line in the destination image
            }
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col, float opacity)
        {
        bitmap += (off >> 3);                       // move to the correct byte
        uint8_t u = (uint8_t)(128 >> (off & 7));    // index of the first bit
        if (dx >= 8)
            { // line has at least 8 pixels
            if (u != 128)
                { // not at the start of a bitmap byte: we first finish it. 
                const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
                while (u > 0)
                    {
                    if (b & u) { if (BLEND) { (*p).blend(col, opacity);  } else { *p = col; } }
                    p++; dx--; u >>= 1;
                    }
                u = 128;
                }
            while (dx >= 8)
                { // now we can write 8 pixels consecutively. 
                const uint8_t b = *(bitmap++);
                if (b)
                    { // there is something to write
                    if (b & 128) {if (BLEND) { p[0].blend(col, opacity); } else { p[0] = col; }}
                    if (b & 64) { if (BLEND) { p[1].blend(col, opacity); } else { p[1] = col; } }
                    if (b & 32) { if (BLEND) { p[2].blend(col, opacity); } else { p[2] = col; } }
                    if (b & 16) { if (BLEND) { p[3].blend(col, opacity); } else { p[3] = col; } }
                    if (b & 8) { if (BLEND) { p[4].blend(col, opacity); } else { p[4] = col; } }
                    if (b & 4) { if (BLEND) { p[5].blend(col, opacity); } else { p[5] = col; } }
                    if (b & 2) { if (BLEND) { p[6].blend(col, opacity); } else { p[6] = col; } }
                    if (b & 1) { if (BLEND) { p[7].blend(col, opacity); } else { p[7] = col; } }
                    }
                p += 8;
                dx -= 8;
                }
            // strictly less than 8 pixels remain 
            if (dx > 0)
                {
                const uint8_t b = *bitmap;
                if (b)                  
                    {
                    do
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--; u >>= 1;
                    } while (dx > 0);
                    }
                }
            }
        else
            { // each row has less than 8 pixels
            if ((u >> (dx - 1)) == 0)
                { // we cannot complete the line with this byte
                const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
                while (u > 0)
                    {
                    if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                    p++; dx--; u >>= 1;
                    }
                u = 128;
                }
            if (dx > 0)
                {
                const uint8_t b = *bitmap; // we know we will complete the line with this byte
                if (b)
                    { // there is something to draw
                    do
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--;  u >>= 1;
                    } while (dx > 0);
                    }
                }
            }
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy,int x, int y, color_t col, float opacity)
        {
        int32_t off = TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
        bitmap += (off >> 3);                       // starting byte in the bitmap
        uint8_t u = (uint8_t)(128 >> (off & 7));    // index of the first bit 
        const int sk = (rsx - sx);              // number of bits to skip at the end of a row.
        color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y); // start position in destination buffer
        if (sx >= 8)
            { // each row has at least 8 pixels
            for (int dy = sy; dy > 0; dy--)
                {
                int dx = sx; // begining of row, number of char to write
                if (u != 128)
                    { // not at the start of a bitmap byte: we first finish it. 
                    const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
                    while (u > 0)
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--; u >>= 1;
                        }
                    u = 128;
                    }
                while (dx >= 8)
                    { // now we can write 8 pixels consecutively. 
                    const uint8_t b = *(bitmap++);
                    if (b)
                        {
                        if (b & 128) { if (BLEND) { p[0].blend(col, opacity); } else { p[0] = col; } }
                        if (b & 64) { if (BLEND) { p[1].blend(col, opacity); } else { p[1] = col; } }
                        if (b & 32) { if (BLEND) { p[2].blend(col, opacity); } else { p[2] = col; } }
                        if (b & 16) { if (BLEND) { p[3].blend(col, opacity); } else { p[3] = col; } }
                        if (b & 8) { if (BLEND) { p[4].blend(col, opacity); } else { p[4] = col; } }
                        if (b & 4) { if (BLEND) { p[5].blend(col, opacity); } else { p[5] = col; } }
                        if (b & 2) { if (BLEND) { p[6].blend(col, opacity); } else { p[6] = col; } }
                        if (b & 1) { if (BLEND) { p[7].blend(col, opacity); } else { p[7] = col; } }
                        }
                    p += 8;
                    dx -= 8;
                    }
                // strictly less than 8 pixels remain on the row 
                if (dx > 0)
                    {
                    const uint8_t b = *bitmap; // do not increment bitmap now since we know we will not finish this byte now.
                    do
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--; u >>= 1;
                        } while (dx > 0);
                    }
                // row is now complete
                p += (_stride - sx); // go to the beginning of the next row
                if (sk != 0)
                    { // we must skip some pixels...
                    bitmap += (sk >> 3);
                    uint16_t v = (((uint16_t)u) << (8 - (sk & 7)));
                    if (v & 255)
                        {
                        u = (uint8_t)(v & 255);
                        bitmap++;
                        }
                    else
                        {
                        u = (uint8_t)(v >> 8);
                        }
                    }
                }
            }
        else
            { // each row has less than 8 pixels
            for (int dy = sy; dy > 0; dy--)
                {
                int dx = sx;
                if ((u >> (sx - 1)) == 0)
                    { // we cannot complete the row with this byte
                    const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
                    while (u > 0)
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--; u >>= 1;
                        }
                    u = 128;
                    }
                if (dx > 0)
                    {
                    const uint8_t b = *bitmap; // we know we will complete the row with this byte
                    do
                        {
                        if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
                        p++; dx--;  u >>= 1;
                        } while (dx > 0);
                    }
                if (u == 0) { bitmap++; u = 128; }
                // row is now complete
                p += (_stride - sx); // go to the beginning of the next row
                if (sk != 0)
                    { // we must skip some pixels...
                    bitmap += (sk >> 3);
                    uint16_t v = (((uint16_t)u) << (8 - (sk & 7)));
                    if (v & 255)
                        {
                        u = (uint8_t)(v & 255);
                        bitmap++;
                        }
                    else
                        {
                        u = (uint8_t)(v >> 8);
                        }
                    }
                }
            }
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
        { 
        int iop = 171 * (int)(256 * opacity);
        if (sx >= 4)
            { // each row has at least 4 pixels
            for (int dy = 0; dy < sy; dy++)
                {
                int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
                color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); 
                int dx = sx;
                const int32_t uu = off & 3;
                if (uu)
                    {// not at the start of a bitmap byte: we first finish it. 
                    const uint8_t b = bitmap[off >> 2];
                    switch (uu)
                        {
                        case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
                        case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
                        case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
                        }
                    }
                while (dx >= 4)
                    { // now we can write 4 pixels consecutively. 
                    const uint8_t b = bitmap[off >> 2];
                    if (b)
                        {
                            { const int v = ((b & 192) >> 6); p[0].blend256(col, (v * iop) >> 9); }
                            { const int v = ((b & 48) >> 4); p[1].blend256(col, (v * iop) >> 9); }
                            { const int v = ((b & 12) >> 2); p[2].blend256(col, (v * iop) >> 9); }
                            { const int v = (b & 3); p[3].blend256(col, (v * iop) >> 9); }
                        }
                    off += 4;
                    p += 4;
                    dx -= 4;
                    }
                // strictly less than 4 pixels remain on the row 
                if (dx > 1)
                    {
                    const uint8_t b = bitmap[off >> 2];
                    {const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); }
                    {const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); }
                    if (dx > 2) { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); }
                    }
                else
                    {
                    if (dx > 0) 
                        { 
                        const uint8_t b = bitmap[off >> 2];
                        const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); 
                        }
                    }                   
                }
            }
        else
            { // each row has less than 4 pixels
            for (int dy = 0; dy < sy; dy++)
                {
                int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left); // offset for the start of this line
                color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); // start position in destination buffer
                int dx = sx;
                const int32_t uu = off & 3;
                if ((4 - uu) < sx)
                    { // we cannot complete the row with this byte
                    const uint8_t b = bitmap[off >> 2];
                    switch (uu)
                        {
                        case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; } // this case should never occur
                        case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
                        case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
                        }
                    }   
                if (dx > 0)
                    {
                    const uint8_t b = bitmap[off >> 2];
                    while (dx-- > 0)
                        {
                        const int32_t uu = (off++) & 3;
                        switch (uu)
                            {
                            case 0: { const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); break; }
                            case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); break; }
                            case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); break; }
                            case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); break; }
                            }
                        }
                    }
                }
            }
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
        { 
        int iop = 137 * (int)(256 * opacity);
        if (sx >= 2)
            { // each row has at least 2 pixels
            for (int dy = 0; dy < sy; dy++)
                {
                int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
                color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); 
                int dx = sx;
                if (off & 1)
                    {// not at the start of a bitmap byte: we first finish it. 
                    const uint8_t b = bitmap[off >> 1];
                    const int v = (b & 15); (p++)->blend256(col, (v * iop) >> 11); 
                    off++; dx--; 
                    }
                while (dx >= 2)
                    {
                    const uint8_t b = bitmap[off >> 1];
                    if (b)
                        {
                            { const int v = ((b & 240) >> 4); p[0].blend256(col, (v * iop) >> 11); }
                            { const int v = (b & 15); p[1].blend256(col, (v * iop) >> 11); }
                        }
                    off += 2;
                    p += 2;
                    dx -= 2;
                    }
                if (dx > 0)
                    {
                    const uint8_t b = bitmap[off >> 1];                 
                    const int v = ((b & 240) >> 4); p->blend256(col, (v * iop) >> 11);
                    }
                }
            }
        else
            { // each row has a single pixel 
            color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y) + TGX_CAST32(x);
            int32_t off = TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
            while(sy-- > 0)
                {
                const uint8_t b = bitmap[off >> 1];
                const int v = (off & 1) ? (b & 15) : ((b & 240) >> 4);
                p->blend256(col, (v * iop) >> 11);
                p += _stride;
                off += rsx;
                }
            }
        }


    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
        {
        int iop = 129 * (int)(256 * opacity);
        const uint8_t * p_src = bitmap + TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
        color_t * p_dst = _buffer + TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y);
        const int sk_src = rsx - sx;
        const int sk_dst = _stride - sx;
        while (sy-- > 0)
            {
            int dx = sx;
            while (dx-- > 0)
                {
                uint32_t cc = *(p_src++);
                (*(p_dst++)).blend256(col, (cc*iop) >> 15); 
                }
            p_src += sk_src;
            p_dst += sk_dst;
            }
        }





}


#endif

/** end of file */


