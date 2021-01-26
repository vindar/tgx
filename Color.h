/** @file Color.h */
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

#pragma once

#include "Misc.h"

#include <stdint.h>
#include <math.h>
#include <type_traits>


namespace tgx
{


/**********************************************************************
* Color types. 
* 
* RGB565, RGB32 and RGB64 are wrappers around basic integer types so
* they can be used as drop in remplacement of uint16_t, uint32_t and 
* uint64_t without any speed penalty.
* 
* RGB24, RGBf, HSV are 'struct-like' and their alignement is that of the 
* color component type (ie 1 byte and 4 bytes). 
* 
* Fast conversion is implemented between color types (and also integer
* types) except when converting from/to HSV.
***********************************************************************/


/** For each RGB color type, we decide separately whether color components 
    are ordered in memory as R,G,B or B,G,R. 
	default ordering can overridden be #definining the constants below before
	including this header */

#ifndef TGX_RGB565_ORDER_BGR
#define TGX_RGB565_ORDER_BGR 1		// for compatibility with adafruit and most SPI display
#endif

#ifndef TGX_RGB24_ORDER_BGR
#define TGX_RGB24_ORDER_BGR 0
#endif

#ifndef TGX_RGB32_ORDER_BGR
#define TGX_RGB32_ORDER_BGR 1		// for compatibility with my mtools library.
#endif

#ifndef TGX_RGB64_ORDER_BGR
#define TGX_RGB64_ORDER_BGR 0
#endif

#ifndef TGX_RGBf_ORDER_BGR
#define TGX_RGBf_ORDER_BGR 0
#endif



/** Forward declarations */

struct RGB565;	// color in 16-bit R5/G6/B5 format (2 bytes aligned) - convertible to/from uint16_t

struct RGB24;	// color in 24-bit R8/G8/B8/ format (unaligned !). 

struct RGB32;	// color in 32-bit R8/G8/B8/(A8) format (4 bytes aligned) - convertible to/from uint32_t

struct RGB64;	// color in 64 bit R16/G16/B16/(A16) format (8 bytes aligned) - convertible to/from uint64_t

struct RGBf;	// color in  RGB float format (4 bytes aligned). 

struct HSV;		// color in H/S/V float format (4 bytes aligned). 



/** check if a type T is one of the color types declared above */
template<typename T> struct is_color
	{
	static const bool value = (std::is_same<T, RGB565>::value)||
		                      (std::is_same<T, RGB24>::value) ||
	                          (std::is_same<T, RGB32>::value) ||
		                      (std::is_same<T, RGB64>::value) ||
		                      (std::is_same<T, RGBf>::value) ||
							  (std::is_same<T, HSV>::value);
	};



/** Predefined colors in RGB32 format */
extern const RGB32 RGB32_Black;
extern const RGB32 RGB32_White;
extern const RGB32 RGB32_Red;
extern const RGB32 RGB32_Blue;
extern const RGB32 RGB32_Green;
extern const RGB32 RGB32_Purple;
extern const RGB32 RGB32_Orange;
extern const RGB32 RGB32_Cyan;
extern const RGB32 RGB32_Lime;
extern const RGB32 RGB32_Salmon;
extern const RGB32 RGB32_Maroon;
extern const RGB32 RGB32_Yellow;
extern const RGB32 RGB32_Magenta;
extern const RGB32 RGB32_Olive;
extern const RGB32 RGB32_Teal;
extern const RGB32 RGB32_Gray;
extern const RGB32 RGB32_Silver;
extern const RGB32 RGB32_Navy;


/** Predefined colors in RGB565 format */
extern const RGB565 RGB565_Black;
extern const RGB565 RGB565_White;
extern const RGB565 RGB565_Red;
extern const RGB565 RGB565_Blue;
extern const RGB565 RGB565_Green;
extern const RGB565 RGB565_Purple;
extern const RGB565 RGB565_Orange;
extern const RGB565 RGB565_Cyan;
extern const RGB565 RGB565_Lime;
extern const RGB565 RGB565_Salmon;
extern const RGB565 RGB565_Maroon;
extern const RGB565 RGB565_Yellow;
extern const RGB565 RGB565_Magenta;
extern const RGB565 RGB565_Olive;
extern const RGB565 RGB565_Teal;
extern const RGB565 RGB565_Gray;
extern const RGB565 RGB565_Silver;
extern const RGB565 RGB565_Navy;




/**********************************************************************
* declaration of class RGB565
***********************************************************************/


/**
* Class representing a color in R5/G6/B5 format. 
* Occupies 2 bytes in memory 
* Can be converted from/to uint16_t.
* 
* This type is compatible with adafruit gfx library 
* and is used with most SPI display. 
**/
struct RGB565
	{

		union // dual memory representation
			{ 
			uint16_t val;			// as a uint16_t

			struct {				// as 3 components R,G,B
#if TGX_RGB565_ORDER_BGR
				uint16_t B : 5,
					     G : 6,
				         R : 5; 
#else
				uint16_t R : 5,
     					 G : 6,
		    			 B : 5; 
#endif
				}; 
			};


		/**
		* Default ctor. Undefined color. 
		**/
		RGB565() = default;


		/**
		* Ctor from raw R,G,B value. 
		* No rescaling: r in 0..31
		*               g in 0..63
		*               b in 0..31
		**/
		constexpr RGB565(int r, int g, int b) : 
											#if TGX_RGB565_ORDER_BGR
												B((uint8_t)b), G((uint8_t)g), R((uint8_t)r)
											#else
												R((uint8_t)r), G((uint8_t)g), B((uint8_t)b)
											#endif
		{}

		
		/**
		* Ctor from float r,g,b in [0.0f, 1.0f].
		**/
		RGB565(float r, float g, float b) : 
											#if TGX_RGB565_ORDER_BGR
												B((uint16_t)(b * 31)),
					                            G((uint16_t)(g * 63)),
						                        R((uint16_t)(r * 31))
											#else
												R((uint16_t)(r * 31)),
												G((uint16_t)(g * 63)),
												B((uint16_t)(b * 31))
											#endif		
		{}


		/**
		* Ctor from a uint16_t.
		**/
		constexpr RGB565(uint16_t c) : val(c) {}


		/**
		* Ctor from a uint32_t (seen as RGB32).
		**/
		constexpr inline RGB565(uint32_t val);


		/**
		* Ctor from a uint64_t (seen as RGB64).
		**/
		constexpr inline RGB565(uint64_t val);


		/**
		* Default Copy ctor
		**/
		constexpr RGB565(const RGB565&) = default;


