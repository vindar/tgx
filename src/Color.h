/**   
 * @file Color.h 
 * Color classes [RGB565, RGB24, RGB32, RGB64, RGBf, HSV].
 */
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

#ifndef _TGX_COLOR_H_
#define _TGX_COLOR_H_

// only C++, no plain C
#ifdef __cplusplus

#include "Misc.h"

#include <stdint.h>
#include <math.h>
#include <type_traits>

#include "Vec3.h"
#include "Vec4.h"

namespace tgx
{


// ********************************************************************************
// Color types. 
//
// The following color types are available for use with the tgx library:
//
// - RGB565: 16bits, R:5 G:6 B:5 colors  
//           no alpha channel wrapper around a uint16_t integer, aligned as uint16_t
//          
// - RGB24:  24bits, R:8 G:8 B:8 colors.   
//           no alpha channel, not aligned in memory
//           
// - RGB32:  32bits, R:8 G:8 B:8 A:8 colors.   
//           alpha channel, wrapper around a uint32_t integer, aligned as uint32_t
//          
// - RGB64:  64bits, R:16 G:16 B:16 A:16 colors.
//           alpha channel, wrapper around a uint64_t integer, aligned as uint64_t
//
// - RGBf:   96bits, R:float G:float, B:float  
//           no alpha channel, aligned as float.
//  
// - HSV:    96bits, H:float, S:float, V:float  
//           hue / saturation / value color space: very slow
//
//
// REMARKS:
//
// 1. RGB565, RGB32 and RGB64 are wrappers around basic integer types they can be 
//    used as drop in remplacment of uint16_t, uint32_t and uint64_t without any 
//    speed penalty.
//   
//   For example:
//       tgx::RGB565 col(12,63,30)
//       uint16_t v = (uint16_t)col; // <- conversion to uint16_t
//       tgx::RGB565 col2 = tgx::RGB565(v) // <- conversion back to RGB565
//   
// 2. RGB32 and RGB64 have an alpha channel. The color are always assumed to have  
//    pre-multiplied alpha. 
//  
// 3. Fast conversion is implemented between color types (and also integer types)   
//    except when converting from/to HSV which is slow. 
//   
// ********************************************************************************




// For each RGB color type, we decide separately whether color components 
// are ordered in memory as R,G,B or B,G,R. 
// The default ordering below can overridden be #definining the constants 
// before including this header 

#ifndef TGX_RGB565_ORDER_BGR
#define TGX_RGB565_ORDER_BGR 1  ///< color ordering for RGB565 (default B,G,R for compatibility with adafruit and most SPI display)
#endif

#ifndef TGX_RGB24_ORDER_BGR
#define TGX_RGB24_ORDER_BGR 0   ///< color ordering for RGB24 (default R,G,B)
#endif

#ifndef TGX_RGB32_ORDER_BGR
#define TGX_RGB32_ORDER_BGR 1   ///< color ordering for RGB565 (default B,G,R for compatibility with the mtools library)
#endif

#ifndef TGX_RGB64_ORDER_BGR
#define TGX_RGB64_ORDER_BGR 0   ///< color ordering for RGB64 (default R,G,B)
#endif

#ifndef TGX_RGBf_ORDER_BGR
#define TGX_RGBf_ORDER_BGR 0    ///< color ordering for RGBf (default R,G,B)
#endif



// Forward declarations

struct RGB565;  // color in 16-bit R5/G6/B5 format (2 bytes aligned) - convertible to/from uint16_t

struct RGB24;   // color in 24-bit R8/G8/B8/ format (unaligned !). 

struct RGB32;   // color in 32-bit R8/G8/B8/(A8) format (4 bytes aligned) - convertible to/from uint32_t

struct RGB64;   // color in 64 bit R16/G16/B16/(A16) format (8 bytes aligned) - convertible to/from uint64_t

struct RGBf;    // color in  RGB float format (4 bytes aligned). 

struct HSV;     // color in H/S/V float format (4 bytes aligned). 



/** 
 * Check if a type T is one of the color types declared above 
 */
template<typename T> struct is_color
    {
    static const bool value = (std::is_same<T, RGB565>::value)||
                              (std::is_same<T, RGB24>::value) ||
                              (std::is_same<T, RGB32>::value) ||
                              (std::is_same<T, RGB64>::value) ||
                              (std::is_same<T, RGBf>::value) ||
                              (std::is_same<T, HSV>::value);
    };



// Predefined colors in RGB32 format 

extern const RGB32 RGB32_Black;         ///< Color black in RGB32 format.
extern const RGB32 RGB32_White;         ///< Color white in RGB32 format.
extern const RGB32 RGB32_Red;           ///< Color red in RGB32 format.
extern const RGB32 RGB32_Blue;          ///< Color blue in RGB32 format.
extern const RGB32 RGB32_Green;         ///< Color green in RGB32 format.
extern const RGB32 RGB32_Purple;        ///< Color purple in RGB32 format.
extern const RGB32 RGB32_Orange;        ///< Color orange in RGB32 format.
extern const RGB32 RGB32_Cyan;          ///< Color cyan in RGB32 format.
extern const RGB32 RGB32_Lime;          ///< Color lime in RGB32 format.
extern const RGB32 RGB32_Salmon;        ///< Color salmon in RGB32 format.
extern const RGB32 RGB32_Maroon;        ///< Color maroon in RGB32 format.
extern const RGB32 RGB32_Yellow;        ///< Color yellow in RGB32 format.
extern const RGB32 RGB32_Magenta;       ///< Color majenta in RGB32 format.
extern const RGB32 RGB32_Olive;         ///< Color olive in RGB32 format.
extern const RGB32 RGB32_Teal;          ///< Color teal in RGB32 format.
extern const RGB32 RGB32_Gray;          ///< Color gray in RGB32 format.
extern const RGB32 RGB32_Silver;        ///< Color silver in RGB32 format.
extern const RGB32 RGB32_Navy;          ///< Color navy in RGB32 format.
extern const RGB32 RGB32_Transparent;   ///< premultiplied transparent black (0,0,0,0)



// Predefined colors in RGB565 format 

extern const RGB565 RGB565_Black;       ///< Color black in RGB565 format.
extern const RGB565 RGB565_White;       ///< Color white in RGB565 format.
extern const RGB565 RGB565_Red;         ///< Color red in RGB565 format.
extern const RGB565 RGB565_Blue;        ///< Color blue in RGB565 format.
extern const RGB565 RGB565_Green;       ///< Color green in RGB565 format.
extern const RGB565 RGB565_Purple;      ///< Color purple in RGB565 format.
extern const RGB565 RGB565_Orange;      ///< Color orange in RGB565 format.
extern const RGB565 RGB565_Cyan;        ///< Color cyan in RGB565 format.
extern const RGB565 RGB565_Lime;        ///< Color lime in RGB565 format.
extern const RGB565 RGB565_Salmon;      ///< Color salmon in RGB565 format.
extern const RGB565 RGB565_Maroon;      ///< Color maroon in RGB565 format.
extern const RGB565 RGB565_Yellow;      ///< Color yellow in RGB565 format.
extern const RGB565 RGB565_Magenta;     ///< Color majenta in RGB565 format.
extern const RGB565 RGB565_Olive;       ///< Color olive in RGB565 format.
extern const RGB565 RGB565_Teal;        ///< Color teal in RGB565 format.
extern const RGB565 RGB565_Gray;        ///< Color gray in RGB565 format.
extern const RGB565 RGB565_Silver;      ///< Color silver in RGB565 format.
extern const RGB565 RGB565_Navy;        ///< Color navy in RGB565 format.




/**
 * Color in R5/G6/B5 format.
 * 
 * The object occupies 2 bytes in memory (aligned as uint16_t)
 * 
 * Can be converted from/to uint16_t.
 * 
 * This type is used mostly with MCU / embedded systems.
 */
struct RGB565
    {
        
        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_RGB565.inl>
        #endif
        

        // dual memory representation
        union
            { 
            uint16_t val; ///< color as uint16_t
            
            struct {                
#if TGX_RGB565_ORDER_BGR
                uint16_t B : 5, ///< Blue channel (5 bits)
                         G : 6, ///< Green channel (6 bits)
                         R : 5; ///< Red channel (5 bits)
#else
                uint16_t R : 5, ///< Red channel (5 bits)
                         G : 6, ///< Green channel (6 bits)
                         B : 5; ///< Blue channel (5 bits)
#endif
                }; 
            };


        /**
        * Default constructor. ** color is undefined ***
        */
        RGB565() = default;


        /**
         * Constructor from R,G,B values.
         *
         * @param   r   in [0,31]
         * @param   g   in [0,63]
         * @param   b   in [0,31]
         */
        constexpr RGB565(int r, int g, int b) : 
                                            #if TGX_RGB565_ORDER_BGR
                                                B((uint8_t)b), G((uint8_t)g), R((uint8_t)r)
                                            #else
                                                R((uint8_t)r), G((uint8_t)g), B((uint8_t)b)
                                            #endif
        {}


        /**
         * Constructor from a #iVec3 vector (x=R, y=G, z=B).
         *
         * - red componenent v.x in [0,31]
         * - green componenent v.y in [0,63]  
         * - blue componenent v.z in [0,31]
         */
        constexpr RGB565(iVec3 v) : RGB565(v.x, v.y, v.z)       
        {}


