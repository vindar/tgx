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
#elif defined(ESP_PLATFORM)
    #include "sdkconfig.h"
#endif

#ifndef PROGMEM
    #define PROGMEM
#endif

#ifndef FLASHMEM
    #define FLASHMEM
#endif



// generic check for FPU support
#if defined(__ARM_FP) && ((__ARM_FP & 0x4) != 0)
    #define TGX_COMPILER_HAS_FPU 1
#elif defined(__XTENSA__) && !defined(__XTENSA_SOFT_FLOAT__)
    #define TGX_COMPILER_HAS_FPU 1
#elif defined(__riscv_flen) && (__riscv_flen >= 32)
    #define TGX_COMPILER_HAS_FPU 1
#else
    #define TGX_COMPILER_HAS_FPU 0
#endif


//
// Board specific optimizations.
//
// - TGX_PROGMEM_DEFAULT_CACHE_SIZE : size of the cache when reading from PROGMEM. Use to optimize cache read when accessing image in flash.
//
// - TGX_USE_FAST_INV_SQRT_TRICK : fast inverse 'quake like' square root trick to speed up computations.
// - TGX_USE_FAST_SQRT_TRICK : fast 'quake like' square root trick to speed up computations.
// - TGX_USE_FAST_INV_TRICK : fast inverse 'quake like' trick to speed up computations.
// - TGX_USE_FMA_MATH : use explicit fused multiply-add in small floating-point vector/matrix operations.
// - TGX_USE_FMA_MATH_MISC : use explicit fused multiply-add in Misc.h fast math approximations.
// - TGX_DRAWSPHERE_USE_STRIP_BANDS : use triangle strips for Gouraud shaded sphere bands.
// - TGX_RASTERIZE_TRIANGLE_INLINE : inlining strategy for the main triangle rasterizer entry point.
//
// - TGX_INLINE/TGX_NOINLINE : inlining strategy for time critical/non-critical functions.
//
/**
 * @def TGX_USE_FAST_INV_TRICK
 * Enable the fast approximate inverse trick on architectures where it is beneficial.
 *
 * This is selected automatically from the target board/architecture below,
 * unless explicitly overridden before including TGX headers or from the
 * compiler command line.
 */
/**
 * @def TGX_USE_FMA_MATH
 * Use explicit fused multiply-add instructions in selected floating-point math
 * helpers when this improves performance on the target architecture.
 *
 * This is selected automatically from the target board/architecture below,
 * unless explicitly overridden before including TGX headers or from the
 * compiler command line.
 */
/**
 * @def TGX_USE_FMA_MATH_MISC
 * Use explicit fused multiply-add instructions in Misc.h fast math
 * approximation helpers when this improves performance on the target
 * architecture.
 *
 * This is selected automatically from the target board/architecture below,
 * unless explicitly overridden before including TGX headers or from the
 * compiler command line.
 */
/**
 * @def TGX_DRAWSPHERE_USE_STRIP_BANDS
 * Use a triangle-strip path for the middle bands of Gouraud shaded spheres.
 *
 * This is selected automatically from the target board/architecture below,
 * unless explicitly overridden before including TGX headers or from the
 * compiler command line.
 */

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__ANDROID__) || defined(__unix__)
    // TGX is running on a computer.
    #define TGX_RUN_ON_CPU
    #define TGX_CONFIG_HAS_FPU 1
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 262144
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
    // teensy 4.0 and 4.1
    #define TGX_RUN_ON_TEENSY4
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 8192
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 1
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 1
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE __attribute__((always_inline))
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_TEENSY36)
    // teensy 3.6
    #define TGX_RUN_ON_TEENSY3
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_TEENSY35)
    // teensy 3.5
    #define TGX_RUN_ON_TEENSY3
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_TEENSY32) || defined(ARDUINO_TEENSY31)
    // teensy 3.1 and 3.2
    #define TGX_RUN_ON_TEENSY3
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 1024
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_TEENSYLC)
    // teensy LC
    #define TGX_RUN_ON_TEENSYLC 
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 256
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_ARCH_RP2350) || defined(PICO_RP2350) || defined(TARGET_RP2350)
    // Raspberry Pico 2350
    #define TGX_RUN_ON_RP2350
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 1
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE  __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE __attribute__((always_inline))
    #define TGX_CONFIG_NOINLINE