		/**
		* Ctor from a RGB24 color.
		**/
		constexpr inline RGB565(const RGB24& c);


		/**
		* Ctor from a RGB32 color.
		* the A component is ignored
		**/
		constexpr inline RGB565(const RGB32& c);


		/**
		* Ctor from a RGB64 color.
		* the A component is ignored
		**/
		constexpr inline RGB565(const RGB64& c);


		/**
		* Ctor from a RGBf color.
		**/
		constexpr inline RGB565(const RGBf & c);


		/**
		* Ctor from a HSV color.
		**/
		RGB565(const HSV& c);


		/**
		* Cast into a uint16_t (non const reference version)
		**/
		constexpr explicit operator uint16_t&()  { return val; }


		/**
		* Cast into a uint16_t (const reference version)
		**/
		constexpr explicit operator const uint16_t& () const { return val; }


		/**
		* Default assignement operator 
		**/ 
		constexpr RGB565& operator=(const RGB565&) = default;


		/**
		* Assignement operator from a RGB24 color
		**/
		constexpr inline RGB565& operator=(const RGB24& c);


		/**
		* Assignement operator from a RGB32 color
		* the A component is ignored
		**/
		constexpr inline RGB565& operator=(const RGB32& c);


		/**
		* Assignement operator from a RGB64 color
		* the A component is ignored
		**/
		constexpr inline RGB565& operator=(const RGB64& c);


		/**
		* Assignement operator from a RGBf color
		**/
		constexpr inline RGB565& operator=(const RGBf& c);


		/**
		* Assignement operator from a HSV color
		**/
		RGB565& operator=(const HSV& c);

		/**
		* Add another color, component by component.
		**/
		constexpr void operator+=(const RGB565& c)
			{
			R += c.R;
			G += c.G;
			B += c.B;
			}


		/**
		* Substract another color, component by component.
		**/
		constexpr void operator-=(const RGB565& c)
			{
			R -= c.R;
			G -= c.G;
			B -= c.B;
			}


		/**
		* Equality comparator
		**/
		constexpr bool operator==(const RGB565& c) const
			{
			return(val == c.val);
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color). 
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		**/
		inline void blend(RGB565 fg_col, float alpha)
			{
			blend256(fg_col, (uint32_t)(alpha * 256));
			}



		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 256 (fg fully opaque).
		**/
		inline void blend256(const RGB565 & fg_col, uint32_t alpha)
			{		
			const uint32_t a = (alpha >> 3); // map to 0 - 32.
			const uint32_t bg = (val | (val << 16)) & 0b00000111111000001111100000011111;
			const uint32_t fg = (fg_col.val | (fg_col.val << 16)) & 0b00000111111000001111100000011111;
			const uint32_t result = ((((fg - bg) * a) >> 5) + bg) & 0b00000111111000001111100000011111;
			val = (uint16_t)((result >> 16) | result); // contract result
			}


		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			R = R * mr >> 8;
			G = G * mg >> 8;
			B = B * mb >> 8;
			}

	};



	/**
	* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
	**/
	inline RGB565 blend(const RGB565 & col1, int32_t C1, const  RGB565 & col2, int32_t C2, const  RGB565 & col3, const int32_t totC)
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
	* Return the "mean" color between colA and colB.
	* TODO : make it faster as for blend() 
	**/
	inline RGB565 meanColor(RGB565 colA, RGB565 colB)
		{
		return RGB565( ((int)colA.R + (int)colB.R) >> 1, 
			           ((int)colA.G + (int)colB.G) >> 1,
			           ((int)colA.B + (int)colB.B) >> 1);
		}


	/**
	* Return the "mean" color between 4 colors.
	* TODO : make it faster as for blend()
	**/
	inline RGB565 meanColor(RGB565 colA, RGB565 colB, RGB565 colC, RGB565 colD)
		{
		return RGB565(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
			          ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
		              ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2);
		}





/**********************************************************************
* declaration of class RGB24
***********************************************************************/