        /**
        * Constructor from an #iVec4 vector (x=R, y=G, z=B, w=ignored).
        * 
        * - red componenent v.x in [0,31]
        * - green componenent v.y in [0,63]
        * - blue componenent v.z in [0,31]  
        * - v.w is ignored
        */
        constexpr RGB565(iVec4 v) : RGB565(v.x, v.y, v.z)
        {}

        
        /**
        * Constructor from float r,g,b in [0.0f, 1.0f].
        */
        RGB565(float r, float g, float b) : 
                                            #if TGX_RGB565_ORDER_BGR
                                                B((uint8_t)(b * 31)),
                                                G((uint8_t)(g * 63)),
                                                R((uint8_t)(r * 31))
                                            #else
                                                R((uint8_t)(r * 31)),
                                                G((uint8_t)(g * 63)),
                                                B((uint8_t)(b * 31))
                                            #endif      
        {}


        /**
        * Constructor from a #fVec3 vector with components (x=R, y=G, z=B) in [0.0f, 1.0f].
        */
        RGB565(fVec3 v) : RGB565(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec4 vector with components (x=R, y=G, z=B, w=ignored) in [0.0f, 1.0f].
        */
        RGB565(fVec4 v) : RGB565(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a uint16_t.
        */
        constexpr RGB565(uint16_t c) : val(c) {}


        /**
        * Constructor from a uint32_t (seen as RGB32).
        */
        constexpr inline RGB565(uint32_t val);


        /**
        * Constructor from a uint64_t (seen as RGB64).
        */
        constexpr inline RGB565(uint64_t val);


        /**
        * Default Copy constructor
        */
        constexpr RGB565(const RGB565&) = default;


        /**
        * Constructor from a RGB24 color.
        */
        constexpr inline RGB565(const RGB24& c);


        /**
        * Constructor from a RGB32 color. The component A is ignored
        */
        constexpr inline RGB565(const RGB32& c);


        /**
        * Constructor from a RGB64 color. The component A is ignored
        */
        constexpr inline RGB565(const RGB64& c);


        /**
        * Constructor from a RGBf color.
        */
        constexpr inline RGB565(const RGBf & c);


        /**
        * Constructor from a HSV color.
        */
        RGB565(const HSV& c);


        /**
        * Cast into a uint16_t (non-const reference version)
        */
        explicit operator uint16_t&()  { return val; }


        /**
        * Cast into a uint16_t (const reference version)
        */
        explicit operator const uint16_t& () const { return val; }


        /**
        * Cast into an #iVec3 vector. raw values.
        */
        explicit operator iVec3() const { return iVec3(R, G, B); }


        /**
        * Cast into an #fVec3 vector. Values in [0.0f, 1.0f].
        */
        explicit operator fVec3() const { return fVec3(R / 31.0f, G / 63.0f , B / 31.0f); }


        /**
        * Default assignement operator.
        */ 
        RGB565& operator=(const RGB565&) = default;


        /**
        * Assignement operator from a RGB24 color.
        */
        inline RGB565& operator=(const RGB24& c);


        /**
        * Assignement operator from a RGB32 color. The component A is ignored
        */
        inline RGB565& operator=(const RGB32& c);


        /**
        * Assignement operator from a RGB64 color. The component A is ignored
        */
        inline RGB565& operator=(const RGB64& c);


        /**
        * Assignement operator from a RGBf color
        */
        inline RGB565& operator=(const RGBf& c);


        /**
        * Assignement operator from a HSV color
        */
        RGB565& operator=(const HSV& c);


        /**
        * Assignement operator from an #iVec3 vector. Raw values (no conversion).
        */
        inline RGB565& operator=(iVec3 v);


        /**
        * Assignement operator from a #iVec4. The `w` component is ignored. Raw values (no conversion).
        */
        inline RGB565& operator=(iVec4 v);


        /**
        * Assignement operator from a #fVec3. All values in [0.0f, 1.0f].
        */
        inline RGB565& operator=(fVec3 v);


        /**
        * Assignement operator from a #fVec4. the `w` component is ignored. All values in [0.0f, 1.0f].
        */
        inline RGB565& operator=(fVec4 v);


        /**
        * Add another color, component by component.
        */
        void operator+=(const RGB565& c)
            {
            R += c.R;
            G += c.G;
            B += c.B;
            }


        /**
        * Substract another color, component by component.
        */
        void operator-=(const RGB565& c)
            {
            R -= c.R;
            G -= c.G;
            B -= c.B;
            }


        /**
        * Equality comparator
        */
        constexpr bool operator==(const RGB565& c) const
            {
            return(val == c.val);
            }


        /**
        * Inequality comparator
        */
        constexpr bool operator!=(const RGB565& c) const
            {
            return !(*this == c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0.0f,1.0f].
         */
        inline void blend(RGB565 fg_col, float alpha)
            {
            blend256(fg_col, (uint32_t)(alpha * 256));
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 256 (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0,256].
         */
        inline void blend256(const RGB565 & fg_col, uint32_t alpha)
            {       
            const uint32_t a = (alpha >> 3); // map to 0 - 32.
            const uint32_t bg = (val | (val << 16)) & 0b00000111111000001111100000011111;
            const uint32_t fg = (fg_col.val | (fg_col.val << 16)) & 0b00000111111000001111100000011111;
            const uint32_t result = ((((fg - bg) * a) >> 5) + bg) & 0b00000111111000001111100000011111;
            val = (uint16_t)((result >> 16) | result); // contract result
            }


        /**
         * Multiply each color component by a given factor m/256 with m in [0,256]
         */
        inline void mult256(int mr, int mg, int mb)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            }


        /**
        * Multiply each color component by a given factor x/256 with x in [0,256].
        * 
        * Parameter ma is ignored since there is not alpha channel.
        */
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            mult256(mr, mg, mb);
            }


        /**         
        * Dummy function for compatibility with color types having an alpha channel.
        * 
        * Does nothing since the color is always fully opaque. 
        */
        inline void premultiply()
            {
            // nothing here. 
            return;
            }


        /**
         * Dummy function for compatibility with color types having an alpha channel.
         * 
         * Return 1.0f (fully opaque)
         */
        float opacity() const
            {
            return 1.0f;
            }


        /**
         * Dummy function for compatibility with color types having an alpha channel.
         * 
         * Does nothing since the color is always fully opaque.
         */
        void setOpacity(float op)
            {
            // nothing here. 
            return;
            }


    };


    /**
    * Interpolate between 3 colors. 
    * 
    * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
    **/
    inline RGB565 interpolateColorsTriangle(const RGB565 & col1, int32_t C1, const  RGB565 & col2, int32_t C2, const  RGB565 & col3, const int32_t totC)
        {       
        C1 <<= 5;
        C1 /= totC;
        C2 <<= 5;
        C2 /= totC;
        const uint32_t bg1 = (col1.val | (col1.val << 16)) & 0b00000111111000001111100000011111;
        const uint32_t bg2 = (col2.val | (col2.val << 16)) & 0b00000111111000001111100000011111;
        const uint32_t bg3 = (col3.val | (col3.val << 16)) & 0b00000111111000001111100000011111;
        const uint32_t result = ((bg1*C1 + bg2 * C2 + bg3*(32 - C1 - C2)) >> 5) & 0b00000111111000001111100000011111;
        return RGB565((uint16_t)((result >> 16) | result)); // contract result      
        }
    

    /**
     * Bilinear interpolation between 4 colors.
     * 
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     * 
     *```
     *  C01          C11
     * 
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline RGB565 interpolateColorsBilinear(const RGB565 & C00, const RGB565 & C10, const RGB565 & C01, const RGB565 & C11, const float ax, const float ay)
            {
            /* flotating point version, slower...
            const float rax = 1.0f - ax;
            const float ray = 1.0f - ay;            
            const int R = (int)(rax*(ray*C00.R + ay*C01.R) + ax*(ray*C10.R + ay*C11.R));
            const int G = (int)(rax*(ray*C00.G + ay*C01.G) + ax*(ray*C10.G + ay*C11.G));
            const int B = (int)(rax*(ray*C00.B + ay*C01.B) + ax*(ray*C10.B + ay*C11.B));
            return RGB565(R,G,B);
            */            
            const int iax = (int)(ax * 256);
            const int iay = (int)(ay * 256);
            const int rax = 256 - iax;
            const int ray = 256 - iay; 
            const int R = rax*(ray*C00.R + iay*C01.R) + iax*(ray*C10.R + iay*C11.R);
            const int G = rax*(ray*C00.G + iay*C01.G) + iax*(ray*C10.G + iay*C11.G);
            const int B = rax*(ray*C00.B + iay*C01.B) + iax*(ray*C10.B + iay*C11.B);
            return RGB565(R >> 16,G >> 16,B >> 16);            
            }


    /**
    * Return the average color between colA and colB.
    * 
    * TODO : make it faster
    */
    inline RGB565 meanColor(RGB565 colA, RGB565 colB)
        {
        return RGB565( ((int)colA.R + (int)colB.R) >> 1, 
                       ((int)colA.G + (int)colB.G) >> 1,
                       ((int)colA.B + (int)colB.B) >> 1);
        }


    /**
    * Return the average color between 4 colors.
    * 
    * TODO : make it faster
    */
    inline RGB565 meanColor(RGB565 colA, RGB565 colB, RGB565 colC, RGB565 colD)
        {
        return RGB565(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
                      ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
                      ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2);
        }





/**
 * Color in R8/G8/B8 format.
 * 
 * Occupies 3 bytes in memory. No alignement.
 * 
 * **Remark** This color type should only be used when memory space is really tight but RGB565
 * does not offer enough resolution. Use RGB32 instead when possible (even if not using the
 * alpha component) because most operations will be faster with correct 4 bytes alignment.
 */
struct RGB24
    {

        
        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_RGB24.inl>
        #endif
        
        
        // no dual memory represention
        // no alignement, just 3 consecutive uint8_t

#if TGX_RGB24_ORDER_BGR
        uint8_t B; ///< Blue channel (8bits)
        uint8_t G; ///< Green channel (8bits)
        uint8_t R; ///< Red channel (8bits)
#else
        uint8_t R; ///< Red channel (8bits)
        uint8_t G; ///< Green channel (8bits)
        uint8_t B; ///< Blue channel (8bits)
#endif