#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) || defined(TARGET_RP2040)
    // Raspberry Pico 2040
    #define TGX_RUN_ON_RP2040
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(ESP32S2)
    // ESP32 S2
    #define TGX_RUN_ON_ESP32S2
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ESP32S3)
    // ESP32 S3
    #define TGX_RUN_ON_ESP32S3
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0 // unused: specific assembly code used instead.
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0     //
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0      // unused: specific assembly code used instead.
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE __attribute__((always_inline))
    #define TGX_CONFIG_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32P4) || defined(ESP32P4)
    // ESP32 P4
    #define TGX_RUN_ON_ESP32P4
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP32)
    // fallback to original
    // nb: `ESP32` is defined for all ESP32 variants so this case should be the last
    #define TGX_RUN_ON_ESP32
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0 // unused: specific assembly code used instead.
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0     //
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0      //
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(__ARM_ARCH_6M__)
    // generic Cortex-M0 (use same setting as Teensy LC)
    #define TGX_RUN_ON_TEENSYLC
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 256
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(__ARM_ARCH_7M__)
    // generic Cortex-M3 (use same setting as Teensy3.2)
    #define TGX_RUN_ON_TEENSY3
    #define TGX_CONFIG_HAS_FPU 0
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 1024
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE  __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif (defined(__ARM_ARCH_7EM__) && defined(__ARM_FP) && ((__ARM_FP & 0x8) != 0)) || defined(STM32H7xx)
    // generic Cortex-M7 (use same setting as Teensy 4.0/4.1)
    #define TGX_RUN_ON_TEENSY4
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 1
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 1
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE __attribute__((always_inline))
    #define TGX_CONFIG_NOINLINE

#elif defined(__ARM_ARCH_7EM__)
    // generic Cortex-M4 (use same setting as Teensy 3.6/3.5)
    #define TGX_RUN_ON_TEENSY3
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 2048
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE

#elif defined(__ARM_ARCH_8M_MAIN__)
    // generic Cortex-M33 (use same setting as RP2350)
    #define TGX_RUN_ON_RP2350
    #define TGX_CONFIG_HAS_FPU TGX_COMPILER_HAS_FPU
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 4096
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 1
    #define TGX_CONFIG_USE_FAST_INV_TRICK 1
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 1
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 1
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE __attribute__((always_inline))
    #define TGX_CONFIG_INLINE_ZDIVIDE __attribute__((always_inline))
    #define TGX_CONFIG_NOINLINE

#else
    // unknown board/architecture
    #define TGX_RUN_ON_CPU
    #define TGX_CONFIG_HAS_FPU 1
    #define TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE 1024
    #define TGX_CONFIG_USE_FAST_INV_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_SQRT_TRICK 0
    #define TGX_CONFIG_USE_FAST_INV_TRICK 0
    #define TGX_CONFIG_USE_FMA_MATH 0
    #define TGX_CONFIG_USE_FMA_MATH_MISC 0
    #define TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 0
    #define TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS 0
    #define TGX_CONFIG_INLINE
    #define TGX_CONFIG_INLINE_ZDIVIDE
    #define TGX_CONFIG_NOINLINE
#endif


// Apply board defaults unless the user supplied an explicit TGX_* override.
#ifndef TGX_HAS_FPU
    #define TGX_HAS_FPU TGX_CONFIG_HAS_FPU
#endif

#ifndef TGX_PROGMEM_DEFAULT_CACHE_SIZE
    #define TGX_PROGMEM_DEFAULT_CACHE_SIZE TGX_CONFIG_PROGMEM_DEFAULT_CACHE_SIZE
#endif

#ifndef TGX_USE_FAST_INV_SQRT_TRICK
    #define TGX_USE_FAST_INV_SQRT_TRICK TGX_CONFIG_USE_FAST_INV_SQRT_TRICK
#endif

#ifndef TGX_USE_FAST_SQRT_TRICK
    #define TGX_USE_FAST_SQRT_TRICK TGX_CONFIG_USE_FAST_SQRT_TRICK
#endif

#ifndef TGX_USE_FAST_INV_TRICK
    #define TGX_USE_FAST_INV_TRICK TGX_CONFIG_USE_FAST_INV_TRICK
#endif

