/**
 * @file Fonts.h
 * Define the GFXFont anf ILI9341_t3 font format if needed. 
 * 
 * @warning If used, the Adafruit and GFX library should be included before including this header !
 */
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


#ifndef _TGX_FONTS_H_
#define _TGX_FONTS_H_

#include <stdint.h>
#include "Misc.h"
#include "Vec2.h"
#include "Box2.h"


//****************************************************************************
//****************************************************************************
//****************************************************************************
// FONT FORMATS
//
// !!! The Adafruit GFX and ILI9341_t3 libraries should be included before 
//     this header if you want ot use these libraries !!!
//****************************************************************************
//****************************************************************************
//****************************************************************************



//Should we use the code below to force include whenever possible? probably not...
//#ifdef __cplusplus
//#if __has_include(<gfxfont.h>)
//#include <gfxfont.h>
//#endif
//#endif

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

// Font data stored PER GLYPH
typedef struct {
    uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
    uint8_t width;         ///< Bitmap dimensions in pixels
    uint8_t height;        ///< Bitmap dimensions in pixels
    uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
    int8_t xOffset;        ///< X dist from cursor pos to UL corner
    int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;


/**
 * GFXFont font format
 *
 * Used by Adafruit GFX library. 
 * 
 * See https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format for details.
 */
typedef struct {
    uint8_t* bitmap;  ///< Glyph bitmaps, concatenated
    GFXglyph* glyph;  ///< Glyph array
    uint8_t first;    ///< ASCII extents (first char)
    uint8_t last;     ///< ASCII extents (last char)
    uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

#endif 




//Should we use the code below to force include whenever possible? probably not...
//#ifdef __cplusplus
//#if __has_include(<ILI9341_t3.h>)
//#include <ILI9341_t3.h>
//#endif
//#endif
//#ifndef _ILI9341_t3H_
//#define _ILI9341_t3H_

#ifndef _ILI9341_FONTS_H_
#define _ILI9341_FONTS_H_

/**
 * ILI9341_t3 PJRC font format
 * 
 * - See https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format for the original format. 
 * - See https://github.com/projectitis/packedbdf/blob/master/packedbdf.md for anti-aliased v23 version 
 */
typedef struct {
    const unsigned char* index;     ///< ...
    const unsigned char* unicode;   ///< ...
    const unsigned char* data;      ///< ...
    unsigned char version;          ///< ...
    unsigned char reserved;         ///< ...
    unsigned char index1_first;     ///< ...
    unsigned char index1_last;      ///< ...
    unsigned char index2_first;     ///< ...
    unsigned char index2_last;      ///< ...
    unsigned char bits_index;       ///< ...
    unsigned char bits_width;       ///< ...
    unsigned char bits_height;      ///< ...
    unsigned char bits_xoffset;     ///< ...
    unsigned char bits_yoffset;     ///< ...
    unsigned char bits_delta;       ///< ...
    unsigned char line_space;       ///< ...
    unsigned char cap_height;       ///< ...
} ILI9341_t3_font_t;

#endif

// add alias to make it compatible with projectitis's format...
typedef ILI9341_t3_font_t packedbdf_t; 



namespace tgx
    {


    /**
     * Query the height of a font.
     *
     * overload for GFXfont.
     *    
     * @param   font    The font.
     *
     * @returns The number of vertical pixels between two lines of text with this font.
     */
    int fontHeight(const GFXfont& font);


    /**
     * Query the height of a font.
     *
     * overload for ILI9341_t3_font_t.
     *    
     * @param   font    The font.
     *
     * @returns The number of vertical pixels between two lines of text with this font.
     */
    int fontHeight(const ILI9341_t3_font_t& font);


    /**
     * Compute the bounding box of a character.
     *  
     * overload for GFXfont.
     * 
     * @param           c           The character.
     * @param           pos         position of the anchor point.
     * @param           font        The font to use.
     * @param           anchor      location of the anchor with respect to the char bounding box. (by default, this is the BASELINE|LEFT).
     * @param [in,out]  xadvance    If non-null, the number of pixel to advance horizontally after drawing the char is stored here.
     *
     * @returns the bounding box of pixels occupied by char `c` when draw with `font` when its chosen `anchor` is at `pos`.
     */
    iBox2 measureChar(char c, iVec2 pos, const GFXfont& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, int* xadvance = nullptr);


    /**
     * Compute the bounding box of a character.
     *  
     * overload for ILI9341_t3_font_t.
     * 
     * @param           c           The character.
     * @param           pos         position of the anchor point.
     * @param           font        The font to use.
     * @param           anchor      location of the anchor with respect to the char bounding box. (by default, this is the BASELINE|LEFT).
     * @param [in,out]  xadvance    If non-null, the number of pixel to advance horizontally after drawing the char is stored here.
     *
     * @returns the bounding box of pixels occupied by char `c` when draw with `font` when its chosen `anchor` is at `pos`.
     */
    iBox2 measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, int* xadvance = nullptr);





    namespace tgx_internals
        {

        /** fetch a single bit from a bit array. Used to decode ILI9341_t3 format */
        inline uint32_t fetchbit(const uint8_t* p, uint32_t index) { return (p[index >> 3] & (0x80 >> (index & 7))); } //fetch a single bit from a bit array. (from ili9341_t3.cpp)        

        /** fetch 'required' bits from a bit array, returned as an unsigned integer. Used to decode ILI9341_t3 format */
        uint32_t fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required);

        /** fetch 'required' bits from a bit array, returned as an signed integer. Used to decode ILI9341_t3 format */
        uint32_t fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required);

        }

    }




#endif 

/** end of file */