/**
* Class representing a color in R8/G8/B8 format. 
* Occupies 3 bytes in memory.
* 
* Remark: This color type should only be used when memory space 
* is tight but RGB565 does not offer enough resolution. Use 
* RGB32 instead when possible (even if not using the alpha 
* component) because will be faster with correct 4 bytes alignment.
**/
struct RGB24
	{

		// no dual memory represention
		// no alignement, just 3 consecutive uint8_t

#if TGX_RGB24_ORDER_BGR
		uint8_t B;
		uint8_t G;
		uint8_t R;
#else
		uint8_t R;
		uint8_t G;
		uint8_t B;
#endif



		/**
		* Default ctor. Undefined color. 
		**/
		RGB24() = default;


		/**
		* Ctor from raw R,G,B value. 
		* No rescaling. All value in [0,255].
		**/
		constexpr RGB24(int r, int g, int b) : 
											#if TGX_RGB24_ORDER_BGR
												B((uint8_t)b), G((uint8_t)g), R((uint8_t)r)
											#else
												R((uint8_t)r), G((uint8_t)g), B((uint8_t)b)
											#endif

		{}

		
		/**
		* Ctor from float r,g,b in [0.0f, 1.0f].
		**/
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
		* Ctor from a pointer to 3 bytes in the following order: R,G,B. 
		**/
		constexpr RGB24(uint8_t * p) : R(p[0]), G(p[1]), B(p[2]) {}


		/**
		* Default Copy ctor
		**/
		constexpr RGB24(const RGB24&) = default;


		/**
		* Ctor from a uint16_t (seen as RGB565).
		**/
		constexpr inline RGB24(uint16_t c);


		/**
		* Ctor from a uint32_t (seen as RGB32).
		**/
		constexpr inline RGB24(uint32_t c);


		/**
		* Ctor from a uint64_t (seen as RGB64).
		**/
		constexpr inline RGB24(uint64_t c);


		/**
		* Ctor from a RGB24 color.
		**/
		constexpr inline RGB24(const RGB565& c);


		/**
		* Ctor from a RGB32 color.
		* the A component is ignored
		**/
		constexpr inline RGB24(const RGB32& c);


		/**
		* Ctor from a RGB64 color.
		* the A component is ignored
		**/
		constexpr inline RGB24(const RGB64& c);


		/**
		* Ctor from a RGBf color.
		**/
		constexpr inline RGB24(const RGBf& c);


		/**
		* Ctor from a HSV color.
		**/
		RGB24(const HSV& c);


		/**
		* Default assignement operator 
		**/ 
		constexpr RGB24& operator=(const RGB24&) = default;


		/**
		* Assignement operator from a RGB565 color
		**/
		constexpr inline RGB24& operator=(const RGB565& c);


		/**
		* Assignement operator from a RGB32 color
		* the A component is ignored
		**/
		constexpr inline RGB24& operator=(const RGB32& c);


		/**
		* Assignement operator from a RGB64 color
		* the A component is ignored
		**/
		constexpr inline RGB24& operator=(const RGB64& c);


		/**
		* Assignement operator from a RGBf color
		**/
		constexpr inline RGB24& operator=(const RGBf& c);


		/**
		* Assignement operator from a HSV color
		**/
		RGB24& operator=(const HSV& c);


		/**
		* Add another color, component by component.
		**/
		void operator+=(const RGB24 & c)
			{
			R += c.R;
			G += c.G;
			B += c.B;
			}


		/**
		* Substract another color, component by component.
		**/
		void operator-=(const RGB24& c)
			{
			R -= c.R;
			G -= c.G;
			B -= c.B;
			}


		/**
		* Add the scalar value v to each component
		**/
		void operator+=(uint8_t v)
			{
			R += v;
			G += v;
			B += v;
			}


		/**
		 * Substract the scalar value v to each component
	     **/
		void operator-=(uint8_t v)
			{
			R -= v;
			G -= v;
			B -= v;
			}


		/**
		* Multiply each component by the scalar value v.
		**/
		void operator*=(uint8_t v)
			{
			R *= v;
			G *= v;
			B *= v;
			}


		/**
		* Multiply each component by the scalar (floating point) value v.
		**/
		void operator*=(float v)
			{
			R = (uint8_t)(R * v);
			G = (uint8_t)(G * v); 
			B = (uint8_t)(B * v);
			}

		/**
		* Divide each component by the scalar value v.
		**/
		void operator/=(uint8_t v)
			{
			R /= v;
			G /= v;
			B /= v;
			}

		/**
		* Divide each component by the scalar (floating point value) v.
		**/
		void operator/=(float v)
			{
			R = (uint8_t)(R / v);
			G = (uint8_t)(G / v);
			B = (uint8_t)(B / v);
			}


		/**
		* Equality comparator
		**/
		constexpr bool operator==(const RGB24& c) const
			{
			return((R == c.R) && (G == c.G) && (B == c.B));
			}



		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		**/
		inline void blend(const RGB24 & fg_col, float alpha)
			{
			blend256(fg_col, (uint32_t)(alpha * 256));
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 256 (fg fully opaque).
		**/
		inline void blend256(const RGB24& fg_col, uint32_t alpha);


		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			R = R * mr >> 8;
			G = G * mg >> 8;
			B = B * mb >> 8;
			}


	};



	/**
	* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
	**/
	inline RGB24 blend(const RGB24& col1, int32_t C1, const  RGB24& col2, int32_t C2, const  RGB24& col3, const int32_t totC)
		{
		return RGB24((int)(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC),
					 (int)(col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC),
					 (int)(col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC));
		}


	/**
	* Return the "mean" color between colA and colB.
	**/
	inline RGB24 meanColor(const RGB24 & colA, const RGB24 & colB)
		{
		return RGB24(((int)colA.R + (int)colB.R) >> 1,
					 ((int)colA.G + (int)colB.G) >> 1,
			         ((int)colA.B + (int)colB.B) >> 1);
		}


	/**
	* Return the "mean" color between 4 colors.
	**/
	inline RGB24 meanColor(const RGB24& colA, const RGB24& colB, const RGB24& colC, const RGB24& colD)
		{
		return RGB24(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
			         ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
			         ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2);
		}






/**********************************************************************
* declaration of class RGB32
***********************************************************************/


