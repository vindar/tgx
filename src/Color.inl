/** @file Color.inl */
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
#ifndef _TGX_COLOR_INL_
#define _TGX_COLOR_INL_


/************************************************************************************
*
* 
* Implementation file for the color classes defined in Color.h
* 
* 
*************************************************************************************/
namespace tgx
{


/**********************************************************************
* implementation of inline method for RGB565
***********************************************************************/


constexpr inline RGB565::RGB565(const RGB24& c) :
        #if TGX_RGB565_ORDER_BGR
        B(c.B >> 3), G(c.G >> 2), R(c.R >> 3)
        #else
        R(c.R >> 3), G(c.G >> 2), B(c.B >> 3)
        #endif
        {
        }


constexpr inline RGB565::RGB565(const RGB32& c) : 
        #if TGX_RGB565_ORDER_BGR
        B(c.B >> 3), G(c.G >> 2), R(c.R >> 3)
        #else
        R(c.R >> 3), G(c.G >> 2), B(c.B >> 3)
        #endif
        {
        }


constexpr inline RGB565::RGB565(const RGB64& c) : 
        #if TGX_RGB565_ORDER_BGR
        B(c.B >> 11), G(c.G >> 10), R(c.R >> 11)
        #else
        R(c.R >> 11), G(c.G >> 10), B(c.B >> 11)
        #endif
        {
        }


constexpr inline RGB565::RGB565(const RGBf& c) :
    #if TGX_RGB565_ORDER_BGR
        B((uint16_t)(c.B * 31)), G((uint16_t)(c.G *63)), R((uint16_t)(c.R * 31))
    #else
        R((uint16_t)(c.R * 31)), G((uint16_t)(c.G * 63)), B((uint16_t)(c.B * 31))
    #endif
        {
        }


constexpr inline RGB565::RGB565(uint32_t val) : RGB565(RGB32(val))
        {
        }


constexpr inline RGB565::RGB565(uint64_t val) : RGB565(RGB64(val))
        {
        }



inline RGB565& RGB565::operator=(const RGB24& c)
        {
        B = (c.B >> 3);
        G = (c.G >> 2);
        R = (c.R >> 3);
        return *this;
        }


inline RGB565& RGB565::operator=(const RGB32& c)
        {
        B = (c.B >> 3);
        G = (c.G >> 2);
        R = (c.R >> 3);
        return *this;
        }


inline RGB565& RGB565::operator=(const RGB64& c)
        {
        B = (c.B >> 11);
        G = (c.G >> 10);
        R = (c.R >> 11);
        return *this;
        }


inline RGB565& RGB565::operator=(const RGBf& c)
        {
        B = (uint8_t)(c.B * 31);
        G = (uint8_t)(c.G * 63);
        R = (uint8_t)(c.R * 31);
        return *this;
        }


inline RGB565& RGB565::operator=(iVec3 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        return *this;
        }


inline RGB565& RGB565::operator=(iVec4 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        return *this;
        }


inline RGB565& RGB565::operator=(fVec3 v)
        {
        B = (uint8_t)(v.x * 31);
        G = (uint8_t)(v.y * 63);
        R = (uint8_t)(v.z * 31);
        return *this;
        }


inline RGB565& RGB565::operator=(fVec4 v)
        {
        B = (uint8_t)(v.x * 31);
        G = (uint8_t)(v.y * 63);
        R = (uint8_t)(v.z * 31);
        return *this;
        }




/**********************************************************************
* implementation of inline method for RGB24
***********************************************************************/


constexpr inline RGB24::RGB24(uint16_t val) : RGB24(RGB565(val)) 
        {
        }


constexpr inline RGB24::RGB24(uint32_t val) : RGB24(RGB32(val)) 
        {
        }


constexpr inline RGB24::RGB24(uint64_t val) : RGB24(RGB64(val)) 
        {
        }


constexpr inline RGB24::RGB24(const RGB565& c) :
        #if TGX_RGB24_ORDER_BGR
        B((((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2)), G((((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4)), R((((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2))
        #else
        R((((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2)), G((((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4)), B((((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2))
        #endif
        {
        }


constexpr inline RGB24::RGB24(const RGB32& c) :
        #if TGX_RGB24_ORDER_BGR
        B(c.B), G(c.G), R(c.R)
        #else
        R(c.R), G(c.G), B(c.B)
        #endif
        {
        }


constexpr inline RGB24::RGB24(const RGB64& c) : 
        #if TGX_RGB24_ORDER_BGR
        B(c.B >> 8), G(c.G >> 8), R(c.R >> 8)
        #else
        R(c.R >> 8), G(c.G >> 8), B(c.B >> 8)
        #endif
        {
        }


constexpr inline RGB24::RGB24(const RGBf& c) :
        #if TGX_RGB24_ORDER_BGR
        B((uint8_t)(c.B * 255)), G((uint8_t)(c.G * 255)), R((uint8_t)(c.R * 255))
        #else
        R((uint8_t)(c.R * 255)), G((uint8_t)(c.G * 255)), B((uint8_t)(c.B * 255))
        #endif
        {
        }


inline RGB24& RGB24::operator=(const RGB565& c)
        {
        B = (((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2);
        G = (((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4);
        R = (((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2);
        return *this;
        }


inline RGB24& RGB24::operator=(const RGB32& c)
        {
        R = c.R;
        G = c.G;
        B = c.B;
        return *this;
        }


inline RGB24& RGB24::operator=(const RGB64& c)
        {
        R = c.R >> 8;
        G = c.G >> 8;
        B = c.B >> 8;
        return *this;
        }


inline RGB24& RGB24::operator=(const RGBf& c)
        {
        R = (uint8_t)(c.R * 255);
        G = (uint8_t)(c.G * 255);
        B = (uint8_t)(c.B * 255);
        return *this;
        }


inline RGB24& RGB24::operator=(iVec3 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        return *this;
        }


inline RGB24& RGB24::operator=(iVec4 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        return *this;
        }


inline RGB24& RGB24::operator=(fVec3 v)
        {
        B = (uint8_t)(v.x * 255);
        G = (uint8_t)(v.y * 255);
        R = (uint8_t)(v.z * 255);
        return *this;
        }


inline RGB24& RGB24::operator=(fVec4 v)
        {
        B = (uint8_t)(v.x * 255);
        G = (uint8_t)(v.y * 255);
        R = (uint8_t)(v.z * 255);
        return *this;
        }






/**********************************************************************
* implementation of inline method for RGB32
***********************************************************************/

constexpr inline RGB32::RGB32(uint16_t val) : RGB32(RGB565(val))
        {
        }

constexpr inline RGB32::RGB32(uint64_t val) : RGB32(RGB64(val))
        {
        }


constexpr inline RGB32::RGB32(const RGB565& c) : 
        #if TGX_RGB32_ORDER_BGR
        B((((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2)), G((((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4)), R((((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2)), A(DEFAULT_A)
        #else
        R((((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2)), G((((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4)), B((((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2)), A(DEFAULT_A)
        #endif
        {
        }


constexpr inline RGB32::RGB32(const RGB24& c) : 
        #if TGX_RGB32_ORDER_BGR
        B(c.B), G(c.G), R(c.R), A(DEFAULT_A)
        #else
        R(c.R), G(c.G), B(c.B), A(DEFAULT_A)
        #endif  
        {
        }


constexpr inline RGB32::RGB32(const RGB64& c) : 
        #if TGX_RGB32_ORDER_BGR
        B(c.B >> 8), G(c.G >> 8), R(c.R >> 8), A(c.A >> 8)
        #else
        R(c.R >> 8), G(c.G >> 8), B(c.B >> 8), A(c.A >> 8)
        #endif  
        {
        }


constexpr inline RGB32::RGB32(const RGBf& c) : 
        #if TGX_RGB32_ORDER_BGR
        B((uint8_t)(c.B * 255)), G((uint8_t)(c.G * 255)), R((uint8_t)(c.R * 255)), A(DEFAULT_A)
        #else
        R((uint8_t)(c.R * 255)), G((uint8_t)(c.G * 255)), B((uint8_t)(c.B * 255)), A(DEFAULT_A)
        #endif  
        {
        }


inline RGB32& RGB32::operator=(const RGB565& c)
        {
        B = (((uint8_t)c.B) << 3) | (((uint8_t)c.B) >> 2);
        G = (((uint8_t)c.G) << 2) | (((uint8_t)c.G) >> 4);
        R = (((uint8_t)c.R) << 3) | (((uint8_t)c.R) >> 2);
        A = DEFAULT_A;
        return *this;
        }


inline RGB32& RGB32::operator=(const RGB24& c)
        {
        B = c.B;
        G = c.G;
        R = c.R;
        A = DEFAULT_A;
        return *this;
        }


inline RGB32& RGB32::operator=(const RGB64& c)
        {
        B = c.B >> 8;
        G = c.G >> 8;
        R = c.R >> 8;
        A = c.A >> 8;
        return *this;
        }


inline RGB32& RGB32::operator=(const RGBf& c)
        {
        B = (uint8_t)(c.B * 255);
        G = (uint8_t)(c.G * 255);
        R = (uint8_t)(c.R * 255);
        A = DEFAULT_A;
        return *this;
        }


inline RGB32& RGB32::operator=(iVec3 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        A = DEFAULT_A;
        return *this;
        }


inline RGB32& RGB32::operator=(iVec4 v)
        {
        R = (uint8_t)v.x;
        G = (uint8_t)v.y;
        B = (uint8_t)v.z;
        A = (uint8_t)v.w;
        return *this;
        }


inline RGB32& RGB32::operator=(fVec3 v)
        {
        B = (uint8_t)(v.x * 255);
        G = (uint8_t)(v.y * 255);
        R = (uint8_t)(v.z * 255);
        A = DEFAULT_A;
        return *this;
        }


inline RGB32& RGB32::operator=(fVec4 v)
        {
        B = (uint8_t)(v.x * 255);
        G = (uint8_t)(v.y * 255);
        R = (uint8_t)(v.z * 255);
        A = (uint8_t)(v.w * 255);
        return *this;
        }




/**********************************************************************
* implementation of inline method for RGB64
***********************************************************************/

inline RGB64::RGB64(uint16_t val) : RGB64(RGB565(val))
        {
        }


inline RGB64::RGB64(uint32_t val) : RGB64(RGB32(val))
        {
        }


inline RGB64::RGB64(const RGB565& c) 
        {
        this->operator=(c);
        }


inline RGB64::RGB64(const RGB24& c) 
        {   
        this->operator=(c);
        }


inline RGB64::RGB64(const RGB32& c)
        {
        this->operator=(c);
        }


inline RGB64::RGB64(const RGBf& c) 
        {
        this->operator=(c);
        }


inline RGB64& RGB64::operator=(const RGB565& c)
        {
        R = (((uint16_t)c.R) << 11) | (((uint16_t)c.R) << 6) | (((uint16_t)c.R) << 1) | (((uint16_t)c.R) >> 4);
        G = (((uint16_t)c.G) << 10) | (((uint16_t)c.G) << 4) | (((uint16_t)c.G) >> 2);
        B = (((uint16_t)c.B) << 11) | (((uint16_t)c.B) << 6) | (((uint16_t)c.B) << 1) | (((uint16_t)c.B) >> 4);
        A = DEFAULT_A;
        return *this;
        }


inline RGB64& RGB64::operator=(const RGB24& c)
        {
        R = (((uint16_t)c.R) << 8) | ((uint16_t)c.R);
        G = (((uint16_t)c.G) << 8) | ((uint16_t)c.G);
        B = (((uint16_t)c.B) << 8) | ((uint16_t)c.B);
        A = DEFAULT_A;
        return *this;
        }


inline RGB64& RGB64::operator=(const RGB32& c)
        {
        R = (((uint16_t)c.R) << 8) | ((uint16_t)c.R);
        G = (((uint16_t)c.G) << 8) | ((uint16_t)c.G);
        B = (((uint16_t)c.B) << 8) | ((uint16_t)c.B);
        A = (((uint16_t)c.A) << 8) | ((uint16_t)c.A);
        return *this;
        }

inline RGB64& RGB64::operator=(const RGBf& c)
        {
        B = (uint16_t)(c.B * 65535);
        G = (uint16_t)(c.G * 65535);
        R = (uint16_t)(c.R * 65535);
        A = DEFAULT_A;
        return *this;
        }


inline RGB64& RGB64::operator=(iVec3 v)
        {
        R = (uint16_t)v.x;
        G = (uint16_t)v.y;
        B = (uint16_t)v.z;
        A = DEFAULT_A;
        return *this;
        }


inline RGB64& RGB64::operator=(iVec4 v)
        {
        R = (uint16_t)v.x;
        G = (uint16_t)v.y;
        B = (uint16_t)v.z;
        A = (uint16_t)v.w;
        return *this;
        }


inline RGB64& RGB64::operator=(fVec3 v)
        {
        B = (uint16_t)(v.x * 65535);
        G = (uint16_t)(v.y * 65535);
        R = (uint16_t)(v.z * 65535);
        A = DEFAULT_A;
        return *this;
        }


inline RGB64& RGB64::operator=(fVec4 v)
        {
        B = (uint16_t)(v.x * 65535);
        G = (uint16_t)(v.y * 65535);
        R = (uint16_t)(v.z * 65535);
        A = (uint16_t)(v.w * 65535);
        return *this;
        }




/**********************************************************************
* implementation of inline method for RGBf
***********************************************************************/


constexpr inline RGBf::RGBf(uint16_t val) : RGBf(RGB565(val)) 
        {
        }


constexpr inline RGBf::RGBf(uint32_t val) : RGBf(RGB32(val)) 
        {
        }


constexpr inline RGBf::RGBf(uint64_t val) : RGBf(RGB64(val)) 
        {
        }


constexpr inline RGBf::RGBf(const RGB565& c) : 
        #if TGX_RGBf_ORDER_BGR
        B(c.B / 31.0f), G(c.G / 63.0f), R(c.R / 31.0f)
        #else
        R(c.R / 31.0f), G(c.G / 63.0f), B(c.B / 31.0f)
        #endif
        {
        }


constexpr inline RGBf::RGBf(const RGB24& c) :
        #if TGX_RGBf_ORDER_BGR
        B(c.B / 255.0f), G(c.G / 255.0f), R(c.R / 255.0f)
        #else
        R(c.R / 255.0f), G(c.G / 255.0f), B(c.B / 255.0f)
        #endif
        {
        }


constexpr inline RGBf::RGBf(const RGB32& c) :
        #if TGX_RGBf_ORDER_BGR
        B(c.B / 255.0f), G(c.G / 255.0f), R(c.R / 255.0f)
        #else
        R(c.R / 255.0f), G(c.G / 255.0f), B(c.B / 255.0f)
        #endif
        {
        }


constexpr inline RGBf::RGBf(const RGB64& c) : 
        #if TGX_RGBf_ORDER_BGR
        B(c.B / 65535.0f), G(c.G / 65535.0f), R(c.R / 65535.0f)
        #else
        R(c.R / 65535.0f), G(c.G / 65535.0f), B(c.B / 65535.0f)
        #endif
        {
        }


inline RGBf& RGBf::operator=(const RGB565& c)
        {
        R = c.R / 31.0f;
        G = c.G / 63.0f;
        B = c.B / 31.0f;
        return *this;
        }


inline RGBf& RGBf::operator=(const RGB24& c)
        {
        R = c.R / 255.0f;
        G = c.G / 255.0f;
        B = c.B / 255.0f;
        return *this;
        }


inline RGBf& RGBf::operator=(const RGB32& c)
        {
        R = c.R / 255.0f;
        G = c.G / 255.0f;
        B = c.B / 255.0f;
        return *this;
        }


inline RGBf& RGBf::operator=(const RGB64& c)
        {
        R = c.R / 65535.0f;
        G = c.G / 65535.0f;
        B = c.B / 65535.0f;
        return *this;
        }


inline RGBf& RGBf::operator=(fVec3 v)
        {
        B = v.x;
        G = v.y;
        R = v.z;
        return *this;
        }


inline RGBf& RGBf::operator=(fVec4 v)
        {
        B = v.x;
        G = v.y;
        R = v.z;
        return *this;
        }




}

#endif

/** end of file */

