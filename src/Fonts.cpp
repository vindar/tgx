/** @file Fonts.cpp */
//
// Copyright 2021 Arvind Singh
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


#include "Fonts.h"


namespace tgx
{


    int fontHeight(const GFXfont& font)
        {
        return font.yAdvance;
        }


    int fontHeight(const ILI9341_t3_font_t& font)
        {
        return font.line_space;
        }


    iBox2 measureChar(char c, iVec2 pos, const GFXfont& font, Anchor anchor, int* xadvance)
        {
        const iVec2 startp = pos;
        uint8_t n = (uint8_t)c;
        if ((n < font.first) || (n > font.last)) return iBox2(pos.x, pos.x, pos.y, pos.y); // nothing to draw. 
        auto& g = font.glyph[n - font.first];
        int x = pos.x + g.xOffset;
        int y = pos.y + g.yOffset;
        int sx = g.width;
        int sy = g.height;
        if (xadvance) *xadvance = g.xAdvance;
        iBox2 B = iBox2(x, x + sx - 1, y, y + sy - 1);        
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            iVec2 pos2 = B.getAnchor(anchor);
            if (anchor & BASELINE) pos2.x = startp.x;
            B += (startp - pos2);
            }
        return B; 
        }


    iBox2 measureChar(char c, iVec2 pos, const ILI9341_t3_font_t & font, Anchor anchor, int* xadvance)
        {
        const iVec2 startp = pos;
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
        uint8_t* data = (uint8_t*)font.data + tgx_internals::fetchbits_unsigned(font.index, (n * font.bits_index), font.bits_index);
        int32_t off = 0;
        uint32_t encoding = tgx_internals::fetchbits_unsigned(data, off, 3);
        if (encoding != 0) return  pos; // wrong/unsupported format
        off += 3;
        const int sx = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_width);
        off += font.bits_width;
        const int sy = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_height);
        off += font.bits_height;
        const int xoffset = (int)tgx_internals::fetchbits_signed(data, off, font.bits_xoffset);
        off += font.bits_xoffset;
        const int yoffset = (int)tgx_internals::fetchbits_signed(data, off, font.bits_yoffset);
        off += font.bits_yoffset;
        if (xadvance)
            {
            *xadvance = (int)tgx_internals::fetchbits_unsigned(data, off, font.bits_delta);
            }
        const int x = pos.x + xoffset;
        const int y = pos.y - sy - yoffset;
        iBox2 B(x, x + sx - 1, y, y + sy - 1);
        if (anchor != DEFAULT_TEXT_ANCHOR)
            {
            iVec2 pos2 = B.getAnchor(anchor);
            if (anchor & BASELINE) pos2.x = startp.x;
            B += (startp - pos2);
            }
        return B;
        }





    namespace tgx_internals
    {


        uint32_t fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required)
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


        uint32_t fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required)
            {
            uint32_t val = fetchbits_unsigned(p, index, required);
            if (val & (1 << (required - 1))) 
                {
                return (int32_t)val - (1 << required);
                }
            return (int32_t)val;                
            }

    }

}


/* end of file */

