/**   
 * @file Misc.h
 * Utility/miscellaneous functions used throughout the library. 
 */
//
// Copyright 2020 Arvind Singh
// Fast approximations copyright 2025 Ken Cooke
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

#ifndef _TGX_MISC_H_
#define _TGX_MISC_H_

#include "tgx_config.h"

#include <stdint.h>
#include <math.h>
#include <string.h>



// c++, no plain c
#ifdef __cplusplus


// macro to cast indexes as 32bit when doing pointer arithmetic 
#define TGX_CAST32(a)   ((int32_t)(a))

#define DEPRECATED(X) [[deprecated(" " X " ")]]


// check that int is at least 4 bytes. 
static_assert(sizeof(int) >= 4, "The TGX library only works on 32 bits or 64 bits architecture. Sorry!");



#if defined(ARDUINO_TEENSY41)

    // check existence of external ram (EXTMEM). 
    extern "C" uint8_t external_psram_size;

    // check is an address is in flash
    #define TGX_IS_PROGMEM(X)  ((((uint32_t)(X)) >= 0x60000000)&&(((uint32_t)(X)) < 0x70000000))

    // check if an address is in external ram
    #define TGX_IS_EXTMEM(X) ((((uint32_t)(X)) >= 0x70000000)&&(((uint32_t)(X)) < 0x80000000))

#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846  ///< define Pi in case math.h is not yet included.
#endif


namespace tgx
{


    /** 
    * Dummy type identified by an integer. 
    * 
    * Use to bypass partial template specialization by using method overloading instead....
    */
    template<int N> struct DummyType
        {
        // nothing here :-)
        };


    /**
    * Dummy type identified by two booleans.
    * 
    * Use to bypass partial template specialization by using method overloading instead....
    */
    template<bool BB1, bool BB2> struct DummyTypeBB
        {
        // nothing here :-)
        };


    /** sets the default floating point type for computations */
    template<typename T = int> struct DefaultFPType
        {
#if TGX_SINGLE_PRECISION_COMPUTATIONS
        typedef float fptype;   ///< use float as default floating point type
#else
        typedef double fptype;  ///< use double as default floating point type
#endif
        };

#if TGX_SINGLE_PRECISION_COMPUTATIONS
    /** ... but when already using double, keep using double...*/
    template<> struct DefaultFPType<double>
        {
        typedef double fptype;
        };
#endif


    /** test equality of types. Same as std::is_same */
    template <typename, typename> struct is_same { static const bool value = false; };
    template <typename T> struct is_same<T, T> { static const bool value = true; };


    /** little endian / big endian conversion */
    TGX_INLINE inline uint16_t BigEndian16(uint16_t v)
        {
#ifdef __GNUC__
        return __builtin_bswap16(v); 
#else
        return ((v >> 8) | (v << 8));
#endif
        }


    /** Baby let me swap you one more time... */
    template<typename T> TGX_INLINE inline void swap(T& a, T& b) { T c(a); a = b; b = c; }


    /** Don't know why but faster than fminf() for floats. */
    template<typename T> TGX_INLINE inline T min(const T & a, const T & b) { return((a < b) ? a : b); }

    
    /** Don't know why but much faster than fmaxf() for floats. */
    template<typename T> TGX_INLINE inline T max(const T & a, const T & b) { return((a > b) ? a : b); }

        
    /** Template clamp version */
    template<typename T> TGX_INLINE inline T clamp(const T & v, const T & vmin, const T & vmax)
        {
        return max(vmin, min(vmax, v));
        }


    /** Rounding for floats */
    TGX_INLINE inline float roundfp(const float f) { return roundf(f);  }
    

    /** Rounding for doubles */
    TGX_INLINE inline double roundfp(const double f) { return round(f); }


    /** Reinterpret the bits of a float as a uint32_t */
    TGX_INLINE inline uint32_t float_as_uint32(float f) { uint32_t u; memcpy(&u, &f, sizeof(uint32_t)); return u; }


    /** Reinterpret the bits of a uint32_t as a float */
    TGX_INLINE inline float uint32_as_float(uint32_t u) { float f; memcpy(&f, &u, sizeof(float)); return f; }


    /**
     * Return a value smaller or equal to B such that the multiplication by A is safe (no overflow with int32).
     */
    TGX_INLINE inline int32_t safeMultB(int32_t A, int32_t B)
        {
        if ((A == 0) || (B == 0)) return B;
        const int32_t max32 = 2147483647;
        const int32_t nB = max32 / ((A > 0) ? A : (-A));
        return ((B <= nB) ? B : nB);
        }



    /**
    * Fast (approximate) computation of 1/x. Version for float.
    * 
    * Credit Ken Cooke (@joestash)
    */
    TGX_INLINE inline float fast_inv(float x)
        {
#if defined(__XTENSA__) && !defined(__XTENSA_SOFT_FLOAT__)
        // 2 NR iterations, error < 1 ULP
        float t, result;
        asm volatile (
            "recip0.s   %0, %2\n\t"
            "const.s    %1, 1\n\t"
            "msub.s     %1, %2, %0\n\t"
            "madd.s     %0, %0, %1\n\t"
            "const.s    %1, 1\n\t"
            "msub.s     %1, %2, %0\n\t"
            "maddn.s    %0, %0, %1"
            : "=&f" (result),
              "=&f" (t)
            : "f" (x)
        );
        return result;
#elif TGX_USE_FAST_INV_TRICK
        // error < 14.3 ULP (8.91e-7)
        // float y = uint32_as_float(0x7ef33409 - float_as_uint32(x));
        //
        // y = fmaf(y, fmaf(-x, y, 1.00127125f), y);
        // y = fmaf(y, fmaf(-x, y, 1.00000083f), y);

        // error < 16.5 ULP (1.03e-6)
        float y = uint32_as_float(0x7ef335a7 - float_as_uint32(x));

        y *= fmaf(-x, y, 2.00128722f);
        y *= fmaf(-x, y, 2.00000072f);
        return y;
#else 
        return ((x == 0) ? 1.0f : (1.0f / x));
#endif            
        }


