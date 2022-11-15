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




/* TODO : Things to add to the Image class...
 *-------------------------------------------------
 * - drawing rotated Ellipse
 * - drawing a general path, svg compatibility
 *
 */




/************************************************************************************
*
* 
* Implementation file for the template class Image<color_t>
* 
* 
*************************************************************************************/
namespace tgx
{




    /** Used for debuging.   
        Detect a collision bug when drawing by looking for a no pure color (i.e. with at least 2  non zero color channels */
    template<typename color_t>
    bool Image<color_t>::_collision()
        {
        bool bug = false;
        iterate(
            [&](tgx::iVec2 pos, tgx::RGB32 col)
            {
                int r = (col.R > 0) ? 1 : 0;
                int g = (col.G > 0) ? 1 : 0;
                int b = (col.B > 0) ? 1 : 0;
                if (r + g + b > 1)
                {
                    bug = true;  return false;
                }
                return true;
            });
        return bug;
        }




    /************************************************************************************
    *************************************************************************************
    *  CREATION OF IMAGES AND SUB-IMAGES
    *
    * The memory buffer should be supplied at creation. Otherwise, the image is set as
    * invalid until a valid buffer is supplied.
    *
    * NOTE: the image class itsel is lightweight as it does not manage the memory buffer.
    *       Creating image and sub-image is very fast and do not use much memory.
    *************************************************************************************
    *************************************************************************************/



    template<typename color_t>
    Image<color_t>::Image() : _buffer(nullptr), _lx(0), _ly(0), _stride(0)
        {
        }


    template<typename color_t>
    template<typename T> Image<color_t>::Image(T* buffer, int lx, int ly, int stride) : _buffer((color_t*)buffer), _lx(lx), _ly(ly), _stride(stride < 0 ? lx : stride)
        {
        _checkvalid(); // make sure dimension/stride are ok else make the image invalid
        }


    template<typename color_t>
    template<typename T> Image<color_t>::Image(T* buffer, iVec2 dim, int stride) : Image<color_t>((color_t*)buffer, dim.x, dim.y, stride)
        {
        }


    template<typename color_t>
    template<typename T> void Image<color_t>::set(T* buffer, iVec2 dim, int stride)
        {
        set<color_t>((color_t*)buffer, dim.x, dim.y, stride);
        }


    template<typename color_t>
    void Image<color_t>::crop(const iBox2& subbox)
        {
        *this = Image<color_t>(*this, subbox);
        }


    template<typename color_t>
    Image<color_t> Image<color_t>::getCrop(const iBox2& subbox) const
        {
        return Image<color_t>(*this, subbox);
        }


    template<typename color_t>
    Image<color_t> Image<color_t>::operator()(const iBox2& B) const
        {
        return Image<color_t>(*this, B);
        }


    template<typename color_t>
    Image<color_t> Image<color_t>::operator()(int min_x, int max_x, int min_y, int max_y) const
        {
        return Image<color_t>(*this, tgx::iBox2(min_x, max_x, min_y, max_y));
        }


    template<typename color_t>
    Image<color_t>::Image(const Image<color_t> & im, iBox2 subbox)
        {
        if (!im.isValid()) { setInvalid();  return; }       
        subbox &= im.imageBox();
        if (subbox.isEmpty()) { setInvalid(); return; }
        _lx = subbox.lx();
        _ly = subbox.ly();
        _stride = im._stride;
        _buffer = im._buffer + TGX_CAST32(subbox.minX) + TGX_CAST32(im._stride) * TGX_CAST32(subbox.minY);
        }


    template<typename color_t>
    template<typename T> void Image<color_t>::set(T* buffer, int lx, int ly, int stride)
        {
        _buffer = (color_t*)buffer;
        _lx = lx;
        _ly = ly;
        _stride = (stride < 0) ? lx : stride;
        _checkvalid(); // make sure dimension/stride are ok else make the image invalid
        }


    template<typename color_t>
    void Image<color_t>::setInvalid()
        {
        _buffer = nullptr;
        _lx = 0;
        _ly = 0;
        _stride = 0;
        }




    /************************************************************************************
    *************************************************************************************
    *
    *  DIRECT PIXEL ACCESS
    *
    *************************************************************************************
    *************************************************************************************/