/**
* Class representing a color in R8/G8/B8/(A8) format. 
* Occupies 4 bytes in memory. 
* Can be converted from/to uint32_t.
* 
* the A component defaults to DEFAULT_A if not used.
**/
struct RGB32
	{

		static const uint8_t DEFAULT_A = 0; // should be 0 to insures stability by type conversion 


		union // dual memory representation
			{ 
			uint32_t val;			// as a uint32_t
			struct 
				{
#if TGX_RGB32_ORDER_BGR
				uint8_t B;			// as 4 components BGRA
				uint8_t G;			//
				uint8_t R;			//
				uint8_t A;			//

#else
				uint8_t R;			//
				uint8_t G;			//
				uint8_t B;			// as 4 components RGBA
				uint8_t A;			//
#endif
				}; 
			};


		/**
		* Default ctor. Undefined color. 
		**/
		RGB32() = default;


		/**
		* Ctor from raw R,G,B, A value. 
		* No rescaling. All values in [0,255].
		**/
		constexpr RGB32(int r, int g, int b, int a = DEFAULT_A) : 
															#if TGX_RGB32_ORDER_BGR
																B((uint8_t)b), G((uint8_t)g), R((uint8_t)r), A((uint8_t)a)
															#else
																R((uint8_t)r), G((uint8_t)g), B((uint8_t)b), A((uint8_t)a)
															#endif
		{}


		/**
		* Ctor from float r,g,b in [0.0f, 1.0f].
		**/
		RGB32(float r, float g, float b, float a = -1.0f) : 
															#if TGX_RGB32_ORDER_BGR
																B((uint8_t)(b * 255)),
																G((uint8_t)(g * 255)),
																R((uint8_t)(r * 255)),
																A((uint8_t)((a < 0.0f) ? DEFAULT_A : ((uint8_t)roundf(a * 31.0f))))
															#else
																R((uint8_t)(r * 255.0f)),
																G((uint8_t)(g * 255.0f)),
																B((uint8_t)(b * 255.0f)),
																A((uint8_t)((a < 0.0f) ? DEFAULT_A : ((uint8_t)roundf(a * 31.0f))))
															#endif
		{}


		/**
		* Ctor from a uint32_t.
		**/
		constexpr RGB32(uint32_t c) : val(c) {}


		/**
		* Ctor from a uint16_t (seen as RGB565).
		**/
		constexpr inline RGB32(uint16_t val);


		/**
		* Ctor from a uint64_t (seen as RGB64).
		**/
		constexpr inline RGB32(uint64_t val);


		/**
		* Default Copy ctor
		**/
		constexpr RGB32(const RGB32&) = default;


		/**
		* Ctor from a RGB565 color.
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32(const RGB565& c);


		/**
		* Ctor from a RGB24 color.
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32(const RGB24& c);


		/**
		* Ctor from a RGB64 color.
		**/
		constexpr inline RGB32(const RGB64& c);


		/**
		* Ctor from a RGBf color.
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32(const RGBf& c);


		/**
		* Ctor from a HSV color.
		* A component is set to DEFAULT_A
		**/
		RGB32(const HSV& c);


		/**
		* Cast into a uint32_t (non const reference version)
		**/
		constexpr explicit operator uint32_t&()  { return val; }


		/**
		* Cast into a uint32_t (const reference version)
		**/
		constexpr explicit operator const uint32_t& () const { return val; }


		/**
		* Default assignement operator 
		**/ 
		constexpr RGB32& operator=(const RGB32&) = default;


		/**
		* Assignement operator from a RGB24 color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32& operator=(const RGB565& c);


		/**
		* Assignement operator from a RGB24 color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32& operator=(const RGB24& c);


		/**
		* Assignement operator from a RGB64 color
		**/
		constexpr inline RGB32& operator=(const RGB64& c);


		/**
		* Assignement operator from a RGBf color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB32& operator=(const RGBf& c);


		/**
		* Assignement operator from a HSV color
		* A component is set to DEFAULT_A
		**/
		RGB32& operator=(const HSV& c);


		/**
		* Add another color, component by component.
		**/
		void operator+=(const RGB32& c)
			{
			R += c.R;
			G += c.G;
			B += c.B;
			A += c.A;
			}


		/**
		* Substract another color, component by component.
		**/
		void operator-=(const RGB32& c)
			{
			R -= c.R;
			G -= c.G;
			B -= c.B;
			A -= c.A;
			}


		/**
		* Add the scalar value v to each component
		**/
		void operator+=(uint8_t v)
			{
			R += v;
			G += v;
			B += v;
			A += v;
			}


		/**
		 * Substract the scalar value v to each component
		 **/
		void operator-=(uint8_t v)
			{
			R -= v;
			G -= v;
			B -= v;
			A -= v;
			}


		/**
		* Multiply each component by the scalar value v.
		**/
		void operator*=(uint8_t v)
			{
			R *= v;
			G *= v;
			B *= v;
			A *= v;
			}


		/**
		* Multiply each component by the scalar (floating point) value v.
		**/
		void operator*=(float v)
			{
			R = (uint8_t)(R * v);
			G = (uint8_t)(G * v);
			B = (uint8_t)(B * v);
			A = (uint8_t)(A * v);
			}

		/**
		* Divide each component by the scalar value v.
		**/
		void operator/=(uint8_t v)
			{
			R /= v;
			G /= v;
			B /= v;
			A /= v;
			}

		/**
		* Divide each component by the scalar (floating point value) v.
		**/
		void operator/=(float v)
			{
			R = (uint8_t)(R / v);
			G = (uint8_t)(G / v);
			B = (uint8_t)(B / v);
			A = (uint8_t)(A / v);
			}


		/**
		* Equality comparator
		**/
		constexpr bool operator==(const RGB32& c) const
			{
			return(val == c.val);
			}


		/**
		* alpha-blend the fg_col color over this one (and save the result in this color).
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		*
		* The alpha channel of the RGB32 colors is ignored !!!!
		**/
		inline void blend(const RGB32 & fg_col, float alpha)
			{
			blend256(fg_col, (uint32_t)(alpha * 256));
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 256 (fg fully opaque).
		* 
		* The alpha channel of the RGB32 colors is ignored !!!!
		**/
		inline void blend256(const RGB32 & fg_col, uint32_t alpha)
			{
			const uint32_t inv_alpha = 256 - alpha;
			const uint32_t ag = ((fg_col.val & 0xFF00FF00) >> 8)*alpha  +  ((val & 0xFF00FF00) >> 8)*inv_alpha;
			const uint32_t rb = (fg_col.val & 0x00FF00FF) * alpha + (val & 0x00FF00FF) * inv_alpha;
			val = (ag & 0xFF00FF00) | ((rb >> 8) & 0x00FF00FF);
			}


		/**
		* alpha-blend the fg_col color over this one (and save the result in this color).
		* 
		* Use the A component of fg_col for the alpha value in [0, 255]. 
		* The A component of this color does not matter as it is considered fully opaque anyway. 
		**/
		inline void blend(RGB32 fg_col)
			{
			blend256(fg_col, ((uint32_t)fg_col.A) + 1); // adding one is ok: still works for A = 0 :-) 
			}


		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			R = R * mr >> 8;
			G = G * mg >> 8;
			B = B * mb >> 8;
			}



	};


	/**
	* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
	**/
	inline RGB32 blend(const RGB32& col1, int32_t C1, const  RGB32& col2, int32_t C2, const  RGB32& col3, const int32_t totC)
		{
		return RGB32((int)(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC),
					 (int)(col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC),
					 (int)(col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC),
					 (int)(col3.A + (C1 * (col1.A - col3.A) + C2 * (col2.A - col3.A)) / totC));
		}



	/**
	* Return the "mean" color between colA and colB.
	**/
	inline RGB32 meanColor(RGB32 colA, RGB32 colB)
		{
		return RGB32(((int)colA.R + (int)colB.R) >> 1,
					 ((int)colA.G + (int)colB.G) >> 1,
			         ((int)colA.B + (int)colB.B) >> 1,
			         ((int)colA.A + (int)colB.A) >> 1);
		}


	/**
	* Return the "mean" color between 4 colors
	**/
	inline RGB32 meanColor(RGB32 colA, RGB32 colB, RGB32 colC, RGB32 colD)
	{
		return RGB32(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
					 ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
					 ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2,
					 ((int)colA.A + (int)colB.A + (int)colC.A + (int)colD.A) >> 2);
	}





/**********************************************************************
* declaration of class RGB64
***********************************************************************/

