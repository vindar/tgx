/** @file Color.cpp */
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



#include "Color.h"

namespace tgx
{

/** definitions of predefined colors in RGB32 format */
const RGB32 RGB32_Black(0, 0, 0);
const RGB32 RGB32_White(255, 255, 255);
const RGB32 RGB32_Red(255, 0, 0);
const RGB32 RGB32_Blue(0, 0, 255);
const RGB32 RGB32_Green(0, 255, 0);
const RGB32 RGB32_Purple(128, 0, 128);
const RGB32 RGB32_Orange(255, 135, 0);
const RGB32 RGB32_Cyan(0, 255, 255);
const RGB32 RGB32_Lime(0, 255, 0);
const RGB32 RGB32_Salmon(250, 128, 114);
const RGB32 RGB32_Maroon(128, 0, 0);
const RGB32 RGB32_Yellow(255, 255, 0);
const RGB32 RGB32_Magenta(255, 0, 255);
const RGB32 RGB32_Olive(128, 128, 0);
const RGB32 RGB32_Teal(0, 128, 128);
const RGB32 RGB32_Gray(128, 128, 128);
const RGB32 RGB32_Silver(192, 192, 192);
const RGB32 RGB32_Navy(0, 0, 128);
const RGB32 RGB32_Transparent(0, 0, 0, 0); // transparent for pre-multiplied alpha


/** definitions of predefined colors in RGB565 format */
const RGB565 RGB565_Black(RGB32_Black);
const RGB565 RGB565_White(RGB32_White);
const RGB565 RGB565_Red(RGB32_Red);
const RGB565 RGB565_Blue(RGB32_Blue);
const RGB565 RGB565_Green(RGB32_Green);
const RGB565 RGB565_Purple(RGB32_Purple);
const RGB565 RGB565_Orange(RGB32_Orange);
const RGB565 RGB565_Cyan(RGB32_Cyan);
const RGB565 RGB565_Lime(RGB32_Lime);
const RGB565 RGB565_Salmon(RGB32_Salmon);
const RGB565 RGB565_Maroon(RGB32_Maroon);
const RGB565 RGB565_Yellow(RGB32_Yellow);
const RGB565 RGB565_Magenta(RGB32_Magenta);
const RGB565 RGB565_Olive(RGB32_Olive);
const RGB565 RGB565_Teal(RGB32_Teal);
const RGB565 RGB565_Gray(RGB32_Gray);
const RGB565 RGB565_Silver(RGB32_Silver);
const RGB565 RGB565_Navy(RGB32_Navy);



RGB565::RGB565(const HSV& hsv)
    {
    *this = (RGB565)(RGBf(hsv));
    }


RGB565& RGB565::operator=(const HSV& hsv)
    {
    *this = (RGB565)(RGBf(hsv));
    return *this;
    }


RGB24::RGB24(const HSV& hsv)
    {
    *this = (RGB24)(RGBf(hsv));
    }


RGB24& RGB24::operator=(const HSV& hsv)
    {
    *this = (RGB24)(RGBf(hsv));
    return *this;
    }


RGB32::RGB32(const HSV& hsv)
    {
    *this = (RGB32)(RGBf(hsv));
    }


RGB32 & RGB32::operator=(const HSV& hsv)
    {
    *this = (RGB32)(RGBf(hsv));
    return *this;
    }


RGB64::RGB64(const HSV& hsv)
    {
    *this = (RGB64)(RGBf(hsv));
    }


RGB64 & RGB64::operator=(const HSV& hsv)
    {
    *this = (RGB64)(RGBf(hsv));
    return *this;
    }


RGBf::RGBf(const HSV& hsv)
    {
    // conversion from HSV to RGBf. 
    // All other conversions from HSV to some RGB format 
    // fall back to this code.
    //
    // adapted from: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both    
    if (hsv.S <= 0.0f)
        {
        R = G = B = hsv.V;
        return;
        }
    const float hh = ((hsv.H >= 1.0f) ? 0 : hsv.H) * 6;
    const int i = (int)hh;
    const float ff = hh - i;
    const float p = hsv.V * (1 - hsv.S);
    const float q = hsv.V * (1 - (hsv.S * ff));
    const float t = hsv.V * (1 - (hsv.S * (1 - ff)));
    switch (i)
        {
    case 0:
        R = hsv.V;
        G = t;
        B = p;
        break;
    case 1:
        R = q;
        G = hsv.V;
        B = p;
        break;
    case 2:
        R = p;
        G = hsv.V;
        B = t;
        break;
    case 3:
        R = p;
        G = q;
        B = hsv.V;
        break;
    case 4:
        R = t;
        G = p;
        B = hsv.V;
        break;
    case 5:
    default:
        R = hsv.V;
        G = p;
        B = q;
        break;
        }
    }


RGBf& RGBf::operator=(const HSV& hsv)
    {
    *this = RGBf(hsv);
    return *this;
    }




HSV::HSV(uint16_t val) : HSV(RGB565(val))
    {
    }


HSV::HSV(uint32_t val) : HSV(RGB32(val))
    {
    }


HSV::HSV(uint64_t val) : HSV(RGB64(val))
    {
    }


HSV::HSV(const RGB565& c)
    {
    *this = HSV(RGBf(c));
    }


HSV::HSV(const RGB24& c)
    {
    *this = HSV(RGBf(c));
    }


HSV::HSV(const RGB32& c)
    {
    *this = HSV(RGBf(c));
    }


HSV::HSV(const RGB64& c)
    {
    *this = HSV(RGBf(c));
    }


HSV::HSV(const RGBf & c) : H(0), S(0), V(0)
    {
    // conversion from RGB64 to HSV. 
    // All other conversions from some RGB format to HSV 
    // fall back to this code. 
    //
    // adapted from: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
    const float r = c.R;
    const float g = c.G;
    const float b = c.B;
    const float min = ((r < g) ? ((b < r) ? b : r) : ((b < g) ? b : g));
    const float max = ((r > g) ? ((b > r) ? b : r) : ((b > g) ? b : g));
    V = max;
    float delta = max - min;
    if (delta < 0.001f) { S = 0; H = 0; return; }
    if (max > 0)
        {
        S = (delta / max);
        }
    else
        {
        S = 0;
        H = 0;
        return;
        }
    if (r >= max)
        H = (g - b) / delta;  // between yellow & magenta
    else
        if (g >= max) H = 2 + (b - r) / delta;  // between cyan & yellow
        else H = 4 + (r - g) / delta;  // between magenta & cyan

    H /= 6.0f;              // [0,1[ range (for degree,use H *= 60.0 instead);  
    if (H < 0.0f) H += 1;   // [0,1][ range  (for degree,use H += 360.0 instead);
    if (H >= 1.0f) H = 0.0f;
    }



HSV& HSV::operator=(const RGB565& c)
    {
    *this = HSV(RGBf(c));
    return *this;
    }


HSV& HSV::operator=(const RGB24 & c)
    {
    *this = HSV(RGBf(c));
    return *this;
    }


HSV& HSV::operator=(const RGB32 & c)
    {
    *this = HSV(RGBf(c));
    return *this;
    }


HSV& HSV::operator=(const RGB64 & c)
    {
    *this = HSV(RGBf(c));
    return *this;
    }


HSV& HSV::operator=(const RGBf & c)
    {
    *this = HSV(c);
    return *this;
    }




}

/* end of file */

