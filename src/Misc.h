/**   
 * @file Misc.h
 * Utility/miscellaneous functions used throughout the library. 
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

#ifndef _TGX_MISC_H_
#define _TGX_MISC_H_


#include <stdint.h>
#include <math.h>
#include <string.h>


// disable mtools extensions by default
#ifndef MTOOLS_TGX_EXTENSIONS
#define MTOOLS_TGX_EXTENSIONS 0 
#endif



#if defined(TEENSYDUINO) || defined(ESP32)
    #include "Arduino.h" // include Arduino to get PROGMEM macro and others
    #define TGX_ON_ARDUINO

    #define TGX_USE_FAST_INV_SQRT_TRICK
    #define TGX_USE_FAST_SQRT_TRICK     
    //#define TGX_USE_FAST_INV_TRICK  // bug and slower then regular inv anyway... 
     
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE __attribute__((noinline, noclone)) FLASHMEM
#else    
    #define TGX_INLINE    
    #define TGX_NOINLINE
#endif

#ifndef PROGMEM
    #define PROGMEM
#endif

#ifndef FLASHMEM
    #define FLASHMEM
#endif


/*  */
#ifndef TGX_SINGLE_PRECISION_COMPUTATIONS
    #define TGX_SINGLE_PRECISION_COMPUTATIONS 1  ///< Set this to 1 to use float as the default floating point type and set it to 0 to use double precision instead.
#endif

 
#define TGX_DEFAULT_NO_BLENDING -1.0f ///< Default blending operation for drawing primitive: overwrite instead of blending. 


#if defined(TEENSYDUINO) || defined(ESP32)
/* Size of the cache when reading in PROGMEM. This value is used to try to optimize cache read to
improve rendering speed when reading large image in flash... On teensy, 8K give good results.*/
#define TGX_PROGMEM_DEFAULT_CACHE_SIZE 8192 
#else 
/* Size of the cache when reading in PROGMEM. This value is used to try to optimize cache read to
improve rendering speed when reading large image in flash... Can use large value on CPU.*/
#define TGX_PROGMEM_DEFAULT_CACHE_SIZE 262144
#endif


/** macro to cast indexes as 32bit when doing pointer arithmetic */
#define TGX_CAST32(a)   ((int32_t)(a))


// c++, no plain c
#ifdef __cplusplus

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
#define M_PI       3.14159265358979323846
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
    */
    TGX_INLINE inline float fast_inv(float x)
        {
#if defined (TGX_USE_FAST_INV_TRICK)            
        union
            {
            float f;
            uint32_t u;
            } v;
        v.f = x;
        v.u = 0x5f375a86 - (v.u >> 1); // slightly more precise than the original 0x5f3759df
        const float x2 = x * 0.5f;
        const float threehalfs = 1.5f;
        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 1st iteration
//        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 2nd iteration (not needed)
        return v.f * v.f;                   
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
    */
    TGX_INLINE inline float fast_sqrt(float x)
        {
#if defined (TGX_USE_FAST_SQRT_TRICK)            
        union
            {
            float f;
            uint32_t u;
            } v;
        v.f = x;
        v.u = 0x5f375a86 - (v.u >> 1); // slightly more precise than the original 0x5f3759df
        const float x2 = x * 0.5f;
        const float threehalfs = 1.5f;
        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 1st iteration
//        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 2nd iteration (not needed)
        return x * v.f;              
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
    */
    TGX_INLINE inline float precise_invsqrt(float x)
        {
        const float s = sqrtf(x);
        return (s == 0) ? 1.0f : (1.0f / s);
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
    */
    TGX_INLINE inline float fast_invsqrt(float x)
        {            
#if defined (TGX_USE_FAST_INV_SQRT_TRICK)            
        // fast reciprocal square root : https://en.wikipedia.org/wiki/Fast_inverse_square_root
        //github.com/JarkkoPFC/meshlete/blob/master/src/core/math/fast_math.inl
        union
            {
            float f;
            uint32_t u;
            } v;
        v.f = x;
        v.u = 0x5f375a86 - (v.u >> 1); // slightly more precise than the original 0x5f3759df
        const float x2 = x * 0.5f;
        const float threehalfs = 1.5f;
        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 1st iteration
//        v.f = v.f * (threehalfs - (x2 * v.f * v.f));		// 2nd iteration (not needed)
        return v.f;
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

}

#endif

#endif


/** end of file */