/**
* Class representing a color in R16/G16/B16/(A16) format. 
* Occupies 8 bytes in memory.
* Can be converted from/to uint64_t.
* 
* the A component defaults to DEFAULT_A if not used.
**/
struct RGB64
	{

		static const uint16_t DEFAULT_A = 0; // should be 0 to insures stability by type conversion 


		union // dual memory representation
			{ 
			uint64_t val;			// as a uint32_t
			struct 
				{

#if TGX_RGB64_ORDER_BGR
				uint16_t B;			// as 4 components BGRA
				uint16_t G;			//
				uint16_t R;			//
				uint16_t A;			//

#else
				uint16_t R;			// as 4 components RGBA
				uint16_t G;			//
				uint16_t B;			// 
				uint16_t A;			//
#endif
				}; 
			};


		/**
		* Default ctor. Undefined color. 
		**/
		RGB64() = default;


		/**
		* Ctor from raw R,G,B, A value. 
		* No rescaling. All values in [0,65535].
		**/
		constexpr RGB64(int r, int g, int b, int a = DEFAULT_A) : 		
																#if TGX_RGB64_ORDER_BGR
																	B((uint16_t)b), G((uint16_t)g), R((uint16_t)r), A((uint16_t)a) 
																#else
																	R((uint16_t)r), G((uint16_t)g), B((uint16_t)b), A((uint16_t)a)
																#endif
		{}

		
		/**
		* Ctor from float r,g,b in [0.0f, 1.0f].
		**/
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
		* Ctor from a uint32_t.
		**/
		constexpr inline RGB64(uint64_t c) : val(c) {}


		/**
		* Ctor from a uint16_t (seen as RGB565).
		**/
		constexpr inline RGB64(uint16_t val);


		/**
		* Ctor from a uint32_t (seen as RGB32).
		**/
		constexpr inline RGB64(uint32_t val);


		/**
		* Default Copy ctor
		**/
		constexpr RGB64(const RGB64&) = default;


		/**
		* Ctor from a RGB565 color.
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB64(const RGB565& c);


		/**
		* Ctor from a RGB24 color.
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB64(const RGB24& c);


		/**
		* Ctor from a RGB32 color.
		**/
		constexpr inline RGB64(const RGB32& c);


		/**
		* Ctor from a RGBf color.
		**/
		constexpr inline RGB64(const RGBf& c);


		/**
		* Ctor from a HSV color.
		* A component is set to DEFAULT_A
		**/
		RGB64(const HSV& c);


		/**
		* Cast into a uint64_t (non const reference version)
		**/
		constexpr explicit operator uint64_t&()  { return val; }


		/**
		* Cast into a uint64_t (const reference version)
		**/
		constexpr explicit operator const uint64_t&() const { return val; }


		/**
		* Default assignement operator 
		**/ 
		constexpr RGB64& operator=(const RGB64&) = default;


		/**
		* Assignement operator from a RGB24 color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB64& operator=(const RGB565& c);


		/**
		* Assignement operator from a RGB24 color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB64& operator=(const RGB24& c);


		/**
		* Assignement operator from a RGB32 color
		**/
		constexpr inline RGB64& operator=(const RGB32& c);


		/**
		* Assignement operator from a RGBf color
		* A component is set to DEFAULT_A
		**/
		constexpr inline RGB64& operator=(const RGBf& c);


		/**
		* Assignement operator from a HSV color
		* A component is set to DEFAULT_A
		**/
		RGB64& operator=(const HSV& c);


		/**
		* Add another color, component by component.
		**/
		void operator+=(const RGB64& c)
			{
			R += c.R;
			G += c.G;
			B += c.B;
			A += c.A;
			}


		/**
		* Substract another color, component by component.
		**/
		void operator-=(const RGB64& c)
			{
			R -= c.R;
			G -= c.G;
			B -= c.B;
			A -= c.A;
			}


		/**
		* Add the scalar value v to each component
		**/
		void operator+=(uint16_t v)
			{
			R += v;
			G += v;
			B += v;
			A += v;
			}


		/**
		 * Substract the scalar value v to each component
		 **/
		void operator-=(uint16_t v)
			{
			R -= v;
			G -= v;
			B -= v;
			A -= v;
			}


		/**
		* Multiply each component by the scalar value v.
		**/
		void operator*=(uint16_t v)
			{
			R *= v;
			G *= v;
			B *= v;
			A *= v;
			}


		/**
		* Multiply each component by the scalar (floating point) value v.
		**/
		void operator*=(float v)
			{
			R = (uint16_t)(R * v);
			G = (uint16_t)(G * v);
			B = (uint16_t)(B * v);
			A = (uint16_t)(A * v);
			}

		/**
		* Divide each component by the scalar value v.
		**/
		void operator/=(uint16_t v)
			{
			R /= v;
			G /= v;
			B /= v;
			A /= v;
			}

		/**
		* Divide each component by the scalar (floating point value) v.
		**/
		void operator/=(float v)
			{
			R = (uint16_t)(R / v);
			G = (uint16_t)(G / v);
			B = (uint16_t)(B / v);
			A = (uint16_t)(A / v);
			}


		/**
		* Equality comparator
		**/
		constexpr bool operator==(const RGB64& c) const
			{
			return(val == c.val);
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		*
		* The alpha channel of the RGB64 colors is ignored !!!!
		**/
		inline void blend(const RGB64& fg_col, float alpha)
			{
			blend65536(fg_col, (uint32_t)(alpha * 65536));
			}


		/**
		* alpha-blend the fg_col color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 256 (fg fully opaque).
		*
		* The alpha channel of the RGB64 colors is ignored !!!!
		**/
		inline void blend256(const RGB64 & fg_col, uint32_t alpha)
			{
			blend65536(fg_col, (alpha << 8));
			}


		/**
		* alpha-blend the fg_col color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 65536 (fg fully opaque).
		*
		* The alpha channel of the RGB64 colors is ignored !!!!
		**/
		inline void blend65536(const RGB64 & fg_col, uint32_t alpha)
			{
			// todo: make it faster 
			const uint32_t inv_alpha = 65536 - alpha;
			R = (uint16_t)((fg_col.R * alpha + R * inv_alpha) >> 16);
			G = (uint16_t)((fg_col.G * alpha + G * inv_alpha) >> 16);
			B = (uint16_t)((fg_col.B * alpha + B * inv_alpha) >> 16);
			A = (uint16_t)((fg_col.A * alpha + A * inv_alpha) >> 16); // hum... maybe better to skip this ? 
			}


		/**
		* alpha-blend the fg_col color over this one (and save the result in this color).
		* 
		* Use the A component of fg_col for the alpha value in [0, 65535]. 
		* The A component of this color does not matter as it is considered fully opaque anyway. 
		**/
		inline void blend(const RGB64 & fg_col)
			{
			blend65536(fg_col, ((uint32_t)fg_col.A) + 1); // adding one is ok: still works for A = 0 :-) 
			}


		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			R = R * mr >> 8;
			G = G * mg >> 8;
			B = B * mb >> 8;
			}



	};




	/**
	* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
	**/
	inline RGB64 blend(const RGB64& col1, int32_t C1, const  RGB64& col2, int32_t C2, const  RGB64& col3, const int32_t totC)
		{
		return RGB32((int)(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC),
					 (int)(col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC),
					 (int)(col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC),
					 (int)(col3.A + (C1 * (col1.A - col3.A) + C2 * (col2.A - col3.A)) / totC));
		}


	/**
	* Return the "mean" color between colA and colB.
	**/
	inline RGB64 meanColor(const RGB64 & colA, const RGB64 & colB)
		{
		return RGB64(((int)colA.R + (int)colB.R) >> 1,
			         ((int)colA.G + (int)colB.G) >> 1,
					 ((int)colA.B + (int)colB.B) >> 1,
					 ((int)colA.A + (int)colB.A) >> 1);
		}



	/**
	* Return the "mean" color between 4 colors.
	**/
	inline RGB64 meanColor(const RGB64& colA, const RGB64& colB, const RGB64& colC, const RGB64& colD)
		{
		return RGB64(((int)colA.R + (int)colB.R + (int)colC.R + (int)colD.R) >> 2,
			         ((int)colA.G + (int)colB.G + (int)colC.G + (int)colD.G) >> 2,
			         ((int)colA.B + (int)colB.B + (int)colC.B + (int)colD.B) >> 2,
			         ((int)colA.A + (int)colB.A + (int)colC.A + (int)colD.A) >> 2);
		}