        /**
        * Default contructor. **Color is undefined**.
        */
        RGB24() = default;


        /**
        * Constructor from raw r,g,b values in [0,255].
        */
        constexpr RGB24(int r, int g, int b) : 
                                            #if TGX_RGB24_ORDER_BGR
                                                B((uint8_t)b), G((uint8_t)g), R((uint8_t)r)
                                            #else
                                                R((uint8_t)r), G((uint8_t)g), B((uint8_t)b)
                                            #endif

        {}


        /**
        * Constructor from a #iVec3 with component (x=R, y=G, z=B). All values in [0,255].
        */
        constexpr RGB24(iVec3 v) : RGB24(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #iVec4 with component (x=R, y=G, z=B, w=ignored). All values in [0,255].
        */
        constexpr RGB24(iVec4 v) : RGB24(v.x, v.y, v.z)
        {}

        
        /**
        * Constructor from float r,g,b in [0.0f, 1.0f].
        */
        RGB24(float r, float g, float b) : 
                                            #if TGX_RGB24_ORDER_BGR
                                                B((uint8_t)(b * 255)),
                                                G((uint8_t)(g * 255)),
                                                R((uint8_t)(r * 255))
                                            #else
                                                R((uint8_t)(r * 255)),
                                                G((uint8_t)(g * 255)),
                                                B((uint8_t)(b * 255))
                                            #endif      
        {}


        /**
        * Constructor from a #fVec3 with component (x=R, y=G, z=B) in [0.0f, 1.0f].
        */
        RGB24(fVec3 v) : RGB24(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec3 with component (x=R, y=G, z=B, w=ignored) in [0.0f, 1.0f].
        */
        RGB24(fVec4 v) : RGB24(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a uint8_t pointer to 3 bytes in the following order: R,G,B. 
        */
        constexpr RGB24(uint8_t * p) : R(p[0]), G(p[1]), B(p[2]) {}


        /**
        * Default Copy constructor.
        */
        RGB24(const RGB24&) = default;


        /**
        * Constructor from a uint16_t (seen as RGB565).
        */
        constexpr inline RGB24(uint16_t c);


        /**
        * Constructor from a uint32_t (seen as RGB32).
        */
        constexpr inline RGB24(uint32_t c);


        /**
        * Constructor from a uint64_t (seen as RGB64).
        */
        constexpr inline RGB24(uint64_t c);


        /**
        * Constructor from a RGB24 color.
        */
        constexpr inline RGB24(const RGB565& c);


        /**
        * Constructor from a RGB32 color. The component A is ignored
        */
        constexpr inline RGB24(const RGB32& c);


        /**
        * Constructor from a RGB64 color. The component A is ignored
        */
        constexpr inline RGB24(const RGB64& c);


        /**
        * Constructor from a RGBf color.
        */
        constexpr inline RGB24(const RGBf& c);


        /**
        * Constructor from a HSV color.
        */
        RGB24(const HSV& c);


        /**
        * Cast into an #iVec3. Raw values in [0,255].
        */
        explicit operator iVec3() const { return iVec3(R, G, B); }


        /**
        * Cast into an #fVec3. Values in [0.0f, 1.0f].
        */
        explicit operator fVec3() const { return fVec3(R / 255.0f, G / 255.0f, B / 255.0f); }


        /**
        * Default assignement operator.
        */ 
        RGB24& operator=(const RGB24&) = default;


        /**
        * Assignement operator from a RGB565 color.
        */
        inline RGB24& operator=(const RGB565& c);


        /**
        * Assignement operator from a RGB32 color. The component A is ignored.
        */
        inline RGB24& operator=(const RGB32& c);


        /**
        * Assignement operator from a RGB64 color. The component A is ignored.
        */
        inline RGB24& operator=(const RGB64& c);


        /**
        * Assignement operator from a RGBf color.
        */
        inline RGB24& operator=(const RGBf& c);


        /**
        * Assignement operator from a HSV color.
        */
        RGB24& operator=(const HSV& c);


        /**
        * Assignement operator from a #iVec3 vector (x=R, y=G, z=B). Raw values.
        */
        inline RGB24& operator=(iVec3 v);


        /**
        * Assignement operator from a #iVec4 vector (x=R, y=G, z=B, w=ignored). Raw values.
        */
        inline RGB24& operator=(iVec4 v);


        /**
        * Assignement operator from a #fVec3 vector (x=R, y=G, z=B). All values in [0.0f, 1.0f].
        */
        inline RGB24& operator=(fVec3 v);


        /**
        * Assignement operator from a #fVec4 vector (x=R, y=G, z=B, w=ignored). All values in [0.0f, 1.0f].
        */
        inline RGB24& operator=(fVec4 v);


        /**
        * Add another color, component by component.
        */
        void operator+=(const RGB24 & c)
            {
            R += c.R;
            G += c.G;
            B += c.B;
            }


        /**
        * Substract another color, component by component.
        */
        void operator-=(const RGB24& c)
            {
            R -= c.R;
            G -= c.G;
            B -= c.B;
            }


        /**
        * Add the scalar value v to each component
        */
        void operator+=(uint8_t v)
            {
            R += v;
            G += v;
            B += v;
            }


        /**
         * Substract the scalar value v to each component
         */
        void operator-=(uint8_t v)
            {
            R -= v;
            G -= v;
            B -= v;
            }


        /**
        * Multiply each component by the scalar value v.
        */
        void operator*=(uint8_t v)
            {
            R *= v;
            G *= v;
            B *= v;
            }


        /**
        * Multiply each component by the scalar (floating point) value v.
        */
        void operator*=(float v)
            {
            R = (uint8_t)(R * v);
            G = (uint8_t)(G * v); 
            B = (uint8_t)(B * v);
            }

        /**
        * Divide each component by the scalar value v.
        */
        void operator/=(uint8_t v)
            {
            R /= v;
            G /= v;
            B /= v;
            }

        /**
        * Divide each component by the scalar (floating point value) v.
        */
        void operator/=(float v)
            {
            R = (uint8_t)(R / v);
            G = (uint8_t)(G / v);
            B = (uint8_t)(B / v);
            }


        /**
        * Equality comparator.
        */
        constexpr bool operator==(const RGB24& c) const
            {
            return((R == c.R) && (G == c.G) && (B == c.B));
            }


        /**
        * Inequality comparator.
        */
        constexpr bool operator!=(const RGB24& c) const
            {
            return !(*this == c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0.0f, 1.0f].
         */
        inline void blend(const RGB24 & fg_col, float alpha)
            {
            blend256(fg_col, (uint32_t)(alpha * 256));
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 256 (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0, 256].
         */
        inline void blend256(const RGB24& fg_col, uint32_t alpha)
            {
            const uint16_t a = (uint16_t)alpha;
            const uint16_t ia = (uint16_t)(256 - alpha);
            R = ((fg_col.R * a) + (R * ia)) >> 8;
            G = ((fg_col.G * a) + (G * ia)) >> 8;
            B = ((fg_col.B * a) + (B * ia)) >> 8;
            }


        /**
        * Multiply each color component by a given factor m/256 with m in [0,256]
        */
        inline void mult256(int mr, int mg, int mb)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            }


        /**
        * Multiply each color component by a given factor m/256 with m in [0,256]
        * 
        * Parameter ma is ignored since there is not alpha channel.
        */
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            mult256(mr, mg, mb);
            }


        /**         
        * Dummy function for compatibility with color types having an alpha channel.
        * 
        * Does nothing since the color is always fully opaque. 
        */
        inline void premultiply()
            {
            // nothing here. 
            return;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Return 1.0f (fully opaque)
        */
        float opacity() const
            {
            return 1.0f;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Does nothing since the color is always fully opaque.
        */
        void setOpacity(float op)
            {
            // nothing here. 
            return;
            }


    };



    /**
    * Interpolate between 3 colors.
    *
    * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
    **/
    inline RGB24 interpolateColorsTriangle(const RGB24& col1, int32_t C1, const  RGB24& col2, int32_t C2, const  RGB24& col3, const int32_t totC)
        {
        return RGB24((int)(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC),
                     (int)(col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC),
                     (int)(col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC));
        }


    /**
     * Bilinear interpolation between 4 colors.
     *
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     *
     *```
     *  C01          C11
     *
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline RGB24 interpolateColorsBilinear(const RGB24 & C00, const RGB24 & C10, const RGB24 & C01, const RGB24 & C11, const float ax, const float ay)
            {           
            const int iax = (int)(ax * 256);
            const int iay = (int)(ay * 256);
            const int rax = 256 - iax;
            const int ray = 256 - iay; 
            const int R = rax*(ray*C00.R + iay*C01.R) + iax*(ray*C10.R + iay*C11.R);
            const int G = rax*(ray*C00.G + iay*C01.G) + iax*(ray*C10.G + iay*C11.G);
            const int B = rax*(ray*C00.B + iay*C01.B) + iax*(ray*C10.B + iay*C11.B);
            return RGB24(R >> 16,G >> 16,B >> 16);            
            }


    /**
    * Return the average color between colA and colB.
    */
    inline RGB24 meanColor(const RGB24 & colA, const RGB24 & colB)
        {
        return RGB24(((int)colA.R + (int)colB.R) >> 1,
                     ((int)colA.G + (int)colB.G) >> 1,
                     ((int)colA.B + (int)colB.B) >> 1);
        }


    /**
    * Return the average color between 4 colors.
    */
    inline RGB24 meanColor(const RGB24& colA, const RGB24& colB, const RGB24& colC, const RGB24& colD)
        {
        return RGB24(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
                     ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
                     ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2);
        }





/**
 * Color in R8/G8/B8/A8 format.
 * 
 * Occupies 4 bytes in memory, aligned as uint32_t.
 * 
 * Can be converted from/to uint32_t.
 * 
 * the component A defaults to #DEFAULT_A = 255 (fully opaque) if not specified.
 * 
 * **Remark** For all drawing/blending operations, the color is assume to have pre-multiplied
 * alpha. Use the premultiply() method to convert a plain alpha color to its pre-multiplied
 * version (https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre).
 */
struct RGB32
    {

        
        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_RGB32.inl>
        #endif


        static const uint8_t DEFAULT_A = 255; ///< fully opaque alpha value

        // dual memory representation
        union 
            { 
            uint32_t val;   ///< color as uint32_t
            struct 
                {
#if TGX_RGB32_ORDER_BGR
                uint8_t B;  ///< Blue channel (8bits)
                uint8_t G;  ///< Green channel (8bits)
                uint8_t R;  ///< Red channel (8bits)
                uint8_t A;  ///< Alpha channel (8bits)

#else
                uint8_t R;  ///< Red channel (8bits)
                uint8_t G;  ///< Green channel (8bits)
                uint8_t B;  ///< Blue channel (8bits)
                uint8_t A;  ///< Alpha channel (8bits)
#endif
                }; 
            };


        /**
        * Default constructor. **Color is undefined**. 
        */
        RGB32() = default;


        /**
        * Constructor from raw r,g,b,a values. All values in [0,255].
        */
        constexpr RGB32(int r, int g, int b, int a = DEFAULT_A) : 
                                                            #if TGX_RGB32_ORDER_BGR
                                                                B((uint8_t)b), G((uint8_t)g), R((uint8_t)r), A((uint8_t)a)
                                                            #else
                                                                R((uint8_t)r), G((uint8_t)g), B((uint8_t)b), A((uint8_t)a)
                                                            #endif
        {}


        /**
         * Constructor from a #fVec3 with components (x=R, y=G, z=B) in [0,255]. Component A is set to
         * DEFAULT_A (opaque).
         */
        constexpr RGB32(iVec3 v) : RGB32(v.x, v.y, v.z)
        {}


        /**
         * Constructor from a #fVec4 with components (x=R, y=G, z=B, w=A) in [0,255].
         */
        constexpr RGB32(iVec4 v) : RGB32(v.x, v.y, v.z, v.w)
        {}


        /**
         * Constructor from float r,g,b,a in [0.0f, 1.0f].  If unspecified, component A is set to DEFAULT_A. 
         */
        RGB32(float r, float g, float b, float a = -1.0f) : 
                                                            #if TGX_RGB32_ORDER_BGR
                                                                B((uint8_t)(b * 255)),
                                                                G((uint8_t)(g * 255)),
                                                                R((uint8_t)(r * 255)),
                                                                A((uint8_t)((a < 0.0f) ? DEFAULT_A : ((uint8_t)roundf(a * 255.0f))))
                                                            #else
                                                                R((uint8_t)(r * 255.0f)),
                                                                G((uint8_t)(g * 255.0f)),
                                                                B((uint8_t)(b * 255.0f)),
                                                                A((uint8_t)((a < 0.0f) ? DEFAULT_A : ((uint8_t)roundf(a * 255.0f))))
                                                            #endif
        {}


        /**
         * Constructor from a #fVec3 with components (x=R, y=G, z=B) in [0.0f, 1.0f]. Component A is set
         * to DEFAULT_A (opaque).
         */
        RGB32(fVec3 v) : RGB32(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec4 with components (x=R, y=G, z=B, w=A) in [0.0f, 1.0f].
        */
        RGB32(fVec4 v) : RGB32(v.x, v.y, v.z, v.w)
        {}


        /**
        * Constructor from a uint32_t.
        */
        constexpr RGB32(uint32_t c) : val(c) {}


        /**
        * Constructor from a uint16_t (seen as RGB565). Component A is set to DEFAULT_A.
        */
        constexpr inline RGB32(uint16_t val);


        /**
        * Constructor from a uint64_t (seen as RGB64). 
        */
        constexpr inline RGB32(uint64_t val);


        /**
        * Default Copy constructor.
        */
        RGB32(const RGB32&) = default;


        /**
        * Constructor from a RGB565 color. Component A is set to DEFAULT_A.
        */
        constexpr inline RGB32(const RGB565& c);


        /**
        * Constructor from a RGB24 color. Component A is set to DEFAULT_A.
        */
        constexpr inline RGB32(const RGB24& c);


        /**
        * Constructor from a RGB64 color.
        */
        constexpr inline RGB32(const RGB64& c);


        /**
        * Constructor from a RGBf color. Component A is set to DEFAULT_A.
        */
        constexpr inline RGB32(const RGBf& c);


        /**
        * Constructor from a HSV color. Component A is set to DEFAULT_A.
        */
        RGB32(const HSV& c);


        /**
        * Cast into a uint32_t (non-const reference version)
        */
        explicit operator uint32_t&()  { return val; }


        /**
        * Cast into a uint32_t (const reference version)
        */
        explicit operator const uint32_t& () const { return val; }


        /**
        * Cast into an #iVec3. Raw values (component A is ignored).
        */
        explicit operator iVec3() const { return iVec3(R, G, B); }


        /**
        * Cast into an #fVec3. Values in [0.0f, 1.0f] (component A is ignored).
        */
        explicit operator fVec3() const { return fVec3(R / 255.0f, G / 255.0f, B / 255.0f); }


        /**
        * Cast into an #iVec4. Raw values.
        */
        explicit operator iVec4() const { return iVec4(R, G, B, A); }


        /**
        * Cast into an #fVec4. Values in [0.0f, 1.0f].
        */
        explicit operator fVec4() const { return fVec4(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f); }


        /**
        * Default assignement operator 
        */ 
        RGB32& operator=(const RGB32&) = default;


        /**
        * Assignement operator from a RGB565 color. Component A is set to DEFAULT_A.
        */
        inline RGB32& operator=(const RGB565& c);


        /**
        * Assignement operator from a RGB24 color. Component A is set to DEFAULT_A.
        */
        inline RGB32& operator=(const RGB24& c);


        /**
        * Assignement operator from a RGB64 color
        */
        inline RGB32& operator=(const RGB64& c);


        /**
        * Assignement operator from a RGBf color. Component A is set to DEFAULT_A.
        */
        inline RGB32& operator=(const RGBf& c);


        /**
        * Assignement operator from a HSV color. Component A is set to DEFAULT_A.
        */
        RGB32& operator=(const HSV& c);


        /**
        * Assignement operator from a vector (x=R, y=G, z=B). Raw values. Component A is set to DEFAULT_A.
        */
        inline RGB32& operator=(iVec3 v);


        /**
        * Assignement operator from a vector (x=R, y=G, z=B, w=A). Raw values.
        */
        inline RGB32& operator=(iVec4 v);


        /**
        * Assignement operator from a vector (x=R, y=G, z=B) in [0.0f, 1.0f]. Component A is set to DEFAULT_A.
        */
        inline RGB32& operator=(fVec3 v);


        /**
        * Assignement operator from a vector (x=R, y=G, z=B, w=A) in [0.0f, 1.0f].
        */
        inline RGB32& operator=(fVec4 v);


        /**
        * Add another color, component by component (including alpha channel).
        */
        void operator+=(const RGB32& c)
            {
            R += c.R;
            G += c.G;
            B += c.B;
            A += c.A;
            }


        /**
        * Substract another color, component by component (including alpha channel).
        */
        void operator-=(const RGB32& c)
            {
            R -= c.R;
            G -= c.G;
            B -= c.B;
            A -= c.A;
            }


        /**
        * Add the scalar value v to each component (including alpha channel).
        */
        void operator+=(uint8_t v)
            {
            R += v;
            G += v;
            B += v;
            A += v;
            }


        /**
         * Substract the scalar value v to each component (including alpha channel).
         */
        void operator-=(uint8_t v)
            {
            R -= v;
            G -= v;
            B -= v;
            A -= v;
            }


        /**
        * Multiply each component by the scalar value v (including alpha channel).
        */
        void operator*=(uint8_t v)
            {
            R *= v;
            G *= v;
            B *= v;
            A *= v;
            }


        /**
        * Multiply each component by the scalar (floating point) value v (including alpha channel).
        */
        void operator*=(float v)
            {
            R = (uint8_t)(R * v);
            G = (uint8_t)(G * v);
            B = (uint8_t)(B * v);
            A = (uint8_t)(A * v);
            }

        /**
        * Divide each component by the scalar value v (including alpha channel).
        */
        void operator/=(uint8_t v)
            {
            R /= v;
            G /= v;
            B /= v;
            A /= v;
            }

        /**
        * Divide each component by the scalar (floating point value) v, including the alpha channel.
        */
        void operator/=(float v)
            {
            R = (uint8_t)(R / v);
            G = (uint8_t)(G / v);
            B = (uint8_t)(B / v);
            A = (uint8_t)(A / v);
            }


        /**
        * Equality comparator.
        */
        constexpr bool operator==(const RGB32& c) const
            {
            return(val == c.val);
            }


        /**
        * Inequality comparator.
        */
        constexpr bool operator!=(const RGB32& c) const
            {
            return !(*this == c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   Additional opacity/alpha multiplier in [0.0f,1.0f].
         */
        inline void blend(const RGB32 & fg_col, float alpha)
            {
            blend256(fg_col, (uint32_t)(alpha * 256));
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0,256].
         */
        inline void blend256(const RGB32 & fg_col, uint32_t alpha)
            {
            // below is the correct alpha blending with pre-multiplied alpha
            // we do 'real' interpolate with eternal 'alpha' but not with the 'A' component of fg_col
            // since it is already pre-multiplied
            const uint32_t inv_alpha = (65536 - (alpha * (fg_col.A + (fg_col.A > 127)))) >> 8;
            const uint32_t ag = ((fg_col.val & 0xFF00FF00) >> 8)*alpha  +  ((val & 0xFF00FF00) >> 8)*inv_alpha;
            const uint32_t rb = (fg_col.val & 0x00FF00FF) * alpha + (val & 0x00FF00FF) * inv_alpha;
            val = (ag & 0xFF00FF00) | ((rb >> 8) & 0x00FF00FF);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color. The alpha channel of the color is used for blending.
         */
        inline void blend(RGB32 fg_col)
            {
            blend256(fg_col, 256);  
            }


        /**
         * Multiply each color component by a given factor m/256 with m in [0,256] except the A component.
         */
        inline void mult256(int mr, int mg, int mb)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            }


        /**
        * Multiply each color component by a given factor m/256 with m in [0,256]
        */
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            A = (B * ma) >> 8;
            }


        /**
         * Convert the color from plain alpha to pre-multiplied alpha.
         * 
         * **Remark** All colors type in TGX should have pre-multiplied alpha. This method should only
         * be used when loading a color from external data (a png image for example) where the colors
         * are not initially pre-multiplied.
         */
        inline void premultiply()
            {
            R = (uint8_t)((((uint16_t)R) * A) / 255);
            G = (uint8_t)((((uint16_t)G) * A) / 255);
            B = (uint8_t)((((uint16_t)B) * A) / 255);
            }


        /**
         * Return the opacity (alpha channel value) of this color in the range [0,1] (0=fully
         * transparent, 1=fully opaque).
         */
        float opacity() const
            {
            return (((float)A)/ 255.0f);
            }


        /**
        * Change the opacity of the color to a given value in [0.0f, 1.0f].
        * 
        * This method assumes (and returns) a color with pre-multiplied alpha.
        * 
        * **Remark** Use method multOpacity() instead whenever possible because it is faster. 
        */
        void setOpacity(float op)
            {
            // slow version
            float mo = op * 255.0f;
            float mult = (A == 0) ? 0.0f : (mo / ((float)A));
            (*this) =  RGB32((int)(R * mult), (int)(G * mult), (int)(B * mult), (int)mo);
            }


        /**
        * Multiply the opacity of the color by a given factor in [0.0f, 1.0f].
        * 
        * This method assumes (and returns) a color with pre-multiplied alpha.
        */
        void multOpacity(float op)
            {
            *this = getMultOpacity(op);
            }


        /**
        * Return a copy of this color with opacity multiplied by a given factor in [0.0f, 1.0f].
        *
        * This method assumes (and returns) a color with pre-multiplied alpha.
        */
        RGB32 getMultOpacity(float op) const
            {
            // faster
            uint32_t o = (uint32_t)(256 * op);
            uint32_t ag = (val & 0xFF00FF00) >> 8;
            uint32_t rb = val & 0x00FF00FF;
            uint32_t sag = o * ag;
            uint32_t srb = o * rb;
            sag = sag & 0xFF00FF00;
            srb = (srb >> 8) & 0x00FF00FF;
            return RGB32(sag | srb);
            }


        /**
        * Set the alpha channel of the color to fully opaque.
        */
        void setOpaque()
            {
            setOpacity(1.0f);
            }


        /**
        * Set the alpha channel of the color to fully transparent.
        */
        void setTransparent()
            {
            R = 0;
            G = 0;
            B = 0;
            A = 0;
            }


    };



    /**
    * Interpolate between 3 colors.
    *
    * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
    */
    inline RGB32 interpolateColorsTriangle(const RGB32& col1, int32_t C1, const  RGB32& col2, int32_t C2, const  RGB32& col3, const int32_t totC)
        {
        return RGB32((int)(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC),
                     (int)(col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC),
                     (int)(col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC),
                     (int)(col3.A + (C1 * (col1.A - col3.A) + C2 * (col2.A - col3.A)) / totC));
        }


    /**
     * Bilinear interpolation between 4 colors.
     *
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     *
     *```
     *  C01          C11
     *
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline RGB32 interpolateColorsBilinear(const RGB32 & C00, const RGB32 & C10, const RGB32 & C01, const RGB32 & C11, const float ax, const float ay)
            {           
            const int iax = (int)(ax * 256);
            const int iay = (int)(ay * 256);
            const int rax = 256 - iax;
            const int ray = 256 - iay; 
            const int R = rax*(ray*C00.R + iay*C01.R) + iax*(ray*C10.R + iay*C11.R);
            const int G = rax*(ray*C00.G + iay*C01.G) + iax*(ray*C10.G + iay*C11.G);
            const int B = rax*(ray*C00.B + iay*C01.B) + iax*(ray*C10.B + iay*C11.B);
            const int A = rax*(ray*C00.A + iay*C01.A) + iax*(ray*C10.A + iay*C11.A);
            return RGB32(R >> 16,G >> 16,B >> 16,A >> 16);            
            }


    /**
    * Return the average color between colA and colB.
    */
    inline RGB32 meanColor(RGB32 colA, RGB32 colB)
        {
        return RGB32(((int)colA.R + (int)colB.R) >> 1,
                     ((int)colA.G + (int)colB.G) >> 1,
                     ((int)colA.B + (int)colB.B) >> 1,
                     ((int)colA.A + (int)colB.A) >> 1);
        }


    /**
    * Return the average color between 4 colors.
    */
    inline RGB32 meanColor(RGB32 colA, RGB32 colB, RGB32 colC, RGB32 colD)
        {
        return RGB32(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
                     ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
                     ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2,
                     ((int)colA.A + (int)colB.A + (int)colC.A + (int)colD.A) >> 2);
        }





/**
 * Color in R16/G16/B16/A16 format.
 * 
 * Occupies 8 bytes in memory, aligned as uint64_t.
 * 
 * Can be converted from/to uint64_t.
 * 
 * the component A defaults to DEFAULT_A = 65535 (fully opaque) if not specified.
 * 
 * **REMARK** For all drawing/blending procedure, the color is assume to have pre-multiplied
 * alpha. Use the premultiply() method to convert a plain alpha color to its pre-multiplied
 * version (https://developer.nvidia.com/content/alpha-blending-pre-or-not-pre).
 */
struct RGB64
    {

        
        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_RGB64.inl>
        #endif
        
        
        static const uint16_t DEFAULT_A = 65535;  ///< fully opaque alpha value


        // dual memory representation
        union 
            { 
            uint64_t val;       ///< color as uint64_t
            struct 
                {

#if TGX_RGB64_ORDER_BGR
                uint16_t B;     ///< Blue channel (16 bits)
                uint16_t G;     ///< Green channel (16 bits)
                uint16_t R;     ///< Red channel (16 bits)
                uint16_t A;     ///< Alpha channel (16 bits)

#else
                uint16_t R;     ///< Red channel (16 bits)
                uint16_t G;     ///< Green channel (16 bits)
                uint16_t B;     ///< Blue channel (16 bits)
                uint16_t A;     ///< Alpha channel (16 bits)
#endif
                }; 
            };


        /**
        * Default constructor. **Color is undefined**
        */
        RGB64() = default;


        /**
        * Constructor from raw r,g,b,a value in [0,65535].
        */
        constexpr RGB64(int r, int g, int b, int a = DEFAULT_A) :       
                                                                #if TGX_RGB64_ORDER_BGR
                                                                    B((uint16_t)b), G((uint16_t)g), R((uint16_t)r), A((uint16_t)a) 
                                                                #else
                                                                    R((uint16_t)r), G((uint16_t)g), B((uint16_t)b), A((uint16_t)a)
                                                                #endif
        {}


        /**
        * Constructor from a #iVec3 with components (x=R, y=G, z=B) in [0,65535]. Component A is set to DEFAULT_A.
        */
        constexpr RGB64(iVec3 v) : RGB64(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #iVec4 with components (x=R, y=G, z=B, w=A) in [0,65535].
        */
        constexpr RGB64(iVec4 v) : RGB64(v.x, v.y, v.z, v.w)
        {}


        /**
        * Constructor from float r,g,b,a in [0.0f, 1.0f]. If unspecified, component A is set to DEFAULT_A. 
        */
        RGB64(float r, float g, float b, float a = -1.0f) : 
                                                            #if TGX_RGB64_ORDER_BGR
                                                                B((uint16_t)(b * 65535)), 
                                                                G((uint16_t)(g * 65535)),
                                                                R((uint16_t)(r * 65535)),
                                                                A((uint16_t)((a < 0.0f) ? DEFAULT_A : a * 65535))
                                                            #else           
                                                                R((uint16_t)(r * 65535)),
                                                                G((uint16_t)(g * 65535)),
                                                                B((uint16_t)(b * 65535)),
                                                                A((uint16_t)((a < 0.0f) ? DEFAULT_A : a * 65535))
                                                            #endif          
        {}


        /**
        * Constructor from a #fVec3 with components (x=R, y=G, z=B) in [0.0f, 1.0f]. Component A is set to DEFAULT_A.
        */
        RGB64(fVec3 v) : RGB64(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec4 with components (x=R, y=G, z=B, w=A) in [0.0f, 1.0f].
        */
        RGB64(fVec4 v) : RGB64(v.x, v.y, v.z, v.w)
        {}


        /**
        * Constructor from a uint64_t.
        */
        constexpr inline RGB64(uint64_t c) : val(c) {}


        /**
        * Constructor from a uint16_t (seen as RGB565).
        */
        inline RGB64(uint16_t val);


        /**
        * Constructor from a uint32_t (seen as RGB32).
        */
        inline RGB64(uint32_t val);


        /**
        * Default Copy ctor.
        */
        RGB64(const RGB64&) = default;


        /**
        * Constructor from a RGB565 color. Component A is set to DEFAULT_A.
        */
        inline RGB64(const RGB565& c);


        /**
        * Constructor from a RGB24 color. Component A is set to DEFAULT_A.
        */
        inline RGB64(const RGB24& c);


        /**
        * Constructor from a RGB32 color.
        */
        inline RGB64(const RGB32& c);


        /**
        * Constructor from a RGBf color. Component A is set to DEFAULT_A.
        */
        inline RGB64(const RGBf& c);


        /**
        * Constructor from a HSV color. Component A is set to DEFAULT_A
        */
        RGB64(const HSV& c);


        /**
        * Cast into a uint64_t (non-const reference version)
        */
        explicit operator uint64_t&()  { return val; }


        /**
        * Cast into a uint64_t (const reference version)
        */
        explicit operator const uint64_t&() const { return val; }


        /**
        * Cast into an #iVec3 with components (x=R, y=G, z=B) in [0,65535]. Component A is ignored.
        */
        explicit operator iVec3() const { return iVec3(R, G, B); }


        /**
        * Cast into an #fVec3 with components (x=R, y=G, z=B) in [0.0f, 1.0f]. Component A is ignored.
        */
        explicit operator fVec3() const { return fVec3(R / 65535.0f, G / 65535.0f, B / 65535.0f); }


        /**
        * Cast into an #iVec4 with components (x=R, y=G, z=B, w=A) in [0,65535].
        */
        explicit operator iVec4() const { return iVec4(R, G, B, A); }


        /**
        * Cast into an #fVec4 with components (x=R, y=G, z=B, w=A) in [0.0f, 1.0f].
        */
        explicit operator fVec4() const { return fVec4(R / 65535.0f, G / 65535.0f, B / 65535.0f, A / 65535.0f); }


        /**
        * Default assignement operator.
        */ 
        RGB64& operator=(const RGB64&) = default;


        /**
        * Assignement operator from a RGB24 color. Component A is set to DEFAULT_A.
        */
        inline RGB64& operator=(const RGB565& c);


        /**
        * Assignement operator from a RGB24 color. Component A is set to DEFAULT_A.
        */
        inline RGB64& operator=(const RGB24& c);


        /**
        * Assignement operator from a RGB32 color.
        */
        inline RGB64& operator=(const RGB32& c);


        /**
        * Assignement operator from a RGBf color. Component A is set to DEFAULT_A.
        */
        inline RGB64& operator=(const RGBf& c);


        /**
        * Assignement operator from a HSV color. Component A is set to DEFAULT_A.
        */
        RGB64& operator=(const HSV& c);


        /**
        * Assignement operator from a #iVec3 with components (x=R, y=G, z=B) in [0,65535]. Component A is set to DEFAULT_A.
        */
        inline RGB64& operator=(iVec3 v);


        /**
        * Assignement operator from a #iVec4 with components (x=R, y=G, z=B, w=A) in [0,65535].
        */
        inline RGB64& operator=(iVec4 v);


        /**
        * Assignement operator from a #fVec3 with components (x=R, y=G, z=B) in [0.0f,1.0f]. Component A is set to DEFAULT_A.
        */
        inline RGB64& operator=(fVec3 v);


        /**
        * Assignement operator from a #fVec3 with components (x=R, y=G, z=B, w=A) in [0.0f,1.0f].
        */
        inline RGB64& operator=(fVec4 v);


        /**
        * Add another color, component by component, including the alpha channel.
        */
        void operator+=(const RGB64& c)
            {
            R += c.R;
            G += c.G;
            B += c.B;
            A += c.A;
            }


        /**
        * Substract another color, component by component, including the alpha channel.
        */
        void operator-=(const RGB64& c)
            {
            R -= c.R;
            G -= c.G;
            B -= c.B;
            A -= c.A;
            }


        /**
        * Add the scalar value v to each component, including the alpha channel.
        */
        void operator+=(uint16_t v)
            {
            R += v;
            G += v;
            B += v;
            A += v;
            }


        /**
         * Substract the scalar value v to each component, including the alpha channel.
         */
        void operator-=(uint16_t v)
            {
            R -= v;
            G -= v;
            B -= v;
            A -= v;
            }


        /**
        * Multiply each component by the scalar value v, including the alpha channel.
        */
        void operator*=(uint16_t v)
            {
            R *= v;
            G *= v;
            B *= v;
            A *= v;
            }


        /**
        * Multiply each component by the scalar (floating point) value v, including the alpha channel.
        */
        void operator*=(float v)
            {
            R = (uint16_t)(R * v);
            G = (uint16_t)(G * v);
            B = (uint16_t)(B * v);
            A = (uint16_t)(A * v);
            }

        /**
        * Divide each component by the scalar value v, including the alpha channel.
        */
        void operator/=(uint16_t v)
            {
            R /= v;
            G /= v;
            B /= v;
            A /= v;
            }

        /**
        * Divide each component by the scalar (floating point value) v, including the alpha channel.
        */
        void operator/=(float v)
            {
            R = (uint16_t)(R / v);
            G = (uint16_t)(G / v);
            B = (uint16_t)(B / v);
            A = (uint16_t)(A / v);
            }


        /**
        * Equality comparator.
        */
        constexpr bool operator==(const RGB64& c) const
            {
            return(val == c.val);
            }


        /**
        * Inequality comparator.
        */
        constexpr bool operator!=(const RGB64& c) const
            {
            return !(*this == c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   Additional opacity/alpha multiplier in [0.0f,1.0f].
         */
        inline void blend(const RGB64& fg_col, float alpha)
            {
            blend65536(fg_col, (uint32_t)(alpha * 65536));
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 256 (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed o have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0,256].
         */
        inline void blend256(const RGB64 & fg_col, uint32_t alpha)
            {
            blend65536(fg_col, (alpha << 8));
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 65536 (fully opaque).
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0,65536].
         */
        inline void blend65536(RGB64 fg_col, uint32_t alpha)
            {            
            // correct version where 'alpha' is interpolated normally but 'A' is only multiplied 
            // with background because the color is assumed pre-multiplied             
            alpha >>= 1; // alpha in [0,32768]
            const uint32_t inv_alpha = (2147483648 - (alpha * (((uint32_t)fg_col.A) + (fg_col.A > 32767)))) >> 16;
            R = (uint16_t)((((uint32_t)fg_col.R) * alpha + ((uint32_t)R) * inv_alpha) >> 15);
            G = (uint16_t)((((uint32_t)fg_col.G) * alpha + ((uint32_t)G) * inv_alpha) >> 15);
            B = (uint16_t)((((uint32_t)fg_col.B) * alpha + ((uint32_t)B) * inv_alpha) >> 15);
            A = (uint16_t)((((uint32_t)fg_col.A) * alpha + ((uint32_t)A) * inv_alpha) >> 15);
            }


        /**
         * alpha-blend `fg_col` over this one.
         * 
         * **WARNING** The color fg_col is assumed to have pre-multiplied alpha.
         *
         * @param   fg_col  The foreground color. The alpha channel of the color is used for blending.
         */
        inline void blend(const RGB64 & fg_col)
            {
            blend65536(fg_col, 65536); 
            }


        /**
         * Multiply each color component by a given factor m/256 with m in [0,256]. Component A is left unchanged.
         */
        inline void mult256(int mr, int mg, int mb)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            }


        /**
        * Multiply each color component by a given factor m/256 with m in [0,256].
        **/
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            R = (R * mr) >> 8;
            G = (G * mg) >> 8;
            B = (B * mb) >> 8;
            A = (A * mb) >> 8;
            }


        /**
         * Convert the color from plain alpha to pre-multiplied alpha.
         * 
         * **Remark** All colors type in TGX should have pre-multiplied alpha. This method should only
         * be used when loading a color from external data (a png image for example) where the colors
         * are not initially pre-multiplied.
         */
        inline void premultiply()
            {
            R = (uint16_t)((((uint32_t)R) * A) / 65535);
            G = (uint16_t)((((uint32_t)G) * A) / 65535);
            B = (uint16_t)((((uint32_t)B) * A) / 65535);
            }


        /**
         * Return the opacity (alpha channel value) of this color in the range [0,1] (0=fully
         * transparent, 1=fully opaque).
         */
        float opacity() const
            {
            return (((float)A) / 65535.0f);
            }


        /**
        * Change the opacity of the color to a given value in [0.0f, 1.0f].
        *
        * This method assumes (and returns) a color with pre-multiplied alpha.
        *
        * **Remark** Use multOpacity() instead whenever possible because it is faster.
        */
        void setOpacity(float op)
            {
            // slow version
            float mo = op * 65535.0f;
            float mult = (A == 0) ? 0.0f : (mo / ((float)A));
            (*this) = RGB64((int)(R * mult), (int)(G * mult), (int)(B * mult), (int)mo);
            }


        /**
        * Multiply the opacity of the color by a given factor in [0.0f, 1.0f].
        *
        * This method assumes (and returns) a color with pre-multiplied alpha.
        */
        void multOpacity(float op)
            {
            *this = getMultOpacity(op);
            }


        /**
        * Return a copy of this color with opacity multiplied by a given factor in [0.0f, 1.0f].
        *
        * This method assumes (and returns) a color with pre-multiplied alpha.
        */
        RGB64 getMultOpacity(float op) const
            {
            return RGB64((int)(R * op), (int)(G * op), (int)(B * op), (int)(A * op));
            }


        /**
        * Set the alpha channel of the color to fully opaque.
        */
        void setOpaque()
            {
            setOpacity(1.0f);
            }


        /**
        * Set the alpha channel of the color to fully transparent.
        */
        void setTransparent()
            {
            R = 0;
            G = 0;
            B = 0;
            A = 0; 
            }


    };




    /**
    * Interpolate between 3 colors.
    *
    * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
    */
    inline RGB64 interpolateColorsTriangle(const RGB64& col1, int32_t C1, const  RGB64& col2, int32_t C2, const  RGB64& col3, const int32_t totC)
        {
        // forward to RGB32 (, maybe improve this but is it worth it, the method is never used ?)
        return RGB64(interpolateColorsTriangle(RGB32(col1), C1, RGB32(col2), C2, RGB32(col3), totC));
        }


    /**
     * Bilinear interpolation between 4 colors.
     *
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     *
     *```
     *  C01          C11
     *
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline RGB64 interpolateColorsBilinear(const RGB64 & C00, const RGB64 & C10, const RGB64 & C01, const RGB64 & C11, const float ax, const float ay)
            {
            // let's use floating point version for max accuraccy, RGB64 is slow anyway...
            const float rax = 1.0f - ax;
            const float ray = 1.0f - ay;            
            const int R = (int)roundf(rax*(ray*C00.R + ay*C01.R) + ax*(ray*C10.R + ay*C11.R));
            const int G = (int)roundf(rax*(ray*C00.G + ay*C01.G) + ax*(ray*C10.G + ay*C11.G));
            const int B = (int)roundf(rax*(ray*C00.B + ay*C01.B) + ax*(ray*C10.B + ay*C11.B));
            const int A = (int)roundf(rax*(ray*C00.A + ay*C01.A) + ax*(ray*C10.A + ay*C11.A));
            return RGB64(R,G,B,A);
            }


    /**
    * Return the average color between colA and colB.
    */
    inline RGB64 meanColor(const RGB64 & colA, const RGB64 & colB)
        {
        return RGB64(((int)colA.R + (int)colB.R) >> 1,
                     ((int)colA.G + (int)colB.G) >> 1,
                     ((int)colA.B + (int)colB.B) >> 1,
                     ((int)colA.A + (int)colB.A) >> 1);
        }


    /**
    * Return the average color between 4 colors.
    */
    inline RGB64 meanColor(const RGB64& colA, const RGB64& colB, const RGB64& colC, const RGB64& colD)
        {
        return RGB64(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
                     ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
                     ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2,
                     ((int)colA.A + (int)colB.A + (int)colC.A + (int)colD.A) >> 2);
        }





    /**
     * Color in R,G,B float format.
     * 
     * Occupies 4*3 = 12 bytes in memory, aligned as float.
     * 
     * - There is no alpha channel.
     * - Useful for high precision computation. This color format is used internally by the 3D
     * rasterizer for all color interpolation/shading.
     */
    struct RGBf
    {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_RGBf.inl>
        #endif


#if TGX_RGBf_ORDER_BGR
        float B;    ///< Blue channel
        float R;    ///< Red channel
#else 
        float R;    ///< Red channel
        float G;    ///< Green channel
        float B;    ///< Blue channel
#endif


        /**
        * Default constructor. **Color is undefined**.
        */
        RGBf() = default;


        /**
        * Constructor from raw r,g,b values in [0.0f, 1.0f]
        **/
        constexpr RGBf(float r, float g, float b) :
        #if TGX_RGBf_ORDER_BGR
            B(b), G(g), R(r)
        #else
            R(r), G(g), B(b)
        #endif
        {}


        /**
        * Constructor from a #fVec3 with components (x=R, y=G, z=B) in [0.0f, 1.0f].
        */
        constexpr RGBf(fVec3 v) : RGBf(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec4 with components (x=R, y=G, z=B, w=ignored) in [0.0f, 1.0f].
        */
        constexpr RGBf(fVec4 v) : RGBf(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a uint16_t (seen as RGB565).
        */
        constexpr inline RGBf(uint16_t val);


        /**
        * Constructor from a uint32_t (seen as RGB32, the A component is ignored).
        */
        constexpr inline RGBf(uint32_t val);


        /**
        * Constructor from a uint64_t (seen as RGB64, the A component is ignored).
        */
        constexpr inline RGBf(uint64_t val);


        /**
        * Default Copy constructor.
        */
        constexpr RGBf(const RGBf&) = default;


        /**
        * Constructor from a RGB565 color.
        */
        constexpr inline RGBf(const RGB565& c);


        /**
        * Constructor from a RGB24 color.
        */
        constexpr inline RGBf(const RGB24& c);


        /**
        * Constructor from a RGB32 color. The A component of c is ignored.
        */
        constexpr inline RGBf(const RGB32& c);


        /**
        * Constructor from a RGB64 color. The A component of c is ignored.
        */
        constexpr inline RGBf(const RGB64& c);


        /**
        * Constructor from a HSV color.
        */
        RGBf(const HSV& c);


        /**
        * Cast into an #fVec3 in [0.0f, 1.0f].
        */
        explicit operator fVec3() const { return fVec3(R, G, B); }


        /**
        * Default assignement operator.
        */
        RGBf& operator=(const RGBf&) = default;


        /**
        * Assignement operator from a RGB565 color.
        */
        inline RGBf& operator=(const RGB565& c);


        /**
        * Assignement operator from a RGB24 color.
        */
        inline RGBf& operator=(const RGB24& c);


        /**
        * Assignement operator from a RGB32 color. The component A is ignored.
        */
        inline RGBf& operator=(const RGB32& c);


        /**
        * Assignement operator from a RGB64 color. The component A is ignored.
        */
        inline RGBf& operator=(const RGB64& c);


        /**
        * Assignement operator from a HSV color.
        */
        RGBf& operator=(const HSV& c);


        /**
        * Assignement operator from a #fVec3 with components (x=R, y=G, z=B) in [0.0f, 1.0f].
        */
        inline RGBf& operator=(fVec3 v);


        /**
        * Assignement operator from a #fVec4 with components (x=R, y=G, z=B, w=ignored) in [0.0f, 1.0f].
        */
        inline RGBf& operator=(fVec4 v);


        /**
        * Add another color, component by component.
        */
        void operator+=(const RGBf& c)
            {
            R += c.R;
            G += c.G;
            B += c.B;
            }


        /**
        * Substract another color, component by component.
        */
        void operator-=(const RGBf& c)
            {
            R -= c.R;
            G -= c.G;
            B -= c.B;
            }


        /**
        * Multiply each channel by the same channel on c. 
        */
        inline void operator*=(const RGBf & c)
            {
            R *= c.R;
            G *= c.G;
            B *= c.B;
            }

        /**
        * Return the color obtained by multipliying the channels of both colors together.
        */
        inline RGBf operator*(const RGBf & c) const 
            {
            return RGBf(R * c.R, G * c.G, B * c.B);
            }

        /**
        * Multiply each channel by a constant
        */
        inline void operator*=(float a)
            {
            R *= a;
            G *= a;
            B *= a;
            }


        /**
        * Return the color where all components are multiplied by a
        */
        inline RGBf operator*(float a) const
            {
            return RGBf(R * a, G * a, B * a);
            }
    

        /**
        * Equality comparator.
        */
        constexpr bool operator==(const RGBf & c) const
            {
            return((R == c.R)&&(G == c.G)&&(B == c.B));
            }


        /**
        * Inequality comparator.
        */
        constexpr bool operator!=(const RGBf & c) const
            {
            return !(*this == c);
            }


        /**
        * Clamp all color channel to [0.0f,1.0f].
        */
        inline void clamp()
            {
            R = tgx::clamp(R, 0.0f, 1.0f);
            G = tgx::clamp(G, 0.0f, 1.0f);
            B = tgx::clamp(B, 0.0f, 1.0f);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 256 (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0,256].
         */
        inline void blend256(const RGB64& fg_col, uint32_t alpha)
            {
            blend(fg_col, alpha / 256.0f);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0.0f, 1.0f].
         */
        inline void blend(const RGBf & fg_col, float alpha)
            {
            R += (fg_col.R - R) * alpha;
            G += (fg_col.G - G) * alpha;
            B += (fg_col.B - B) * alpha;
            }


        /**
        * multiply each color component by a given factor m/256 with m in [0,256].
        */
        inline void mult256(int mr, int mg, int mb)
            {
            R = (R * mr)/256;
            G = (G * mg)/256;
            B = (B * mb)/256;
            }


        /**
         * multiply each color component by a given factor m/256 with m in [0,256]. Parameter ma is
         * ignored since there is not alpha channel.
         */
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            mult256(mr, mg, mb);
            }


        /**         
        * Dummy function for compatibility with color types having an alpha channel.
        * 
        * Does nothing since the color is always fully opaque. 
        */
        inline void premultiply()
            {
            // nothing here. 
            return;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Return 1.0f (fully opaque)
        */
        float opacity() const
            {
            return 1.0f;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Does nothing since the color is always fully opaque.
        */
        void setOpacity(float op)
            {
            // nothing here. 
            return;
            }


    };



    /**
    * Interpolate between 2 colors.
    *
    * Return the color  col1 + alpha*(col2 - col1)
    */
    inline RGBf interpolate(const RGBf& col1, const  RGBf& col2, float alpha)
        {
        return RGBf(col1.R + alpha * (col2.R - col1.R),
                    col1.G + alpha * (col2.G - col1.G),
                    col1.B + alpha * (col2.B - col1.B));
        }


    /**
     * Interpolate between 3 colors.
     * 
     * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
     */
    inline RGBf interpolateColorsTriangle(const RGBf & col1, int32_t C1, const  RGBf & col2, int32_t C2, const  RGBf & col3, int32_t totC)
        {
        return RGBf(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC,
                    col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC,
                    col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC);
        }


    /**
     * Bilinear interpolation between 4 colors.
     *
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     *
     *```
     *  C01          C11
     *
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline RGBf interpolateColorsBilinear(const RGBf & C00, const RGBf & C10, const RGBf & C01, const RGBf & C11, const float ax, const float ay)
            {
            const float rax = 1.0f - ax;
            const float ray = 1.0f - ay;            
            const float R = rax*(ray*C00.R + ay*C01.R) + ax*(ray*C10.R + ay*C11.R);
            const float G = rax*(ray*C00.G + ay*C01.G) + ax*(ray*C10.G + ay*C11.G);
            const float B = rax*(ray*C00.B + ay*C01.B) + ax*(ray*C10.B + ay*C11.B);
            return RGBf(R,G,B);
            }
            
            
    /**
    * Return the average color between colA and colB.
    */
    inline RGBf meanColor(const RGBf & colA, const  RGBf & colB)
        {
        return RGBf((colA.R + colB.R)/2, (colA.G + colB.G)/2, (colA.B + colB.B)/2);
        }


    /**
    * Return the average color between 4 colors.
    */
    inline RGBf meanColor(const RGBf & colA, const  RGBf & colB, const  RGBf & colC, const  RGBf & colD)
        {
        return RGBf((colA.R + colB.R + colC.R + colD.R) / 4, (colA.G + colB.G + colC.G + colD.G) / 4, (colA.B + colB.B + colC.B + colD.B) / 4);
        }





/**
 * Color in H/S/V format [**experimental**].
 * 
 * The color is stored in Hue/Saturation/Value color space. Each component is a float in `[0,1.0f]`.
 * The total size of the object is 4*3 = 12 bytes, aligned as float.
 * 
 * See: https://en.wikipedia.org/wiki/HSL_and_HSV
 * 
 * @warning **Experimental** Operations with HSV colors are very slow. This color format should
 * not be used with the 3D rasterizer.
 */
struct HSV
    {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Color_HSV.inl>
        #endif

        // color components
        float H;  ///< Hue in [0.0f ,1.0f]
        float S;  ///< Saturation in [0;0f, 1.0f]
        float V;  ///< Value in [0.F ,1.0f]


        /**
        * Default constructor. **Color is undefined**.
        */
        HSV() = default;


        /**
        * Constructor from raw h, s, v values in [0.0f, 1.0f]
        */
        constexpr HSV(float h, float s, float v) : H(h), S(s), V(v) {}


        /**
        * Constructor from a #fVec3 with components (x=H, y=S, z=V) in [0.0f, 1.0f].
        */
        constexpr HSV(fVec3 v) : HSV(v.x, v.y, v.z)
        {}


        /**
        * Constructor from a #fVec4  with components (x=H, y=S, z=V, w=ignored) in [0.0f, 1.0f].
        */
        constexpr HSV(fVec4 v) : HSV(v.x, v.y, v.z)
        {}


        /**
        * Default Copy constructor.
        */
        HSV(const HSV&) = default;


        /**
        * Constructor from a uint16_t (seen as RGB565).
        */
        HSV(uint16_t val);


        /**
        * Constructor from a uint32_t (seen as RGB32, component A is ignored).
        */
        HSV(uint32_t val);


        /**
        * Constructor from a uint64_t (seen as RGB64, component A is ignored).
        */
        HSV(uint64_t val);


        /**
        * Constructor from a RGB565 color.
        */
        HSV(const RGB565& c);


        /**
        * Constructor from a RGB24 color.
        */
        HSV(const RGB24 & c);


        /**
        * Constructor from a RGB32 color. The component A is ignored.
        */
        HSV(const RGB32 & c);


        /**
        * Constructor from a RGB64 color. The component A is ignored.
        */
        HSV(const RGB64 & c);


        /**
        * Constructor from a RGBf color.
        */
        HSV(const RGBf& c);


        /**
        * Cast into an #fVec3 with components (x=H, y=S, z=V) in [0.0f, 1.0f].
        */
        explicit operator fVec3() const { return fVec3(H, S, V); }


        /** 
        * Default assignement operator.
        */
        HSV& operator=(const HSV&) = default;


        /**
        * Assignement operator from a RGB565 color.
        */
        HSV& operator=(const RGB565& c);


        /**
        * Assignement operator from a RGB24 color.
        */
        HSV& operator=(const RGB24 & c);


        /**
        * Assignement operator from a RGB32 color. The component A is ignored.
        */
        HSV& operator=(const RGB32 & c);


        /**
        * Assignement operator from a RGB64 color. The component A is ignored.
        */
        HSV& operator=(const RGB64 & c);


        /**
        * Assignement operator from a RGBf color.
        */
        HSV& operator=(const RGBf& c);


        /**
        * Assignement operator from a #fVec3 with components (x=H, y=S, z=V) in [0.0f, 1.0f].
        */
        inline HSV& operator=(fVec3 v)
            {
            H = v.x;
            S = v.y;
            V = v.z;
            return *this;
            }


        /**
        * Assignement operator from a #fVec4 with components (x=H, y=S, z=V, w=ignored) in [0.0f, 1.0f].
        */
        inline HSV& operator=(fVec4 v)
            {
            H = v.x;
            S = v.y;
            V = v.z;
            return *this;
            }


        /**
        * Equality comparator.
        */
        constexpr bool operator==(const HSV & c) const
            {
            return((H == c.H)&&(S == c.S)&&(V == c.V));
            }


        /**
        * Inequality comparator.
        */
        constexpr bool operator!=(const HSV & c) const
            {
            return !(*this == c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the range 0.0f (fully transparent)
         * to 1.0f (fully opaque).
         * 
         * Blending is done by converting the color to RGB space, then alpha blending, and then
         * converting back to HSV.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0.0f, 1.0f].
         */
        inline void blend(const HSV & fg_col, float alpha)
            {
            // is there a natural blending formula in HSV space ? 
            // let us do it in RGB space for the time being             
            RGBf c(*this);
            c.blend(RGBf(fg_col), alpha);
            *this = HSV(c);
            }


        /**
         * alpha-blend `fg_col` over this one with a given opacity in the integer range 0 (fully
         * transparent) to 256 (fully opaque).
         * 
         * Blending is done by converting the color to RGB space, then alpha blending, and then
         * converting back to HSV.
         *
         * @param   fg_col  The foreground color.
         * @param   alpha   The opacity/alpha multiplier in [0, 256].
         */
        inline void blend256(const HSV & fg_col, uint32_t alpha)
            {
            blend(fg_col, (alpha / 256.0f));
            }


        /**
         * Multiply each color component by factor (m/256) with m in [0,256].
         * 
         * Uses the R,G,B components for compatibility (and just forward to the corresponding RGBf
         * method).
         */
        inline void mult256(int mr, int mg, int mb)
            {
            RGBf c = RGBf(*this); 
            c.mult256(mr, mg, mb);
            *this = HSV(c);
            }


        /**
         * Multiply each color component by a given factor x/256 with x in [0,256]. Parameter ma is
         * ignored since there is not alpha channel.
         * 
         * Uses the R,G,B components for compatibility (and just forward to the corresponding RGBf
         * method).
         */
        inline void mult256(int mr, int mg, int mb, int ma)
            {
            mult256(mr, mg, mb);
            }


        /**         
        * Dummy function for compatibility with color types having an alpha channel.
        * 
        * Does nothing since the color is always fully opaque. 
        */
        inline void premultiply()
            {
            // nothing here. 
            return;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Return 1.0f (fully opaque)
        */
        float opacity() const
            {
            return 1.0f;
            }


        /**
        * Dummy function for compatibility with color types having an alpha channel.
        *
        * Does nothing since the color is always fully opaque.
        */
        void setOpacity(float op)
            {
            // nothing here. 
            return;
            }

    };



    /**
    * Interpolate between 3 colors.
    *
    * Return the color (C1*col1 + C2*col2 + (totC-C1-C2)*col3) / totC.
    */
    inline HSV interpolateColorsTriangle(HSV col1, int32_t C1, HSV col2, int32_t C2, HSV col3, int32_t totC)
        {
        return HSV(interpolateColorsTriangle(RGBf(col1), C1, RGBf(col2), C2, RGBf(col3), totC));
        }


    /**
     * Bilinear interpolation between 4 colors.
     *
     * Return the bilinear interpolation of four neighouring pixels in an image with respect to
     * position X where ax and ay are in [0.0f,1.0f] and represent the distance to the mininum
     * coord. in direction x and y, as illustrated in the drawing below:
     *
     *```
     *  C01          C11
     *
     *   --ax--X
     *         |
     *         ay
     *  C00    |     C10
     *```
     */
    inline HSV interpolateColorsBilinear(const HSV & C00, const HSV & C10, const HSV & C01, const HSV & C11, const float ax, const float ay)
        {
        // just forward to RGBf          ..
        return HSV(interpolateColorsBilinear(RGBf(C00), RGBf(C10), RGBf(C01), RGBf(C11), ax, ay));
        }
            

    /**
    * Return the average color between colA and colB.
    */
    inline HSV meanColor(const  HSV & colA, const  HSV & colB)
        {
        // hum.. what does that mean in HSV, let's do it in RGB color space instead
        return HSV(meanColor(RGBf(colA), RGBf(colB)));
        }


    /**
    * Return the average color between 4 colors.
    */
    inline HSV meanColor(const  HSV& colA, const  HSV& colB, const  HSV& colC, const  HSV& colD)
        {
        // hum.. what does that mean in HSV, let's do it in RGB color space instead
        return HSV(meanColor(RGBf(colA), RGBf(colB), RGBf(colC), RGBf(colD)));
        }


}



#include "Color.inl"


#endif

#endif


/* end of file */