    /**
    * Fast (approximate) computation of 1/x. Version for double.
    */
    TGX_INLINE inline double fast_inv(double x)
        {
        // do not use fast approximation for double type.             
        return ((x == 0) ? 1.0 : (1.0 / x));    
        }



    /**
    * Compute the square root of a float (exact computation). 
    */
    TGX_INLINE inline float precise_sqrt(float x)
        {
        return sqrtf(x);
        }


    /**
    * Compute the square root of a double (exact computation).
    */
    TGX_INLINE inline double precise_sqrt(double x)
        {
        return sqrt(x);
        }


    /**
    * Compute a fast approximation of the square root of a float.
    * 
    * Credit Ken Cooke (@joestash)
    */
    TGX_INLINE inline float fast_sqrt(float x)
        {
#if TGX_USE_FAST_SQRT_TRICK
        // error < 11321 ULP (8.81e-4)
        float y = uint32_as_float(0x5f0b3892 - (float_as_uint32(x) >> 1));

        return x * y * fmaf(-x, y * y, 1.89099002f);
#else 
        return precise_sqrt(x);
#endif            
        }


    /**
    * Compute a fast approximation of the square root of a double.
    */
    TGX_INLINE inline double fast_sqrt(double x)
        {
        // do not use fast approximation for double type.             
        return precise_sqrt(x);
        }


    /**
    * Compute the inverse square root of a float (exact computation).
    * 
    * Credit Ken Cooke (@joestash)
    */
    TGX_INLINE inline float precise_invsqrt(float x)
        {
#if defined(__XTENSA__) && !defined(__XTENSA_SOFT_FLOAT__)
        // 2 NR iterations, error < 2 ULP
        float t0, t1, t2, t3, result;
        asm volatile (
            "rsqrt0.s   %0, %5\n\t"
            "mul.s      %1, %5, %0\n\t"
            "const.s    %2, 3\n\t"
            "mul.s      %3, %2, %0\n\t"
            "const.s    %4, 1\n\t"
            "msub.s     %4, %1, %0\n\t"
            "madd.s     %0, %3, %4\n\t"
            "mul.s      %1, %5, %0\n\t"
            "mul.s      %3, %2, %0\n\t"
            "const.s    %4, 1\n\t"
            "msub.s     %4, %1, %0\n\t"
            "maddn.s    %0, %3, %4"
            : "=&f" (result),
              "=&f" (t0),
              "=&f" (t1),
              "=&f" (t2),
              "=&f" (t3)
            : "f" (x)
        );
        return result;
#else
        const float s = sqrtf(x);
        return (s == 0) ? 1.0f : (1.0f / s);
#endif
        }


    /**
    * Compute the inverse square root of a double (exact computation).
    */
    TGX_INLINE inline double precise_invsqrt(double x)
        {
        const double s = sqrt(x);
        return (s == 0) ? 1.0 : (1.0 / sqrt(s));
        }

    
    /**
    * Compute a fast approximation of the inverse square root of a float.
    * 
    * Credit Ken Cooke (@joestash)
    */
    TGX_INLINE inline float fast_invsqrt(float x)
        {
#if defined(__XTENSA__) && !defined(__XTENSA_SOFT_FLOAT__)
        // 1 NR iteration, error < 728 ULP
        float t0, t1, t2, t3, result;
        asm volatile (
            "rsqrt0.s   %0, %5\n\t"
            "mul.s      %1, %5, %0\n\t"
            "const.s    %2, 3\n\t"
            "mul.s      %3, %2, %0\n\t"
            "const.s    %4, 1\n\t"
            "msub.s     %4, %1, %0\n\t"
            "maddn.s    %0, %3, %4"
            : "=&f" (result),
              "=&f" (t0),
              "=&f" (t1),
              "=&f" (t2),
              "=&f" (t3)
            : "f" (x)
        );
        return result;
#elif TGX_USE_FAST_INV_SQRT_TRICK
        // error < 12536 ULP (8.81e-4)
        float y = uint32_as_float(0x5f0b3892 - (float_as_uint32(x) >> 1));

        return y * fmaf(-x, y * y, 1.89099002f);
#else      
        return precise_invsqrt(x);
#endif
        }

   
    /**
    * Compute a fast approximation of the inverse square root of a float.
    */
    TGX_INLINE inline double fast_invsqrt(double x)
        {            
        // do not use fast approximation for double type.             
        return precise_invsqrt(x);
        }


    /**
    * Compute (int32_t)floorf(x).
    * 
    * Credit Ken Cooke (@joestash)
    */
    TGX_INLINE inline int32_t lfloorf(float x)
        {
#if defined(__XTENSA__) && !defined(__XTENSA_SOFT_FLOAT__)
        uint32_t result;
        asm volatile (
            "floor.s    %0, %1, 0"
            : "=a" (result)
            : "f" (x)
        );
        return result;
#else
        return (int32_t)floorf(x);
#endif
        }
}

#endif

#endif


/** end of file */