#ifndef TGX_USE_FMA_MATH
    #define TGX_USE_FMA_MATH TGX_CONFIG_USE_FMA_MATH
#endif

#ifndef TGX_USE_FMA_MATH_MISC
    #define TGX_USE_FMA_MATH_MISC TGX_CONFIG_USE_FMA_MATH_MISC
#endif

#ifndef TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
    #define TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS TGX_CONFIG_SHADER_USE_INCREMENTAL_PIXEL_POINTERS
#endif

#ifndef TGX_DRAWSPHERE_USE_STRIP_BANDS
    #define TGX_DRAWSPHERE_USE_STRIP_BANDS TGX_CONFIG_DRAWSPHERE_USE_STRIP_BANDS
#endif

#ifndef TGX_INLINE
    #define TGX_INLINE TGX_CONFIG_INLINE
#endif

#ifndef TGX_INLINE_ZDIVIDE
    #define TGX_INLINE_ZDIVIDE TGX_CONFIG_INLINE_ZDIVIDE
#endif

#ifndef TGX_NOINLINE
    #define TGX_NOINLINE TGX_CONFIG_NOINLINE
#endif

#ifndef TGX_RASTERIZE_TRIANGLE_INLINE
    #ifdef TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
        #define TGX_RASTERIZE_TRIANGLE_INLINE TGX_CONFIG_RASTERIZE_TRIANGLE_INLINE
    #else
        #define TGX_RASTERIZE_TRIANGLE_INLINE TGX_INLINE
    #endif
#endif

#ifndef TGX_UBER_SHADER_INLINE
    #define TGX_UBER_SHADER_INLINE TGX_INLINE
#endif

#ifndef TGX_SHADER_SELECT_INLINE
    #define TGX_SHADER_SELECT_INLINE TGX_INLINE
#endif

#ifndef TGX_RENDERER3D_SHADING_INLINE
    #define TGX_RENDERER3D_SHADING_INLINE TGX_INLINE
#endif

#if defined(ESP32) || defined(ESP_PLATFORM)
    #include "esp_attr.h"
    #define TGX_HOT_IRAM IRAM_ATTR
#else
    #define TGX_HOT_IRAM
#endif





/**
 * flags for uber_shader compilation
 */

#ifndef TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL
#if TGX_HAS_FPU
#define TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL 1
#else // disable incremental float computation for Gouraud shading on architectures without FPU
#define TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL 0
#endif
#endif

#ifndef TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL
#if TGX_HAS_FPU
#define TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL 1
#else  // disable incremental float computation for Gouraud shading on architectures without FPU
#define TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL 0
#endif
#endif

#ifndef TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE
#define TGX_SHADER_RGB565_FAST_TEXTURE_MODULATE 1
#endif

#ifndef TGX_SHADER_RGB565_FAST_BILINEAR
#define TGX_SHADER_RGB565_FAST_BILINEAR 1
#endif

#ifndef TGX_SHADER_RGB565_FAST_CLAMP
#define TGX_SHADER_RGB565_FAST_CLAMP 0
#endif







/**
 * @def TGX_SINGLE_PRECISION_COMPUTATIONS
 * Set to 1 to use float as the default floating point type, or to 0 to use double precision.
 */
#ifndef TGX_SINGLE_PRECISION_COMPUTATIONS
    #define TGX_SINGLE_PRECISION_COMPUTATIONS 1
#endif


/**
 * @def TGX_DEFAULT_NO_BLENDING
 * Default opacity sentinel used by drawing primitives to request overwrite/no-blending mode.
 */
#define TGX_DEFAULT_NO_BLENDING -1.0f


/**
 * @def TGX_MESHLET_SPHERE_CLIP
 * Set to 1 to let Mesh3Dv2 skip per-triangle clipping for meshlets whose bounding sphere is fully inside the frustum, and discard meshlets fully outside.
 */
#ifndef TGX_MESHLET_SPHERE_CLIP
    #define TGX_MESHLET_SPHERE_CLIP 1
#endif

/**
 * @def TGX_MESHLET_WIREFRAME_CULL
 * Set to 1 to let Mesh3Dv2 wireframe rendering discard meshlets using their visibility cones.
 */
#ifndef TGX_MESHLET_WIREFRAME_CULL
    #define TGX_MESHLET_WIREFRAME_CULL 1
#endif


/** end of file */
