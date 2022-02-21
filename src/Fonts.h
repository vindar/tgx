/** @file Fonts.h */
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

#include "Misc.h"

/****************************************************************************
* FONT FORMATS
* 
* !!! The Adafruit GFX and ILI9341_t3 libraries should be included before 
*     this header if youwant ot use these libraries !!!
****************************************************************************/



/**
* ADAFRUIT FONT
* Documentation on the Adafruit font format:
* https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format/
**/

//Should we use the code below to force include whenever possible? probably not...
//#ifdef __cplusplus
//#if __has_include(<gfxfont.h>)
//#include <gfxfont.h>
//#endif
//#endif

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

#include <stdint.h>


// Font data stored PER GLYPH
typedef struct {
    uint16_t bitmapOffset; // Pointer into GFXfont->bitmap
    uint8_t width;         // Bitmap dimensions in pixels
    uint8_t height;        // Bitmap dimensions in pixels
    uint8_t xAdvance;      // Distance to advance cursor (x axis)
    int8_t xOffset;        // X dist from cursor pos to UL corner
    int8_t yOffset;        // Y dist from cursor pos to UL corner
} GFXglyph;


// Data stored for FONT AS A WHOLE
typedef struct {
    uint8_t* bitmap;  // Glyph bitmaps, concatenated
    GFXglyph* glyph;  // Glyph array
    uint8_t first;    // ASCII extents (first char)
    uint8_t last;     // ASCII extents (last char)
    uint8_t yAdvance; // Newline distance (y axis)
} GFXfont;

#endif 



/**
* ILI9341_t3 PJRC FONT FORMAT
* Documentation on the ILI9341_t3 font data format:
* https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format (basic version)
* https://github.com/projectitis/packedbdf/blob/master/packedbdf.md (anti-aliased extension)
**/

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
typedef struct {
    const unsigned char* index;
    const unsigned char* unicode;
    const unsigned char* data;
    unsigned char version;
    unsigned char reserved;
    unsigned char index1_first;
    unsigned char index1_last;
    unsigned char index2_first;
    unsigned char index2_last;
    unsigned char bits_index;
    unsigned char bits_width;
    unsigned char bits_height;
    unsigned char bits_xoffset;
    unsigned char bits_yoffset;
    unsigned char bits_delta;
    unsigned char line_space;
    unsigned char cap_height;
} ILI9341_t3_font_t;

#endif


typedef ILI9341_t3_font_t packedbdf_t;


#endif 

/** end of file */