/**********************************************************************
* declaration of class RGBf
***********************************************************************/


	/**
	* Class representing a color in R5/G6/B5 format.
	* Occupies 2 bytes in memory
	* Can be converted from/to uint16_t.
	*
	* This type is compatible with adafruit gfx library
	* and is used with most SPI display.
	**/
	struct RGBf
	{

#if TGX_RGBf_ORDER_BGR
		float B;
		float G;
		float R;
#else 
		float R;
		float G;
		float B;
#endif


		/**
		* Default ctor. Undefined color.
		**/
		RGBf() = default;


		/**
		* Ctor from raw R,G,B value.
		* No rescaling: r in 0..31
		*               g in 0..63
		*               b in 0..31
		**/
		constexpr RGBf(float r, float g, float b) :
		#if TGX_RGBf_ORDER_BGR
			B(b), G(g), R(r)
		#else
			R(r), G(g), B(b)
		#endif
		{}



		/**
		* Ctor from a uint16_t (seen as RGB565).
		**/
		constexpr inline RGBf(uint16_t val);


		/**
		* Ctor from a uint32_t.
		**/
		constexpr inline RGBf(uint32_t val);


		/**
		* Ctor from a uint64_t (seen as RGB64).
		**/
		constexpr inline RGBf(uint64_t val);


		/**
		* Default Copy ctor
		**/
		constexpr RGBf(const RGBf&) = default;


		/**
		* Ctor from a RGB565 color.
		**/
		constexpr inline RGBf(const RGB565& c);


		/**
		* Ctor from a RGB24 color.
		**/
		constexpr inline RGBf(const RGB24& c);


		/**
		* Ctor from a RGB32 color.
		* the A component is ignored
		**/
		constexpr inline RGBf(const RGB32& c);


		/**
		* Ctor from a RGB64 color.
		* the A component is ignored
		**/
		constexpr inline RGBf(const RGB64& c);


		/**
		* Ctor from a HSV color.
		**/
		RGBf(const HSV& c);



		/**
		* Default assignement operator
		**/
		constexpr RGBf& operator=(const RGBf&) = default;


		/**
		* Assignement operator from a RGB565 color
		**/
		constexpr inline RGBf& operator=(const RGB565& c);


		/**
		* Assignement operator from a RGB24 color
		**/
		constexpr inline RGBf& operator=(const RGB24& c);


		/**
		* Assignement operator from a RGB32 color
		* the A component is ignored
		**/
		constexpr inline RGBf& operator=(const RGB32& c);


		/**
		* Assignement operator from a RGB64 color
		* the A component is ignored
		**/
		constexpr inline RGBf& operator=(const RGB64& c);


		/**
		* Assignement operator from a HSV color
		**/
		RGBf& operator=(const HSV& c);


		/**
		* Add another color, component by component.
		**/
		constexpr void operator+=(const RGBf& c)
			{
			R += c.R;
			G += c.G;
			B += c.B;
			}


		/**
		* Substract another color, component by component.
		**/
		constexpr void operator-=(const RGBf& c)
			{
			R -= c.R;
			G -= c.G;
			B -= c.B;
			}


		/**
		* Multiply each channel by the same channel on c. 
		**/
		inline void operator*=(const RGBf & c)
			{
			R *= c.R;
			G *= c.G;
			B *= c.B;
			}

		/**
		* Return the color obtained by multipliying the channels 
		* of both colors together.
		**/
		inline RGBf operator*(const RGBf & c) const 
			{
			return RGBf(R * c.R, G * c.G, B * c.B);
			}

		/**
		* Multiply each channel by a constant
		**/
		inline void operator*=(float a)
			{
			R *= a;
			G *= a;
			B *= a;
			}


		/**
		* Return the color where all components are 
		* multiplied by a
		**/
		inline RGBf operator*(float a) const
			{
			return RGBf(R * a, G * a, B * a);
			}
	

		/**
		* Equality comparator
		**/
		constexpr bool operator==(const RGBf & c) const
			{
			return((R == c.R)&&(G == c.G)&&(B == c.B));
			}

		/**
		* Clamp color channel to [0,1]
		**/
		inline void clamp()
			{
			R = tgx::clamp(R, 0.0f, 1.0f);
			G = tgx::clamp(G, 0.0f, 1.0f);
			B = tgx::clamp(B, 0.0f, 1.0f);
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		**/
		inline void blend(const RGBf & fg_col, float alpha)
			{
			R += (fg_col.R - R) * alpha;
			G += (fg_col.G - G) * alpha;
			B += (fg_col.B - B) * alpha;
			}



		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			R = (R * mr)/256;
			G = (G * mg)/256;
			B = (B * mb)/256;
			}



	};


	/**
	* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
	**/
	inline RGBf blend(const RGBf & col1, int32_t C1, const  RGBf & col2, int32_t C2, const  RGBf & col3, int32_t totC)
		{
		return RGBf(col3.R + (C1 * (col1.R - col3.R) + C2 * (col2.R - col3.R)) / totC,
   			        col3.G + (C1 * (col1.G - col3.G) + C2 * (col2.G - col3.G)) / totC,
	         	    col3.B + (C1 * (col1.B - col3.B) + C2 * (col2.B - col3.B)) / totC);
		}


	/**
	* Return the "mean" color between colA and colB.
	**/
	inline RGBf meanColor(const RGBf & colA, const  RGBf & colB)
		{
		return RGBf((colA.R + colB.R)/2, (colA.G + colB.G)/2, (colA.B + colB.B)/2);
		}


	/**
	* Return the "mean" color between 4 colors.
	**/
	inline RGBf meanColor(const RGBf & colA, const  RGBf & colB, const  RGBf & colC, const  RGBf & colD)
		{
		return RGBf((colA.R + colB.R + colC.R + colD.R) / 4, (colA.G + colB.G + colC.G + colD.G) / 4, (colA.B + colB.B + colC.B + colD.B) / 4);
		}








