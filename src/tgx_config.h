/**   
 * @file tgx_config.h
 * Configuration file depending on the architecture
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




// disable mtools extensions by default
#ifndef MTOOLS_TGX_EXTENSIONS
#define MTOOLS_TGX_EXTENSIONS 0 
#endif


#ifdef ARDUINO
    #include <Arduino.h>
    #define TGX_ON_ARDUINO
#endif

#ifndef PROGMEM
    #define PROGMEM
#endif

#ifndef FLASHMEM
    #define FLASHMEM
#endif



//
// Board specific optimizations.
// 
// - TGX_PROGMEM_DEFAULT_CACHE_SIZE : size of the cache when reading from PROGMEM. Use to optimize cache read when accesing image in flash. 
// 
// - TGX_USE_FAST_INV_SQRT_TRICK : fast inverse 'quake like' square root trick to speed up computations.
// - TGX_USE_FAST_SQRT_TRICK : fast 'quake like' square root trick to speed up computations.
// - TGX_USE_FAST_INV_TRICK : fast inverse 'quake like' trick to speed up computations.
//  
// - TGX_INLINE/TGX_NOINLINE : inlining strategy for time critical/non-critical functions.
//
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__ANDROID__) || defined(__unix__)
    // TGX is running on a computer. 
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 262144
    #define TGX_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE 
    #define TGX_NOINLINE

#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
    // teensy 4.0 and 4.1
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 8192
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 1
    #define TGX_USE_FAST_INV_TRICK 1
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE __attribute__((noinline, noclone)) FLASHMEM

#elif defined(ARDUINO_TEENSY36)
    // teensy 3.6
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE __attribute__((always_inline)) 
    #define TGX_NOINLINE 

#elif defined(ARDUINO_TEENSY35)
    // teensy 3.5
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE 

#elif defined(ARDUINO_TEENSY32) || defined(ARDUINO_TEENSY31)
    // teensy 3.1 and 3.2
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 1024
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE 

#elif defined(ARDUINO_TEENSYLC)
    // teensy LC
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 256
    #define TGX_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE 
    #define TGX_NOINLINE

#elif defined(ARDUINO_ARCH_RP2350) || defined(PICO_RP2350)
    // Raspberry Pico 2350
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 1
    #define TGX_USE_FAST_INV_TRICK 1
    #define TGX_INLINE  __attribute__((always_inline))
    #define TGX_NOINLINE

#elif defined(ARUINO_ARCH_RP2040) ||defined(PICO_RP2040)
    // Raspberry Pico 2040
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE 
    #define TGX_NOINLINE 

#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ESP32S2)
    // ESP32 S2
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_USE_FAST_SQRT_TRICK 0     
    #define TGX_USE_FAST_INV_TRICK 0      
    #define TGX_INLINE 
    #define TGX_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ESP32S3)
    // ESP32 S3
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_USE_FAST_INV_SQRT_TRICK 0 // unused: specific assembly code used instead. 
    #define TGX_USE_FAST_SQRT_TRICK 0     // 
    #define TGX_USE_FAST_INV_TRICK 0      //
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP32)
    // fallback to original
    // nb: `ESP32` is defined for all ESP32 variants so this case should be the last
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_USE_FAST_INV_SQRT_TRICK 0 // unused: specific assembly code used instead. 
    #define TGX_USE_FAST_SQRT_TRICK 0     // 
    #define TGX_USE_FAST_INV_TRICK 0      //
    #define TGX_INLINE __attribute__((always_inline)) 
    #define TGX_NOINLINE

#elif defined(__ARM_ARCH_6M__)
    // generic Cortex-M0 (use same setting as Teensy LC)
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 256
    #define TGX_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE 
    #define TGX_NOINLINE

#elif defined(__ARM_ARCH_7M__)
    // generic Cortex-M3 (use same setting as Teensy3.2)
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 1024
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE  __attribute__((always_inline))
    #define TGX_NOINLINE

#elif (defined(__ARM_ARCH_7EM__) && defined(__ARM_FP) && (__ARM_FP == 0xC)) || defined(STM32H7xx)
    // generic Cortex-M7 (use same setting as Teensy 4.0/4.1)
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 1
    #define TGX_USE_FAST_INV_TRICK 1
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE

#elif defined(__ARM_ARCH_7EM__) 
    // generic Cortex-M4 (use same setting as Teensy 3.6/3.5)
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE __attribute__((always_inline))
    #define TGX_NOINLINE

#elif defined(__ARM_ARCH_8M_MAIN__)
    // generic Cortex-M33 (use same setting as RP2350)
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_USE_FAST_SQRT_TRICK 1
    #define TGX_USE_FAST_INV_TRICK 1
    #define TGX_INLINE __attribute__((always_inline)) 
    #define TGX_NOINLINE

#else
    // unkwown board/architecture
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE 1024 
    #define TGX_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_USE_FAST_SQRT_TRICK 0
    #define TGX_USE_FAST_INV_TRICK 0
    #define TGX_INLINE 
    #define TGX_NOINLINE
#endif




// Set this to 1 to use float as the default floating point type and set it to 0 to use double precision instead.
#ifndef TGX_SINGLE_PRECISION_COMPUTATIONS
    #define TGX_SINGLE_PRECISION_COMPUTATIONS 1  
#endif

 
// Default blending operation for drawing primitive: overwrite instead of blending.
#define TGX_DEFAULT_NO_BLENDING -1.0f  


/** end of file */ 