    template<typename color_t>
    template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun) const
        {
        iterate(cb_fun, imageBox());
        }


    template<typename color_t>
    template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun)
        {
        iterate(cb_fun, imageBox());
        }


    template<typename color_t>
    template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun, tgx::iBox2 B)
        {
        B &= imageBox();
        if (B.isEmpty()) return;
        for (int j = B.minY; j <= B.maxY; j++)
            {
            for (int i = B.minX; i <= B.maxX; i++)
                {
                if (!cb_fun(tgx::iVec2(i, j), operator()({ i, j }))) return;
                }
            }
        }


    template<typename color_t>
    template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun, tgx::iBox2 B) const
        {
        B &= imageBox();
        if (B.isEmpty()) return;
        for (int j = B.minY; j <= B.maxY; j++)
            {
            for (int i = B.minX; i <= B.maxX; i++)
                {
                if (!cb_fun(tgx::iVec2(i, j), operator()({ i, j }))) return;
                }
            }
        }








    /************************************************************************************
    *************************************************************************************
    *
    * BLITTING, COPYING AND RESIZING IMAGES
    *
    *************************************************************************************
    *************************************************************************************/


    template<typename color_t>
    void Image<color_t>::blit(const Image<color_t>& sprite, iVec2 upperleftpos, float opacity)
        {
        if ((opacity < 0) || (opacity > 1))
            _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly());
        else
            _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), opacity);
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::blit(const Image<color_t_src>& sprite, iVec2 upperleftpos, const BLEND_OPERATOR& blend_op)
        {
        _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), blend_op);
        }


    template<typename color_t>
    void Image<color_t>::blitMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 upperleftpos, float opacity)
        {
        _blitMasked(sprite, transparent_color, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), opacity);
        }


    template<typename color_t>
    void Image<color_t>::blitBackward(Image<color_t>& dst_sprite, iVec2 upperleftpos) const
        {
        dst_sprite._blit(*this, 0, 0, upperleftpos.x, upperleftpos.y, dst_sprite.lx(), dst_sprite.ly());
        }


    template<typename color_t>
    template<typename color_t_src, int CACHE_SIZE>
    void Image<color_t>::blitScaledRotated(const Image<color_t_src> src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity)
        {
        if ((opacity < 0) || (opacity > 1))
            _blitScaledRotated<color_t_src, CACHE_SIZE, false, false, false>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, 1.0f, [](color_t_src cola, color_t colb) {return colb; });
        else
            _blitScaledRotated<color_t_src, CACHE_SIZE, true, false, false>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, opacity, [](color_t_src cola, color_t colb) {return colb; });
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR, int CACHE_SIZE>
    void Image<color_t>::blitScaledRotated(const Image<color_t_src>& src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, const BLEND_OPERATOR& blend_op)
        {
        _blitScaledRotated<color_t_src, CACHE_SIZE, true, false, true>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, 1.0f, blend_op);
        }


    template<typename color_t>
    template<typename color_t_src, int CACHE_SIZE>
    void Image<color_t>::blitScaledRotatedMasked(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity)
        {
        _blitScaledRotated<color_t_src, CACHE_SIZE, true, true, false>(src_im, transparent_color, anchor_src, anchor_dst, scale, angle_degrees, ((opacity < 0.0f) || (opacity > 1.0f)) ? 1.0f : opacity, [](color_t_src cola, color_t colb) {return colb; });
        }


    template<typename color_t>
    Image<color_t> Image<color_t>::reduceHalf()
        {
        return copyReduceHalf(*this);
        }




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
        return _blitClip(sprite._lx, sprite._ly, dest_x, dest_y, sprite_x, sprite_y, sx, sy);
        }


    template<typename color_t>
    bool Image<color_t>::_blitClip(int sprite_lx, int sprite_ly, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy)
        {
        if (sprite_x < 0) { dest_x -= sprite_x; sx += sprite_x; sprite_x = 0; }
        if (sprite_y < 0) { dest_y -= sprite_y; sy += sprite_y; sprite_y = 0; }
        if (dest_x < 0) { sprite_x -= dest_x;   sx += dest_x; dest_x = 0; }
        if (dest_y < 0) { sprite_y -= dest_y;   sy += dest_y; dest_y = 0; }
        if ((dest_x >= _lx) || (dest_y >= _ly) || (sprite_x >= sprite_lx) || (sprite_y >= sprite_ly)) return false;
        sx -= max(0, (dest_x + sx - _lx));
        sy -= max(0, (dest_y + sy - _ly));
        sx -= max(0, (sprite_x + sx - sprite_lx));
        sy -= max(0, (sprite_y + sy - sprite_ly));
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
        if ((opacity < 0.0f) || (opacity > 1.0f)) opacity = 1.0f;
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
    void Image<color_t>::blitRotated(const Image<color_t> & sprite, iVec2 upperleftpos, int angle, float opacity)
        {
        if ((!sprite.isValid()) || (!isValid())) return; 
        switch (angle)
            {
            case 0: 
                blit(sprite, upperleftpos, opacity);
                break;
            case 90:
                _blitRotated90(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, opacity);
                break;
            case 180:
                _blitRotated180(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, opacity);
                break;
            case 270: 
                _blitRotated270(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, opacity);
                break;
            default:
                // do nothing...
                break;
            }
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::blitRotated(const Image<color_t_src>& sprite, iVec2 upperleftpos, int angle, const BLEND_OPERATOR& blend_op)
        {
        if ((!sprite.isValid()) || (!isValid())) return; 
        switch (angle)
            {
            case 0: 
                blit(sprite, upperleftpos, blend_op);
                break;
            case 90:
                _blitRotated90<color_t_src, BLEND_OPERATOR>(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, blend_op);
                break;
            case 180:
                _blitRotated180<color_t_src, BLEND_OPERATOR>(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, blend_op);
                break;
            case 270: 
                _blitRotated270<color_t_src, BLEND_OPERATOR>(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite._lx, sprite._ly, blend_op);
                break;
            default:
                // do nothing...
                break;
            }
        }





    template<typename color_t>
    void Image<color_t>::_blitRotated90(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
        {      
        if (!_blitClip(sprite._ly, sprite._lx, dest_x, dest_y, sprite_x, sprite_y, sy, sx)) return;
        const int spx = sprite._lx - sprite_y - sx;
        const int spy = sprite_x;                     
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        if ((opacity < 0) || (opacity > 1))
            {
            for (int i = sx - 1; i >= 0; i--)
                {
                for (int j = 0; j < sy; j++)
                    {
                    *(pdst++) = psrc[i + (sp_stride * j)];
                    }
                pdst +=(_stride - sy);
                }
            }
        else
            {
            const int op256 = (int)(opacity * 256);
            for (int i = sx - 1; i >= 0; i--)
                {
                for (int j = 0; j < sy; j++)
                    {
                    (pdst++)->blend256(psrc[i + (sp_stride * j)], op256);
                    }
                pdst += (_stride - sy);
                }
            }
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blitRotated90(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {      
        if (!_blitClip(sprite._ly, sprite._lx, dest_x, dest_y, sprite_x, sprite_y, sy, sx)) return;
        const int spx = sprite._lx - sprite_y - sx;
        const int spy = sprite_x;                     
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        for (int i = sx - 1; i >= 0; i--)
            {
            for (int j = 0; j < sy; j++)
                {
                color_t& c = *(pdst++);
                c = (color_t)blend_op(psrc[i + (sp_stride * j)], c);
                }
            pdst +=(_stride - sy);
            }
        }



    template<typename color_t>
    void Image<color_t>::_blitRotated180(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
        {        
        if (!_blitClip(sprite._lx, sprite._ly, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        const int spx = sprite._lx - sprite_x - sx;
        const int spy = sprite._ly - sprite_y - sy;
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        if ((opacity < 0) || (opacity > 1))
            {
            for (int j = sy - 1; j >= 0; j--)
                {
                for (int i = sx - 1; i >= 0; i--)
                    {
                    *(pdst++) = psrc[i + (sp_stride * j)];
                    }
                pdst += (_stride - sx);
                }
            } 
        else
            {
            const int op256 = (int)(opacity * 256);
            for (int j = sy - 1; j >= 0; j--)
                {
                for (int i = sx - 1; i >= 0; i--)
                    {
                    (pdst++)->blend256(psrc[i + (sp_stride * j)], op256);
                    }
                pdst += (_stride - sx);
                }            
            }
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blitRotated180(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {        
        if (!_blitClip(sprite._lx, sprite._ly, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
        const int spx = sprite._lx - sprite_x - sx;
        const int spy = sprite._ly - sprite_y - sy;
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        for (int j = sy - 1; j >= 0; j--)
            {
            for (int i = sx - 1; i >= 0; i--)
                {
                color_t& c = *(pdst++);
                c = (color_t)blend_op(psrc[i + (sp_stride * j)], c);
                }
            pdst += (_stride - sx);
            }
        }



    template<typename color_t>
    void Image<color_t>::_blitRotated270(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
        {        
        if (!_blitClip(sprite._ly, sprite._lx, dest_x, dest_y, sprite_x, sprite_y, sy, sx)) return;
        const int spx = sprite_y;
        const int spy = sprite._ly - sprite_x - sy;
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        if ((opacity < 0) || (opacity > 1))
            {
            for (int i = 0; i < sx; i++)
                {
                for (int j = sy - 1; j >= 0; j--)
                    {
                    *(pdst++) = psrc[i + (sp_stride * j)];
                    }
                pdst += (_stride - sy);
                }
            } 
        else
            {
            const int op256 = (int)(opacity * 256);
            for (int i = 0; i < sx; i++)
                {
                for (int j = sy - 1; j >= 0; j--)
                    {
                    (pdst++)->blend256(psrc[i + (sp_stride * j)], op256);
                    }
                pdst += (_stride - sy);
                }
            }
        }


    template<typename color_t>
    template<typename color_t_src, typename BLEND_OPERATOR>
    void Image<color_t>::_blitRotated270(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op)
        {        
        if (!_blitClip(sprite._ly, sprite._lx, dest_x, dest_y, sprite_x, sprite_y, sy, sx)) return;
        const int spx = sprite_y;
        const int spy = sprite._ly - sprite_x - sy;
        // ok draw subsprite (spx,spy) with size (sx,sy) to this image at upper left pos (dest_x, dest_y)
        const int sp_stride = sprite._stride;
        color_t* psrc = sprite._buffer + TGX_CAST32(spx) + TGX_CAST32(spy) * TGX_CAST32(sp_stride);
        color_t* pdst = _buffer + TGX_CAST32(dest_x) + TGX_CAST32(dest_y) * TGX_CAST32(_stride);
        for (int i = 0; i < sx; i++)
            {
            for (int j = sy - 1; j >= 0; j--)
                {
                color_t& c = *(pdst++);
                c = (color_t)blend_op(psrc[i + (sp_stride * j)], c);
                }
            pdst += (_stride - sy);
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
                return Image<color_t>(*this, iBox2(0, 0, 0, 0));
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
            return Image<color_t>(*this, iBox2(0, 0, 0, (src_image._ly >> 1) - 1));
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
            return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, 0));
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
        return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, (src_image._ly >> 1) - 1));
        }


    template<typename color_t>
    template<typename color_t_src, int CACHE_SIZE, bool USE_BLENDING, bool USE_MASK, bool USE_CUSTOM_OPERATOR, typename BLEND_OPERATOR>
    void Image<color_t>::_blitScaledRotated(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity, const BLEND_OPERATOR& blend_op)
        {
        if ((!isValid()) || (!src_im.isValid())) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
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
                drawTexturedMaskedQuad(src_im, transparent_color, fVec2(0.0f, y1), fVec2(tlx, y1), fVec2(tlx, y2), fVec2(0.0f, y2), U1, U2, U3, U4, opacity);
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






    template<typename color_t>
    template<typename color_dst>
    Image<color_dst> Image<color_t>::convert()
        {
        static_assert((sizeof(color_t) % sizeof(color_dst)) == 0, "Cannot convert image in place: the size of 'color_dest' must divides that of 'color_t'.");
        const int stride = (_stride == _lx) ? _stride : (_stride * (int)(sizeof(color_t) / sizeof(color_dst)));
        if ((!std::is_same<color_t, color_dst>::value)&&(isValid()))
            {
            color_t* p = _buffer;
            color_dst* q = (color_dst*)_buffer;
            for (int j = 0; j < _ly; j++)
                {
                for (int i = 0; i < _lx; i++)
                    {
                    (*q++) = (color_dst)(*p++);
                    }
                q += (stride - _lx);
                p += (_stride - _lx);
                }
            }
        return Image<color_dst>((color_dst*)_buffer, _lx, _ly, stride);
        }





    /************************************************************************************
    *************************************************************************************
    *
    *  DRAWING PRIMITIVES
    *
    *************************************************************************************
    *************************************************************************************/






    /********************************************************************************
    *
    * FILLING (A REGION OF) AN IMAGE.
    *
    *********************************************************************************/



    template<typename color_t>
    void Image<color_t>::fillScreen(color_t color) 
        {
        clear(color);
        }

    template<typename color_t>
    void Image<color_t>::clear(color_t color)
        {
        fillRect(imageBox(), color);
        }



    template<typename color_t>
    void Image<color_t>::fillScreenVGradient(color_t top_color, color_t bottom_color)
        {
        fillRectVGradient(imageBox(), top_color, bottom_color);
        }


    template<typename color_t>
    void Image<color_t>::fillScreenHGradient(color_t left_color, color_t right_color)
        {
        fillRectHGradient(imageBox(), left_color, right_color);
        }


    template<typename color_t>
    template<int STACK_SIZE> int Image<color_t>::fill(iVec2 start_pos, color_t new_color)
        {
        return _scanfill<true, STACK_SIZE>(start_pos.x, start_pos.y, new_color, new_color);
        }


    template<typename color_t>
    template<int STACK_SIZE > int Image<color_t>::fill(iVec2 start_pos, color_t border_color, color_t new_color)
        {
        return _scanfill<false, STACK_SIZE>(start_pos.x, start_pos.y, border_color, new_color);
        }


    /** scanline seed fill algorithm taken from Graphic Gems 1 (1995), chap IV.10 p271. */
    template<typename color_t>
    template<bool UNICOLOR_COMP, int STACK_SIZE_BYTES> int Image<color_t>::_scanfill(int x, int y, color_t border_color, color_t new_color)
        {

        #define TGX_SCANFILL_PUSH(X1,X2,Y,DY)   { \
                                                if (stp == (STACK_LEN)) return -1; \
                                                if (((Y) + (DY) >= 0)&((Y) + (DY) < _ly)) \
                                                    { \
                                                    Qx1[stp] = (uint16_t)(X1); \
                                                    Qx2[stp] = (uint16_t)(X2); \
                                                    Qy[stp] = (uint16_t)(((Y) << 1) | (((DY) > 0) ? 1 : 0)); \
                                                    stp++; \
                                                    if (stp > max_st) { max_st = stp; } \
                                                    } \
                                                }


        #define TGX_SCANFILL_POP(X1,X2,Y,DY)    { \
                                                stp--; \
                                                X1 = (int)Qx1[stp]; \
                                                X2 = (int)Qx2[stp]; \
                                                DY = (int)((Qy[stp] & 1) ? 1 : -1); \
                                                Y = (int)(Qy[stp] >> 1) + (DY); \
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
        const color_t orig_color = readPixel<false>({ x, y });
        if ((UNICOLOR_COMP) && (orig_color == new_color)) return 0; // nothing to do
        if (!TGX_SCANFILL_INSIDE(orig_color)) return 0; // nothing to do 

        TGX_SCANFILL_PUSH(x, x, y, 1);
        TGX_SCANFILL_PUSH(x, x, y + 1, -1);
        int start;
        while (TGX_SCANFILL_STACKSIZE > 0)
            {
            int x1, x2, dy;
            TGX_SCANFILL_POP(x1, x2, y, dy); // segment previously filled was [x1,x2] x {y - dy}
            x = x1;
            while ((x >= 0) && (TGX_SCANFILL_INSIDE(readPixel<false>({ x, y }))))
                {
                _drawPixel<false>({ x--, y }, new_color);
                }
            if (x >= x1) goto TGX_SCANFILL_SKIP;
            start = x + 1;
            if (start < x1) TGX_SCANFILL_PUSH(start, x1 - 1, y, -dy); // leak on left
            x = x1 + 1;
            do
                {
                while ((x < _lx) && (TGX_SCANFILL_INSIDE(readPixel<false>({ x, y }))))
                    {
                    _drawPixel<false>({ x++, y }, new_color);
                    }
                TGX_SCANFILL_PUSH(start, x - 1, y, dy);
                if (x > x2 + 1) TGX_SCANFILL_PUSH(x2 + 1, x - 1, y, -dy); // leak on right
            TGX_SCANFILL_SKIP:
                x++;
                while ((x <= x2) && (!(TGX_SCANFILL_INSIDE(readPixel<false>({ x, y }))))) { x++; }
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









    /********************************************************************************
    *
    * DRAWING LINES
    *
    *********************************************************************************/



    /*****************************************************
    * BRESENHAM METHODS (DRAWING LINES AND TRIANGLE FILLING
    ******************************************************/



    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_draw_template(BSeg& seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange)
        {
        if (!draw_first) seg.move();
        if (draw_last) seg.inclen();
        if (checkrange)
            {
            auto B = imageBox();
            seg.move_inside_box(B);
            seg.len() = tgx::min(seg.lenght_inside_box(B), seg.len());	// truncate to stay inside the box
            }
        if (seg.x_major())
            {
            const bool X_MAJOR = true;
            if (op >= 0)
                while (seg.len() > 0) { _bseg_update_pixel<X_MAJOR, true, SIDE>(seg, color, op); seg.move<X_MAJOR>(); }
            else
                while (seg.len() > 0) { _bseg_update_pixel<X_MAJOR, false, SIDE>(seg, color, op); seg.move<X_MAJOR>(); }
            }
        else
            {
            const bool X_MAJOR = false;
            if (op >= 0)
                while (seg.len() > 0) { _bseg_update_pixel<X_MAJOR, true, SIDE>(seg, color, op); seg.move<X_MAJOR>(); }
            else
                while (seg.len() > 0) { _bseg_update_pixel<X_MAJOR, false, SIDE>(seg, color, op); seg.move<X_MAJOR>(); }
            }
        }



    /**
     * Draw a Bresenham segment [P,Q|.
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_draw(BSeg& seg, bool draw_first, bool draw_last, color_t color, int side, int32_t op, bool checkrange)
        {
        auto segS = seg.save();
        if (side > 0)
            _bseg_draw_template<1>(seg, draw_first, draw_last, color, op, checkrange);
        else if (side < 0)
            _bseg_draw_template<-1>(seg, draw_first, draw_last, color, op, checkrange);
        else
            _bseg_draw_template<0>(seg, draw_first, draw_last, color, op, checkrange);
        seg.restore(segS);
        }


    /**
    * Draw an antialiased Bresenham segment [P,Q|.
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_draw_AA(BSeg& seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange)
        {
        auto segS = seg.save();
        if (!draw_first) seg.move();
        if (draw_last) seg.inclen();
        if (checkrange)
            {
            auto B = imageBox();
            seg.move_inside_box(B);
            seg.len() = tgx::min(seg.lenght_inside_box(B), seg.len());	// truncate to stay inside the box          
            }
        if (seg.x_major())
            {
            const bool X_MAJOR = true;
            while (seg.len() > 0)
                {
                int dir;
                const int aa = seg.AA_bothside<X_MAJOR>(dir);
                const int aa2 = 256 - aa;
                int x = seg.X(), y = seg.Y();
                operator()(x,y).blend256(color, (uint32_t)((op * aa) >> 8));
                if ((y + dir >= 0)&&(y + dir < _ly))
                    operator()(x, y + dir).blend256(color, (uint32_t)((op * aa2) >> 8));
                seg.move<X_MAJOR>();
                }
            }
        else
            {
            const bool X_MAJOR = false;
            while (seg.len() > 0)
                {
                int dir;
                const int aa = seg.AA_bothside<X_MAJOR>(dir);
                const int aa2 = 256 - aa;
                int x = seg.X(), y = seg.Y();
                operator()(x, y).blend256(color, (uint32_t)((op * aa) >> 8));
                if ((x + dir >= 0) && (x + dir < _lx))
                    operator()(x + dir, y).blend256(color, (uint32_t)((op * aa2) >> 8));
                seg.move<X_MAJOR>();
                }
            }
        seg.restore(segS);
        }


    /** used by _bseg_avoid1 */
    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_avoid1_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, color_t color, int32_t op, bool checkrange)
        {
        if (lastA) segA.inclen();
        if (lastB) segB.inclen();
        if (checkrange)
            {
            auto B = imageBox();
            int r = segA.move_inside_box(B);
            if (segA.len() <= 0) return;
            segB.move(r);														// move the second line by the same amount.
            segA.len() = tgx::min(segA.lenght_inside_box(B), segA.len());		// truncate to stay inside the box
            }
        int lena = segA.len() - 1;
        int lenb = segB.len() - 1;
        int l = 0;
        if (op >= 0)
            {
            const bool BLEND = true;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if ((l > lenb) || (segA != segB)) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if ((l > lenb) || (segA != segB)) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); l++;
                    }
                }
            }
        else
            {
            const bool BLEND = false;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if ((l > lenb) || (segA != segB)) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if ((l > lenb) || (segA != segB)) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); l++;
                    }
                }
            }
        }

    /**
     * Draw the bresenham segment [P,Q| while avoiding [P,A|
     * if drawP is set and AA on on , the pixel at P is draw using the minimum AA value.
     *
     *             A
     *            /
     *           /
     *          /
     *        P+-------------Q
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_avoid1(BSeg& PQ, BSeg& PA, bool drawP, bool drawQ, bool closedPA, color_t color, int side, int32_t op, bool checkrange)
        {
        auto PQs = PQ.save();
        auto PAs = PA.save();
       
        if (drawP)
            {
            int32_t aa;
            if (side != 0)
                {
                int g = PQ.angle(side, PA);
                aa = (g < 0) ? (tgx::max(PQ.AA(side), PA.AA(-side))) : (tgx::min(PQ.AA(side), PA.AA(-side)));
                }
            else
                {   
                aa = 256;
                }
            _bseg_update_pixel<true>(PQ, color, op, aa);
            }

        if (side > 0)
            _bseg_avoid1_template<1>(PQ, drawQ, PA, closedPA, color, op, checkrange);
        else if (side < 0)
            _bseg_avoid1_template<-1>(PQ, drawQ, PA, closedPA, color, op, checkrange);
        else
            _bseg_avoid1_template<0>(PQ, drawQ, PA, closedPA, color, op, checkrange);
        PQ.restore(PQs);
        PA.restore(PAs);
        }


    /** Used by _bseg_avoid2 */
    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_avoid2_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, color_t color, int32_t op, bool checkrange)
        {
        if (lastA) segA.inclen();
        if (lastB) segB.inclen();
        if (lastC) segC.inclen();
        if (checkrange)
            {
            auto B = imageBox();
            int r = segA.move_inside_box(B);
            if (segA.len() <= 0) return;
            segB.move(r);		    											// move the second line by the same amount.
            segC.move(r);		    											// move the third line by the same amount.
            segA.len() = tgx::min(segA.lenght_inside_box(B), segA.len());		// truncate to stay inside the box
            }
        int lena = segA.len() - 1;
        int lenb = segB.len() - 1;
        int lenc = segC.len() - 1;
        int l = 0;

        if (op >= 0)
            {
            const bool BLEND = true;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); l++;
                    }
                }
            }
        else
            {
            const bool BLEND = false;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); l++;
                    }
                }
            }
        }


    /**
    * Draw the bresenham segment [P,Q| while avoiding [P,A| and [P,B|
    *
    *     A       B
    *      \     /
    *       \   /
    *        \ /
    *         +--------------
    *         P             Q
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_avoid2(BSeg& PQ, BSeg& PA, BSeg& PB, bool drawQ, bool closedPA, bool closedPB, color_t color, int side, int32_t op, bool checkrange)
        {
        auto PQs = PQ.save();
        auto PAs = PA.save();
        auto PBs = PB.save();
        if (side > 0)
            _bseg_avoid2_template<1>(PQ, drawQ, PA, closedPA, PB, closedPB, color, op, checkrange);
        else if (side < 0)
            _bseg_avoid2_template<-1>(PQ, drawQ, PA, closedPA, PB, closedPB, color, op, checkrange);
        else
            _bseg_avoid2_template<0>(PQ, drawQ, PA, closedPA, PB, closedPB, color, op, checkrange);
        PQ.restore(PQs);
        PA.restore(PAs);
        PB.restore(PBs);
        }



    /** Used by _bseg_avoid11 */
    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_avoid11_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange)
        {
        if (lastB) segB.inclen();
        int dd = (segA.len() - segD.len()) + (lastD ? 0 : 1); segD.len() = segA.len(); segD.reverse();	// D is now synchronized with A
        if (checkrange)
            {
            auto B = imageBox();
            int r = segA.move_inside_box(B);
            if (segA.len() <= 0) return;
            segB.move(r);														// move the second line by the same amount.
            segD.move(r); dd -= r;												// move the third line by the same amount.
            segA.len() = tgx::min(segA.lenght_inside_box(B), segA.len());		// truncate to stay inside the box
            }

        int lena = segA.len() - 1;
        int lenb = segB.len() - 1;
        int l = 0;

        if (op >= 0)
            {
            const bool BLEND = true;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segD.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segD.move(); l++;
                    }
                }
            }
        else
            {
            const bool BLEND = false;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segD.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segD.move(); l++;
                    }
                }
            }
        }



    /**
    * Draw the bresenham segment [P,Q| while avoiding [P,A| and [Q,B|
    * if drawP is set and AA on on , the pixel at P is draw using the minimum AA value.
    * if drawQ is set and AA on on , the pixel at Q is draw using the minimum AA value.
    *
    *     A                     B
    *      \                   /
    *       \                 /
    *        \               /
    *         +--------------
    *         P             Q
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_avoid11(BSeg& PQ, BSeg& PA, BSeg& QB, bool drawP, bool drawQ, bool closedPA, bool closedQB, color_t color, int side, int32_t op, bool checkrange)
        {
        auto PQs = PQ.save();
        auto PAs = PA.save();
        auto QBs = QB.save();

        if (drawP)
            {      
            int32_t aa;
            if (side != 0)
                {
                int g = PQ.angle(side, PA);
                aa = (g < 0) ? (tgx::max(PQ.AA(side), PA.AA(-side))) : (tgx::min(PQ.AA(side), PA.AA(-side)));
                }
            else
                {   
                aa = 256;
                }
            _bseg_update_pixel<true>(PQ, color, op, aa);
            }
        if (drawQ)
            {
            int32_t aa;
            if (side != 0)
                {
                PQ.move(PQ.len());
                int g = PQ.angle(side, QB);          
                aa = (g < 0) ? (tgx::max(PQ.AA(side), QB.AA(side))) : (tgx::min(PQ.AA(side), QB.AA(side)));
                PQ.restore(PQs);
                }
            else
                {
                aa = 256;
                }                
            _bseg_update_pixel<true>(QB, color, op, aa);
            }

        if (side > 0)
            _bseg_avoid11_template<1>(PQ, PA, closedPA, QB, closedQB, color, op, checkrange);
        else if (side < 0)
            _bseg_avoid11_template<-1>(PQ, PA, closedPA, QB, closedQB, color, op, checkrange);
        else
            _bseg_avoid11_template<0>(PQ, PA, closedPA, QB, closedQB, color, op, checkrange);
        PQ.restore(PQs);
        PA.restore(PAs);
        QB.restore(QBs);
        }




    /** Used by _bseg_avoid21 */
    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_avoid21_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange)
        {
        if (lastB) segB.inclen();
        if (lastC) segC.inclen();
        int dd = (segA.len() - segD.len()) + (lastD ? 0 : 1); segD.len() = segA.len(); segD.reverse();	// D is now synchronized with A

        if (checkrange)
            {
            auto B = imageBox();
            int r = segA.move_inside_box(B);
            if (segA.len() <= 0) return;
            segB.move(r);
            segC.move(r);
            segD.move(r); dd -= r;
            segA.len() = tgx::min(segA.lenght_inside_box(B), segA.len());
            }

        int lena = segA.len() - 1;
        int lenb = segB.len() - 1;
        int lenc = segC.len() - 1;
        int l = 0;
        if (op >= 0)
            {
            const bool BLEND = true;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); l++;
                    }
                }
            }
        else
            {
            const bool BLEND = false;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); l++;
                    }
                }
            }
        }


    /**
    * Draw the bresenham segment [P,Q| while avoiding [P,A| , [P, B| and [Q,C|
    *
    *      A      B             C
    *      \     /             /
    *       \   /             /
    *        \ /             /
    *         +--------------
    *         P             Q
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_avoid21(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, bool closedPA, bool closedPB, bool closedQC, color_t color, int side, int32_t op, bool checkrange)
        {
        auto PQs = PQ.save();
        auto PAs = PA.save();
        auto PBs = PB.save();
        auto QCs = QC.save();
        if (side > 0)
            _bseg_avoid21_template<1>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, color, op, checkrange);
        else if (side < 0)
            _bseg_avoid21_template<-1>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, color, op, checkrange);
        else
            _bseg_avoid21_template<0>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, color, op, checkrange);
        PQ.restore(PQs);
        PA.restore(PAs);
        PB.restore(PBs);
        QC.restore(QCs);
        }




    /** Used by _bseg_avoid22 */
    template<typename color_t>
    template<int SIDE> void Image<color_t>::_bseg_avoid22_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, BSeg& segE, bool lastE, color_t color, int32_t op, bool checkrange)
        {
        if (lastB) segB.inclen();
        if (lastC) segC.inclen();

        int dd = (segA.len() - segD.len()) + (lastD ? 0 : 1); segD.len() = segA.len(); segD.reverse();	// D is now synchronized with A
        int ee = (segA.len() - segE.len()) + (lastE ? 0 : 1); segE.len() = segA.len(); segE.reverse();	// E is now synchronized with A

        if (checkrange)
            {
            auto B = imageBox();
            int r = segA.move_inside_box(B);
            if (segA.len() <= 0) return;
            segB.move(r);
            segC.move(r);
            segD.move(r); dd -= r;
            segE.move(r); ee -= r;
            segA.len() = tgx::min(segA.lenght_inside_box(B), segA.len());
            }

        int lena = segA.len() - 1;
        int lenb = segB.len() - 1;
        int lenc = segC.len() - 1;
        int l = 0;
        if (op >= 0)
            {
            const bool BLEND = true;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD)) && ((l < ee) || (segA != segE))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); segE.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD)) && ((l < ee) || (segA != segE))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); segE.move(); l++;
                    }
                }
            }
        else
            {
            const bool BLEND = false;
            if (segA.x_major())
                {
                const bool X_MAJOR = true;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD)) && ((l < ee) || (segA != segE))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); segE.move(); l++;
                    }
                }
            else
                {
                const bool X_MAJOR = false;
                while (l <= lena)
                    {
                    if (((l > lenb) || (segA != segB)) && ((l > lenc) || (segA != segC)) && ((l < dd) || (segA != segD)) && ((l < ee) || (segA != segE))) _bseg_update_pixel<X_MAJOR, BLEND, SIDE>(segA, color, op);
                    segA.move<X_MAJOR>(); segB.move(); segC.move(); segD.move(); segE.move(); l++;
                    }
                }
            }
        }


    /**
    * Draw the bresenham segment [P,Q| while avoiding [P,A| , [P, B|,  [Q,C| and [Q,D|
    *
    *     A       B         C       D
    *      \     /           \     /
    *       \   /             \   /
    *        \ /               \ /
    *         +-----------------+
    *         P                 Q
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_avoid22(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, BSeg& QD, bool closedPA, bool closedPB, bool closedQC, bool closedQD, color_t color, int side, int32_t op, bool checkrange)
        {
        auto PQs = PQ.save();
        auto PAs = PA.save();
        auto PBs = PB.save();
        auto QCs = QC.save();
        auto QDs = QD.save();
        if (side > 0)
            _bseg_avoid22_template<1>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, QD, closedQD, color, op, checkrange);
        else if (side < 0)
            _bseg_avoid22_template<-1>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, QD, closedQD, color, op, checkrange);
        else
            _bseg_avoid22_template<0>(PQ, PA, closedPA, PB, closedPB, QC, closedQC, QD, closedQD, color, op, checkrange);
        PQ.restore(PQs);
        PA.restore(PAs);
        PB.restore(PBs);
        QC.restore(QCs);
        QD.restore(QDs);
        }




    /**
    * Fill the interior of a triangle.
    * integer valued version
    ***/
    /*    
    template<typename color_t>
    void Image<color_t>::_bseg_fill_triangle(iVec2 P1, iVec2 P2, iVec2 P3, color_t fillcolor, float opacity)
        {
        if (P1.y > P2.y) { tgx::swap(P1, P2); } // reorder by increasing Y value
        if (P1.y > P3.y) { tgx::swap(P1, P3); } //
        if (P2.y > P3.y) { tgx::swap(P2, P3); } //
        const int y1 = P1.y;
        const int y2 = P2.y;
        const int y3 = P3.y;
        if (y1 == y3) return; //flat, nothing to draw. 
        if (y1 == y2)
            {
            BSeg seg31(P3, P1);
            BSeg seg32(P3, P2);
            _bseg_fill_interior_angle(P3, P1, P2, seg31, seg32, fillcolor, false, opacity);
            return;
            }
        if (y2 == y3)
            {
            BSeg seg12(P1, P2);
            BSeg seg13(P1, P3);
            _bseg_fill_interior_angle(P1, P2, P3, seg12, seg13, fillcolor, false, opacity);
            return;
            }
        BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();

        iVec2 vA = (P3 - P1), vB = (P2 - P1);
        const int det = (vA.x * vB.y) - (vB.x * vA.y);
        seg23.move_y_dir();
        seg21.move_y_dir();
        bool fl3;
        if (det < 0)
            {
            fl3 = (seg23.X() < seg21.X()) ? true : false;
            }
        else
            {
            fl3 = (seg23.X() > seg21.X()) ? true : false;
            }
        _bseg_fill_interior_angle(P3, P2, P1, seg32, seg31, fillcolor, fl3, opacity);
        _bseg_fill_interior_angle(P1, P2, P3, seg12, seg13, fillcolor, !fl3, opacity);
        return;
        }
    */



    /**
    * Fill the interior of a triangle.
    * floating point version with sub pixel precision
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_fill_triangle(fVec2 fP1, fVec2 fP2, fVec2 fP3, color_t fillcolor, float opacity)
        {
        if (fP1.y > fP2.y) { tgx::swap(fP1, fP2); } // reorder by increasing Y value
        if (fP1.y > fP3.y) { tgx::swap(fP1, fP3); } // so we can directly call 
        if (fP2.y > fP3.y) { tgx::swap(fP2, fP3); } // _bseg_fill_triangle_precomputed_sub()
        BSeg seg12(fP1, fP2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(fP1, fP3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(fP2, fP3); BSeg seg32 = seg23.get_reverse();
        _bseg_fill_triangle_precomputed_sub(fP1, fP2, fP3, seg12, seg21, seg23, seg32, seg31, seg13, fillcolor, opacity);
        }


    /**
    * Fill the interior of a triangle.
    * floating point version with sub pixel precision and BSeg already defined
    **/
    template<typename color_t>
    void Image<color_t>::_bseg_fill_triangle_precomputed(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg& seg12, BSeg& seg21, BSeg& seg23, BSeg& seg32, BSeg& seg31, BSeg& seg13, color_t fillcolor, float opacity)
        {
        auto s12 = seg12.save();
        auto s21 = seg21.save();
        auto s23 = seg23.save();
        auto s32 = seg32.save();
        auto s31 = seg31.save();
        auto s13 = seg13.save();
        fVec2* P[3] = { &fP1, &fP2, &fP3 };
        BSeg* S[3][3];
        S[0][1] = &seg12;
        S[0][2] = &seg13;
        S[1][0] = &seg21;
        S[1][2] = &seg23;
        S[2][0] = &seg31;
        S[2][1] = &seg32;
        int u1, u2, u3;
        if ((fP1.y <= fP2.y) && (fP2.y <= fP3.y))
            {
            u1 = 0; u2 = 1; u3 = 2;
            }
        else if ((fP1.y <= fP3.y) && (fP3.y <= fP2.y))
            {
            u1 = 0; u2 = 2; u3 = 1;
            }
        else if ((fP2.y <= fP1.y) && (fP1.y <= fP3.y))
            {
            u1 = 1; u2 = 0; u3 = 2;
            }
        else if ((fP2.y <= fP3.y) && (fP3.y <= fP1.y))
            {
            u1 = 1; u2 = 2; u3 = 0;
            }
        else if ((fP3.y <= fP1.y) && (fP1.y <= fP2.y))
            {
            u1 = 2; u2 = 0; u3 = 1;
            }
        else
            {
            u1 = 2; u2 = 1; u3 = 0;
            }
        _bseg_fill_triangle_precomputed_sub(*(P[u1]), *(P[u2]), *(P[u3]), *(S[u1][u2]), *(S[u2][u1]), *(S[u2][u3]), *(S[u3][u2]), *(S[u3][u1]), *(S[u1][u3]), fillcolor, opacity);
        seg12.restore(s12);
        seg21.restore(s21);
        seg23.restore(s23);
        seg32.restore(s32);
        seg31.restore(s31);
        seg13.restore(s13);
        }


    template<typename color_t>
    void Image<color_t>::_bseg_fill_triangle_precomputed_sub(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg & seg12, BSeg & seg21, BSeg & seg23, BSeg & seg32, BSeg & seg31, BSeg & seg13, color_t fillcolor, float opacity)
        {
        iVec2 P1((int)roundf(fP1.x), (int)roundf(fP1.y)); int y1 = P1.y;
        iVec2 P2((int)roundf(fP2.x), (int)roundf(fP2.y)); int y2 = P2.y;
        iVec2 P3((int)roundf(fP3.x), (int)roundf(fP3.y)); int y3 = P3.y;
        if (y1 == y3) return; //flat, nothing to draw. 
        if (y1 == y2)
            {
            _bseg_fill_interior_angle(P3, P1, P2, seg31, seg32, fillcolor, false, opacity);
            }
        else if (y2 == y3)
            {
            _bseg_fill_interior_angle(P1, P2, P3, seg12, seg13, fillcolor, false, opacity);
            }
        else
            {
            fVec2 vA = (fP3 - fP1), vB = (fP2 - fP1);
            const float det = vA.x * vB.y - vB.x * vA.y;
            seg23.move_y_dir();
            seg21.move_y_dir();
            bool fl3;
            if (det < 0)
                {
                fl3 = (seg23.X() < seg21.X()) ? true : false;
                }
            else
                {
                fl3 = (seg23.X() > seg21.X()) ? true : false;
                }
            _bseg_fill_interior_angle(P3, P2, P1, seg32, seg31, fillcolor, fl3, opacity);
            _bseg_fill_interior_angle(P1, P2, P3, seg12, seg13, fillcolor, !fl3, opacity);
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::_bseg_fill_interior_angle(iVec2 P, iVec2 Q1, iVec2 Q2, BSeg& seg1, BSeg& seg2, color_t color, bool fill_last, float opacity)
        {
        const int dir = (P.y > Q1.y) ? -1 : 1;
        const int y = P.y;
        const int ytarget = Q1.y + dir * (fill_last ? 1 : 0);
        const bool swapseg = ((Q1.x - P.x) * abs(Q2.y - P.y) > (Q2.x - P.x) * abs(Q1.y - P.y));
        if (swapseg)
            _bseg_fill_interior_angle_sub(dir, y, ytarget, seg2, seg1, color, opacity);
        else
            _bseg_fill_interior_angle_sub(dir, y, ytarget, seg1, seg2, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::_bseg_fill_interior_angle_sub(int dir, int y, int ytarget, BSeg& sega, BSeg& segb, color_t color, float opacity)
        {
        if (opacity > 1) opacity = -1;
        // fix the range. 
        if (dir > 0)
            {
            if (ytarget >= _ly) { ytarget = _ly; }
            if ((ytarget <= 0) || (y >= ytarget)) return;
            if (y < 0)
                { // move y up to 0
                sega.move_y_dir(-y);
                segb.move_y_dir(-y);
                y = 0;
                }
            }
        else
            {
            if (ytarget < 0) { ytarget = -1; }
            if ((ytarget >= _ly - 1) || (y <= ytarget)) return;
            if (y > _ly - 1)
                { // move y down to ly-1
                sega.move_y_dir(y - _ly + 1);
                segb.move_y_dir(y - _ly + 1);
                y = _ly - 1;
                }
            }
        if (sega.x_major())
            {
            if (segb.x_major())
                {
                if (sega.step_x() < 0)
                    {
                    if (segb.step_x() > 0)
                        {
                        while (y != ytarget)
                            {
                            _triangle_hline(sega.X() + 1, segb.X() - 1, y, color, opacity);
                            sega.move_y_dir<true>();
                            segb.move_y_dir<true>();
                            y += dir;
                            }
                        }
                    else
                        {
                        while (y != ytarget)
                            {
                            segb.move_y_dir<true>();
                            _triangle_hline(sega.X() + 1, segb.X(), y, color, opacity);
                            sega.move_y_dir<true>();
                            y += dir;
                            }
                        }
                    }
                else
                    {
                    if (segb.step_x() > 0)
                        {
                        while (y != ytarget)
                            {
                            sega.move_y_dir<true>();
                            _triangle_hline(sega.X(), segb.X() - 1, y, color, opacity);
                            segb.move_y_dir<true>();
                            y += dir;
                            }
                        }
                    else
                        {
                        while (y != ytarget)
                            {
                            sega.move_y_dir<true>();
                            segb.move_y_dir<true>();
                            _triangle_hline(sega.X(), segb.X(), y, color, opacity);
                            y += dir;
                            }
                        }
                    }
                }
            else
                {
                if (sega.step_x() < 0)
                    {
                    while (y != ytarget)
                        {
                        _triangle_hline(sega.X() + 1, segb.X() - 1, y, color, opacity);
                        sega.move_y_dir<true>();
                        segb.move_y_dir<false>();
                        y += dir;
                        }
                    }
                else
                    {
                    while (y != ytarget)
                        {
                        sega.move_y_dir<true>();
                        _triangle_hline(sega.X(), segb.X() - 1, y, color, opacity);
                        segb.move_y_dir<false>();
                        y += dir;
                        }
                    }
                }
            }
        else
            {
            if (segb.x_major())
                {
                if (segb.step_x() > 0)
                    {
                    while (y != ytarget)
                        {
                        _triangle_hline(sega.X() + 1, segb.X() - 1, y, color, opacity);
                        segb.move_y_dir<true>();
                        sega.move_y_dir<false>();
                        y += dir;
                        }
                    }
                else
                    {
                    while (y != ytarget)
                        {
                        segb.move_y_dir<true>();
                        _triangle_hline(sega.X() + 1, segb.X(), y, color, opacity);
                        sega.move_y_dir<false>();
                        y += dir;
                        }
                    }
                }
            else
                {
                while (y != ytarget)
                    {
                    _triangle_hline(sega.X() + 1, segb.X() - 1, y, color, opacity);
                    segb.move_y_dir<false>();
                    sega.move_y_dir<false>();
                    y += dir;
                    }
                }
            }
        }




    /*****************************************************
    * LINE DRAWING
    * 
    * LOW QUALITY (FAST) DRAWING METHODS
    ******************************************************/

    

    template<typename color_t>
    void Image<color_t>::drawFastVLine(iVec2 pos, int h, color_t color, float opacity)
        {
        if (!isValid()) return;
        _drawFastVLine<true>(pos, h, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::drawFastHLine(iVec2 pos, int w, color_t color, float opacity)
        {
        if (!isValid()) return;
        _drawFastHLine<true>(pos, w, color, opacity);
        }



    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawFastVLine(iVec2 pos, int h, color_t color)
        {
        int x = pos.x;
        int y = pos.y;
        if (CHECKRANGE) // optimized away at compile time
            {
            if ((x < 0) || (x >= _lx) || (y >= _ly)) return;
            if (y < 0) { h += y; y = 0; }
            if (y + h > _ly) { h = _ly - y; }
            }
        color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
        while (h-- > 0)
            {
            (*p) = color;
            p += _stride;
            }
        }


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawFastVLine(iVec2 pos, int h, color_t color, float opacity)
        {
        int x = pos.x;
        int y = pos.y;
        if (CHECKRANGE) // optimized away at compile time
            {
            if ((x < 0) || (x >= _lx) || (y >= _ly)) return;
            if (y < 0) { h += y; y = 0; }
            if (y + h > _ly) { h = _ly - y; }
            }
        color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
        if ((opacity < 0) || (opacity > 1))
            {
            while (h-- > 0)
                {
                (*p) = color;
                p += _stride;
                }
            }
        else
            {
            while (h-- > 0)
                {
                (*p).blend(color, opacity);
                p += _stride;
                }
            }
        }


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawFastHLine(iVec2 pos, int w, color_t color)
        {
        int x = pos.x;
        int y = pos.y;
        if (CHECKRANGE) // optimized away at compile time
            {
            if ((y < 0) || (y >= _ly) || (x >= _lx)) return;
            if (x < 0) { w += x; x = 0; }
            if (x + w > _lx) { w = _lx - x; }
            }
        _fast_memset(_buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), color, w);
        }




    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawFastHLine(iVec2 pos, int w, color_t color, float opacity)
        {
        int x = pos.x;
        int y = pos.y;
        if (CHECKRANGE) // optimized away at compile time
            {
            if ((y < 0) || (y >= _ly) || (x >= _lx)) return;
            if (x < 0) { w += x; x = 0; }
            if (x + w > _lx) { w = _lx - x; }
            }
        color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
        if ((opacity < 0) || (opacity > 1))
            {
            _fast_memset(_buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), color, w);
            }
        else
            {
            while (w-- > 0)
                {
                (*p++).blend(color, opacity);
                }
            }
        }




    template<typename color_t>
    void Image<color_t>::drawLine(iVec2 P1, iVec2 P2, color_t color, float opacity)
        {
        if (!isValid()) return;
        _drawSeg(P1, true, P2, true, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::drawSegment(iVec2 P1, bool drawP1, iVec2 P2, bool drawP2, color_t color, float opacity)
        {
        if (!isValid()) return;
        _drawSeg(P1, drawP1, P2, drawP2, color, opacity);
        }





    /*****************************************************
    * LINE DRAWING
    *
    * HIGH QUALITY (SLOW) DRAWING METHODS
    ******************************************************/



    template<typename color_t>
    void Image<color_t>::drawLineAA(fVec2 P1, fVec2 P2, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        int32_t op = (int32_t)(256 * opacity);
        BSeg seg(P1, P2);
        _bseg_draw_AA(seg, true, true, color, op, true);
        }


    template<typename color_t>
    void Image<color_t>::drawThickLineAA(fVec2 P1, fVec2 P2, float line_width, EndPath end_P1, EndPath end_P2, color_t color, float opacity)
        {  
        if (line_width < 0) return;
        drawWedgeLineAA(P1, P2, line_width, end_P1, line_width, end_P2, color, opacity);
        }



    /**
    * Draw the end of a thick line (straight, rounded or arrow). 
    **/
    template<typename color_t>
    void Image<color_t>::_drawEnd(float distAB, fVec2 A, fVec2 B, BSeg& segAB, BSeg& segBA, BSeg& segAP, BSeg& segBQ, EndPath end, int w, color_t color, float opacity)
        {
        const int op = (int)(opacity * 256);
        if (end < END_STRAIGHT) return;
        if (end == END_STRAIGHT)
            {
            _bseg_avoid11(segAB, segAP, segBQ, true, true, true, true, color, -w, op, true);
            return;
            }
        if (end == END_ROUNDED)
            {
            _bseg_avoid11(segAB, segAP, segBQ, false, false, true, true, color, 0, op, true);
            _drawPixel<true>(iVec2((int)(roundf(A.x)), (int)(roundf(A.y))), color, (op * segAP.AA(w) >> 8) / 256.0f);
            _drawPixel<true>(iVec2((int)(roundf(B.x)), (int)(roundf(B.y))), color, (op * segBQ.AA(-w) >> 8) / 256.0f);
            _fillSmoothCircleInterHP((A + B) * 0.5f, distAB * 0.5f, color, opacity, segAB, w);
            return;
            }   
        if (end >= END_ARROW_1)
            {
            const int n = ((int)(end)) - ((end >= END_ARROW_SKEWED_1) ? (END_ARROW_SKEWED_1  - 1) : 0);
            fVec2 H = (B - A);      
            fVec2 V = H.getRotate90() * ((float)(n * w));
            fVec2 E = ((A + B) * 0.5f) + V;
            H = H * (0.5f * n);
            fVec2 C = A - H - V * ((end >= END_ARROW_SKEWED_1) ? 0.5f : 0);
            fVec2 D = B + H - V * ((end >= END_ARROW_SKEWED_1) ? 0.5f : 0);
            BSeg segAC(A, C); BSeg segCA = segAC.get_reverse();
            BSeg segCE(C, E); BSeg segEC = segCE.get_reverse();
            BSeg segEA(E, A); BSeg segAE = segEA.get_reverse();
            BSeg segBD(B, D); BSeg segDB = segBD.get_reverse();
            BSeg segDE(D, E); BSeg segED = segDE.get_reverse();
            BSeg segEB(E, B); BSeg segBE = segEB.get_reverse();
            _bseg_fill_triangle_precomputed(A, C, E, segAC, segCA, segCE, segEC, segEA, segAE, color, opacity);
            _bseg_fill_triangle_precomputed(A, E, B, segAE, segEA, segEB, segBE, segBA, segAB, color, opacity);
            _bseg_fill_triangle_precomputed(B, D, E, segBD, segDB, segDE, segED, segEB, segBE, color, opacity);
            _bseg_avoid1(segAC, segAP, true, false, true, color, -w, op, true);
            _bseg_avoid1(segCE, segCA, true, false, true, color, -w, op, true);
            _bseg_avoid1(segED, segEC, true, false, true, color, -w, op, true);
            _bseg_avoid11(segDB, segDE, segBQ, true, true, true, true, color, -w, op, true);
            _bseg_avoid22(segAB, segAP, segAC, segBQ, segBD, true, true, true, true, color, 0, op, true);
            _bseg_avoid22(segAE, segAP, segAC, segEC, segED, true, true, true, true, color, 0, op, true);
            _bseg_avoid22(segBE, segBQ, segBD, segEA, segED, true, true, true, true, color, 0, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::drawWedgeLineAA(fVec2 P1, fVec2 P2, float line_width_P1, EndPath end_P1, float line_width_P2, EndPath end_P2, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (line_width_P2 < 1.1f) line_width_P2 += (1.1f - line_width_P2) * 0.5f; // make pointy end
        if (line_width_P1 < 1.1f) line_width_P1 += (1.1f - line_width_P1) * 0.5f; // look better
        if (line_width_P2 < line_width_P1)
            {
            tgx::swap(P1, P2); 
            tgx::swap(line_width_P1, line_width_P2);
            tgx::swap(end_P1, end_P2);
            }
        if (line_width_P2 <= 0) return; 
        const int op = (int)(opacity * 256);
        if (line_width_P1 <= 1)
            { // draw triangle: here line_width_P1 <= 1
            if (line_width_P2 < 1)
                {
                drawLineAA(P1, P1, color, opacity * line_width_P2);
                return;
                }
            const fVec2 H = (P1 - P2).getRotate90().getNormalize() * (line_width_P2/2);
            const fVec2 PA = P2 + H, PB = P2 - H;
            const int w = -1; 
            BSeg seg1A(P1, PA); BSeg segA1 = seg1A.get_reverse();
            BSeg seg1B(P1, PB); BSeg segB1 = seg1B.get_reverse();
            BSeg segAB(PA, PB); BSeg segBA = segAB.get_reverse();
            _bseg_fill_triangle_precomputed(P1, PA, PB, seg1A, segA1, segAB, segBA, segB1, seg1B, color, opacity);            
            _bseg_draw(seg1A, false, false, color, -w, op, true);                
            _bseg_avoid1(seg1B, seg1A, true, false, true, color, w, op, true);
            _drawEnd(line_width_P2, PA, PB, segAB, segBA, segA1, segB1, end_P2, w, color, opacity);
            return;
            }        
        // here line_width_P1 > 1  and line_width_P2 > 1
        const fVec2 H = (P1 - P2).getRotate90().getNormalize();
        const fVec2 H1 = H * (line_width_P1*0.5f);
        const fVec2 H2 = H * (line_width_P2*0.5f);
        const fVec2 PA = P1 + H1, PB = P2 + H2, PC = P2 - H2, PD = P1 - H1;
        const int w = 1; 
        BSeg segAB(PA, PB); BSeg segBA = segAB.get_reverse();
        BSeg segAC(PA, PC); BSeg segCA = segAC.get_reverse();
        BSeg segBC(PB, PC); BSeg segCB = segBC.get_reverse();
        BSeg segCD(PC, PD); BSeg segDC = segCD.get_reverse();
        BSeg segDA(PD, PA); BSeg segAD = segDA.get_reverse();
        _bseg_fill_triangle_precomputed(PA, PB, PC, segAB, segBA, segBC, segCB, segCA, segAC, color, opacity);
        _bseg_fill_triangle_precomputed(PA, PC, PD, segAC, segCA, segCD, segDC, segDA, segAD, color, opacity);
        _bseg_draw(segAB, false, false, color, w, op, true);
        _bseg_draw(segCD, false, false, color, w, op, true);
        _bseg_avoid22(segAC, segAB, segAD, segCB, segCD, true, true, true, true, color, 0, op, true);
        _drawEnd(line_width_P1, PA, PD, segAD, segDA, segAB, segDC, end_P1, w, color, opacity);
        _drawEnd(line_width_P2, PC, PB, segCB, segBC, segCD, segBA, end_P2, w, color, opacity);
        }







    /** taken from bodmer e_tft library (legacy, unused now) **/
    template<typename color_t>
    float Image<color_t>::_wedgeLineDistance(float pax, float pay, float bax, float bay, float dr)
        {
        float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
        float dx = pax - bax * h, dy = pay - bay * h;
        return tgx::fast_sqrt(dx * dx + dy * dy) + h * dr;
        }

    
    /** Adapted from Bodmer e_tft library (legacy, not used anymore) */
    template<typename color_t>
    void Image<color_t>::_drawWedgeLine(float ax, float ay, float bx, float by, float ar, float br, color_t color, float opacity)
        {
        const float LoAlphaTheshold = 64.0f / 255.0f;
        const float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

        if ((abs(ax - bx) < 0.01f) && (abs(ay - by) < 0.01f)) bx += 0.01f;  // Avoid divide by zero

        ar = ar / 2.0f;
        br = br / 2.0f;

        iBox2 B((int)floorf(fminf(ax - ar, bx - br)), (int)ceilf(fmaxf(ax + ar, bx + br)), (int)floorf(fminf(ay - ar, by - br)), (int)ceilf(fmaxf(ay + ar, by + br)));
        B &= imageBox();
        if (B.isEmpty()) return;
        // Find line bounding box
        int x0 = B.minX;
        int x1 = B.maxX;
        int y0 = B.minY;
        int y1 = B.maxY;

        // Establish x start and y start
        int32_t ys = (int32_t)ay;
        if ((ax - ar) > (bx - br)) ys = (int32_t)by;

        float rdt = ar - br; // Radius delta
        float alpha = 1.0f;
        ar += 0.5f;

        float xpax, ypay, bax = bx - ax, bay = by - ay;

        int32_t xs = x0;
        // Scan bounding box from ys down, calculate pixel intensity from distance to line
        for (int32_t yp = ys; yp <= y1; yp++) 
            {
            bool endX = false; // Flag to skip pixels
            ypay = yp - ay;
            for (int32_t xp = xs; xp <= x1; xp++) 
                {
                if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side
                xpax = xp - ax;
                alpha = ar - _wedgeLineDistance(xpax, ypay, bax, bay, rdt);
                if (alpha <= LoAlphaTheshold) continue;
                // Track edge to minimise calculations
                if (!endX) { endX = true; xs = xp; }
                if (alpha > HiAlphaTheshold) 
                    {
                    _drawPixel<false, false>( { xp, yp }, color, opacity);
                    continue;
                    }
                //Blend color with background and plot
                _drawPixel<false, false>({ xp, yp }, color, opacity * alpha);
                }
            }
        // Reset x start to left side of box
        xs = x0;
        // Scan bounding box from ys-1 up, calculate pixel intensity from distance to line
        for (int32_t yp = ys - 1; yp >= y0; yp--) 
            {
            bool endX = false; // Flag to skip pixels
            ypay = yp - ay;
            for (int32_t xp = xs; xp <= x1; xp++) 
                {
                if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
                xpax = xp - ax;
                alpha = ar - _wedgeLineDistance(xpax, ypay, bax, bay, rdt);
                if (alpha <= LoAlphaTheshold) continue;
                // Track line boundary
                if (!endX) { endX = true; xs = xp; }
                if (alpha > HiAlphaTheshold) 
                    {
                    _drawPixel<false, false>({ xp, yp }, color, opacity);
                    continue;
                    }
                //Blend color with background and plot
                _drawPixel<false, false>({ xp, yp }, color, opacity * alpha);
                }
            }
        }





    /********************************************************************************
    *
    * DRAWING RECTANGLES
    *
    *********************************************************************************/






    template<typename color_t>
    void Image<color_t>::drawRect(const iBox2& B, color_t color, float opacity)
        {
        if (!isValid()) return;
        const int x = B.minX;
        const int y = B.minY;
        const int w = B.maxX - B.minX + 1;
        const int h = B.maxY - B.minY + 1;
        if ((w <= 0) || (h <= 0)) return;
        _drawFastHLine<true>({ x,y }, w, color, opacity);
        if (h > 1) _drawFastHLine<true>({ x, y + h - 1 }, w, color, opacity);
        _drawFastVLine<true>({ x, y + 1 }, h - 2, color, opacity);
        if (w > 1) _drawFastVLine<true>({ x + w - 1, y + 1 }, h - 2, color, opacity);
        }



    /** Fill a rectangle region with a single color */
    template<typename color_t>
    void Image<color_t>::fillRect(const iBox2 & B, color_t color, float opacity)
        {
        if (!isValid()) return;
        _fillRect(B, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::drawThickRect(const iBox2& B, int thickness, color_t color, float opacity)
        {
        if (B.isEmpty() || (!isValid()) ||(thickness < 1)) return;        
        int r = tgx::min(B.lx(), B.ly()) / 2; 
        if (r <= 1) { fillRect(B, color, opacity); return; }
        if (thickness > r) thickness = r; 
        thickness--; 
        fillRect(iBox2(B.minX, B.maxX, B.minY, B.minY + thickness), color, opacity);
        fillRect(iBox2(B.minX, B.maxX, B.maxY - thickness, B.maxY), color, opacity);
        fillRect(iBox2(B.minX, B.minX + thickness, B.minY + thickness + 1, B.maxY - thickness - 1), color, opacity);
        fillRect(iBox2(B.maxX - thickness, B.maxX, B.minY + thickness + 1, B.maxY - thickness - 1), color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::fillThickRect(const iBox2& B, int thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if (B.isEmpty() || (!isValid()) || (thickness < 1)) return;
        int r = tgx::min(B.lx(), B.ly()) / 2;
        if (r <= 1) { fillRect(B, color_interior, opacity); return; }
        if (thickness > r) thickness = r;
        thickness--;
        fillRect(iBox2(B.minX, B.maxX, B.minY, B.minY + thickness), color_border, opacity);
        fillRect(iBox2(B.minX, B.maxX, B.maxY - thickness, B.maxY), color_border, opacity);
        fillRect(iBox2(B.minX, B.minX + thickness, B.minY + thickness + 1, B.maxY - thickness - 1), color_border, opacity);
        fillRect(iBox2(B.maxX - thickness, B.maxX, B.minY + thickness + 1, B.maxY - thickness - 1), color_border, opacity);
        thickness++; 
        fillRect(iBox2(B.minX + thickness, B.maxX - thickness, B.minY + thickness, B.maxY - thickness), color_interior, opacity);
        }




    template<typename color_t>
    void Image<color_t>::_fillRect(iBox2 B, color_t color, float opacity)
        {
        B &= imageBox();
        if (B.isEmpty()) return;
        const int sx = B.lx();
        int sy = B.ly();
        color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(B.minY) * TGX_CAST32(_stride);
        if (sx == _stride) 
            { // fast, set everything at once
            int32_t len = TGX_CAST32(sy) * TGX_CAST32(_stride);
            if ((opacity < 0) || (opacity > 1))
                {
                _fast_memset(p, color, len);
                }
            else
                {
                while (len-- > 0) { (*(p++)).blend(color, opacity); }
                }
            }
        else
            { // set each line separately
            if ((opacity < 0) || (opacity > 1))
                {
                while (sy-- > 0)
                    {
                    _fast_memset(p, color, sx);
                    p += _stride;
                    }
                }
            else
                {
                while (sy-- > 0)
                    {
                    int len = sx;
                    while (len-- > 0) { (*(p++)).blend(color, opacity); }
                    p += (_stride - sx);
                    }
                }

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
        if ((opacity < 0) || (opacity > 1))
            {
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
        else
            {
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

        if ((opacity < 0) || (opacity > 1))
            {
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
        else
            {
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
        }




    template<typename color_t>
    void Image<color_t>::drawThickRectAA(const fBox2& B, float thickness, color_t color, float opacity)
        {
        if (B.isEmpty()) return;
        drawThickQuadAA(fVec2(B.minX, B.minY), fVec2(B.maxX, B.minY), fVec2(B.maxX, B.maxY), fVec2(B.minX, B.maxY), thickness, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::fillRectAA(const fBox2& B, color_t color, float opacity)
        {
        if ((!isValid()) || (B.isEmpty())) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _fillSmoothRect(B, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::fillThickRectAA(const fBox2& B, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if (B.isEmpty()) return;
        fillThickQuadAA(fVec2(B.minX, B.minY), fVec2(B.maxX, B.minY), fVec2(B.maxX, B.maxY), fVec2(B.minX, B.maxY), thickness, color_interior, color_border, opacity);
        }








    /********************************************************************************
    *
    * DRAWING ROUNDED RECTANGLES
    *
    *********************************************************************************/




    template<typename color_t>
    void Image<color_t>::drawRoundRect(const iBox2& B, int r, color_t color, float opacity)
        {
        const int x = B.minX;
        const int y = B.minY;
        const int w = B.lx();
        const int h = B.ly();
        if (!isValid() || (w <= 0) || (h <= 0)) return;
        if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
            _drawRoundRect<false>(x, y, w, h, r, color, opacity);
        else
            _drawRoundRect<true>(x, y, w, h, r, color, opacity);
        }


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        _drawFastHLine<CHECKRANGE>( { x + r, y }, w - 2 * r, color, opacity);
        _drawFastHLine<CHECKRANGE>({ x + r, y + h - 1 }, w - 2 * r, color, opacity);
        _drawFastVLine<CHECKRANGE>({ x, y + r }, h - 2 * r, color, opacity);
        _drawFastVLine<CHECKRANGE>({ x + w - 1, y + r }, h - 2 * r, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + r, y + r, r, 1, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 2, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + h - r - 1, r, 4, color, opacity);
        _drawCircleHelper<CHECKRANGE>(x + r, y + h - r - 1, r, 8, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::fillRoundRect(const iBox2& B, int r, color_t color, float opacity)
        {
        const int x = B.minX;
        const int y = B.minY;
        const int w = B.lx();
        const int h = B.ly();
        if (!isValid() || (w <= 0) || (h <= 0)) return;
        if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
            _fillRoundRect<false>(x, y, w, h, r, color, opacity);
        else
            _fillRoundRect<true>(x, y, w, h, r, color, opacity);
        }


    template<typename color_t>
    template<bool CHECKRANGE> void Image<color_t>::_fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
        {
        int max_radius = ((w < h) ? w : h) / 2;
        if (r > max_radius) r = max_radius;
        fillRect(iBox2(x + r, x + r + w - (2*r) - 1, y, y + h - 1), color, opacity);
        _fillCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color, opacity);
        _fillCircleHelper<CHECKRANGE>(x + r, y + r, r, 2, h - 2 * r - 1, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::drawRoundRectAA(const fBox2& B, float corner_radius, color_t color, float opacity)
        {
        if ((!isValid()) || (B.isEmpty())) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _drawSmoothRoundRect(iBox2((int)roundf(B.minX), (int)roundf(B.maxX), (int)roundf(B.minY), (int)roundf(B.maxY)), corner_radius, color, opacity); // cheat, convert to iBox2 for the time being. todo improve this !
        }


    template<typename color_t>
    void Image<color_t>::drawThickRoundRectAA(const fBox2& B, float corner_radius, float thickness, color_t color, float opacity)
        {
        if ((!isValid()) || (B.isEmpty())) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (corner_radius - thickness < 1) thickness = corner_radius - 1.0f;
        _drawSmoothWideRoundRect(iBox2((int)roundf(B.minX), (int)roundf(B.maxX), (int)roundf(B.minY), (int)roundf(B.maxY)), corner_radius, thickness, color, opacity); // cheat, convert to iBox2 for the time being. todo improve this !
        }


    template<typename color_t>
    void Image<color_t>::fillRoundRectAA(const fBox2& B, float corner_radius, color_t color, float opacity)
        {
        if ((!isValid()) || (B.isEmpty())) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _fillSmoothRoundedRect(iBox2((int)roundf(B.minX), (int)roundf(B.maxX), (int)roundf(B.minY), (int)roundf(B.maxY)), corner_radius, color, opacity); // cheat, convert to iBox2 for the time being. todo improve this !
        }




    template<typename color_t>
    void Image<color_t>::_fillSmoothRect(const fBox2& B, color_t color, float opacity)
        {
        tgx::iBox2 eB((int)floorf(B.minX + 0.5f), (int)ceilf(B.maxX - 0.5f), (int)floorf(B.minY + 0.5f), (int)ceilf(B.maxY - 0.5f));
        const bool checkrange = (imageBox().contains(eB)) ? false : true;
        if (eB.minX == eB.maxX)
            { // just a vertical line
            if (eB.minY == eB.maxY)
                { // single point
                const float aera = (B.maxX - B.minX) * (B.maxY - B.minY);
                _drawPixel(checkrange, { eB.minX, eB.minY }, color, opacity * aera);
                return;
                }
            const float w = B.maxX - B.minX;
            const float a_up = 0.5f + eB.minY - B.minY;
            const float a_down = 0.5f + B.maxY - eB.maxY;
            _drawPixel(checkrange, { eB.minX, eB.minY }, color, opacity * a_up * w);
            _drawPixel(checkrange, { eB.minX, eB.maxY }, color, opacity * a_down * w);
            _drawFastVLine(checkrange, { eB.minX, eB.minY + 1 }, eB.maxY - eB.minY - 1, color, opacity * w);
            return;
            }
        if (eB.minY == eB.maxY)
            { // just an horizontal line
            const float h = B.maxY - B.minY;
            const float a_left = 0.5f + eB.minX - B.minX;
            const float a_right = 0.5f + B.maxX - eB.maxX;
            _drawPixel(checkrange, { eB.minX, eB.minY }, color, opacity * a_left * h);
            _drawPixel(checkrange, { eB.maxX, eB.minY }, color, opacity * a_right * h);
            _drawFastHLine(checkrange, { eB.minX + 1, eB.minY }, eB.maxX - eB.minX - 1, color, opacity * h);
            return;
            }
        fillRect(tgx::iBox2(eB.minX + 1, eB.maxX - 1, eB.minY + 1, eB.maxY - 1), color, opacity); // fill interior (may be empty)	
        const float a_left = 0.5f + eB.minX - B.minX;
        const float a_right = 0.5f + B.maxX - eB.maxX;
        const float a_up = 0.5f + eB.minY - B.minY;
        const float a_down = 0.5f + B.maxY - eB.maxY;
        _drawPixel(checkrange, { eB.minX, eB.minY }, color, opacity * a_left * a_up);
        _drawPixel(checkrange, { eB.minX, eB.maxY }, color, opacity * a_left * a_down);
        _drawPixel(checkrange, { eB.maxX, eB.minY }, color, opacity * a_right * a_up);
        _drawPixel(checkrange, { eB.maxX, eB.maxY }, color, opacity * a_right * a_down);
        _drawFastHLine(checkrange, { eB.minX + 1, eB.minY }, eB.maxX - eB.minX - 1, color, opacity * a_up);
        _drawFastHLine(checkrange, { eB.minX + 1, eB.maxY }, eB.maxX - eB.minX - 1, color, opacity * a_down);
        _drawFastVLine(checkrange, { eB.minX, eB.minY + 1 }, eB.maxY - eB.minY - 1, color, opacity * a_left);
        _drawFastVLine(checkrange, { eB.maxX, eB.minY + 1 }, eB.maxY - eB.minY - 1, color, opacity * a_right);
        return;
        }




    template<typename color_t>
    void Image<color_t>::_fillSmoothRoundedRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity)
        {
        // check radius >0 and B not empty and im valid...
        float maxl = (B.maxX - B.minX) / 2.0f;
        float maxh = (B.maxY - B.minY) / 2.0f;
        corner_radius = tgx::min(corner_radius, tgx::min(maxl, maxh));

        const float eps = 0.5f;
        fVec2 P1(B.minX + corner_radius - eps, B.minY + corner_radius - eps);
        _fillSmoothQuarterCircleInterHPsub(P1, corner_radius, 2, false, false, color, opacity, 0);

        fVec2 P2(B.maxX - corner_radius + eps, B.minY + corner_radius - eps);
        _fillSmoothQuarterCircleInterHPsub(P2, corner_radius, 3, false, false, color, opacity, 0);

        fVec2 P3(B.maxX - corner_radius + eps, B.maxY - corner_radius + eps);
        _fillSmoothQuarterCircleInterHPsub(P3, corner_radius, 1, false, false, color, opacity, 0);

        fVec2 P4(B.minX + corner_radius - eps, B.maxY - corner_radius + eps);
        _fillSmoothQuarterCircleInterHPsub(P4, corner_radius, 0, false, false, color, opacity, 0);

        const int x1 = (int)roundf(B.minX + corner_radius - eps);
        const int x2 = (int)roundf(B.maxX - corner_radius + eps);
        fillRect(iBox2(x1, x2, B.minY, B.maxY), color, opacity);
        const int y1 = (int)roundf(B.minY + corner_radius - eps);
        const int y2 = (int)roundf(B.maxY - corner_radius + eps);
        fillRect(iBox2(B.minX, x1 - 1, y1, y2), color, opacity);
        fillRect(iBox2(x2 + 1, B.maxX, y1, y2), color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::_drawSmoothRoundRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity)
        {
        // check radius >0 and B not empty and im valid...
        float maxl = (B.maxX - B.minX) / 2.0f;
        float maxh = (B.maxY - B.minY) / 2.0f;
        corner_radius = tgx::min(corner_radius, tgx::min(maxl, maxh));

        const float eps = 0;

        fVec2 P1(B.minX + corner_radius - eps, B.minY + corner_radius - eps);
        _drawSmoothQuarterCircleInterHPsub(P1, corner_radius, 2, false, false, color, opacity, 0);

        fVec2 P2(B.maxX - corner_radius + eps, B.minY + corner_radius - eps);
        _drawSmoothQuarterCircleInterHPsub(P2, corner_radius, 3, false, false, color, opacity, 0);

        fVec2 P3(B.maxX - corner_radius + eps, B.maxY - corner_radius + eps);
        _drawSmoothQuarterCircleInterHPsub(P3, corner_radius, 1, false, false, color, opacity, 0);

        fVec2 P4(B.minX + corner_radius - eps, B.maxY - corner_radius + eps);
        _drawSmoothQuarterCircleInterHPsub(P4, corner_radius, 0, false, false, color, opacity, 0);

        const int x1 = (int)roundf(B.minX + corner_radius - eps);
        const int x2 = (int)roundf(B.maxX - corner_radius + eps);
        const int y1 = (int)roundf(B.minY + corner_radius - eps);
        const int y2 = (int)roundf(B.maxY - corner_radius + eps);
        _drawFastHLine<true>({ x1,B.minY }, x2 - x1 + 1, color, opacity);
        _drawFastHLine<true>({ x1, B.maxY }, x2 - x1 + 1, color, opacity);
        _drawFastVLine<true>({ B.minX,y1 }, y2 - y1 + 1, color, opacity);
        _drawFastVLine<true>({ B.maxX,y1 }, y2 - y1 + 1, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothWideRoundRect(const tgx::iBox2& B, float corner_radius, float thickness, color_t color, float opacity)
        {
        // check radius >0 and B not empty and im valid...
        float maxl = (B.maxX - B.minX) / 2.0f;
        float maxh = (B.maxY - B.minY) / 2.0f;
        corner_radius = tgx::min(corner_radius, tgx::min(maxl, maxh));

        const float eps = 0;

        fVec2 P1(B.minX + corner_radius - eps, B.minY + corner_radius - eps);
        _drawSmoothThickQuarterCircleInterHPsub(P1, corner_radius, thickness, 2, false, false, color, opacity, 0);

        fVec2 P2(B.maxX - corner_radius + eps, B.minY + corner_radius - eps);
        _drawSmoothThickQuarterCircleInterHPsub(P2, corner_radius, thickness, 3, false, false, color, opacity, 0);

        fVec2 P3(B.maxX - corner_radius + eps, B.maxY - corner_radius + eps);
        _drawSmoothThickQuarterCircleInterHPsub(P3, corner_radius, thickness, 1, false, false, color, opacity, 0);

        fVec2 P4(B.minX + corner_radius - eps, B.maxY - corner_radius + eps);
        _drawSmoothThickQuarterCircleInterHPsub(P4, corner_radius, thickness, 0, false, false, color, opacity, 0);

        const int x1 = (int)roundf(B.minX + corner_radius - eps);
        const int x2 = (int)roundf(B.maxX - corner_radius + eps);
        const int y1 = (int)roundf(B.minY + corner_radius - eps);
        const int y2 = (int)roundf(B.maxY - corner_radius + eps);

        _fillSmoothRect(tgx::fBox2(x1 - 0.5f, x2 + 0.5f, B.minY - 0.5f, B.minY + thickness - 0.5f), color, opacity);
        _fillSmoothRect(tgx::fBox2(x1 - 0.5f, x2 + 0.5f, B.maxY - thickness + 0.5f, B.maxY + 0.5f), color, opacity);
        _fillSmoothRect(tgx::fBox2(B.minX - 0.5f, B.minX + thickness - 0.5f, y1 - 0.5f, y2 + 0.5f), color, opacity);
        _fillSmoothRect(tgx::fBox2(B.maxX - thickness + 0.5f, B.maxX + 0.5f, y1 - 0.5f, y2 + 0.5f), color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::fillThickRoundRectAA(const fBox2& B, float corner_radius, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if (corner_radius - thickness < 1) thickness = corner_radius - 1.0f;
        if (thickness < 1)
            {
            drawRoundRectAA(B, corner_radius, color_border, opacity);
            fillRoundRectAA(fBox2(B.minX + 0.5f, B.maxX - 0.5f, B.minY + 0.5f, B.maxY - 0.5f), corner_radius, color_interior, opacity);
            }
        else
            {
            const float tt = thickness;
            drawThickRoundRectAA(B, corner_radius, thickness, color_border, opacity);
            fillRoundRectAA(fBox2(B.minX + tt, B.maxX - tt, B.minY + tt, B.maxY - tt), corner_radius - tt, color_interior, opacity);
            }
        }






    /********************************************************************************
    *
    * DRAWING TRIANGLES
    *
    *********************************************************************************/






    template<typename color_t>
    void Image<color_t>::drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity >= 0.0f)&&(opacity <= 1.0f))
            {
            const int32_t op = (int)(256 * opacity);
            BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
            BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
            BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();
            _bseg_draw(seg12, false, false, color, 0, op, true);
            _bseg_avoid1(seg23, seg21, true, false, true, color, 0, op, true);
            _bseg_avoid11(seg31, seg32, seg12, true, true, true, true, color, 0, op, true);
            }
        else
            {
            _bseg_draw(BSeg(P1, P2), true, false, color, 0, 256, true);
            _bseg_draw(BSeg(P2, P3), true, false, color, 0, 256, true);
            _bseg_draw(BSeg(P3, P1), true, false, color, 0, 256, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();
        fVec2 fP1((float)P1.x, (float)P1.y);
        fVec2 fP2((float)P2.x, (float)P2.y);
        fVec2 fP3((float)P3.x, (float)P3.y);
        _bseg_fill_triangle_precomputed(fP1, fP2, fP3, seg12, seg21, seg23, seg32, seg31, seg13, interior_color, opacity);
        _bseg_draw(seg12, false, false, outline_color, 0, op, true);
        _bseg_avoid1(seg23, seg21, true, false, true, outline_color, 0, op, true);
        _bseg_avoid11(seg31, seg32, seg12, true, true, true, true, outline_color, 0, op, true);
        }




    template<typename color_t>
    void Image<color_t>::drawTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        _bseg_draw_AA(BSeg(P1, P2), true, false, color, op, true);
        _bseg_draw_AA(BSeg(P2, P3), true, false, color, op, true);
        _bseg_draw_AA(BSeg(P3, P1), true, false, color, op, true);
        }



    template<typename color_t>
    void Image<color_t>::drawThickTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color, float opacity)
        {
        fVec2 tab[3] = { P1,P2,P3 };
        drawThickPolygonAA(3, tab, thickness, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::fillTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        float a = _triangleAera(P1, P2, P3); // winding direction of the polygon            
        const int w = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
        const int op = (int)(opacity * 256);
        BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();
        _bseg_fill_triangle_precomputed(P1, P2, P3, seg12, seg21, seg23, seg32, seg31, seg13, color, opacity);	// fill the triangle 
        _bseg_avoid1(seg12, seg13, true, false, true, color, w, op, true);
        _bseg_avoid1(seg23, seg21, true, false, true, color, w, op, true);
        _bseg_avoid1(seg31, seg32, true, false, true, color, w, op, true);
        }


    template<typename color_t>
    void Image<color_t>::fillThickTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        fVec2 P[3] = { P1, P2, P3 };
        fillThickPolygonAA(3, P, thickness, color_interior, color_border, opacity);
        }




    /********************************************************************************
    * DRAWING TRIANGLES WITH THE 3D TRIANGLE RASTERIZER.
    *********************************************************************************/


    template<typename color_t>
    template<typename color_alt>
    void Image<color_t>::drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity)
        {
        if ((opacity >= 0) && (opacity <= 1))
            {
            _drawGradientTriangle<color_alt, true>(P1, P2, P3, colorP1, colorP2, colorP3, opacity);
            }
        else
            {
            _drawGradientTriangle<color_alt, false>(P1, P2, P3, colorP1, colorP2, colorP3, 1.0f);
            }
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity)
        {
        if ((opacity >= 0) && (opacity <= 1))
            {
            _drawTexturedTriangle<color_t_tex, false, true, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), opacity);
            }
        else
            {
            _drawTexturedTriangle<color_t_tex, false, false, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), 1.0f);
            }
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedGradientTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity)
        {
        if ((opacity >= 0) && (opacity <= 1))
            {
            _drawTexturedTriangle<color_t_tex, true, true, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
            }
        else
            {
            _drawTexturedTriangle<color_t_tex, true, false, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, 1.0f);
            }
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity)
        {
        _drawTexturedTriangle<color_t_tex, false, true, true>(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), opacity);
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedGradientMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity)
        {
        _drawTexturedTriangle<color_t_tex, true, true, true>(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
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
            tgx::rasterizeTriangle(_lx, _ly, V1, V2, V3, 0, 0, rparam, tgx::shader_2D_gradient<true, color_t>);
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








    /********************************************************************************
    *
    * DRAWING QUADS
    *
    *********************************************************************************/


    template<typename color_t>
    void Image<color_t>::drawQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity)
        {
        if ((opacity >= 0.0f) && (opacity <= 1.0f))
            {
            const int32_t op = (int)(256 * opacity);
            BSeg P1P2(P1, P2); BSeg P2P1 = P1P2.get_reverse();
            BSeg P2P3(P2, P3); BSeg P3P2 = P2P3.get_reverse();
            BSeg P3P4(P3, P4); BSeg P4P3 = P3P4.get_reverse();
            BSeg P4P1(P4, P1); BSeg P1P4 = P4P1.get_reverse();
            _bseg_avoid1(P1P2, P1P4, true, false, true, color, 0, op, true);
            _bseg_avoid1(P2P3, P2P1, true, false, true, color, 0, op, true);
            _bseg_avoid1(P3P4, P3P2, true, false, true, color, 0, op, true);
            _bseg_avoid1(P4P1, P4P3, true, false, true, color, 0, op, true);
            }
        else
            {
            _bseg_draw(BSeg(P1, P2), true, false, color, 0, 256, true);
            _bseg_draw(BSeg(P2, P3), true, false, color, 0, 256, true);
            _bseg_draw(BSeg(P3, P4), true, false, color, 0, 256, true);
            _bseg_draw(BSeg(P4, P1), true, false, color, 0, 256, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::fillQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (((P1.x == P2.x) && (P3.x == P4.x) && (P2.y == P3.y) && (P1.y == P4.y)) || ((P1.x == P4.x) && (P2.x == P3.x) && (P1.y == P2.y) && (P3.y == P4.y)))
            { // quad is a box...
            iBox2 B(tgx::min(P1.x, P3.x), tgx::max(P1.x, P3.x), tgx::min(P2.y, P4.y), tgx::max(P2.y, P4.y));
            _fillRect(B, color, opacity);
            return;
            }
        const int op = (int)(opacity * 256);
        BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();
        BSeg seg34(P3, P4); BSeg seg43 = seg34.get_reverse();
        BSeg seg41(P4, P1); BSeg seg14 = seg41.get_reverse();
        _bseg_fill_triangle_precomputed(P1, P2, P3, seg12, seg21, seg23, seg32, seg31, seg13, color, opacity);	// fill the triangles 
        _bseg_fill_triangle_precomputed(P1, P3, P4, seg13, seg31, seg34, seg43, seg41, seg14, color, opacity);	// fill the triangles 
        _bseg_avoid1(seg12, seg14, true, false, true, color, 0, op, true);
        _bseg_avoid1(seg23, seg21, true, false, true, color, 0, op, true);
        _bseg_avoid1(seg34, seg32, true, false, true, color, 0, op, true);
        _bseg_avoid1(seg41, seg43, true, false, true, color, 0, op, true);
        _bseg_avoid22(seg13, seg12, seg14, seg32, seg34, true, true, true, true, color, 0, op, true);
        }


    template<typename color_t>
    void Image<color_t>::drawQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        int op = (int)(256 * opacity);
        _bseg_draw_AA(BSeg(P1, P2), true, false, color, op, true);
        _bseg_draw_AA(BSeg(P2, P3), true, false, color, op, true);
        _bseg_draw_AA(BSeg(P3, P4), true, false, color, op, true);
        _bseg_draw_AA(BSeg(P4, P1), true, false, color, op, true);
        }


    template<typename color_t>
    void Image<color_t>::drawThickQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color, float opacity)
        {
        fVec2 tab[4] = { P1, P2, P3, P4 };
        drawThickPolygonAA(4, tab, thickness, color, opacity);
        }



    template<typename color_t>
    void Image<color_t>::fillQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (((P1.x == P2.x) && (P3.x == P4.x) && (P2.y == P3.y) && (P1.y == P4.y)) || ((P1.x == P4.x) && (P2.x == P3.x) && (P1.y == P2.y) && (P3.y == P4.y)))
            { // quad is a box...
            fBox2 B(tgx::min(P1.x, P3.x), tgx::max(P1.x, P3.x), tgx::min(P2.y, P4.y), tgx::max(P2.y, P4.y));
            _fillSmoothRect(B, color, opacity);
            return;
            }
        const float a = _triangleAera(P1, P2, P3); // winding direction of the polygon
        const int w = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
        const int op = (int)(opacity * 256);
        BSeg seg12(P1, P2); BSeg seg21 = seg12.get_reverse();
        BSeg seg13(P1, P3); BSeg seg31 = seg13.get_reverse();
        BSeg seg23(P2, P3); BSeg seg32 = seg23.get_reverse();
        BSeg seg34(P3, P4); BSeg seg43 = seg34.get_reverse();
        BSeg seg41(P4, P1); BSeg seg14 = seg41.get_reverse();
        _bseg_fill_triangle_precomputed(P1, P2, P3, seg12, seg21, seg23, seg32, seg31, seg13, color, opacity);	// fill the triangles 
        _bseg_fill_triangle_precomputed(P1, P3, P4, seg13, seg31, seg34, seg43, seg41, seg14, color, opacity);	// fill the triangles 
        _bseg_avoid1(seg12, seg14, true, false, true, color, w, op, true);
        _bseg_avoid1(seg23, seg21, true, false, true, color, w, op, true);
        _bseg_avoid1(seg34, seg32, true, false, true, color, w, op, true);
        _bseg_avoid1(seg41, seg43, true, false, true, color, w, op, true);
        _bseg_avoid22(seg13, seg12, seg14, seg32, seg34, true, true, true, true, color, 0, op, true);
        }




    template<typename color_t>
    void Image<color_t>::fillThickQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        fVec2 P[4] = { P1, P2, P3, P4 };
        fillThickPolygonAA(4, P, thickness, color_interior, color_border, opacity);
        }



    /********************************************************************************
    * DRAWING QUADS WITH THE 3D TRIANGLE RASTERIZER.
    *********************************************************************************/


    template<typename color_t>
    template<typename color_alt>
    void Image<color_t>::drawGradientQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_alt colorP1, color_alt colorP2, color_alt colorP3, color_alt colorP4, float opacity)
        {
        drawGradientTriangle(P1, P2, P3, colorP1, colorP2, colorP3, opacity);
        drawGradientTriangle(P1, P3, P4, colorP1, colorP3, colorP4, opacity);
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity)
        {
        drawTexturedTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, opacity);
        drawTexturedTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, opacity);
        }


    template<typename color_t>
    template<typename color_t_tex, typename BLEND_OPERATOR>
    void Image<color_t>::drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, const BLEND_OPERATOR& blend_op)
        {
        drawTexturedTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, blend_op);
        drawTexturedTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, blend_op);
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedGradientQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity)
        {
        drawTexturedGradientTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
        drawTexturedGradientTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, C1, C3, C4, opacity);
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity)
        {
        drawTexturedMaskedTriangle(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, opacity);
        drawTexturedMaskedTriangle(src_im, transparent_color, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, opacity);
        }


    template<typename color_t>
    template<typename color_t_tex>
    void Image<color_t>::drawTexturedGradientMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity)
        {
        drawTexturedGradientMaskedTriangle(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
        drawTexturedGradientMaskedTriangle(src_im, transparent_color, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, C1, C3, C4, opacity);
        }





    /********************************************************************************
    *
    * DRAWING POLYLINES AND POLYGONS
    *
    *********************************************************************************/




    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawPolyline(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        iVec2 P, Q;
        if (!next_point(Q)) return;
        while (1)
            {
            P = Q;
            if (!next_point(Q))
                { // last point 
                BSeg seg(P, Q);
                _bseg_draw(seg, true, true, color, 0, op, true);
                return;
                }
            BSeg seg(P, Q);
            _bseg_draw(seg, true, false, color, 0, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::drawPolyline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        drawPolyline(
            [&k, &nbpoints, &tabPoints](tgx::iVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }


    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawPolylineAA(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        fVec2 P, Q;
        if (!next_point(Q)) return;
        while (1)
            {
            P = Q;
            if (!next_point(Q))
                { // last point 
                BSeg seg(P, Q);
                _bseg_draw_AA(seg, true, true, color, op, true);
                return;
                }
            BSeg seg(P, Q);
            _bseg_draw_AA(seg, true, false, color, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::drawPolylineAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        drawPolylineAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }



    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawThickPolylineAA(FUNCTOR_NEXT next_point, float line_width, EndPath end_P0, EndPath end_Pn, color_t color, float opacity)
        {
        if (!isValid() || (line_width <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (line_width <= 1)
            {
            opacity *= line_width;
            drawPolylineAA(next_point, color, opacity);
            return;
            }
        fVec2 P1, P2; 
        if (!next_point(P1)) return;
        if (!next_point(P2))
            {
            drawThickLineAA(P1, P2, line_width, end_P0, end_Pn, color, opacity);
            return;
            }
        float thickness = line_width / 2; 
        int op = (int)(opacity * 256);
        fVec2 H1 = (P2 - P1).getRotate90().getNormalize_fast() * thickness;
        fVec2 I1 = P1 + H1;
        fVec2 J1 = P1 - H1;
        bool first = true; 
        bool last = false; 
        while(1)        
            {
            bool hasmore;
            fVec2 I0 = I1; 
            fVec2 J0 = J1;
            fVec2 H0 = H1;            
            if (last)
                {
                I1 = P2 + H1;
                J1 = P2 - H1; 
                }
            else
                {
                P1 = P2;
                hasmore = next_point(P2);
                H1 = (P2 - P1).getRotate90().getNormalize_fast() * thickness;
                if (!I1.setAsIntersection(I0, P1 + H0, P1 + H1, P2 + H1)) return; //fail
                if (!J1.setAsIntersection(J0, P1 - H0, P1 - H1, P2 - H1)) return; //fail
                }          
            tgx::BSeg J0J1(J0, J1); tgx::BSeg J1J0 = J0J1.get_reverse();
            tgx::BSeg J1I1(J1, I1); tgx::BSeg I1J1 = J1I1.get_reverse();
            tgx::BSeg I1I0(I1, I0); tgx::BSeg I0I1 = I1I0.get_reverse();
            tgx::BSeg I0J0(I0, J0); tgx::BSeg J0I0 = I0J0.get_reverse();
            tgx::BSeg I0J1(I0, J1); tgx::BSeg J1I0 = I0J1.get_reverse();
            _bseg_fill_triangle_precomputed(I0, J0, J1, I0J0, J0I0, J0J1, J1J0, J1I0, I0J1, color, opacity);
            _bseg_fill_triangle_precomputed(I0, J1, I1, I0J1, J1I0, J1I1, I1J1, I1I0, I0I1, color, opacity);
            const int side = 1;
            if (first)
                { // draw first end
                _drawEnd(line_width, I0, J0, I0J0, J0I0, I0I1, J0J1, end_P0, -side, color, opacity); 
                }
            if (last)
                {
                _bseg_draw(J1J0, false, false, color, -side, op, true);
                _bseg_draw(I1I0, false, false, color, side, op, true);
                _bseg_avoid22(I0J1, I0J0, I0I1, J1J0, J1I1, true, true, true, true, color, 0, op, true);
                _drawEnd(line_width, I1, J1, I1J1, J1I1, I1I0, J1J0, end_Pn, side, color, opacity);
                return;
                }
            tgx::BSeg J1J2(J1, J1 + (P2 - P1));
            tgx::BSeg I1I2(I1, I1 + (P2 - P1));
            _bseg_avoid1(J1J0, J1J2, true, false, true, color, -side, op, true);
            _bseg_avoid1(I1I0, I1I2, true, false, true, color, side, op, true);
            _bseg_avoid22(I0J1, I0J0, I0I1, J1J0, J1I1, true, true, true, true, color, 0, op, true);
            _bseg_avoid22(J1I1, J1J0, J1J2, I1I0, I1I2, true, true, true, true, color, 0, op, true);
            if (!hasmore) last = true; 
            first = false;
            }

        }



    template<typename color_t>
    void Image<color_t>::drawThickPolylineAA(int nbpoints, const fVec2 tabPoints[], float line_width, EndPath end_P0, EndPath end_Pn, color_t color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        drawThickPolylineAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            line_width, end_P0, end_Pn, color, opacity);
        }






    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        iVec2 P, Q;
        if (!next_point(Q)) return;
        const iVec2 Q0 = Q; 
        while (1)
            {
            P = Q;
            if (!next_point(Q))
                { // last point 
                _bseg_draw(BSeg(P, Q), true, false, color, 0, op, true);
                _bseg_draw(BSeg(Q, Q0), true, false, color, 0, op, true);
                return;
                }
            _bseg_draw(BSeg(P, Q), true, false, color, 0, op, true);
            }
        }


    template<typename color_t> 
    void Image<color_t>::drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        drawPolygon(
            [&k, &nbpoints, &tabPoints](tgx::iVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }




    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::fillPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);

        fVec2 C(0,0);
        iVec2 iP;
        int nb = 0;
        bool hasmore = true; 
        while (hasmore)
            {
            hasmore = next_point(iP);
            C += fVec2(iP); 
            nb++; 
            }
        if (nb < 3) return;
        C = C * (1.0f / nb);
        _drawPixel<true>(iVec2{ (int32_t)roundf(C.x), (int32_t)roundf(C.y) }, color, opacity);
        fVec2 P0, P1, P2; 
        P1 = fVec2(iP);
        next_point(iP);
        P2 = fVec2(iP);
        const fVec2 sP0 = P2; 
        for (int i = 1; i <= nb; i++)
            {
            P0 = P1; 
            P1 = P2; 
            if (i == nb)
                {
                P2 = sP0;
                }
            else
                {
                next_point(iP);
                P2 = fVec2(iP);
                }
            BSeg P1P2(P1, P2); auto P2P1 = P1P2.get_reverse();
            BSeg P2C(P2, C); auto CP2 = P2C.get_reverse();
            BSeg CP1(C, P1); auto P1C = CP1.get_reverse();
            BSeg P1P0(P1, P0);
            BSeg CP0(C, P0);
            _bseg_fill_triangle_precomputed(P1, P2, C, P1P2, P2P1, P2C, CP2, CP1, P1C, color, opacity);
            _bseg_avoid1(P1P2, P1P0, true, false, true, color, 0, op, true);
            _bseg_avoid21(P1C, P1P0, P1P2, CP2, true, true, true, color, 0, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::fillPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
        {   
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        fillPolygon(
            [&k, &nbpoints, &tabPoints](tgx::iVec2& P)
                {
                if (k == nbpoints) k = 0; 
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }



    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawPolygonAA(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
        fVec2 P, Q;
        if (!next_point(Q)) return;
        const fVec2 Q0 = Q; 
        while (1)
            {
            P = Q;
            if (!next_point(Q))
                { // last point 
                _bseg_draw_AA(BSeg(P, Q), true, false, color, op, true);
                _bseg_draw_AA(BSeg(Q, Q0), true, false, color, op, true);
                return;
                }
            _bseg_draw_AA(BSeg(P, Q), true, false, color, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::drawPolygonAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity)
        {
        if ((!isValid()) || (nbpoints < 2)) return;
        int k = 0;
        drawPolygonAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }



    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::fillPolygonAA(FUNCTOR_NEXT next_point, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);

        fVec2 C(0,0);
        fVec2 iP;
        int nb = 0;
        bool hasmore = true; 
        while (hasmore)
            {
            hasmore = next_point(iP);
            C += iP; 
            nb++; 
            }
        if (nb < 3) return;
        C = C * (1.0f / nb);
        _drawPixel<true>(iVec2{ (int32_t)roundf(C.x), (int32_t)roundf(C.y) }, color, opacity);

        fVec2 P0, P1, P2; 
        P1 = iP;
        next_point(P2);
        const fVec2 sP0 = P2; 

        const float a = _triangleAera(C, P1, P2);
        int w = (a > 0) ? 1 : ((a < 0) ? -1 : 0);

        for (int i = 1; i <= nb; i++)
            {
            P0 = P1; 
            P1 = P2; 
            if (i == nb)
                {
                P2 = sP0;
                }
            else
                {
                next_point(P2);
                }
            BSeg P1P2(P1, P2); auto P2P1 = P1P2.get_reverse();
            BSeg P2C(P2, C); auto CP2 = P2C.get_reverse();
            BSeg CP1(C, P1); auto P1C = CP1.get_reverse();
            BSeg P1P0(P1, P0);
            BSeg CP0(C, P0);
            _bseg_fill_triangle_precomputed(P1, P2, C, P1P2, P2P1, P2C, CP2, CP1, P1C, color, opacity);
            _bseg_avoid1(P1P2, P1P0, true, false, true, color, w, op, true);
            _bseg_avoid21(P1C, P1P0, P1P2, CP2, true, true, true, color, 0, op, true);
            }
        }


    template<typename color_t>
    void Image<color_t>::fillPolygonAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity)
        {   
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        fillPolygonAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            color, opacity);
        }


    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::drawThickPolygonAA(FUNCTOR_NEXT next_point, float thickness, color_t color, float opacity)
        {
        if (!isValid() || (thickness <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (thickness <= 1)
            {
            opacity *= thickness;
            drawPolygonAA(next_point, color, opacity);
            return;
            }

        const int op = (int)(opacity * 256);
        fVec2 Q;
        int nb = 0;
        bool hasmore = true;
        while (hasmore)
            {
            hasmore = next_point(Q);
            nb++;
            }
        if (nb < 3) return;

        fVec2 P[4];
        fVec2 sP[4];
        next_point(P[0]); sP[0] = P[0];
        next_point(P[1]); sP[1] = P[1];
        next_point(P[2]); sP[2] = P[2];
        if (nb > 3) { next_point(P[3]); } else { P[3] = P[0]; } sP[3] = P[3];

        int side = -1;

        fVec2 H0 = (P[1] - P[0]).getRotate90().getNormalize_fast() * thickness;
        fVec2 H1 = (P[2] - P[1]).getRotate90().getNormalize_fast() * thickness;
        fVec2 H2 = (P[3] - P[2]).getRotate90().getNormalize_fast() * thickness;

        fVec2 I0, I1, I2;
        if (!I1.setAsIntersection(P[0] + H0, P[1] + H0, P[1] + H1, P[2] + H1)) return; //fail
        if (!I2.setAsIntersection(P[1] + H1, P[2] + H1, P[2] + H2, P[3] + H2)) return; //fail

        for (int i = 4; i <= nb + 3; i++)
            {
            P[0] = P[1];
            P[1] = P[2];
            P[2] = P[3];
            if (i >= nb)
                {
                P[3] = sP[i - nb];
                }
            else
                {
                next_point(P[3]);
                }
            H0 = H1;
            H1 = H2;
            I0 = I1;
            I1 = I2;
            H2 = (P[3] - P[2]).getRotate90().getNormalize_fast() * thickness;
            if (!I2.setAsIntersection(P[1] + H1, P[2] + H1, P[2] + H2, P[3] + H2)) return; //fail
            tgx::BSeg P0P1(P[0], P[1]); tgx::BSeg P1P0 = P0P1.get_reverse();
            tgx::BSeg P1I1(P[1], I1); tgx::BSeg I1P1 = P1I1.get_reverse();
            tgx::BSeg I1I0(I1, I0); tgx::BSeg I0I1 = I1I0.get_reverse();
            tgx::BSeg I0P0(I0, P[0]); tgx::BSeg P0I0 = I0P0.get_reverse();
            tgx::BSeg I0P1(I0, P[1]); tgx::BSeg P1I0 = I0P1.get_reverse();
            tgx::BSeg P1P2(P[1], P[2]);
            tgx::BSeg I1I2(I1, I2);
            _bseg_fill_triangle_precomputed(I0, P[0], P[1], I0P0, P0I0, P0P1, P1P0, P1I0, I0P1, color, opacity);
            _bseg_fill_triangle_precomputed(I0, P[1], I1, I0P1, P1I0, P1I1, I1P1, I1I0, I0I1, color, opacity);
            _bseg_avoid1(P1P0, P1P2, true, false, true, color, side, op, true);
            _bseg_avoid1(I1I0, I1I2, true, false, true, color, -side, op, true);
            _bseg_avoid22(P1I1, P1P0, P1P2, I1I0, I1I2, true, true, true, true, color, 0, op, true);
            _bseg_avoid22(I0P1, I0P0, I0I1, P1P0, P1I1, true, true, true, true, color, 0, op, true);
            }
        }



    template<typename color_t>
    void Image<color_t>::drawThickPolygonAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        drawThickPolygonAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            thickness, color, opacity);
        }



    template<typename color_t>
    template<typename FUNCTOR_NEXT>
    void Image<color_t>::fillThickPolygonAA(FUNCTOR_NEXT next_point, float thickness, color_t interior_color, color_t border_color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);

        fVec2 C(0, 0);
        fVec2 Q;
        int nb = 0;
        bool hasmore = true;
        while (hasmore)
            {
            hasmore = next_point(Q);
            C += Q;
            nb++;
            }
        if (nb < 3) return;
        C = C * (1.0f / nb);

        fVec2 P[4];
        fVec2 sP[4];
        next_point(P[0]); sP[0] = P[0];
        next_point(P[1]); sP[1] = P[1];
        next_point(P[2]); sP[2] = P[2];
        if (nb > 3) { next_point(P[3]); } else { P[3] = P[0]; } sP[3] = P[3];

        const float w = _triangleAera(C, P[1], P[2]);
        if (w == 0) return; // flat, nothing to draw..
        int side = -1;
        if (w < 0)
            {
            side = 1;
            thickness = -thickness;
            }

        _drawPixel<true>(iVec2{ (int32_t)roundf(C.x), (int32_t)roundf(C.y) }, interior_color, opacity);

        fVec2 H0 = (P[1] - P[0]).getRotate90().getNormalize_fast() * thickness;
        fVec2 H1 = (P[2] - P[1]).getRotate90().getNormalize_fast() * thickness;
        fVec2 H2 = (P[3] - P[2]).getRotate90().getNormalize_fast() * thickness;

        fVec2 I0, I1, I2;
        if (!I1.setAsIntersection(P[0] + H0, P[1] + H0, P[1] + H1, P[2] + H1)) return; //fail
        if (!I2.setAsIntersection(P[1] + H1, P[2] + H1, P[2] + H2, P[3] + H2)) return; //fail

        for (int i = 4; i <= nb + 3; i++)
            {
            P[0] = P[1];
            P[1] = P[2];
            P[2] = P[3];
            if (i >= nb)
                {
                P[3] = sP[i - nb];
                }
            else
                {
                next_point(P[3]);
                }
            H0 = H1;
            H1 = H2;
            I0 = I1;
            I1 = I2;
            H2 = (P[3] - P[2]).getRotate90().getNormalize_fast() * thickness;
            if (!I2.setAsIntersection(P[1] + H1, P[2] + H1, P[2] + H2, P[3] + H2)) return; //fail
            tgx::BSeg P0P1(P[0], P[1]); tgx::BSeg P1P0 = P0P1.get_reverse();
            tgx::BSeg P1I1(P[1], I1); tgx::BSeg I1P1 = P1I1.get_reverse();
            tgx::BSeg I1I0(I1, I0); tgx::BSeg I0I1 = I1I0.get_reverse();
            tgx::BSeg I0P0(I0, P[0]); tgx::BSeg P0I0 = I0P0.get_reverse();
            tgx::BSeg I0P1(I0, P[1]); tgx::BSeg P1I0 = I0P1.get_reverse();
            tgx::BSeg P1P2(P[1], P[2]);
            tgx::BSeg I1I2(I1, I2);
            _bseg_fill_triangle_precomputed(I0, P[0], P[1], I0P0, P0I0, P0P1, P1P0, P1I0, I0P1, border_color, opacity);
            _bseg_fill_triangle_precomputed(I0, P[1], I1, I0P1, P1I0, P1I1, I1P1, I1I0, I0I1, border_color, opacity);
            _bseg_avoid1(P1P0, P1P2, true, false, true, border_color, side, op, true);
            _bseg_avoid1(I1I0, I1I2, true, false, true, border_color, -side, op, true);
            _bseg_avoid22(P1I1, P1P0, P1P2, I1I0, I1I2, true, true, true, true, border_color, 0, op, true);
            _bseg_avoid22(I0P1, I0P0, I0I1, P1P0, P1I1, true, true, true, true, border_color, 0, op, true);
            tgx::BSeg I1C(I1, C);  tgx::BSeg CI1 = I1C.get_reverse();
            tgx::BSeg I0C(I0, C);  tgx::BSeg CI0 = I0C.get_reverse();
            _bseg_fill_triangle_precomputed(C, I0, I1, CI0, I0C, I0I1, I1I0, I1C, CI1, interior_color, opacity);
            _bseg_avoid1(I1I0, I1I2, true, false, true, interior_color, side, op, true);
            _bseg_avoid21(I1C, I1I0, I1I2, CI0, true, true, true, interior_color, 0, op, true);

            }
        }
 

    template<typename color_t>
    void Image<color_t>::fillThickPolygonAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t interior_color, color_t border_color, float opacity)
        {
        if ((nbpoints < 2) || (!isValid())) return;
        int k = 0;
        fillThickPolygonAA(
            [&k, &nbpoints, &tabPoints](tgx::fVec2& P)
                {
                if (k == nbpoints) k = 0;
                P = tabPoints[k++];
                return (k < nbpoints);
                },
            thickness, interior_color, border_color, opacity);
        }



   






    /********************************************************************************
    *
    * DRAWING CIRCLES 
    *
    *********************************************************************************/




    /********************************************
    * DRAWING CIRCLES, LOW QUALITY
    *********************************************/


    template<typename color_t>
    void Image<color_t>::drawCircle(iVec2 center, int r, color_t color, float opacity)
        {
        if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
            _drawFilledCircle<true, false, false>(center.x, center.y, r, color, color, opacity);
        else
            _drawFilledCircle<true, false, true>(center.x, center.y, r, color, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::fillCircle(iVec2 center, int r, color_t interior_color, color_t outline_color, float opacity)
        {
        if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
            _drawFilledCircle<true, true, false>(center.x, center.y, r, outline_color, interior_color, opacity);
        else
            _drawFilledCircle<true, true, true>(center.x, center.y, r, outline_color, interior_color, opacity);
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
                _drawPixel<CHECKRANGE>({ x0 + x, y0 + y }, color, opacity);
                if (x != y) _drawPixel<CHECKRANGE>({ x0 + y, y0 + x }, color, opacity);
                }
            if (cornername & 0x2)
                {
                _drawPixel<CHECKRANGE>({ x0 + x, y0 - y }, color, opacity);
                if (x != y) _drawPixel<CHECKRANGE>({ x0 + y, y0 - x }, color, opacity);
                }
            if (cornername & 0x8)
                {
                _drawPixel<CHECKRANGE>({ x0 - y, y0 + x }, color, opacity);
                if (x != y) _drawPixel<CHECKRANGE>({ x0 - x, y0 + y }, color, opacity);
                }
            if (cornername & 0x1)
                {
                _drawPixel<CHECKRANGE>({ x0 - y, y0 - x }, color, opacity);
                if (x != y) _drawPixel<CHECKRANGE>({ x0 - x, y0 - y }, color, opacity);
                }
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
                if (corners & 1) _drawFastVLine<CHECKRANGE>({ x0 + x, y0 - y }, 2 * y + delta, color, opacity);
                if (corners & 2) _drawFastVLine<CHECKRANGE>({ x0 - x, y0 - y }, 2 * y + delta, color, opacity);
                }
            if (y != py)
                {
                if (corners & 1) _drawFastVLine<CHECKRANGE>({ x0 + py, y0 - px }, 2 * px + delta, color, opacity);
                if (corners & 2) _drawFastVLine<CHECKRANGE>({ x0 - py, y0 - px }, 2 * px + delta, color, opacity);
                py = y;
                }
            px = x;
            }
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
                    _drawPixel<CHECKRANGE>({ xm, ym }, color, opacity);
                    }
                else if (FILL)
                    {
                    _drawPixel<CHECKRANGE>({ xm, ym }, fillcolor, opacity);
                    }
                return;
                }
            case 1:
                {
                if (FILL)
                    {
                    _drawPixel<CHECKRANGE>({ xm, ym }, fillcolor, opacity);
                    }
                _drawPixel<CHECKRANGE>({ xm + 1, ym }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm - 1, ym }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm, ym - 1 }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm, ym + 1 }, color, opacity);
                return;
                }
            }
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            if (OUTLINE)
                {
                _drawPixel<CHECKRANGE>({ xm - x, ym + y }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm - y, ym - x }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm + x, ym - y }, color, opacity);
                _drawPixel<CHECKRANGE>({ xm + y, ym + x }, color, opacity);
                }
            r = err;
            if (r <= y)
                {
                if (FILL)
                    {
                    _drawFastHLine<CHECKRANGE>({ xm, ym + y }, -x, fillcolor, opacity);
                    _drawFastHLine<CHECKRANGE>({ xm + x + 1, ym - y }, -x - 1, fillcolor, opacity);
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
                        _drawFastHLine<CHECKRANGE>({ xm - y + 1, ym - x }, y - 1, fillcolor, opacity);
                        _drawFastHLine<CHECKRANGE>({ xm, ym + x }, y, fillcolor, opacity);
                        }
                    }
                }
            } while (x < 0);
        }







    /********************************************
    * DRAWING CIRCLES, ARC AND PIES, HIGH QUALITY
    *********************************************/


    template<typename color_t>
    float Image<color_t>::_rectifyAngle(float a)
        {
        bool b = false;
        if (a < 0) { a = -a; b = true; }
        a = fmodf(a, 360);
        if (b) a = 360 - a;
        return a;
        }



    template<typename color_t>
    void Image<color_t>::_defaultQuarterVH(int quarter, int & v, int & h)
        {
        switch (quarter)
            {
            case 0: v = 1; h = 1; break;
            case 1: v = 0; h = 1; break;
            case 2: v = 1; h = 0; break;
            default: v = 0; h = 0; break;
            }
        }




    /* Fill a quarter circle intersected with 0,1 or 2 half plane of equation x*kx + y*ky + off > 0  (and x*kx + y*ky + off_full > 0 for full pixel intensity). 
    * 
        2    x=1, y=-1  |  3   x=-1; y=-1
       ---------------------------------
        0    x=1, y=1   |  1   x=-1, y=1    
    */    
    template<typename color_t>
    void Image<color_t>::_fillSmoothQuarterCircleInterHPsub(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity,
        int nb_planes, int32_t kx1, int32_t ky1, int32_t off1, int32_t off1_full, int32_t kx2, int32_t ky2, int32_t off2, int32_t off2_full)
        {
        const int32_t df1 = off1 - off1_full + ((off1 == off1_full) ? 1 : 0);
        const int32_t df2 = off2 - off2_full + ((off2 == off2_full) ? 1 : 0);
        const int32_t op = (int32_t)(opacity * 256); 

        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();
        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - R + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + R - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - R + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + R - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;
        const float RT = (R < 0.5f) ? 4 * R * R : (R + 0.5f);
        const float RA2 = RT * RT;
        const float RB2 = (R < 0.5f) ? -1 : (R - 0.5f) * (R - 0.5f);
        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            float dy2 = (j - C.y); dy2 *= dy2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                float dx2 = (i - C.x); dx2 *= dx2;
                const float e2 = dx2 + dy2;
                if (e2 >= RA2) { i_min = i + dir_x; continue; }
                if (e2 <= RB2)
                    {
                    int x1 = i;
                    int x2 = B.maxX - dir_x;
                    if (x2 < x1) tgx::swap(x1, x2);
                    if (nb_planes & 2)
                        {
                        int32_t o1 = kx1 * x1 + ky1 * j + off1;
                        int32_t o2 = kx2 * x1 + ky2 * j + off2;
                        while (x1 <= x2)
                            {
                            if ((o1 > 0) && (o2 > 0))  operator()(x1, j).blend256(color, ((o1 > df1) && (o2 > df2)) ? op :  tgx::min((o1 * op)/df1, (o2 * op)/ df2));
                            o1 += kx1; o2 += kx2; x1++;
                            }
                        }
                    else if (nb_planes & 1)
                        {
                        int o1 = kx1 * x1 + ky1 * j + off1;
                        while (x1 <= x2)
                            {
                            if (o1 > 0) operator()(x1, j).blend256(color, (o1 > df1) ? op : (o1 * op) / df1);
                            o1 += kx1; x1++;
                            }
                        }
                    else 
                        {
                        while (x1 <= x2)
                            {
                            operator()(x1, j).blend256(color, op);
                            x1++;
                            }
                        }
                    break;
                    }

                float nop = opacity;
                if (nb_planes & 2)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    int32_t o2 = kx2 * i + ky2 * j + off2;
                    if ((o1 <= 0) || (o2 < 0)) continue;
                    nop = tgx::min(opacity, tgx::min((opacity*o1)/df1, (opacity*o2)/df2));
                    }
                else if (nb_planes & 1)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    if (o1 <= 0) continue;
                    nop = tgx::min(opacity, (opacity * o1) / df1);
                    }

                const float alpha = RT - sqrtf(e2);
                _drawPixel<false>({ i, j }, color, alpha * nop);                    
                }
            }
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, color_t color, float opacity)
        {
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 1, kx1, ky1, -minleft1, -maxright1); // include boundary lines
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        int32_t kx2, ky2, minleft2, maxright2;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        seg2.equation(kx2, ky2, minleft2, maxright2, (side2 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 2, kx1, ky1, -minleft1, -maxright1, kx2, ky2, -minleft2, -maxright2); // exclude boundary lines
        }


    /** fill a circle but only the pixel on a given side of seg. */
    template<typename color_t>
    void Image<color_t>::_fillSmoothCircleInterHP(tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg, int side)
        {
        int32_t kx, ky, minleft, maxright;
        seg.equation(kx, ky, minleft, maxright, (side > 0));
        const int32_t off = -maxright;
        _fillSmoothQuarterCircleInterHPsub(C, R, 0, 1, 1, color, opacity, 1, kx, ky, off, off);  // do not draw the boundary line
        _fillSmoothQuarterCircleInterHPsub(C, R, 1, 0, 1, color, opacity, 1, kx, ky, off, off);  //
        _fillSmoothQuarterCircleInterHPsub(C, R, 2, 1, 0, color, opacity, 1, kx, ky, off, off);  //
        _fillSmoothQuarterCircleInterHPsub(C, R, 3, 0, 0, color, opacity, 1, kx, ky, off, off);  //
        }



    template<typename color_t>
    void Image<color_t>::fillCircleAA(fVec2 center, float r, color_t color, float opacity)
        {
        if ((!isValid())||(r<=0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _fillSmoothQuarterCircleInterHPsub(center, r, 0, 1, 1, color, opacity, 0);
        _fillSmoothQuarterCircleInterHPsub(center, r, 1, 0, 1, color, opacity, 0);
        _fillSmoothQuarterCircleInterHPsub(center, r, 2, 1, 0, color, opacity, 0);
        _fillSmoothQuarterCircleInterHPsub(center, r, 3, 0, 0, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::fillCircleSectorAA(fVec2 center, float r, float angle_start, float angle_end, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
	    float a1 = _rectifyAngle(angle_start);
	    float a2 = _rectifyAngle(angle_end);
        const float deg_to_rad = 0.01745329251f;
	    const float rad1 = a1* deg_to_rad;
	    const float rad2 = a2* deg_to_rad;
	    const fVec2 A1(center.x + r * sin(rad1), center.y - r * cos(rad1));
	    const fVec2 A2(center.x + r * sin(rad2), center.y - r * cos(rad2));
        BSeg seg1(center, A1);
        BSeg seg2(center, A2);
        color_t col_origin = readPixel(seg1.pos());
        for (int i = 0; i < 4; i++)
            {
            float m;
            switch (i)
            {
            case 0: m = 180; break;
            case 1: m = 90; break;
            case 2: m = 270; break;
            default: m = 0; break;
            }
            float u1 = a1 - m; if (u1 < 0) u1 += 360;
            float u2 = a2 - m; if (u2 < 0) u2 += 360;
            if (u1 <= u2)
                {
                if (u1 > 135) continue;
                else
                    {
                    if (u2 > 135) 
                        _fillSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    else
                        _fillSmoothQuarterCircleInterHP2(i, center, r, color, opacity, seg1, +1, seg2, -1);
                    }
                }
            else
                {
                if (u1 < 135)
                    {
                    _fillSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    _fillSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg2, -1);
                    }
                else if (u1 < 315)
                    {
                    if (u2 < 135)
                        _fillSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg2, -1);
                    else
                        _fillSmoothQuarterCircleInterHP0(i, center, r, color, opacity);
                    }
                else
                    {
                    if (u2 < 135)
                        _fillSmoothQuarterCircleInterHP2(i, center, r, color, opacity, seg1, +1, seg2, -1);
                    else
                        _fillSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    }
                }
            }
        col_origin.blend256(color, (op * tgx::min(seg1.AA(+1), seg2.AA(-1)))>> 8);        
        drawPixel<true>(seg1.pos(), col_origin);
        }





    /* draw a quarter circle intersected with 0,1 or 2 half plane of equation x*kx + y*ky + off > 0 (and x*kx + y*ky + off_full > 0 for full pixel intensity). 
        2    x=1, y=-1  |  3   x=-1; y=-1
       ---------------------------------
        0    x=1, y=1   |  1   x=-1, y=1
    */
    template<typename color_t>
    void Image<color_t>::_drawSmoothQuarterCircleInterHPsub(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity,
        int nb_planes, int32_t kx1, int32_t ky1, int32_t off1, int32_t off1_full, int32_t kx2, int32_t ky2, int32_t off2, int32_t off2_full)
        {
        const int32_t df1 = off1 - off1_full + ((off1 == off1_full) ? 1 : 0);
        const int32_t df2 = off2 - off2_full + ((off2 == off2_full) ? 1 : 0);
//        const int32_t op = (int32_t)(opacity * 256);

        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();
        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - R + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + R - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - R + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + R - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;
        const float RA2 = (R < 1) ? (4 * R * R) : (R + 1) * (R + 1);
        const float RB2 = (R < 1) ? -1 : (R - 1) * (R - 1);
        if (R < 1) opacity *= R;
        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            float dy2 = (j - C.y); dy2 *= dy2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                float dx2 = (i - C.x); dx2 *= dx2;
                const float e2 = dx2 + dy2;
                if (e2 >= RA2) { i_min = i + dir_x; continue; }
                if (e2 <= RB2) break;

                float nop = opacity;
                if (nb_planes & 2)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    int32_t o2 = kx2 * i + ky2 * j + off2;
                    if ((o1 <= 0) || (o2 < 0)) continue;
                    nop = tgx::min(opacity, tgx::min((opacity*o1)/df1, (opacity*o2)/df2));
                    }
                else if (nb_planes & 1)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    if (o1 <= 0) continue;
                    nop = tgx::min(opacity, (opacity * o1) / df1);
                    }
                const float alpha = 1.0f - fabsf(R - sqrtf(e2));
                _drawPixel<false>({ i,j }, color, alpha * nop);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, color_t color, float opacity)
        {
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 1, kx1, ky1, -minleft1, -maxright1);  // include the boudary line with AA
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        int32_t kx2, ky2, minleft2, maxright2;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        seg2.equation(kx2, ky2, minleft2, maxright2, (side2 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothQuarterCircleInterHPsub(C, R, quarter, v, h, color, opacity, 2, kx1, ky1, -minleft1, -maxright1, kx2, ky2, -minleft2, -maxright2);    // include the boudary lines with AA
        }


    template<typename color_t>
    void Image<color_t>::drawCircleAA(fVec2 center, float r, color_t color, float opacity)
        {
        if ((!isValid()) || (r <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _drawSmoothQuarterCircleInterHPsub(center, r, 0, 1, 1, color, opacity, 0);
        _drawSmoothQuarterCircleInterHPsub(center, r, 1, 0, 1, color, opacity, 0);
        _drawSmoothQuarterCircleInterHPsub(center, r, 2, 1, 0, color, opacity, 0);
        _drawSmoothQuarterCircleInterHPsub(center, r, 3, 0, 0, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::drawCircleArcAA(fVec2 center, float r, float angle_start, float angle_end, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        //const int op = (int)(opacity * 256);
	    float a1 = _rectifyAngle(angle_start);
	    float a2 = _rectifyAngle(angle_end);
        const float deg_to_rad = 0.01745329251f;
	    const float rad1 = a1* deg_to_rad;
	    const float rad2 = a2* deg_to_rad;
	    const fVec2 A1(center.x + r * sin(rad1), center.y - r * cos(rad1));
	    const fVec2 A2(center.x + r * sin(rad2), center.y - r * cos(rad2));
        BSeg seg1(center, A1);
        BSeg seg2(center, A2);
        for (int i = 0; i < 4; i++)
            {
            float m;
            switch (i)
            {
            case 0: m = 180; break;
            case 1: m = 90; break;
            case 2: m = 270; break;
            default: m = 0; break;
            }
            float u1 = a1 - m; if (u1 < 0) u1 += 360;
            float u2 = a2 - m; if (u2 < 0) u2 += 360;
            if (u1 <= u2)
                {
                if (u1 > 135) continue;
                else
                    {
                    if (u2 > 135) 
                        _drawSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    else
                        _drawSmoothQuarterCircleInterHP2(i, center, r, color, opacity, seg1, +1, seg2, -1);
                    }
                }
            else
                {
                if (u1 < 135)
                    {
                    _drawSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    _drawSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg2, -1);
                    }
                else if (u1 < 315)
                    {
                    if (u2 < 135)
                        _drawSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg2, -1);
                    else
                        _drawSmoothQuarterCircleInterHP0(i, center, r, color, opacity);
                    }
                else
                    {
                    if (u2 < 135)
                        _drawSmoothQuarterCircleInterHP2(i, center, r, color, opacity, seg1, +1, seg2, -1);
                    else
                        _drawSmoothQuarterCircleInterHP1(i, center, r, color, opacity, seg1, +1);
                    }
                }
            }
        }





    /* draw a thick quarter circle intersected with 0,1 or 2 half plane of equation x*kx + y*ky + off > 0  (and x*kx + y*ky + off_full > 0 for full pixel intensity). 
        2    x=1, y=-1  |  3   x=-1; y=-1
       ---------------------------------
        0    x=1, y=1   |  1   x=-1, y=1
    */
    template<typename color_t>
    void Image<color_t>::_drawSmoothThickQuarterCircleInterHPsub(tgx::fVec2 C, float R, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity,
        int nb_planes, int32_t kx1, int32_t ky1, int32_t off1, int32_t off1_full, int32_t kx2, int32_t ky2, int32_t off2, int32_t off2_full)
        {
        const int32_t df1 = off1 - off1_full + ((off1 == off1_full) ? 1 : 0);
        const int32_t df2 = off2 - off2_full + ((off2 == off2_full) ? 1 : 0);
        //const int32_t op = (int32_t)(opacity * 256);

        if (thickness > R) thickness = R;
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();
        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - R + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + R - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - R + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + R - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;
        const float RA2 = (R < 1) ? (4 * R * R) : (R + 1) * (R + 1);
        const float RB2 = (R < 1) ? -1 : (R - thickness) * (R - thickness);
        if (R < 1) opacity *= R;
        if (thickness < 0.5f) opacity *= (thickness * 2);
        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            float dy2 = (j - C.y); dy2 *= dy2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                float dx2 = (i - C.x); dx2 *= dx2;
                const float e2 = dx2 + dy2;
                if (e2 >= RA2) { i_min = i + dir_x; continue; }
                if (e2 <= RB2) break;
                float nop = opacity;
                if (nb_planes & 2)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    int32_t o2 = kx2 * i + ky2 * j + off2;
                    if ((o1 <= 0) || (o2 < 0)) continue;
                    nop = tgx::min(opacity, tgx::min((opacity*o1)/df1, (opacity*o2)/df2));
                    }
                else if (nb_planes & 1)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    if (o1 <= 0) continue;
                    nop = tgx::min(opacity, (opacity * o1) / df1);
                    }
                const float se = sqrtf(e2);
                const float d2 = se - R;  const float alpha2 = (d2 > 0) ? (1.0f - d2) : 1.0f;
                const float d1 = se - (R - thickness); const float alpha1 = (d1 < 1) ? d1 : 1.0f;
                const float alpha = alpha1 * alpha2;
                _drawPixel<false>({ i,j }, color, alpha * nop);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothThickQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity)
        {
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothThickQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity, const BSeg& seg1, int side1)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color, opacity, 1, kx1, ky1, -minleft1, -maxright1); // include boundary line with AA
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothThickQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        int32_t kx2, ky2, minleft2, maxright2;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        seg2.equation(kx2, ky2, minleft2, maxright2, (side2 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _drawSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color, opacity, 2, kx1, ky1, -minleft1, -maxright1, kx2, ky2, -minleft2, -maxright2); // include boundary lines with AA
        }


    template<typename color_t>
    void Image<color_t>::drawThickCircleAA(fVec2 center, float r, float thickness, color_t color, float opacity)
        {
        if ((!isValid()) || (r <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;        
        _drawSmoothThickQuarterCircleInterHPsub(center, r, thickness, 0, 1, 1, color, opacity, 0);
        _drawSmoothThickQuarterCircleInterHPsub(center, r, thickness, 1, 0, 1, color, opacity, 0);
        _drawSmoothThickQuarterCircleInterHPsub(center, r, thickness, 2, 1, 0, color, opacity, 0);
        _drawSmoothThickQuarterCircleInterHPsub(center, r, thickness, 3, 0, 0, color, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::drawThickCircleArcAA(fVec2 center, float r, float angle_start, float angle_end, float thickness, color_t color, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        //const int op = (int)(opacity * 256);
	    float a1 = _rectifyAngle(angle_start);
	    float a2 = _rectifyAngle(angle_end);
        const float deg_to_rad = 0.01745329251f;
	    const float rad1 = a1* deg_to_rad;
	    const float rad2 = a2* deg_to_rad;
	    const fVec2 A1(center.x + r * sin(rad1), center.y - r * cos(rad1));
	    const fVec2 A2(center.x + r * sin(rad2), center.y - r * cos(rad2));
        BSeg seg1(center, A1);
        BSeg seg2(center, A2);
        for (int i = 0; i < 4; i++)
            {
            float m;
            switch (i)
            {
            case 0: m = 180; break;
            case 1: m = 90; break;
            case 2: m = 270; break;
            default: m = 0; break;
            }
            float u1 = a1 - m; if (u1 < 0) u1 += 360;
            float u2 = a2 - m; if (u2 < 0) u2 += 360;
            if (u1 <= u2)
                {
                if (u1 > 135) continue;
                else
                    {
                    if (u2 > 135) 
                        _drawSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color, opacity, seg1, +1);
                    else
                        _drawSmoothThickQuarterCircleInterHP2(i, center, r, thickness, color, opacity, seg1, +1, seg2, -1);
                    }
                }
            else
                {
                if (u1 < 135)
                    {
                    _drawSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color, opacity, seg1, +1);
                    _drawSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color, opacity, seg2, -1);
                    }
                else if (u1 < 315)
                    {
                    if (u2 < 135)
                        _drawSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color, opacity, seg2, -1);
                    else
                        _drawSmoothThickQuarterCircleInterHP0(i, center, r, thickness, color, opacity);
                    }
                else
                    {
                    if (u2 < 135)
                        _drawSmoothThickQuarterCircleInterHP2(i, center, r, thickness, color, opacity, seg1, +1, seg2, -1);
                    else
                        _drawSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color, opacity, seg1, +1);
                    }
                }
            }
        }




    /* fill a thick quarter circle intersected with 0,1 or 2 half plane of equation x*kx + y*ky + off > 0  (and x*kx + y*ky + off_full > 0 for full pixel intensity). 
        2    x=1, y=-1  |  3   x=-1; y=-1
       ---------------------------------
        0    x=1, y=1   |  1   x=-1, y=1
    */
    template<typename color_t>
    void Image<color_t>::_fillSmoothThickQuarterCircleInterHPsub(tgx::fVec2 C, float R, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color_interior, color_t color_border, float opacity,
        int nb_planes, int32_t kx1, int32_t ky1, int32_t off1, int32_t off1_full, int32_t kx2, int32_t ky2, int32_t off2, int32_t off2_full)
        {
        const int32_t df1 = off1 - off1_full + ((off1 == off1_full) ? 1 : 0);
        const int32_t df2 = off2 - off2_full + ((off2 == off2_full) ? 1 : 0);
        const int32_t op = (int32_t)(opacity * 256);

        if (thickness > R) thickness = R;
        // check radius >0  and im valid...
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();
        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - R + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + R - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - R + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + R - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;
        const float RA2 = (R < 1) ? (4 * R * R) : (R + 1) * (R + 1);
        const float RB2 = (R < 1) ? -1 : (R - thickness) * (R - thickness);
        if (R < 1) opacity *= R;
        if (thickness < 0.5f) opacity *= (thickness * 2);
        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            float dy2 = (j - C.y); dy2 *= dy2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                float dx2 = (i - C.x); dx2 *= dx2;
                const float e2 = dx2 + dy2;
                if (e2 >= RA2) { i_min = i + dir_x; continue; }
                if (e2 <= RB2)
                    { 
                    int x1 = i;
                    int x2 = B.maxX - dir_x;
                    if (x2 < x1) tgx::swap(x1, x2);
                    if (nb_planes & 2)
                        {
                        int32_t o1 = kx1 * x1 + ky1 * j + off1;
                        int32_t o2 = kx2 * x1 + ky2 * j + off2;
                        while (x1 <= x2)
                            {
                            if ((o1 > 0) && (o2 > 0))  operator()(x1, j).blend256(color_interior, ((o1 > df1) && (o2 > df2)) ? op :  tgx::min((o1 * op)/df1, (o2 * op)/ df2));
                            o1 += kx1; o2 += kx2; x1++;
                            }
                        }
                    else if (nb_planes & 1)
                        {
                        int o1 = kx1 * x1 + ky1 * j + off1;
                        while (x1 <= x2)
                            {
                            if (o1 > 0) operator()(x1, j).blend256(color_interior, (o1 > df1) ? op : (o1 * op) / df1);
                            o1 += kx1; x1++;
                            }
                        }
                    else 
                        {
                        while (x1 <= x2)
                            {
                            operator()(x1, j).blend256(color_interior, op);
                            x1++;
                            }
                        }
                    break;
                    }
                float nop = opacity;
                if (nb_planes & 2)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    int32_t o2 = kx2 * i + ky2 * j + off2;
                    if ((o1 <= 0) || (o2 < 0)) continue;
                    nop = tgx::min(opacity, tgx::min((opacity*o1)/df1, (opacity*o2)/df2));
                    }
                else if (nb_planes & 1)
                    {
                    int32_t o1 = kx1 * i + ky1 * j + off1;
                    if (o1 <= 0) continue;
                    nop = tgx::min(opacity, (opacity * o1) / df1);
                    }
                const float se = sqrtf(e2);
                const float d2 = se - R;  const float alpha2 = (d2 > 0) ? (1.0f - d2) : 1.0f;
                const float d1 = se - (R - thickness); const float alpha1 = (d1 < 1) ? d1 : 1.0f;
                const float alpha = alpha1 * alpha2;
                if (d1 < 1.5f) _drawPixel<false>({ i,j }, color_interior, (1 - d1/2) * nop);
                _drawPixel<false>({ i,j }, color_border, alpha * nop);                
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothThickQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color_interior, color_border, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothThickQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity, const BSeg& seg1, int side1)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color_interior, color_border, opacity, 1, kx1, ky1, -minleft1, -maxright1); // include boundary lines with AA
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothThickQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2)
        {
        int32_t kx1, ky1, minleft1, maxright1;
        int32_t kx2, ky2, minleft2, maxright2;
        seg1.equation(kx1, ky1, minleft1, maxright1, (side1 > 0));
        seg2.equation(kx2, ky2, minleft2, maxright2, (side2 > 0));
        int v, h; _defaultQuarterVH(quarter, v, h);
        _fillSmoothThickQuarterCircleInterHPsub(C, R, thickness, quarter, v, h, color_interior, color_border, opacity, 2, kx1, ky1, -minleft1, -maxright1, kx2, ky2, -minleft2, -maxright2); // include boundary line with AA
        }


    template<typename color_t>
    void Image<color_t>::fillThickCircleAA(fVec2 center, float r, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if ((!isValid()) || (r <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _fillSmoothThickQuarterCircleInterHPsub(center, r, thickness, 0, 1, 1, color_interior, color_border, opacity, 0);
        _fillSmoothThickQuarterCircleInterHPsub(center, r, thickness, 1, 0, 1, color_interior, color_border, opacity, 0);
        _fillSmoothThickQuarterCircleInterHPsub(center, r, thickness, 2, 1, 0, color_interior, color_border, opacity, 0);
        _fillSmoothThickQuarterCircleInterHPsub(center, r, thickness, 3, 0, 0, color_interior, color_border, opacity, 0);
        }


    template<typename color_t>
    void Image<color_t>::fillThickCircleSectorAA(fVec2 center, float r, float angle_start, float angle_end, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if (!isValid()) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        const int op = (int)(opacity * 256);
	    float a1 = _rectifyAngle(angle_start);
	    float a2 = _rectifyAngle(angle_end);
        const float deg_to_rad = 0.01745329251f;
	    const float rad1 = a1* deg_to_rad;
	    const float rad2 = a2* deg_to_rad;
	    const fVec2 A1(center.x + r * sin(rad1), center.y - r * cos(rad1));
	    const fVec2 A2(center.x + r * sin(rad2), center.y - r * cos(rad2));
        BSeg seg1(center, A1);
        BSeg seg2(center, A2);
        color_t col_origin = readPixel(seg1.pos());
        for (int i = 0; i < 4; i++)
            {
            float m;
            switch (i)
            {
            case 0: m = 180; break;
            case 1: m = 90; break;
            case 2: m = 270; break;
            default: m = 0; break;
            }
            float u1 = a1 - m; if (u1 < 0) u1 += 360;
            float u2 = a2 - m; if (u2 < 0) u2 += 360;
            if (u1 <= u2)
                {
                if (u1 > 135) continue;
                else
                    {
                    if (u2 > 135) 
                        _fillSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color_interior, color_border, opacity, seg1, +1);
                    else
                        _fillSmoothThickQuarterCircleInterHP2(i, center, r, thickness, color_interior, color_border, opacity, seg1, +1, seg2, -1);
                    }
                }
            else
                {
                if (u1 < 135)
                    {
                    _fillSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color_interior, color_border, opacity, seg1, +1);
                    _fillSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color_interior, color_border, opacity, seg2, -1);
                    }
                else if (u1 < 315)
                    {
                    if (u2 < 135)
                        _fillSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color_interior, color_border, opacity, seg2, -1);
                    else
                        _fillSmoothThickQuarterCircleInterHP0(i, center, r, thickness, color_interior, color_border, opacity);
                    }
                else
                    {
                    if (u2 < 135)
                        _fillSmoothThickQuarterCircleInterHP2(i, center, r, thickness, color_interior, color_border, opacity, seg1, +1, seg2, -1);
                    else
                        _fillSmoothThickQuarterCircleInterHP1(i, center, r, thickness, color_interior, color_border, opacity, seg1, +1);
                    }
                }
            }
        col_origin.blend256(color_interior, (op * tgx::min(seg1.AA(+1), seg2.AA(-1)))>> 8);        
        drawPixel<true>(seg1.pos(), col_origin);
        }












    /********************************************************************************
    *
    * DRAWING ELLIPSES
    *
    *********************************************************************************/




    /********************************************
    * DRAWING ELLIPSES, LOW QUALITY
    *********************************************/


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
                _drawPixel<CHECKRANGE>({ x0 - x, y0 - y }, outline_color, opacity);
                _drawPixel<CHECKRANGE>({ x0 - x, y0 + y }, outline_color, opacity);
                if (x != 0)
                    {
                    _drawPixel<CHECKRANGE>({ x0 + x, y0 - y }, outline_color, opacity);
                    _drawPixel<CHECKRANGE>({ x0 + x, y0 + y }, outline_color, opacity);
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
                        _drawFastHLine<CHECKRANGE>({ x0 - x, y0 - y }, x + x + 1, interior_color, opacity);
                        _drawFastHLine<CHECKRANGE>({ x0 - x, y0 + y }, x + x + 1, interior_color, opacity);
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
                _drawPixel<CHECKRANGE>({ x0 - x, y0 - y }, outline_color, opacity);
                _drawPixel<CHECKRANGE>({ x0 + x, y0 - y }, outline_color, opacity);
                if (y != 0)
                    {
                    _drawPixel<CHECKRANGE>({ x0 - x, y0 + y }, outline_color, opacity);
                    _drawPixel<CHECKRANGE>({ x0 + x, y0 + y }, outline_color, opacity);
                    }
                }
            if (FILL)
                {
                if (y != yt)
                    {
                    if (y != 0)
                        {
                        _drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 - y }, x + x - 1, interior_color, opacity);
                        }
                    _drawFastHLine<CHECKRANGE>({ x0 - x + 1, y0 + y }, x + x - 1, interior_color, opacity);
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


    template<typename color_t>
    void Image<color_t>::drawEllipse(iVec2 center, iVec2 radiuses, color_t color, float opacity)
        {
        const int cx = center.x;
        const int cy = center.y;
        const int rx = radiuses.x;
        const int ry = radiuses.y;
        if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
            _drawEllipse<true, false, false>(cx, cy, rx, ry, color, color, opacity);
        else
            _drawEllipse<true, false, true>(cx, cy, rx, ry, color, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::fillEllipse(iVec2 center, iVec2 radiuses, color_t interior_color, color_t outline_color, float opacity)
        {
        const int cx = center.x;
        const int cy = center.y;
        const int rx = radiuses.x;
        const int ry = radiuses.y;
        if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
            _drawEllipse<true, true, false>(cx, cy, rx, ry, outline_color, interior_color, opacity);
        else
            _drawEllipse<true, true, true>(cx, cy, rx, ry, outline_color, interior_color, opacity);
        }


   



    /********************************************
    * DRAWING ELLIPSES, HIGH QUALITY
    *********************************************/


    template<typename color_t>
    void Image<color_t>::_drawSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity)
        {
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();

        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - rx + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + rx - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - ry + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + ry - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;

        const float thickness = 1.0f;

        const float inv_rx2 = 1.0f / (rx * rx);
        const float inv_ry2 = 1.0f / (ry * ry);

        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            const float dy = (j - C.y);
            const float dy2 = dy * dy * inv_ry2;
            const float zy = fabsf(dy) * inv_ry2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                const float dx = (i - C.x);
                const float dx2 = dx * dx * inv_rx2;
                const float zx = fabsf(dx) * inv_rx2;
                const float tt = 2 * tgx::max(zx, zy);
                const float e2 = (dx2 + dy2 - 1);
                if (e2 >= tt) { i_min = i + dir_x; continue; }
                if (e2 <= -tt) { break; }
                const float alpha = 1.0f - fabs(e2 / (tt)); // single line                
                _drawPixel<false>({ i,j }, color, alpha * opacity);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::drawEllipseAA(fVec2 center, fVec2 radiuses, color_t color, float opacity)
        {
        if ((!isValid()) || (radiuses.x <= 0) || (radiuses.y<= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _drawSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 0, 1, 1, color, opacity);
        _drawSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 1, 0, 1, color, opacity);
        _drawSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 2, 1, 0, color, opacity);
        _drawSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 3, 0, 0, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::_drawSmoothThickQuarterEllipse(tgx::fVec2 C, float rx, float ry, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity)
        {
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();

        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - rx + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + rx - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - ry + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + ry - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;

        const float inv_rx2 = 1.0f / (rx * rx);
        const float inv_ry2 = 1.0f / (ry * ry);

        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            const float dy = (j - C.y);
            const float dy2 = dy * dy * inv_ry2;
            const float zy = fabsf(dy) * inv_ry2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                const float dx = (i - C.x);
                const float dx2 = dx * dx * inv_rx2;
                const float zx = fabsf(dx) * inv_rx2;
                const float tt = 2 * tgx::max(zx, zy);
                const float e2 = (dx2 + dy2 - 1);
                if (e2 >= tt) { i_min = i + dir_x; continue; }
                if (e2 <= -thickness * tt)
                    {
                    break;
                    }
                float alpha = 1.0f;
                if (e2 > 0)
                    {
                    alpha = 1.0f - (e2 / tt);
                    }
                else if (e2 < (1 - thickness) * tt)
                    {
                    alpha = thickness + (e2 / tt);
                    }
                _drawPixel<false>({ i,j }, color, alpha * opacity);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::drawThickEllipseAA(fVec2 center, fVec2 radiuses, float thickness, color_t color, float opacity)
        {
        if ((!isValid()) || (radiuses.x <= 0) || (radiuses.y <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _drawSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 0, 1, 1, color, opacity);
        _drawSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 1, 0, 1, color, opacity);
        _drawSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 2, 1, 0, color, opacity);
        _drawSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 3, 0, 0, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::_fillSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity)
        {
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();

        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - rx + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + rx - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - ry + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + ry - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;

        const float thickness = 4.0f;

        const float inv_rx2 = 1.0f / (rx * rx);
        const float inv_ry2 = 1.0f / (ry * ry);

        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            const float dy = (j - C.y);
            const float dy2 = dy * dy * inv_ry2;
            const float zy = fabsf(dy) * inv_ry2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                const float dx = (i - C.x);
                const float dx2 = dx * dx * inv_rx2;
                const float zx = fabsf(dx) * inv_rx2;
                const float tt = 2 * tgx::max(zx, zy);
                const float e2 = (dx2 + dy2 - 1);
                if (e2 >= tt) { i_min = i + dir_x; continue; }
                if (e2 <= -tt)
                    {
                    const int h = B.maxX - dir_x - i;
                    if (h >= 0)
                        _drawFastHLine<false>({ i,j }, h + 1, color, opacity);
                    else
                        _drawFastHLine<false>({ B.maxX - dir_x,j },  1 - h, color, opacity);
                    break;
                    }
                const float alpha =  (1 -(e2/tt)) * 0.5f ;
                _drawPixel<false>({ i,j }, color, alpha * opacity);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::fillEllipseAA(fVec2 center, fVec2 radiuses, color_t color, float opacity)
        {
        if ((!isValid()) || (radiuses.x <= 0) || (radiuses.y <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        _fillSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 0, 1, 1, color, opacity);
        _fillSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 1, 0, 1, color, opacity);
        _fillSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 2, 1, 0, color, opacity);
        _fillSmoothQuarterEllipse(center, radiuses.x, radiuses.y, 3, 0, 0, color, opacity);
        }




    template<typename color_t>
    void Image<color_t>::_fillSmoothThickQuarterEllipse(tgx::fVec2 C, float rx, float ry, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color_interior, color_t color_border, float opacity)
        {
        const int dir_x = (quarter & 1) ? -1 : 1;
        const int dir_y = (quarter & 2) ? -1 : 1;
        auto B = imageBox();

        B &= tgx::iBox2(
            ((dir_x > 0) ? (int)floorf(C.x - rx + 0.5f) : (int)roundf(C.x) + ((vertical_center_line) ? 0 : 1)),
            ((dir_x > 0) ? ((int)roundf(C.x) - ((vertical_center_line) ? 0 : 1)) : (int)ceilf(C.x + rx - 0.5f)),
            ((dir_y > 0) ? ((int)roundf(C.y) + ((horizontal_center_line) ? 0 : 1)) : (int)floorf(C.y - ry + 0.5f)),
            ((dir_y > 0) ? (int)ceilf(C.y + ry - 0.5f) : (int)roundf(C.y) - ((horizontal_center_line) ? 0 : 1)));
        if (B.isEmpty()) return;
        if (dir_y < 0) { tgx::swap(B.minY, B.maxY); }
        B.maxY += dir_y;
        if (dir_x < 0) { tgx::swap(B.minX, B.maxX); }
        B.maxX += dir_x;

        const float inv_rx2 = 1.0f / (rx * rx);
        const float inv_ry2 = 1.0f / (ry * ry);

        int i_min = B.minX;
        for (int j = B.minY; j != B.maxY; j += dir_y)
            {
            const float dy = (j - C.y);
            const float dy2 = dy * dy * inv_ry2;
            const float zy = fabsf(dy) * inv_ry2;
            for (int i = i_min; i != B.maxX; i += dir_x)
                {
                const float dx = (i - C.x);
                const float dx2 = dx * dx * inv_rx2;
                const float zx = fabsf(dx) * inv_rx2;
                const float tt = 2 * tgx::max(zx, zy);
                const float e2 = (dx2 + dy2 - 1);
                if (e2 >= tt) { i_min = i + dir_x; continue; }
                if (e2 <= -thickness * tt)
                    {
                    const int h = B.maxX - dir_x - i;
                    if (h >= 0)
                        _drawFastHLine<false>({ i,j }, h + 1, color_interior, opacity);
                    else
                        _drawFastHLine<false>({ B.maxX - dir_x,j }, 1 - h, color_interior, opacity);
                    break;
                    }
                float alpha = 1.0f;
                if (e2 > 0)
                    {
                    alpha = 1.0f - (e2 / tt);
                    }
                else if (e2 < (1 - thickness) * tt)
                    {
                    alpha = thickness + (e2 / tt);
                    _drawPixel<false>({ i,j }, color_interior, (1-alpha/2) * opacity);
                    }
                _drawPixel<false>({ i,j }, color_border, alpha * opacity);
                }
            }
        return;
        }


    template<typename color_t>
    void Image<color_t>::fillThickEllipseAA(fVec2 center, fVec2 radiuses, float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if ((!isValid()) || (radiuses.x <= 0) || (radiuses.y <= 0)) return;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;        
        _fillSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 0, 1, 1, color_interior, color_border, opacity);
        _fillSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 1, 0, 1, color_interior, color_border, opacity);
        _fillSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 2, 1, 0, color_interior, color_border, opacity);
        _fillSmoothThickQuarterEllipse(center, radiuses.x, radiuses.y, thickness, 3, 0, 0, color_interior, color_border, opacity);
        }







    /********************************************************************************
    *
    * DRAWING BEZIER CURVES AND SPLINES
    *
    *********************************************************************************/






    template<typename color_t>
    void Image<color_t>::drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity)
        {
        _drawQuadBezier(P1, P2, PC, wc, drawP2, color, opacity);
        }

    template<typename color_t>
    void Image<color_t>::drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity)
        {
        _drawCubicBezier(P1, P2, PA, PB, drawP2, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawQuadSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
        {
        _drawQuadSpline<SPLINE_MAX_POINTS>(nbpoints, tabPoints, draw_last_point, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawCubicSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
        {
        _drawCubicSpline<SPLINE_MAX_POINTS>(nbpoints, tabPoints, draw_last_point, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawClosedSpline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
        {
        _drawClosedSpline<SPLINE_MAX_POINTS>(nbpoints, tabPoints, color, opacity);
        }




    /**
    * draw a limited rational Bezier segment, squared weight. Used by _plotQuadRationalBezier().
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotQuadRationalBezierSeg(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const color_t color, const float opacity)
        {
        if ((x0 == x2) && (y0 == y2)) { return; }
        int sx = x2 - x1, sy = y2 - y1;
        float dx = (float)(x0 - x2), dy = (float)(y0 - y2), xx = (float)(x0 - x1), yy = (float)(y0 - y1);
        float xy = xx * sy + yy * sx, cur = xx * sy - yy * sx, err;
        if ((cur == 0) || (w <= 0))
            {
            _drawSeg({ x0, y0 }, true, { x2, y2 }, false, color, opacity);
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
                _plotQuadRationalBezierSeg(checkrange, sx, sy, (int)dx, (int)dy, x0, y0, cur, color, opacity);
                dx = floorf((w * x1 + x2) * xy + 0.5f); dy = floorf((y1 * w + y2) * xy + 0.5f);
                _plotQuadRationalBezierSeg(checkrange, x2, y2, (int)dx, (int)dy, sx, sy, cur, color, opacity);
                }
            else
                {
                _plotQuadRationalBezierSeg(checkrange, x0, y0, (int)dx, (int)dy, sx, sy, cur, color, opacity);
                dx = floorf((w * x1 + x2) * xy + 0.5f); dy = floorf((y1 * w + y2) * xy + 0.5f);
                _plotQuadRationalBezierSeg(checkrange, sx, sy, (int)dx, (int)dy, x2, y2, cur, color, opacity);
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
                if (sw) _drawPixel(checkrange, { x0, y0 }, color, opacity); // write last if swaped
                return;
                }
            _drawPixel(checkrange, { x0, y0 }, color, opacity);
            x1 = 2 * err > dy; y1 = 2 * (err + yy) < -dy;
            if (2 * err < dx || y1) { y0 += sy; dy += xy; err += dx += xx; }
            if (2 * err > dx || x1) { x0 += sx; dx += xy; err += dy += yy; }
            }
        _drawSeg({ x0, y0 }, true, { x2, y2 }, sw, color, opacity);
        }


    /**
    * Plot any quadratic rational Bezier curve.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotQuadRationalBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const bool draw_P2, const color_t color, const float opacity)
        {
        if (checkrange)
            { // check if we can discard the whole curve
            iBox2 mbr(iVec2{ x0, y0 });
            mbr |= iVec2{ x1, y1 };
            mbr |= iVec2{ x2, y2 };
            if ((mbr & iBox2(0, _lx - 1, 0, _ly - 1)).isEmpty()) return;
            }
        if (draw_P2) { _drawPixel(checkrange, { x2, y2 }, color, opacity); }
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
            _plotQuadRationalBezierSeg(checkrange, x0, y0, x, (int)floorf(yy + 0.5f), x, y, ww, color, opacity);
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
            _plotQuadRationalBezierSeg(checkrange, x0, y0, (int)floorf(xx + 0.5f), y, x, y, ww, color, opacity);
            xx = (x1 - x2) * (yy - y2) / (y1 - y2) + x2;
            x1 = (int)floorf(xx + 0.5f); x0 = x; y0 = y1 = y;
            }
        _plotQuadRationalBezierSeg(checkrange, x0, y0, x1, y1, x2, y2, w * w, color, opacity);
        }


    template<typename color_t>
    void Image<color_t>::_drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity)
        {
        if (!isValid()) return;
        if (wc < 0) wc = 0;
        const bool checkrange = ((P1.x < 0) || (P2.x < 0) || (PC.x < 0) ||
            (P1.y < 0) || (P2.y < 0) || (PC.y < 0) ||
            (P1.x >= _lx) || (P2.x >= _lx) || (PC.x >= _lx) ||
            (P1.y >= _ly) || (P2.y >= _ly) || (PC.y >= _ly));
        _plotQuadRationalBezier(checkrange, P1.x, P1.y, PC.x, PC.y, P2.x, P2.y, wc, drawP2, color, opacity);
        }



    /**
    * Plot a limited cubic Bezier segment.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotCubicBezierSeg(const bool checkrange, int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, const color_t color, const float opacity)
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
            _plotQuadRationalBezierSeg(checkrange, x0, y0, sx, sy, x3, y3, 1.0f, color, opacity);
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
                if ((x0 != sax3) || (y0 != say3)) _drawPixel(checkrange, { x0, y0 }, color, opacity);
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
        if ((x0 == sax3) && (y0 == say3)) { _drawSeg({ x3, y3 }, true, { x0, y0 }, false, color, opacity); }
        else if ((x3 == sax3) && (y3 == say3)) { _drawSeg({ x0, y0 }, true, { x3, y3 }, false, color, opacity); }
        else _drawSeg({ x0, y0 }, true, { x3, y3 }, true, color, opacity);
        }



    /**
    * Plot any cubic Bezier curve.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotCubicBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool draw_P2, const color_t color, const float opacity)
        {
        if (checkrange)
            { // check if we can discard the whole curve
            iBox2 mbr(iVec2{ x0, y0 });
            mbr |= iVec2{ x1, y1 };
            mbr |= iVec2{ x2, y2 };
            mbr |= iVec2{ x3, y3 };
            if ((mbr & iBox2(0, _lx - 1, 0, _ly - 1)).isEmpty()) return;
            }
        if (draw_P2) { _drawPixel(checkrange, { x3, y3 }, color, opacity); }
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
                _plotCubicBezierSeg(checkrange, x0, y0, x0 + fx1, y0 + fy1, x0 + fx2, y0 + fy2, x3, y3, color, opacity);
                }
            x0 = x3; y0 = y3; fx0 = fx3; fy0 = fy3; t1 = t2;
            }
        }


    template<typename color_t>
    void Image<color_t>::_drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity)
        {
        if (!isValid()) return;
        const bool checkrange = ((P1.x < 0) || (P2.x < 0) || (PA.x < 0) || (PB.x < 0) ||
            (P1.y < 0) || (P2.y < 0) || (PA.y < 0) || (PB.y < 0) ||
            (P1.x >= _lx) || (P2.x >= _lx) || (PA.x >= _lx) || (PB.x >= _lx) ||
            (P1.y >= _ly) || (P2.y >= _ly) || (PA.y >= _ly) || (PB.y >= _ly));
        _plotCubicBezier(checkrange, P1.x, P1.y, PA.x, PA.y, PB.x, PB.y, P2.x, P2.y, drawP2, color, opacity);
        }


    /**
    * plot quadratic spline, destroys input arrays x,y.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotQuadSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity)
        {
        if (draw_last) { _drawPixel(true, { x[n], y[n] }, color, opacity); }
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
            _drawQuadBezier({ (x0 + x1) / 2, (y0 + y1) / 2 }, { x2, y2 }, { x1, y1 }, 1.0f, false, color, opacity);
            x2 = (x0 + x1) / 2; x1 = x0;
            y2 = (y0 + y1) / 2; y1 = y0;
            }
        _drawQuadBezier({ x[0], y[0] }, { x2, y2 }, { x1, y1 }, 1.0f, false, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS> void Image<color_t>::_drawQuadSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity)
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
            if (draw_last_point) _drawPixel(true, { tabPoints[0].x, tabPoints[0].y }, color, opacity);
            return;
            }
        case 2:
            {
            _drawSeg({ tabPoints[0].x, tabPoints[0].y }, true, { tabPoints[1].x, tabPoints[1].y }, draw_last_point, color, opacity);
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
            _plotQuadSpline(nbpoints - 1, xx, yy, draw_last_point, color, opacity);
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
    void Image<color_t>::_plotClosedSpline(int n, int x[], int y[], const color_t color, const float opacity)
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
        _drawQuadBezier({ (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 }, { (x[0] + x[1]) / 2, (y[0] + y[1]) / 2 }, { x[0], y[0] }, 1.0f, false, color, opacity);
        for (int i = 1; i < n - 1; i++)
            {
            _drawQuadBezier({ (x[i - 1] + x[i]) / 2, (y[i - 1] + y[i]) / 2 }, { (x[i] + x[i + 1]) / 2, (y[i] + y[i + 1]) / 2 }, { x[i], y[i] }, 1.0f, false, color, opacity);
            }
        _drawQuadBezier({ (x[n - 2] + x[n - 1]) / 2, (y[n - 2] + y[n - 1]) / 2 }, { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 }, { x[n - 1], y[n - 1] }, 1.0f, false, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS> void Image<color_t>::_drawClosedSpline(int nbpoints, const iVec2* tabPoints, color_t color, float opacity)
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
            _drawPixel(true, { tabPoints[0].x, tabPoints[0].y }, color, opacity);
            return;
            }
        case 2:
            {
            _drawSeg({ tabPoints[0].x, tabPoints[0].y }, true, { tabPoints[1].x, tabPoints[1].y }, true, color, opacity);
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
            _plotClosedSpline(nbpoints, xx, yy, color, opacity);
            }
            }
        }


    /**
    * plot cubic spline, destroys input arrays x,y.
    * adapted from Alois Zingl  (http://members.chello.at/easyfilter/bresenham.html)
    * change: do not draw always the endpoint (x2,y2)
    **/
    template<typename color_t>
    void Image<color_t>::_plotCubicSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity)
        {
        if (draw_last) { _drawPixel(true, { x[n], y[n] }, color, opacity); }
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
        _drawCubicBezier({ x3, y3 }, { x4, y4 }, { (x2 + x4) / 2, (y2 + y4) / 2 }, { x4, y4 }, false, color, opacity);
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
            _drawCubicBezier({ x4, y4 }, { x3, y3 }, { (int)floorf((2 * x1 + x2) / 3 + 0.5f), (int)floorf((2 * y1 + y2) / 3 + 0.5f) }, { (int)floorf((x1 + 2 * x2) / 3 + 0.5f), (int)floorf((y1 + 2 * y2) / 3 + 0.5f) }, false, color, opacity);
            x3 = x4; y3 = y4; x2 = x1; y2 = y1; x1 = x0; y1 = y0;
            }
        x0 = x[0]; x4 = (int)floorf((3 * x0 + 7 * x1 + 2 * x2 + 6) / 12.0f);
        y0 = y[0]; y4 = (int)floorf((3 * y0 + 7 * y1 + 2 * y2 + 6) / 12.0f);
        _drawCubicBezier({ x4, y4 }, { x3, y3 }, { (int)floorf((2 * x1 + x2) / 3 + 0.5f), (int)floorf((2 * y1 + y2) / 3 + 0.5f) }, { (int)floorf((x1 + 2 * x2) / 3 + 0.5f), (int)floorf((y1 + 2 * y2) / 3 + 0.5f) }, false, color, opacity);
        _drawCubicBezier({ x0, y0 }, { x4, y4 }, { x0, y0 }, { (x0 + x1) / 2, (y0 + y1) / 2 }, false, color, opacity);
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS> void Image<color_t>::_drawCubicSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity)
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
            if (draw_last_point) _drawPixel(true, { tabPoints[0].x, tabPoints[0].y }, color, opacity);
            return;
            }
        case 2:
            {
            _drawSeg({ tabPoints[0].x, tabPoints[0].y }, true, { tabPoints[1].x, tabPoints[1].y }, draw_last_point, color, opacity);
            return;
            }
        case 3:
            {
            _drawQuadSpline<SPLINE_MAX_POINTS>(nbpoints, tabPoints, draw_last_point, color, opacity);
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
            _plotCubicSpline(nbpoints - 1, xx, yy, draw_last_point, color, opacity);
        }
        }
    }








    template<typename color_t>
    bool Image<color_t>::_splitRationalQuadBezier(fVec2 P1, fVec2 P2, fVec2 PC, float w, fVec2& Q, fVec2& PB, float& wb)
        {
        const int MAX_ITER = 20;    // maximum number of iteration to prevent stalling
        const float l = 0.25f;
        const float a = _triangleAera(P1, P2, PC);
        if (a * a < l * (P1 - P2).norm2())
            { // done !
            return true;
            }
        float t = 0.5f;
        int d = 0;
        while (1)
            {
            float ua = (1 - t) + w * t;
            float ub = w * (1 - t) + t;
            float u3 = ((1 - t) * ua + t * ub);

            tgx::fVec2 PA = (P1 * (1 - t) + PC * (w * t)) / ua;
            PB = (PC * w * (1 - t) + P2 * t) / ub;
            Q = (PA * (1 - t) * ua + PB * ub * t) / u3;

            const float a = _triangleAera(P1, Q, PA);
            const float n2 = (P1 - Q).norm2();
            d++;
            if ((n2 < 2) || (a * a < l * n2) || (d > MAX_ITER))
                { // done !
                wb = ub / sqrtf(u3);
                return false;
                }
            t /= 2;
            }
        }



    template<typename color_t>
    void Image<color_t>::drawThickQuadBezierAA(fVec2 P1, fVec2 P2, fVec2 PC, float wc, float thickness, EndPath end_P1, EndPath end_P2, color_t color, float opacity)
        {
        if (!isValid() || (thickness <=0)) return;
        if (wc <= 0)
            {
            drawThickLineAA(P1, P2, thickness, end_P1, end_P2, color, opacity);
            return;
            }
        bool done = false; 
        drawThickPolylineAA(            
            [&P1, &P2, &PC, &wc, &done](tgx::fVec2& P)
                {
                if (done)
                    {
                    P = P2;
                    return false;
                    }
                P = P1;
                tgx::fVec2 Q, PB;
                float wb;
                done = _splitRationalQuadBezier(P1, P2, PC, wc, Q, PB, wb);
                P1 = Q;
                PC = PB;
                wc = wb;
                return true;
                },
            thickness, end_P1, end_P2, color, opacity);
        }




    template<typename color_t>
    bool Image<color_t>::_splitCubicBezier(fVec2 P1, fVec2 P2, fVec2 PC1, fVec2 PC2, fVec2& Q, fVec2& C, fVec2& D)
        {
        const int MAX_ITER = 20;    // maximum number of iteration to prevent stalling
        const float l = 0.25f;        
        const float nn = (P1 - P2).norm2();
        const float a1 = _triangleAera(P1, P2, PC1);
        const float a2 = _triangleAera(P1, P2, PC2);
        if ((nn < 2) || ((a1 * a1 < l*nn)&&(a2 * a2 < l*nn))) return true;
        float t = 0.5f;
        int d = 0;
        while (1)
            {
            const fVec2 A = (1 - t) * P1 + t * PC1; 
            const fVec2 X = (1 - t) * PC1 + t * PC2;
            const fVec2 B = (1 - t) * A + t * X;
            D = (1 - t) * PC2 + t * P2;
            C = (1 - t) * X + t * D;
            Q = (1 - t) * B + t * C;
            const float nn = (P1 - Q).norm2();
            const float a1 = _triangleAera(P1, A, Q);
            const float a2 = _triangleAera(P1, B, Q);
            if ((nn < 2) || (d++ > MAX_ITER) || ((a1 * a1 < l*nn) && (a2 * a2 < l*nn)) ) return false;
            t /= 2;
            }
        }



    template<typename color_t>
    void Image<color_t>::drawThickCubicBezierAA(fVec2 P1, fVec2 P2, fVec2 PC1, fVec2 PC2, float thickness, EndPath end_P1, EndPath end_P2, color_t color, float opacity)
        {
        if (!isValid() || (thickness <=0)) return;
        bool done = false; 
        drawThickPolylineAA(            
            [&P1, &P2, &PC1, &PC2, &done](tgx::fVec2& P)
                {
                if (done)
                    {
                    P = P2;
                    return false;
                    }
                P = P1;
                tgx::fVec2 Q, C, D;
                done = _splitCubicBezier(P1, P2, PC1, PC2, Q, C, D);
                P1 = Q;
                PC1 = C;
                PC2 = D;
                return true;
                },
            thickness, end_P1, end_P2, color, opacity);
        }





    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawThickQuadSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity)
        {
        if (!isValid() || (thickness <= 0)) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
            {
        case 0:
        case 1:
            return;
        case 2:
            {
            drawThickLineAA(tabPoints[0], tabPoints[1], thickness, end_P0, end_Pn, color, opacity);
            return;
            }
        default:
            {
            float x[SPLINE_MAX_POINTS];
            float y[SPLINE_MAX_POINTS];
            for (int i = 0; i < nbpoints; i++)
                {
                x[i] = tabPoints[i].x;
                y[i] = tabPoints[i].y;
                }
            const int n = nbpoints - 1;
            const int M_MAX = 6;
            float mi = 1, m[M_MAX];
            float x0, y0, x1, y1, x2 = x[n], y2 = y[n];
            x[1] = x0 = 8 * x[1] - 2 * x[0];
            y[1] = y0 = 8 * y[1] - 2 * y[0];
            for (int i = 2; i < n; i++)
                {
                if (i - 2 < M_MAX) m[i - 2] = mi = 1.0f / (6.0f - mi);
                x[i] = x0 = 8 * x[i] - x0 * mi;
                y[i] = y0 = 8 * y[i] - y0 * mi;
                }
            x1 = (x0 - 2 * x2) / (5.0f - mi);
            y1 = (y0 - 2 * y2) / (5.0f - mi);

            int i = n - 2; 
            bool loadstart = true; 
            bool begin = true; 
            fVec2 P1, P2, PC; 

            drawThickPolylineAA(
                [&](tgx::fVec2& P)
                    {
                    if (loadstart)
                        { // we are at the beginning of a new Bezier segment, load P1, P2, PC
                        if (i > 0)
                            {
                            if (i <= M_MAX) mi = m[i - 1];
                            x0 = (x[i] - x1) * mi;
                            y0 = (y[i] - y1) * mi;
                            P2 = { (x0 + x1) / 2, (y0 + y1) / 2 };
                            P1 = { x2, y2 };
                            PC = { x1, y1 };
                            x2 = (x0 + x1) / 2; x1 = x0;
                            y2 = (y0 + y1) / 2; y1 = y0;
                            }
                        else
                            { // last segment
                            P2 = { x[0], y[0] };
                            P1 = { x2, y2 };
                            PC = { x1, y1 };
                            }                         
                        i--; 
                        loadstart = false; 
                        }
                    if (begin)
                        {
                        begin = false; 
                        P = P1; 
                        return true; 
                        }
                    // here, we are on curve P1, P2, PC and P1 has already been plotted.                     
                    fVec2 Q, PB;
                    float wb;
                    if (_splitRationalQuadBezier(P1, P2, PC, 1.0f, Q, PB, wb))
                        { // done with this curve. 
                        P = P2; 
                        if (i == -1) return false; 
                        loadstart = true;
                        return true;
                        }
                    P = Q; 
                    P1 = Q;
                    PC = PB;
                    return true;
                    },
                thickness, end_Pn, end_P0, color, opacity);
            }
            }
        }





    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawThickCubicSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity)
        {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
            {
        case 0:
        case 1:
            return;
        case 2:
            {
            drawThickLineAA(tabPoints[0], tabPoints[1], thickness, end_P0, end_Pn, color, opacity);
            return;
            }
        case 3:
            {
            drawThickQuadSplineAA<SPLINE_MAX_POINTS>(nbpoints, tabPoints, thickness, end_P0, end_Pn, color, opacity);
            return;
            }
        default:
            {
            float x[SPLINE_MAX_POINTS];
            float y[SPLINE_MAX_POINTS];
            for (int k = 0; k < nbpoints; k++)
                {
                x[k] = tabPoints[k].x;
                y[k] = tabPoints[k].y;
                }
            const int n = nbpoints - 1;

            const int M_MAX = 6;
            float mi = 0.25f, m[M_MAX];
            float x3 = x[n - 1], y3 = y[n - 1], x4 = x[n], y4 = y[n];
            float x0, y0, x1, y1, x2, y2;
            x[1] = x0 = 12 * x[1] - 3 * x[0];
            y[1] = y0 = 12 * y[1] - 3 * y[0];
            for (int i = 2; i < n; i++)
                {
                if (i - 2 < M_MAX) m[i - 2] = mi = 0.25f / (2.0f - mi);
                x[i] = x0 = 12 * x[i] - 2 * x0 * mi;
                y[i] = y0 = 12 * y[i] - 2 * y0 * mi;
                }
            x2 = (x0 - 3 * x4) / (7 - 4 * mi);
            y2 = (y0 - 3 * y4) / (7 - 4 * mi);

            int i = n - 2;
            bool loadstart = true;
            bool begin = true;
            fVec2 P1, P2, PC1, PC2;

            drawThickPolylineAA(
                [&](tgx::fVec2& P)
                    {
                    if (loadstart)
                        { // we are at the beginning of a new Bezier segment, load P1, P2, PC1, PC2
                        if (i == n - 2)
                            {
                            P2 = { x3, y3 };
                            P1 = { x4, y4 };
                            PC2 = { (x2 + x4) / 2, (y2 + y4) / 2 };
                            PC1 = { x4, y4 };
                            if (n - 3 < M_MAX) mi = m[n - 3];
                            x1 = (x[n - 2] - 2 * x2) * mi;
                            y1 = (y[n - 2] - 2 * y2) * mi;
                            }
                        else if (i > 0)
                            {
                            if (i <= M_MAX) mi = m[i - 1];
                            x0 = (x[i] - 2 * x1) * mi;
                            y0 = (y[i] - 2 * y1) * mi;
                            x4 = (x0 + 4 * x1 + x2 + 3) / 6.0f;
                            y4 = (y0 + 4 * y1 + y2 + 3) / 6.0f;
                            P2 = { x4, y4 };
                            P1 = { x3, y3 };
                            PC2 = { (2 * x1 + x2) / 3 , (2 * y1 + y2) / 3 };
                            PC1 = { (x1 + 2 * x2) / 3 , (y1 + 2 * y2) / 3 };
                            x3 = x4; y3 = y4; x2 = x1; y2 = y1; x1 = x0; y1 = y0;
                            }
                        else if (i == 0)
                            {              
                            x0 = x[0]; x4 = (3 * x0 + 7 * x1 + 2 * x2 + 6) / 12.0f;
                            y0 = y[0]; y4 = (3 * y0 + 7 * y1 + 2 * y2 + 6) / 12.0f;
                            P2 = { x4, y4 };
                            P1 = { x3, y3 };
                            PC2 = { (2 * x1 + x2) / 3 , (2 * y1 + y2) / 3 };
                            PC1 = { (x1 + 2 * x2) / 3 , (y1 + 2 * y2) / 3 };
                            }
                        else
                            {
                            P2 = { x0, y0 };
                            P1 = { x4, y4 };
                            PC2 = { x0, y0 };
                            PC1 = { (x0 + x1) / 2, (y0 + y1) / 2 };
                            }
                        i--;
                        loadstart = false;
                        }
                    if (begin)
                        {
                        begin = false;
                        P = P1;
                        return true;
                        }
                    // here, we are on curve P1, P2, PC1 and PC2 has already been plotted.                     
                    fVec2 Q, C, D;
                    if (_splitCubicBezier(P1, P2, PC1, PC2, Q, C, D))
                        { // done with this curve. 
                        P = P2;                   
                        if (i == -2) return false;
                        loadstart = true;
                        return true;
                        }
                    P = Q;
                    P1 = Q;
                    PC1 = C;
                    PC2 = D;
                    return true;
                    },
                thickness, end_Pn, end_P0, color, opacity);

            }
            }
        }



    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::drawThickClosedSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color, float opacity)
        {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
            {
        case 0:
        case 1:
            return;
        case 2:
            {
            drawThickLineAA(tabPoints[0], tabPoints[1], thickness, END_ROUNDED, END_ROUNDED, color, opacity);
            return;
            }
        default:
           {
            float x[SPLINE_MAX_POINTS];
            float y[SPLINE_MAX_POINTS];
            for (int i = 0; i < nbpoints; i++)
                {
                x[i] = tabPoints[i].x;
                y[i] = tabPoints[i].y;
                }
            const int n = nbpoints;
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
            x[0] = xx;
            y[0] = yy;
            for (int i = 1; i < n; i++)
                {
                xx = a * (x[i] - xx);
                yy = a * (y[i] - yy);
                x[i] = xx;
                y[i] = yy;
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
            x[n - 1] = xx;
            y[n - 1] = yy;
            for (int i = n - 2; i >= 0; i--)
                {
                xx = x[i] - a * xx;
                yy = y[i] - a * yy;
                x[i] = xx;
                y[i] = yy;
                }

            int i = 0; 
            bool loadstart = true; 
            bool begin = true; 
            fVec2 P1, P2, PC; 
            drawThickPolygonAA(
                [&](tgx::fVec2& P)
                    {
                    while (1)
                        {
                        if (loadstart)
                            { // we are at the beginning of a new Bezier segment, load P1, P2, PC
                            if (i == 0)
                                {
                                P1 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                P2 = { (x[0] + x[1]) / 2, (y[0] + y[1]) / 2 };
                                PC = { x[0], y[0] };
                                }
                            else if (i < n - 1)
                                {
                                P1 = { (x[i - 1] + x[i]) / 2, (y[i - 1] + y[i]) / 2 };
                                P2 = { (x[i] + x[i + 1]) / 2, (y[i] + y[i + 1]) / 2 };
                                PC = { x[i], y[i] };
                                }
                            else
                                { // last segment
                                P1 = { (x[n - 2] + x[n - 1]) / 2, (y[n - 2] + y[n - 1]) / 2 };
                                P2 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                PC = { x[n - 1], y[n - 1] };
                                }
                            i++;
                            loadstart = false;
                            }
                        if (begin)
                            {
                            begin = false;
                            continue; // skip the first point P1 as this will be the last one !
                            }
                        // here, we are on curve P1, P2, PC and P1 has already been plotted.                     
                        fVec2 Q, PB;
                        float wb;
                        if (_splitRationalQuadBezier(P1, P2, PC, 1.0f, Q, PB, wb))
                            { // done with this curve. 
                            P = P2;
                            if (i == n)
                                { // reset because we need to go over the list of points twice
                                i = 0;
                                loadstart = true;
                                begin = true;                                
                                return false;
                                }
                            loadstart = true;
                            return true;
                            }
                        P = Q;
                        P1 = Q;
                        PC = PB;
                        return true;
                        }
                    },
                thickness, color, opacity);
            }
            }
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::fillClosedSplineAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity)
        {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
            {
        case 0:
        case 1:
        case 2:         
            return;
        default:
           {
            float x[SPLINE_MAX_POINTS];
            float y[SPLINE_MAX_POINTS];
            for (int i = 0; i < nbpoints; i++)
                {
                x[i] = tabPoints[i].x;
                y[i] = tabPoints[i].y;
                }
            const int n = nbpoints;
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
            x[0] = xx;
            y[0] = yy;
            for (int i = 1; i < n; i++)
                {
                xx = a * (x[i] - xx);
                yy = a * (y[i] - yy);
                x[i] = xx;
                y[i] = yy;
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
            x[n - 1] = xx;
            y[n - 1] = yy;
            for (int i = n - 2; i >= 0; i--)
                {
                xx = x[i] - a * xx;
                yy = y[i] - a * yy;
                x[i] = xx;
                y[i] = yy;
                }

            int i = 0; 
            bool loadstart = true; 
            bool begin = true; 
            fVec2 P1, P2, PC; 
            fillPolygonAA(
                [&](tgx::fVec2& P)
                    {
                    while (1)
                        {
                        if (loadstart)
                            { // we are at the beginning of a new Bezier segment, load P1, P2, PC
                            if (i == 0)
                                {
                                P1 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                P2 = { (x[0] + x[1]) / 2, (y[0] + y[1]) / 2 };
                                PC = { x[0], y[0] };
                                }
                            else if (i < n - 1)
                                {
                                P1 = { (x[i - 1] + x[i]) / 2, (y[i - 1] + y[i]) / 2 };
                                P2 = { (x[i] + x[i + 1]) / 2, (y[i] + y[i + 1]) / 2 };
                                PC = { x[i], y[i] };
                                }
                            else
                                { // last segment
                                P1 = { (x[n - 2] + x[n - 1]) / 2, (y[n - 2] + y[n - 1]) / 2 };
                                P2 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                PC = { x[n - 1], y[n - 1] };
                                }
                            i++;
                            loadstart = false;
                            }
                        if (begin)
                            {
                            begin = false;
                            continue; // skip the first point P1 as this will be the last one !
                            }
                        // here, we are on curve P1, P2, PC and P1 has already been plotted.                     
                        fVec2 Q, PB;
                        float wb;
                        if (_splitRationalQuadBezier(P1, P2, PC, 1.0f, Q, PB, wb))
                            { // done with this curve. 
                            P = P2;
                            if (i == n)
                                { // reset because we need to go over the list of points twice
                                i = 0;
                                loadstart = true;
                                begin = true;
                                return false;
                                }
                            loadstart = true;
                            return true;
                            }
                        P = Q;
                        P1 = Q;
                        PC = PB;
                        return true;
                        }
                    },
                color, opacity);
            }
            }
        }


    template<typename color_t>
    template<int SPLINE_MAX_POINTS>
    void Image<color_t>::fillThickClosedSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color_interior, color_t color_border, float opacity)
        {
        if (!isValid()) return;
        if (nbpoints > SPLINE_MAX_POINTS) nbpoints = SPLINE_MAX_POINTS;
        switch (nbpoints)
            {
        case 0:
        case 1:
        case 2:         
            return;

        default:
           {
            float x[SPLINE_MAX_POINTS];
            float y[SPLINE_MAX_POINTS];
            for (int i = 0; i < nbpoints; i++)
                {
                x[i] = tabPoints[i].x;
                y[i] = tabPoints[i].y;
                }
            const int n = nbpoints;
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
            x[0] = xx;
            y[0] = yy;
            for (int i = 1; i < n; i++)
                {
                xx = a * (x[i] - xx);
                yy = a * (y[i] - yy);
                x[i] = xx;
                y[i] = yy;
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
            x[n - 1] = xx;
            y[n - 1] = yy;
            for (int i = n - 2; i >= 0; i--)
                {
                xx = x[i] - a * xx;
                yy = y[i] - a * yy;
                x[i] = xx;
                y[i] = yy;
                }

            int i = 0; 
            bool loadstart = true; 
            bool begin = true; 
            fVec2 P1, P2, PC; 
            fillThickPolygonAA(
                [&](tgx::fVec2& P)
                    {
                    while (1)
                        {
                        if (loadstart)
                            { // we are at the beginning of a new Bezier segment, load P1, P2, PC
                            if (i == 0)
                                {
                                P1 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                P2 = { (x[0] + x[1]) / 2, (y[0] + y[1]) / 2 };
                                PC = { x[0], y[0] };
                                }
                            else if (i < n - 1)
                                {
                                P1 = { (x[i - 1] + x[i]) / 2, (y[i - 1] + y[i]) / 2 };
                                P2 = { (x[i] + x[i + 1]) / 2, (y[i] + y[i + 1]) / 2 };
                                PC = { x[i], y[i] };
                                }
                            else
                                { // last segment
                                P1 = { (x[n - 2] + x[n - 1]) / 2, (y[n - 2] + y[n - 1]) / 2 };
                                P2 = { (x[n - 1] + x[0]) / 2, (y[n - 1] + y[0]) / 2 };
                                PC = { x[n - 1], y[n - 1] };
                                }
                            i++;
                            loadstart = false;
                            }
                        if (begin)
                            {
                            begin = false;
                            continue; // skip the first point P1 as this will be the last one !
                            }
                        // here, we are on curve P1, P2, PC and P1 has already been plotted.                     
                        fVec2 Q, PB;
                        float wb;
                        if (_splitRationalQuadBezier(P1, P2, PC, 1.0f, Q, PB, wb))
                            { // done with this curve. 
                            P = P2;
                            if (i == n)
                                { // reset because we need to go over the list of points twice
                                i = 0;
                                loadstart = true;
                                begin = true;
                                return false;
                                }
                            loadstart = true;
                            return true;
                            }
                        P = Q;
                        P1 = Q;
                        PC = PB;
                        return true;
                        }
                    },
                thickness, color_interior, color_border, opacity);
            }
            }
        }



    


    
    
    











    /************************************************************************************
    * 
    *  Drawing Text
    * 
    *************************************************************************************/




    /** used for clipping a font bitmap */
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
    int Image<color_t>::fontHeight(const GFXfont& font) const
        {
        return tgx::fontHeight(font);
        }



    template<typename color_t>
    int Image<color_t>::fontHeight(const ILI9341_t3_font_t& font) const
        {
        return tgx::fontHeight(font);
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const GFXfont& font, Anchor anchor, int* xadvance) const
        {
        return tgx::measureChar(c, pos, font, anchor, xadvance);
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const ILI9341_t3_font_t & font, Anchor anchor, int* xadvance) const
        {
        return tgx::measureChar(c, pos, font, anchor, xadvance);
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureText(const char * text, iVec2 pos, const GFXfont& font, Anchor anchor, bool wrap_text, bool start_newline_at_0) const
        {
        const iVec2 startp = pos;
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const int hh = fontHeight(font);
        iBox2 B;
        B.empty();
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += hh;
                }
            else
                {
                int xa = 0;
                auto U = measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR, &xa);
                if ((wrap_text) && (pos.x + xa >= _lx))
                    {
                    auto pos2 = pos; 
                    pos.x = startx;
                    pos.y += hh;
                    U += (pos - pos2);
                    }
                B |= U;
                pos.x += xa;
                }
            }
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            iVec2 pos2 = B.getAnchor(anchor);
            if (anchor & BASELINE) pos2.x = startp.x;
            B += (startp - pos2);
            }
        return B;
        }


    template<typename color_t>
    iBox2 Image<color_t>::measureText(const char * text, iVec2 pos, const ILI9341_t3_font_t& font, Anchor anchor, bool wrap_text, bool start_newline_at_0) const
        {
        const iVec2 startp = pos;
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const int hh = fontHeight(font);
        iBox2 B;
        B.empty();
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += hh;
                }
            else
                {
                int xa = 0;
                auto U = measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR, &xa);
                if ((wrap_text)&& (pos.x + xa >= _lx))
                    {
                    auto pos2 = pos; 
                    pos.x = startx;
                    pos.y += hh;
                    U += (pos - pos2);
                    }
                B |= U;
                pos.x += xa;
                }
            }
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            iVec2 pos2 = B.getAnchor(anchor);
            if (anchor & BASELINE) pos2.y = startp.y;
            B += (startp - pos2);
            }
        return B;
        }



    template<typename color_t>    
    iVec2 Image<color_t>::drawChar(char c, iVec2 pos, const GFXfont& font, color_t color, float opacity)
        {
        if (!isValid()) return pos; 
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        /*
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            auto B = measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR);
            iVec2 pos2 = B.getAnchor(anchor);
            pos += pos - pos2;
            }
        */
        return _drawCharGFX<true>(c, pos, color, font, opacity);
        }

    template<typename color_t>
    iVec2 Image<color_t>::drawChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, color_t color, float opacity)
        {
        if (!isValid()) return pos; 
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        /*
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            auto B = measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR);
            iVec2 pos2 = B.getAnchor(anchor);
            pos += pos - pos2;
            }
        */
        return _drawCharILI<false>(c, pos, color, font, opacity);
        }


    template<typename color_t>
    iVec2 Image<color_t>::drawText(const char* text, iVec2 pos, const GFXfont& font, color_t color, float opacity)
        {
        return drawTextEx(text, pos, DEFAULT_TEXT_ANCHOR, font, false, false, color, opacity);
        }


    template<typename color_t>
    iVec2 Image<color_t>::drawText(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, color_t color, float opacity)
        {
        return drawTextEx(text, pos, DEFAULT_TEXT_ANCHOR, font, false, false, color, opacity);
        }


    template<typename color_t>
    iVec2 Image<color_t>::drawTextEx(const char* text, iVec2 pos, Anchor anchor, const GFXfont& font, bool wrap_text, bool start_newline_at_0, color_t color, float opacity)
        {
        if (!isValid()) return pos;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            start_newline_at_0 = false;
            if (!(anchor & LEFT)) wrap_text = false; // cannot perform wrapping if we need to move. 
            auto B = measureText(text, pos, font, DEFAULT_TEXT_ANCHOR, wrap_text, start_newline_at_0);
            iVec2 pos2 = B.getAnchor(anchor);
            pos += pos - pos2;
            }
        return _drawTextGFX<true>(text, pos, font, color, opacity, wrap_text, start_newline_at_0);
        }


    template<typename color_t>
    iVec2 Image<color_t>::drawTextEx(const char* text, iVec2 pos, Anchor anchor, const ILI9341_t3_font_t& font, bool wrap_text, bool start_newline_at_0, color_t color, float opacity)
        {
        if (!isValid()) return pos;
        if ((opacity < 0) || (opacity > 1)) opacity = 1.0f;
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            start_newline_at_0 = false;
            if (!(anchor & LEFT)) wrap_text = false; // cannot perform wrapping if we need to move. 
            auto B = measureText(text, pos, font, DEFAULT_TEXT_ANCHOR, wrap_text,start_newline_at_0);
            iVec2 pos2 = B.getAnchor(anchor);
            pos += pos - pos2;
            }
        return _drawTextILI<true>(text, pos, font, color, opacity, wrap_text, start_newline_at_0);
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
        uint8_t * data = (uint8_t *)font.data + tgx_internals::fetchbits_unsigned(font.index, (n*font.bits_index), font.bits_index);
        int32_t off = 0; 
        uint32_t encoding = tgx_internals::fetchbits_unsigned(data, off, 3);
        if (encoding != 0) return  pos; // wrong/unsupported format
        off += 3;
        int sx = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_width);
        off += font.bits_width;         
        int sy = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_height);
        off += font.bits_height;            
        const int xoffset = (int)tgx_internals::fetchbits_signed(data, off, font.bits_xoffset);
        off += font.bits_xoffset;
        const int yoffset = (int)tgx_internals::fetchbits_signed(data, off, font.bits_yoffset);
        off += font.bits_yoffset;
        const int delta = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_delta);
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
    template<bool BLEND>
    iVec2 Image<color_t>::_drawTextGFX(const char* text, iVec2 pos, const GFXfont& font, color_t col, float opacity,  bool wrap, bool start_newline_at_0)
        {
        const int hh = fontHeight(font);
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += hh;
                }
            else
                {
                if (wrap)
                    {
                    int xa = 0;
                    measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR, &xa);
                    if ((wrap) && (pos.x + xa >= _lx))
                        {
                        pos.x = startx;
                        pos.y += hh;
                        }
                    }
                pos = _drawCharGFX<BLEND>(c, pos, col, font, opacity);
                }
            }
        return pos;
        }


    template<typename color_t>
    template<bool BLEND>
    iVec2 Image<color_t>::_drawTextILI(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, color_t col, float opacity,  bool wrap, bool start_newline_at_0)
        {
        const int hh = fontHeight(font);
        const int startx = start_newline_at_0 ? 0 : pos.x;
        const size_t l = strlen(text);
        for (size_t i = 0; i < l; i++)
            {
            const char c = text[i];
            if (c == '\n')
                {
                pos.x = startx;
                pos.y += hh;
                }
            else
                {
                if (wrap)
                    {
                    int xa = 0;
                    measureChar(c, pos, font, DEFAULT_TEXT_ANCHOR, &xa);
                    if ((wrap) && (pos.x + xa >= _lx))
                        {
                        pos.x = startx;
                        pos.y += hh;
                        }
                    }
                pos = _drawCharILI<BLEND>(c, pos, col, font, opacity);
                }
            }
        return pos;
        }





    /**
     * draw a character from an ILI9341_t3 font bitmap (version 1, with line compression).
     * The ILI9341_t3 font format is described here: https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format
     **/
    template<typename color_t>
    template<bool BLEND>
    void Image<color_t>::_drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
        {
        uint32_t rl = 0; // number of line repeat remaining. 
        while (b_up > 0)
            { // need to skip lines
            if (tgx_internals::fetchbit(bitmap, off++))
                { // this is a repeating line
                int n = (int)tgx_internals::fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
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
                if (tgx_internals::fetchbit(bitmap, off++))
                    { // repeating line
                    rl = tgx_internals::fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
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


    /** used by _drawCharILI9341_t3 to draw a single row of a font bitmap */
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


    /**
     * draw a 1 bit per pixel char bitmap on the image
     * Use to draw char from Adafruit font format: https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-tgx-font-format/
     **/
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


    /** draw a 2 bit per pixel char bitmap on the image 
    *  packed bdf format 23 : https://github.com/projectitis/packedbdf/blob/master/packedbdf.md
    **/
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


    /** draw a 4 bit per pixel char bitmap on the image */
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


    /** draw a 8 bit per pixel char bitmap on the image */
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