/**********************************************************************
* declaration of class HSV
***********************************************************************/


/**
* Class representing a color in Hue/Saturation/Value
* format where each component is in float (4 bytes). 
* see: https://en.wikipedia.org/wiki/HSL_and_HSV
*
* The total size of the object is 12 bytes. 
**/
struct HSV
	{

		// color components
		float H;		// hue in [0.0f ,1.0f]
		float S;		// saturation in [0;0f, 1.0f]
		float V;		// value in [0.F ,1.0f]


		/**
		* Default ctor. Undefined color.
		**/
		HSV() = default;


		/**
		* Ctor from raw H, S, V values in [0.0f, 1.0f]
		**/
		constexpr HSV(float h, float s, float v) : H(h), S(s), V(v) {}


		/**
		* Default Copy ctor
		**/
		constexpr HSV(const HSV&) = default;


		/**
		* Ctor from a uint16_t (seen as RGB565).
		**/
		HSV(uint16_t val);


		/**
		* Ctor from a uint32_t (seen as RGB32).
		**/
		HSV(uint32_t val);


		/**
		* Ctor from a uint64_t (seen as RGB64).
		**/
		HSV(uint64_t val);


		/**
		* Ctor from a RGB565 color.
		**/
		HSV(const RGB565& c);


		/**
		* Ctor from a RGB24 color.
		**/
		HSV(const RGB24 & c);


		/**
		* Ctor from a RGB32 color.
		* the A component is ignored
		**/
		HSV(const RGB32 & c);


		/**
		* Ctor from a RGB64 color.
		* the A component is ignored
		**/
		HSV(const RGB64 & c);


		/**
		* Ctor from a RGBf color.
		**/
		HSV(const RGBf& c);


		/**	
		* Default assignement operator
		**/
		constexpr HSV& operator=(const HSV&) = default;


		/**
		* Assignement operator from a RGB565 color
		**/
		HSV& operator=(const RGB565& c);


		/**
		* Assignement operator from a RGB24 color
		**/
		HSV& operator=(const RGB24 & c);


		/**
		* Assignement operator from a RGB32 color
		* the A component is ignored
		**/
		HSV& operator=(const RGB32 & c);


		/**
		* Assignement operator from a RGB64 color
		* the A component is ignored
		**/
		HSV& operator=(const RGB64 & c);


		/**
		* Assignement operator from a RGBf color
		* the A component is ignored
		**/
		HSV& operator=(const RGBf& c);



		/**
		* Equality comparator
		**/
		constexpr bool operator==(const HSV & c) const
			{
			return((H == c.H)&&(S == c.S)&&(V == c.V));
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* floating point alpha value between 0.0f (fg fully transparent) and 1.0f (fg fully opaque).
		*
		* Blending is done by converting the color to RGB space, then alpha blending, 
		* then converting back to HSV 
		**/
		inline void blend(const HSV & fg_col, float alpha)
			{
			// is there a natural blending formula in HSV space ? 
			// let us do it in RGB space for the time being 			
			RGBf c(*this);
			c.blend(RGBf(fg_col), alpha);
			*this = HSV(c);
			}


		/**
		* alpha-blend the fg color over this one (and save the result in this color).
		* integer alpha value between 0 (fg fully transparent) and 256 (fg fully opaque).
		*
		* The alpha channel of the RGB32 colors is ignored !!!!
		**/
		inline void blend256(const HSV & fg_col, uint32_t alpha)
			{
			blend(fg_col, (alpha / 256.0f));
			}


		/**
		* multiply each color component by a given factor (in [0,256]).
		**/
		inline void mult256(int mr, int mg, int mb)
			{
			RGBf c = RGBf(*this); 
			c.mult256(mr, mg, mb);
			*this = HSV(c);
			}


	};



/**
* Return the color  (c1*col1 + c2*col2 + (totc-c1-c2)*col3)/totc
**/
inline HSV blend(HSV col1, int32_t C1, HSV col2, int32_t C2, HSV col3, int32_t totC)
	{
	return HSV(blend(RGBf(col1), C1, RGBf(col2), C2, RGBf(col3), totC));
	}



/**
* Return the "mean" color between colA and colB.
**/
inline HSV meanColor(const  HSV & colA, const  HSV & colB)
		{
		// hum.. what does that mean in HSV, let's do it in RGB color space instead
		return HSV(meanColor(RGBf(colA), RGBf(colB)));
		}


/**
* Return the "mean" color between 4 colors
**/
inline HSV meanColor(const  HSV& colA, const  HSV& colB, const  HSV& colC, const  HSV& colD)
		{
		// hum.. what does that mean in HSV, let's do it in RGB color space instead
		return HSV(meanColor(RGBf(colA), RGBf(colB), RGBf(colC), RGBf(colD)));
		}














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



constexpr inline RGB565& RGB565::operator=(const RGB24& c)
		{
		B = (c.B >> 3);
		G = (c.G >> 2);
		R = (c.R >> 3);
		return *this;
		}


constexpr inline RGB565& RGB565::operator=(const RGB32& c)
		{
		B = (c.B >> 3);
		G = (c.G >> 2);
		R = (c.R >> 3);
		return *this;
		}


constexpr inline RGB565& RGB565::operator=(const RGB64& c)
		{
		B = (c.B >> 11);
		G = (c.G >> 10);
		R = (c.R >> 11);
		return *this;
		}


constexpr inline RGB565& RGB565::operator=(const RGBf& c)
		{
		B = (uint16_t)(c.B * 31);
		G = (uint16_t)(c.G * 63);
		R = (uint16_t)(c.R * 31);
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
		B(((uint8_t)c.B) << 3), G(((uint8_t)c.G) << 2), R(((uint8_t)c.R) << 3)
		#else
		R(((uint8_t)c.R) << 3), G(((uint8_t)c.G) << 2), B(((uint8_t)c.B) << 3)
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


constexpr inline RGB24& RGB24::operator=(const RGB565& c)
		{
		R = ((uint8_t)c.R) << 3;
		G = ((uint8_t)c.G) << 2;
		B = ((uint8_t)c.B) << 3;
		return *this;
		}


constexpr inline RGB24& RGB24::operator=(const RGB32& c)
		{
		R = c.R;
		G = c.G;
		B = c.B;
		return *this;
		}


constexpr inline RGB24& RGB24::operator=(const RGB64& c)
		{
		R = c.R >> 8;
		G = c.G >> 8;
		B = c.B >> 8;
		return *this;
		}


constexpr inline RGB24& RGB24::operator=(const RGBf& c)
		{
		R = (uint8_t)(c.R * 255);
		G = (uint8_t)(c.G * 255);
		B = (uint8_t)(c.B * 255);
		return *this;
		}


inline void RGB24::blend256(const RGB24& fg_col, uint32_t alpha)
		{
		// just forward to RGB32 methods. Might do something a bit faster but don't really care as RGB24 will be slow anyway...
		RGB32 c(*this);
		c.blend256(RGB32(fg_col), alpha);
		*this = RGB24(c); // forward to RGB32
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
		B(((uint8_t)c.B) << 3), G(((uint8_t)c.G) << 2), R(((uint8_t)c.R) << 3), A(DEFAULT_A)
		#else
		R(((uint8_t)c.R) << 3), G(((uint8_t)c.G) << 2), B(((uint8_t)c.B) << 3), A(DEFAULT_A)
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


constexpr inline RGB32& RGB32::operator=(const RGB565& c)
		{
		B = ((uint8_t)c.B) << 3;
		G = ((uint8_t)c.G) << 2;
		R = ((uint8_t)c.R) << 3;
		A = DEFAULT_A;
		return *this;
		}


constexpr inline RGB32& RGB32::operator=(const RGB24& c)
		{
		B = c.B;
		G = c.G;
		R = c.R;
		A = DEFAULT_A;
		return *this;
		}


constexpr inline RGB32& RGB32::operator=(const RGB64& c)
		{
		B = c.B >> 8;
		G = c.G >> 8;
		R = c.R >> 8;
		A = c.A >> 8;
		return *this;
		}


constexpr inline RGB32& RGB32::operator=(const RGBf& c)
		{
		B = (uint8_t)(c.B * 255);
		G = (uint8_t)(c.G * 255);
		R = (uint8_t)(c.R * 255);
		A = DEFAULT_A;
		return *this;
		}





/**********************************************************************
* implementation of inline method for RGB64
***********************************************************************/

constexpr inline RGB64::RGB64(uint16_t val) : RGB64(RGB565(val))
		{
		}


constexpr inline RGB64::RGB64(uint32_t val) : RGB64(RGB32(val))
		{
		}


constexpr inline RGB64::RGB64(const RGB565& c) : 
		#if TGX_RGB64_ORDER_BGR
		B(((uint16_t)c.B) << 11), G(((uint16_t)c.G) << 10), R(((uint16_t)c.R) << 11), A(DEFAULT_A)
		#else
		R(((uint16_t)c.R) << 11), G(((uint16_t)c.G) << 10), B(((uint16_t)c.B) << 11), A(DEFAULT_A)
		#endif	
		{
		}


constexpr inline RGB64::RGB64(const RGB24& c) : 
		#if TGX_RGB64_ORDER_BGR
		B(((uint16_t)c.B) << 8), G(((uint16_t)c.G) << 8), R(((uint16_t)c.R) << 8), A(DEFAULT_A)
		#else
		R(((uint16_t)c.R) << 8), G(((uint16_t)c.G) << 8), B(((uint16_t)c.B) << 8), A(DEFAULT_A)
		#endif		
		{	
		}


constexpr inline RGB64::RGB64(const RGB32& c) : 
		#if TGX_RGB64_ORDER_BGR
		B(((uint16_t)c.B) << 8), G(((uint16_t)c.G) << 8), R(((uint16_t)c.R) << 8), A(((uint16_t)c.A) << 8)
		#else
		R(((uint16_t)c.R) << 8), G(((uint16_t)c.G) << 8), B(((uint16_t)c.B) << 8), A(((uint16_t)c.A) << 8)
		#endif			
		{
		}


constexpr inline RGB64::RGB64(const RGBf& c) : 
		#if TGX_RGB64_ORDER_BGR
		B((uint16_t)(c.B * 65535)), G((uint16_t)(c.G * 65535)), R((uint16_t)(c.R * 65535)), A(DEFAULT_A)
		#else
		R((uint16_t)(c.R * 65535)), G((uint16_t)(c.G * 65535)), B((uint16_t)(c.B * 65535)), A(DEFAULT_A)
		#endif	
		{
		}


constexpr inline RGB64& RGB64::operator=(const RGB565& c)
		{
		B = ((uint16_t)c.B) << 11;
		G = ((uint16_t)c.G) << 10;
		R = ((uint16_t)c.R) << 11;
		A = DEFAULT_A;
		return *this;
		}


constexpr inline RGB64& RGB64::operator=(const RGB24& c)
		{
		B = ((uint16_t)c.B) << 8;
		G = ((uint16_t)c.G) << 8;
		R = ((uint16_t)c.R) << 8;
		A = DEFAULT_A;
		return *this;
		}


constexpr inline RGB64& RGB64::operator=(const RGB32& c)
		{
		B = ((uint16_t)c.B) << 8;
		G = ((uint16_t)c.G) << 8;
		R = ((uint16_t)c.R) << 8;
		A = ((uint16_t)c.A) << 8;
		return *this;
		}

constexpr inline RGB64& RGB64::operator=(const RGBf& c)
		{
		B = (uint16_t)(c.B * 65535);
		G = (uint16_t)(c.G * 65535);
		R = (uint16_t)(c.R * 65535);
		A = DEFAULT_A;
		return *this;
		}




/**********************************************************************
* implementation of inline method for RGB24
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


constexpr inline RGBf& RGBf::operator=(const RGB565& c)
		{
		R = c.R / 31.0f;
		G = c.G / 63.0f;
		B = c.B / 31.0f;
		return *this;
		}


constexpr inline RGBf& RGBf::operator=(const RGB24& c)
		{
		R = c.R / 255.0f;
		G = c.G / 255.0f;
		B = c.B / 255.0f;
		return *this;
		}


constexpr inline RGBf& RGBf::operator=(const RGB32& c)
		{
		R = c.R / 255.0f;
		G = c.G / 255.0f;
		B = c.B / 255.0f;
		return *this;
		}


constexpr inline RGBf& RGBf::operator=(const RGB64& c)
		{
		R = c.R / 65535.0f;
		G = c.G / 65535.0f;
		B = c.B / 65535.0f;
		return *this;
		}




}


/* end of file */

