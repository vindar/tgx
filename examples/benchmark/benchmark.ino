/********************************************************************
*
* tgx library example
*
* 
* Run a very crude 'benchmark' of the library 3D mesh rendering engine
* 
* Used to assert the impact of code changes in the library...
*
********************************************************************/
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


/**
* Define below the platform used.
*
* If no platform is defined externally, Teensy 4.x is selected by default.
**/
//#define TGX_BENCHMARK_T4            // use  600MHz, fastest without LTO
//#define TGX_BENCHMARK_T36           // use  180MHz, optimized
//#define TGX_BENCHMARK_T35           // use  120MHz, optimized
//#define TGX_BENCHMARK_ESP32         // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_ESP32S2       // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_ESP32S3       // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_RP2040        // use  200MHz, Ofast (RTTI, stack and exception disabled)  
//#define TGX_BENCHMARK_RP2350        // use  150MHz, Ofast (RTTI, stack and exception disabled)
//#define TGX_BENCHMARK_CPU          

#if !defined(TGX_BENCHMARK_T4) && !defined(TGX_BENCHMARK_T36) && !defined(TGX_BENCHMARK_T35) && !defined(TGX_BENCHMARK_ESP32) && !defined(TGX_BENCHMARK_ESP32S2) && !defined(TGX_BENCHMARK_ESP32S3) && !defined(TGX_BENCHMARK_RP2040) && !defined(TGX_BENCHMARK_RP2350) && !defined(TGX_BENCHMARK_CPU)
    #if defined(__IMXRT1062__)
        #define TGX_BENCHMARK_T4
    #elif defined(__MK66FX1M0__)
        #define TGX_BENCHMARK_T36
    #elif defined(__MK64FX512__)
        #define TGX_BENCHMARK_T35
    #elif defined(ARDUINO_ARCH_RP2040)
        #define TGX_BENCHMARK_RP2040
    #else
        #define TGX_BENCHMARK_T4
    #endif
#endif

#if ((defined(TGX_BENCHMARK_T4) ? 1 : 0) + (defined(TGX_BENCHMARK_T36) ? 1 : 0) + (defined(TGX_BENCHMARK_T35) ? 1 : 0) + (defined(TGX_BENCHMARK_ESP32) ? 1 : 0) + (defined(TGX_BENCHMARK_ESP32S2) ? 1 : 0) + (defined(TGX_BENCHMARK_ESP32S3) ? 1 : 0) + (defined(TGX_BENCHMARK_RP2040) ? 1 : 0) + (defined(TGX_BENCHMARK_RP2350) ? 1 : 0) + (defined(TGX_BENCHMARK_CPU) ? 1 : 0)) != 1
    #error "Define exactly one TGX_BENCHMARK_* platform"
#endif

#ifdef TGX_BENCHMARK_CPU
    #include <chrono>
    #include <cstdarg>
    #include <cstdio>
    #include <thread>
    #include <stdint.h>

    class SerialCPU
        {
        public:
            void begin(uint32_t) {}
            void flush() {}
            int available() { return 1; }
            void print(const char* s) { std::fputs(s, stdout); }
            void println(const char* s = "") { std::fputs(s, stdout); std::fputc('\n', stdout); }
            int printf(const char* format, ...)
                {
                va_list args;
                va_start(args, format);
                const int r = std::vprintf(format, args);
                va_end(args);
                return r;
                }
        };

    static SerialCPU Serial;
    static const auto TGX_CPU_START_TIME = std::chrono::steady_clock::now();

    uint32_t micros()
        {
        return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - TGX_CPU_START_TIME).count();
        }

    uint32_t millis()
        {
        return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - TGX_CPU_START_TIME).count();
        }

    void delay(uint32_t ms)
        {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
#else
    #include <Arduino.h>
#endif

#include <tgx.h>
#include <stdlib.h>
using namespace tgx;

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef FLASHMEM
#define FLASHMEM
#endif

#include "3Dmodels/bunny_fig_small.h"

#if !defined(TGX_BENCHMARK_T36) && !defined(TGX_BENCHMARK_T35)
    #define TGX_BENCHMARK_FULL_PROFILE 1
#else
    #define TGX_BENCHMARK_FULL_PROFILE 0
#endif

#if TGX_BENCHMARK_FULL_PROFILE
    #include "3Dmodels/buddha.h"
    #include "3Dmodels/R2D2.h"
#endif


#ifdef TGX_BENCHMARK_T4
    #define NBFRAMES 60
    #define DEV_NAME "Teensy 4.0/4.1"
    #define MAX_ALLOC_BYTES (220*1024)
    uint16_t fb[MAX_ALLOC_BYTES/2];
    DMAMEM uint16_t zbuf[MAX_ALLOC_BYTES/2];
    void allocateBuffers() { }
#endif

#ifdef TGX_BENCHMARK_T36
    #define NBFRAMES 24
    #define DEV_NAME "Teensy 3.6"
    #define MAX_ALLOC_BYTES (80*1024)
    uint16_t fb[MAX_ALLOC_BYTES/2];
    uint16_t zbuf[MAX_ALLOC_BYTES/2];
    void allocateBuffers() { }
#endif

#ifdef TGX_BENCHMARK_T35
    #define NBFRAMES 20
    #define DEV_NAME "Teensy 3.5"
    #define MAX_ALLOC_BYTES (70*1024)
    uint16_t fb[MAX_ALLOC_BYTES/2];
    uint16_t zbuf[MAX_ALLOC_BYTES/2];
    void allocateBuffers() { }
#endif

#ifdef TGX_BENCHMARK_ESP32
    #define NBFRAMES 45
    #define DEV_NAME "ESP32"
    #define MAX_ALLOC_BYTES (90*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if (!zbuf) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_ESP32S2
    #define NBFRAMES 30
    #define DEV_NAME "ESP32-S2"
    #define MAX_ALLOC_BYTES (80*1024)
    uint16_t* fb = nullptr;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        fb = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if ((!fb) || (!zbuf)) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_ESP32S3
    #define NBFRAMES 45
    #define DEV_NAME "ESP32-S3"
    #define MAX_ALLOC_BYTES (120*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if (!zbuf) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_RP2040
    #define NBFRAMES 30
    #define DEV_NAME "RP2040"
    #define MAX_ALLOC_BYTES (90*1024)
    uint16_t* fb = nullptr;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        fb = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        zbuf = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        if ((!fb) || (!zbuf)) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_RP2350
    #define NBFRAMES 30
    #define DEV_NAME "RP2350"
    #define MAX_ALLOC_BYTES (220*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        if (!zbuf) { while (1) { Serial.println("*** Failed to allocate buffers ***"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_CPU
    #define NBFRAMES 120
    #define DEV_NAME "CPU"
    #define MAX_ALLOC_BYTES (9*1024*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t zbuf[MAX_ALLOC_BYTES / 2];
    void allocateBuffers() {}
#endif





Image<RGB565> im;
#if TGX_BENCHMARK_FULL_PROFILE
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ORTHO | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_FLAT | SHADER_NOTEXTURE | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP | SHADER_TEXTURE_WRAP_POW2;
#else
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_FLAT | SHADER_NOTEXTURE | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;
#endif
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


bool setRenderer(int slx, int sly)
    {
    if ((slx * sly * 2) > MAX_ALLOC_BYTES) return false; // too big
    im.set(fb, slx, sly);
    renderer.setViewportSize(slx, sly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, ((float)slx) / sly, 1.0f, 300.0f);
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    return true;
    }


#define DIST_NORMAL     0
#define DIST_CLOSE      1
#define DIST_CLIPPED    2
#define DIST_FAR        3
#define DIST_FAR_CLIPPED 4

#define SCENE_BIG_TRIANGLES      0
#define SCENE_SMALL_TRI_GRID     1
#define SCENE_SMALL_TRI_CLIPPED  2
#define SCENE_DIRECT_CUBE        3
#define SCENE_DIRECT_TEX_CUBE    4
#define SCENE_DIRECT_SPHERE      5
#define SCENE_DIRECT_TEX_SPHERE  6
#define SCENE_VERTEX_COLOR       7
#define SCENE_PIXELS             8
#define SCENE_DOTS               9
#define SCENE_WIRE_CUBE          10
#define SCENE_WIRE_SPHERE        11
#define SCENE_WIRE_BUNNY         12


static const int DIRECT_TEXTURE_LX = 64;
static const int DIRECT_TEXTURE_LY = 64;
static RGB565* direct_texture_buffer = nullptr;
static Image<RGB565> direct_texture;

static const int GRID_N = 14;
static const int GRID_VERTICES = (GRID_N + 1) * (GRID_N + 1);
static const int GRID_TRIANGLES = GRID_N * GRID_N * 2;
static fVec3* grid_vertices = nullptr;
static fVec3* grid_normals = nullptr;
static fVec2* grid_texcoords = nullptr;
static uint16_t* grid_triangle_indices = nullptr;

static const int POINT_COUNT = 256;
static fVec3* point_positions = nullptr;
static int* point_color_indices = nullptr;
static int* point_opacity_indices = nullptr;
static int* point_radius_indices = nullptr;
static const RGB565 point_colors[6] = { RGB565_Red, RGB565_Green, RGB565_Blue, RGB565_Yellow, RGB565_Cyan, RGB565_Orange };
static const float point_opacities[3] = { 0.45f, 0.70f, 1.0f };
static const int point_radii[3] = { 1, 2, 3 };

struct RendererSize
    {
    int x;
    int y;
    int frame_divisor;
    const char* label;
    };

#ifdef TGX_BENCHMARK_T4
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 200, 200, 1, "medium / balanced" },
    { 320, 240, 1, "large / ILI9341" }
};
#elif defined(TGX_BENCHMARK_T36)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 160, 120, 1, "medium / 4:3" },
    { 200, 200, 1, "large for this target" }
};
#elif defined(TGX_BENCHMARK_T35)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 160, 120, 1, "large for this target" }
};
#elif defined(TGX_BENCHMARK_ESP32) || defined(TGX_BENCHMARK_ESP32S2) || defined(TGX_BENCHMARK_RP2040)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 200, 200, 1, "large for this target" }
};
#elif defined(TGX_BENCHMARK_ESP32S3)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 200, 200, 1, "medium / balanced" },
    { 240, 240, 1, "large for this target" }
};
#elif defined(TGX_BENCHMARK_RP2350)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 200, 200, 1, "medium / balanced" },
    { 320, 240, 1, "large / ILI9341" }
};
#elif defined(TGX_BENCHMARK_CPU)
static const RendererSize renderer_size_tab[] = {
    { 100, 100, 1, "small / vertex-heavy" },
    { 320, 240, 1, "large MCU-class" },
    { 2048, 2048, 30, "CPU ultra / fill stress" }
};
#endif

static const int NB_RENDERER_SIZES = sizeof(renderer_size_tab) / sizeof(renderer_size_tab[0]);
static int current_size_frame_divisor = 1;


FLASHMEM int benchFrames(int divisor)
    {
    int n = NBFRAMES / (divisor * current_size_frame_divisor);
    const int min_frames = (current_size_frame_divisor >= 16) ? 2 : 8;
    return (n < min_frames) ? min_frames : n;
    }


FLASHMEM void* benchAlloc(size_t bytes)
    {
    void* p = malloc(bytes);
    if (!p)
        {
        Serial.printf("Failed to allocate %u bytes\n", (unsigned)bytes);
        while (1) { delay(1000); }
        }
    return p;
    }


FLASHMEM void initSyntheticAssets()
    {
    if (!direct_texture_buffer)
        {
        direct_texture_buffer = (RGB565*)benchAlloc(DIRECT_TEXTURE_LX * DIRECT_TEXTURE_LY * sizeof(RGB565));
        grid_vertices = (fVec3*)benchAlloc(GRID_VERTICES * sizeof(fVec3));
        grid_normals = (fVec3*)benchAlloc(GRID_VERTICES * sizeof(fVec3));
        grid_texcoords = (fVec2*)benchAlloc(GRID_VERTICES * sizeof(fVec2));
        grid_triangle_indices = (uint16_t*)benchAlloc(GRID_TRIANGLES * 3 * sizeof(uint16_t));
        point_positions = (fVec3*)benchAlloc(POINT_COUNT * sizeof(fVec3));
        point_color_indices = (int*)benchAlloc(POINT_COUNT * sizeof(int));
        point_opacity_indices = (int*)benchAlloc(POINT_COUNT * sizeof(int));
        point_radius_indices = (int*)benchAlloc(POINT_COUNT * sizeof(int));
        }

    direct_texture.set(direct_texture_buffer, DIRECT_TEXTURE_LX, DIRECT_TEXTURE_LY);
    for (int y = 0; y < DIRECT_TEXTURE_LY; y++)
        {
        for (int x = 0; x < DIRECT_TEXTURE_LX; x++)
            {
            const int checker = (((x >> 3) ^ (y >> 3)) & 1);
            const uint8_t r = (uint8_t)(checker ? 235 : (40 + x * 3));
            const uint8_t g = (uint8_t)(checker ? (40 + y * 3) : 210);
            const uint8_t b = (uint8_t)(80 + ((x + y) & 31) * 4);
            direct_texture_buffer[x + y * DIRECT_TEXTURE_LX] = RGB565(r, g, b);
            }
        }

    int v = 0;
    for (int y = 0; y <= GRID_N; y++)
        {
        const float fy = -1.0f + 2.0f * y / GRID_N;
        for (int x = 0; x <= GRID_N; x++)
            {
            const float fx = -1.0f + 2.0f * x / GRID_N;
            grid_vertices[v] = { fx, fy, 0.08f * sinf((fx + fy) * 4.5f) };
            grid_normals[v] = { 0.0f, 0.0f, 1.0f };
            grid_texcoords[v] = { (float)x / GRID_N, (float)y / GRID_N };
            v++;
            }
        }

    int k = 0;
    for (int y = 0; y < GRID_N; y++)
        {
        for (int x = 0; x < GRID_N; x++)
            {
            const uint16_t i00 = (uint16_t)(x + y * (GRID_N + 1));
            const uint16_t i10 = (uint16_t)((x + 1) + y * (GRID_N + 1));
            const uint16_t i11 = (uint16_t)((x + 1) + (y + 1) * (GRID_N + 1));
            const uint16_t i01 = (uint16_t)(x + (y + 1) * (GRID_N + 1));
            grid_triangle_indices[k++] = i00;
            grid_triangle_indices[k++] = i10;
            grid_triangle_indices[k++] = i11;
            grid_triangle_indices[k++] = i00;
            grid_triangle_indices[k++] = i11;
            grid_triangle_indices[k++] = i01;
            }
        }

    for (int i = 0; i < POINT_COUNT; i++)
        {
        const float u = (float)i / POINT_COUNT;
        const float a = 6.2831853f * (i * 37 % POINT_COUNT) / POINT_COUNT;
        const float y = -1.0f + 2.0f * u;
        const float r = sqrtf(1.0f - y * y);
        point_positions[i] = { r * cosf(a), y, r * sinf(a) };
        point_color_indices[i] = i % 6;
        point_opacity_indices[i] = i % 3;
        point_radius_indices[i] = i % 3;
        }
    }


FLASHMEM void printBenchmarkProfile()
    {
    Serial.print("Profile: ");
#if TGX_BENCHMARK_FULL_PROFILE
    Serial.println("full real meshes + synthetic stress scenes");
    Serial.println("Real meshes: Buddha 20k triangles, R2D2 textured 8k triangles, Bunny textured 2.1k triangles");
    Serial.println("Shaders: flat, gouraud, nearest/bilinear texture, perspective/ortho, near/far clipping");
#else
    Serial.println("reduced real mesh + synthetic stress scenes");
    Serial.println("Real meshes: Bunny textured 2.1k triangles");
    Serial.println("Shaders: flat, gouraud, nearest texture, perspective, near/far clipping");
#endif
    Serial.println("Synthetic scenes: large triangles, small-triangle grid, clipped grid, direct cube/sphere, vertex colors, pixels/dots, wireframe");
    Serial.println("Renderer sizes:");
    for (int i = 0; i < NB_RENDERER_SIZES; i++)
        {
        Serial.printf("  - %dx%d: %s", renderer_size_tab[i].x, renderer_size_tab[i].y, renderer_size_tab[i].label);
        if (renderer_size_tab[i].frame_divisor > 1)
            Serial.printf(" (frames / %d)", renderer_size_tab[i].frame_divisor);
        Serial.println();
        }
    Serial.println();
    }


FLASHMEM void setProjectionForBench(bool ortho)
    {
    const float aspect = ((float)im.lx()) / im.ly();
#if TGX_BENCHMARK_FULL_PROFILE
    if (ortho)
        renderer.setOrtho(-18.0f * aspect, 18.0f * aspect, -18.0f, 18.0f, 1.0f, 300.0f);
    else
        renderer.setPerspective(45, aspect, 1.0f, 300.0f);
#else
    (void)ortho;
    renderer.setPerspective(45, aspect, 1.0f, 300.0f);
#endif
    }


FLASHMEM void setModelForMeshDistance(int dist, float a)
    {
    switch (dist)
        {
        case DIST_CLOSE:
            renderer.setModelPosScaleRot({ 0, -2.0f, -10.0f }, { 13,13,13 }, a);
            break;
        case DIST_CLIPPED:
            renderer.setModelPosScaleRot({ 0, -1.0f, -6.0f }, { 13,13,13 }, a);
            break;
        case DIST_FAR:
            renderer.setModelPosScaleRot({ 0, 0.5f, -260.0f }, { 13,13,13 }, a);
            break;
        case DIST_FAR_CLIPPED:
            renderer.setModelPosScaleRot({ 0, 0.5f, -296.0f }, { 13,13,13 }, a);
            break;
        default:
            renderer.setModelPosScaleRot({ 0, 0.5f, -35.0f }, { 13,13,13 }, a);
            break;
        }
    }


FLASHMEM void setModelForScene(int scene, float a)
    {
    switch (scene)
        {
        case SCENE_BIG_TRIANGLES:
            renderer.setModelPosScaleRot({ 0, 0.0f, -7.0f }, { 2.7f, 2.2f, 1.0f }, a * 0.35f, { 0.0f, 1.0f, 0.0f });
            break;
        case SCENE_SMALL_TRI_GRID:
            renderer.setModelPosScaleRot({ 0, 0.0f, -8.0f }, { 4.2f, 3.2f, 1.0f }, a, { 0.25f, 1.0f, 0.1f });
            break;
        case SCENE_SMALL_TRI_CLIPPED:
            renderer.setModelPosScaleRot({ 0, 0.0f, -2.6f }, { 3.6f, 3.0f, 1.0f }, a, { 0.2f, 1.0f, 0.0f });
            break;
        case SCENE_DIRECT_SPHERE:
        case SCENE_DIRECT_TEX_SPHERE:
            renderer.setModelPosScaleRot({ 0, 0.0f, -8.0f }, { 2.6f, 2.6f, 2.6f }, a, { 0.2f, 1.0f, 0.0f });
            break;
        case SCENE_PIXELS:
        case SCENE_DOTS:
            renderer.setModelPosScaleRot({ 0, 0.0f, -8.0f }, { 3.3f, 3.3f, 3.3f }, a, { 0.3f, 1.0f, 0.1f });
            break;
        case SCENE_WIRE_BUNNY:
            renderer.setModelPosScaleRot({ 0, 0.4f, -10.0f }, { 5.0f, 5.0f, 5.0f }, a, { 0.2f, 1.0f, 0.0f });
            break;
        default:
            renderer.setModelPosScaleRot({ 0, 0.0f, -8.0f }, { 2.5f, 2.5f, 2.5f }, a, { 0.25f, 1.0f, 0.15f });
            break;
        }
    }


FLASHMEM void drawSyntheticScene(int scene)
    {
    static const fVec3 N = { 0.0f, 0.0f, 1.0f };
    static const fVec3 P0 = { -1.2f, -1.0f, 0.0f };
    static const fVec3 P1 = {  1.2f, -1.0f, 0.0f };
    static const fVec3 P2 = {  1.2f,  1.0f, 0.0f };
    static const fVec3 P3 = { -1.2f,  1.0f, 0.0f };
    static const fVec3 BP0 = { -1.7f, -1.3f, 0.0f };
    static const fVec3 BP1 = {  1.7f, -1.2f, 0.0f };
    static const fVec3 BP2 = {  0.0f,  1.7f, 0.0f };
    static const fVec3 BP3 = { -1.5f,  1.1f, -0.2f };
    static const fVec3 BP4 = {  1.5f,  1.1f, 0.2f };
    static const fVec3 BP5 = {  0.0f, -1.8f, 0.0f };

    switch (scene)
        {
        case SCENE_BIG_TRIANGLES:
            renderer.drawTriangle(BP0, BP1, BP2, &N, &N, &N);
            renderer.drawTriangle(BP3, BP4, BP5, &N, &N, &N);
            break;
        case SCENE_SMALL_TRI_GRID:
        case SCENE_SMALL_TRI_CLIPPED:
            renderer.drawTriangles(GRID_TRIANGLES, grid_triangle_indices, grid_vertices, grid_triangle_indices, grid_normals, grid_triangle_indices, grid_texcoords, &direct_texture);
            break;
        case SCENE_DIRECT_CUBE:
            renderer.drawCube();
            break;
        case SCENE_DIRECT_TEX_CUBE:
            renderer.drawCube(&direct_texture, &direct_texture, &direct_texture, &direct_texture, &direct_texture, &direct_texture);
            break;
        case SCENE_DIRECT_SPHERE:
            renderer.drawSphere(18, 10);
            break;
        case SCENE_DIRECT_TEX_SPHERE:
            renderer.drawSphere(18, 10, &direct_texture);
            break;
        case SCENE_VERTEX_COLOR:
            renderer.drawQuadWithVertexColor(P0, P1, P2, P3, RGBf(1, 0.1f, 0.1f), RGBf(0.1f, 1, 0.1f), RGBf(0.1f, 0.2f, 1), RGBf(1, 1, 0.2f), &N, &N, &N, &N);
            renderer.drawTriangleWithVertexColor(BP0, BP1, BP2, RGBf(1, 0.5f, 0.1f), RGBf(0.2f, 0.9f, 1), RGBf(0.9f, 0.1f, 1), &N, &N, &N);
            break;
        case SCENE_PIXELS:
            renderer.drawPixels(POINT_COUNT, point_positions, point_color_indices, point_colors, point_opacity_indices, point_opacities);
            break;
        case SCENE_DOTS:
            renderer.drawDots(POINT_COUNT, point_positions, point_radius_indices, point_radii, point_color_indices, point_colors, point_opacity_indices, point_opacities);
            break;
        case SCENE_WIRE_CUBE:
            renderer.drawWireFrameCube(1.6f, RGB565_Yellow, 1.0f);
            break;
        case SCENE_WIRE_SPHERE:
            renderer.drawWireFrameSphere(16, 8, 1.4f, RGB565_Cyan, 1.0f);
            break;
        case SCENE_WIRE_BUNNY:
            renderer.drawWireFrameMesh(&bunny_fig_small, true, 1.2f, RGB565_White, 1.0f);
            break;
        }
    }


struct Score
    {
    uint32_t count; 
    uint32_t min_render;
    uint32_t max_render; 
    uint64_t sum_render;
    uint64_t sum_render2;
    float avg_render; 
    float std_render; 

    Score()
        {
        reset();
        }

    void reset()
        {
        count = 0;
        min_render = 0xFFFFFFFF;
        max_render = 0;
        sum_render = 0;
        sum_render2 = 0;
        avg_render = 0.0f;
        std_render = 0.0f;
        }

    void add(uint32_t us)
        {
        count++;
        if (us < min_render) min_render = us;
        if (us > max_render) max_render = us;
        sum_render += us;
        sum_render2 += ((uint64_t)us * (uint64_t)us);
        avg_render = ((float)sum_render) / count;
        std_render = sqrtf((((float)sum_render2) / count) - (avg_render * avg_render));
        }

    float fps()
        {
        return 1000000.0f / avg_render;
        }

    void print()
        {
        Serial.printf(" frames: %u\t", count);
        Serial.printf(" min: %u us\t", min_render);
        Serial.printf(" max: %u us\t", max_render);
        Serial.printf(" avg: %.2f us\t", avg_render);
        Serial.printf(" stddev: %.2f us\t", std_render);
        Serial.printf(" fps: %.2f\n", fps());
        }


    void add(Score& s, int weight)
        {
        count += s.count * weight;
        if (s.min_render < min_render) min_render = s.min_render;
        if (s.max_render > max_render) max_render = s.max_render;
        sum_render += s.sum_render * (uint64_t)weight;
        sum_render2 += s.sum_render2 * weight;
        avg_render = ((float)sum_render) / count;
        std_render = sqrtf((((float)sum_render2) / count) - (avg_render * avg_render));
        }
         
    };








/**
* Compute the score for a given mesh, with a given set of shaders at a given distance
**/
FLASHMEM Score computeScore(Score & finaleS, int weight, const char * text, const tgx::Mesh3D<tgx::RGB565>* mesh,
                   tgx::Shader shaders, int dist, int nb_frames = -1,
                   bool ortho = false, bool use_mesh_material = false)
    {
    if (nb_frames <= 0) nb_frames = benchFrames(1);
    Serial.print(text);
    Serial.print(" [");
    Score score;
    int r = 0; 
    setProjectionForBench(ortho);

    for (int i = 0; i < nb_frames; i++)
        {
        float a = i * 360.0f / nb_frames; // angle

        int nr = (i * 10)/nb_frames;
        if (nr > r)
            {
            r = nr;
            Serial.printf("."); // progress indicator
            }

        setModelForMeshDistance(dist, a);
        im.fillScreen(RGB565_Black); // clear the image        
        renderer.clearZbuffer(); // clear the z-buffer
        renderer.setShaders(shaders);
        const uint32_t m = micros();
        renderer.drawMesh(mesh, use_mesh_material);
        score.add(micros() - m);
        }
    Serial.print("] ");    
    score.print();
    finaleS.add(score, weight);
    return score;
    }


/**
* Compute the score for a synthetic scene. These scenes isolate specific costs:
* large triangles, many small triangles, direct primitive generation, point drawing and wireframe.
**/
FLASHMEM Score computeSceneScore(Score & finaleS, int weight, const char * text, int scene,
                        tgx::Shader shaders, int nb_frames = -1, bool ortho = false)
    {
    if (nb_frames <= 0) nb_frames = benchFrames(1);
    Serial.print(text);
    Serial.print(" [");
    Score score;
    int r = 0;
    setProjectionForBench(ortho);

    for (int i = 0; i < nb_frames; i++)
        {
        const float a = i * 360.0f / nb_frames;

        int nr = (i * 10) / nb_frames;
        if (nr > r)
            {
            r = nr;
            Serial.printf(".");
            }

        setModelForScene(scene, a);
        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();
        renderer.setShaders(shaders);
        const uint32_t m = micros();
        drawSyntheticScene(scene);
        score.add(micros() - m);
        }
    Serial.print("] ");
    score.print();
    finaleS.add(score, weight);
    return score;
    }


void setup()
    {
    Serial.begin(9600);
    Serial.flush(); 
    while (1)
        {
        Serial.println("Press any key to start benchmark...\n");
        for (int i = 0; i < 300; i++)
            {
            if (Serial.available()) { break; }
            delay(10);
            }
        if (Serial.available()) { break; }
        }
   
    Serial.print("--------------------------------------\n");
    Serial.print("TGX Benchmark for [");
    Serial.print(DEV_NAME);
    Serial.print("]\n--------------------------------------\n\n");
    allocateBuffers();
    initSyntheticAssets();
    printBenchmarkProfile();

    const uint32_t ems = millis(); 

    for (int k = 0; k < NB_RENDERER_SIZES; k++)
        {
        const int SLX = renderer_size_tab[k].x;
        const int SLY = renderer_size_tab[k].y;
        current_size_frame_divisor = renderer_size_tab[k].frame_divisor;
        Serial.print("\n-------------------------\n");
        Serial.printf("RENDERER SIZE: %d x %d (%s)\n", SLX, SLY, renderer_size_tab[k].label);
        Serial.printf("BASE FRAMES: %d\n", benchFrames(1));
        Serial.print("-------------------------\n");
        if (!setRenderer(SLX, SLY))
            {
            Serial.println("-> skipped (too big)\n\n");
            continue;
            }

        Score sfinalBuddha;
        Score sfinalR2D2;

#if TGX_BENCHMARK_FULL_PROFILE
        computeScore(sfinalBuddha, 10, "Buddha GOURAUD   ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalBuddha, 10,  "Buddha FLAT     ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalBuddha, 2,  "Buddha ORTHO    ", &buddha, SHADER_ORTHO | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL, -1, true);
        computeScore(sfinalBuddha, 3,  "Buddha (close)   ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_CLOSE);
        computeScore(sfinalBuddha, 1,  "Buddha (clipped) ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_CLIPPED);
        computeScore(sfinalBuddha, 3,  "Buddha (far)     ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_FAR);
        computeScore(sfinalBuddha, 1,  "Buddha (farclip) ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_FAR_CLIPPED);
        Serial.print("total: "); sfinalBuddha.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalBuddha.fps());

        computeScore(sfinalR2D2, 8, "R2D2 GOURAUD        ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalR2D2, 8, "R2D2 FLAT           ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalR2D2, 10, "R2D2 TEX_NEAREST   ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_NORMAL);
        computeScore(sfinalR2D2, 10, "R2D2 TEX_BILINEAR  ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP, DIST_NORMAL);
        computeScore(sfinalR2D2, 3,  "R2D2 (close)       ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2, DIST_CLOSE);
        computeScore(sfinalR2D2, 3,  "R2D2 (far)         ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2, DIST_FAR);
        computeScore(sfinalR2D2, 1,  "R2D2 (farclip)     ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2, DIST_FAR_CLIPPED);
        Serial.print("total: "); sfinalR2D2.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalR2D2.fps());
#endif

        Score sfinalBunny;
        computeScore(sfinalBunny, 8, "Bunny GOURAUD      ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalBunny, 8, "Bunny FLAT         ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalBunny, 10, "Bunny TEX_NEAREST ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_NORMAL);
#if TGX_BENCHMARK_FULL_PROFILE
        computeScore(sfinalBunny, 10, "Bunny TEX_BILINEAR", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP, DIST_NORMAL);
        computeScore(sfinalBunny, 2, "Bunny ORTHO TEX   ", &bunny_fig_small, SHADER_ORTHO | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_NORMAL, -1, true);
#endif
        computeScore(sfinalBunny, 3, "Bunny (close)      ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_CLOSE);
        computeScore(sfinalBunny, 3, "Bunny (far)        ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_FAR);
        computeScore(sfinalBunny, 1, "Bunny (farclip)    ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_FAR_CLIPPED);
        Serial.print("total: "); sfinalBunny.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalBunny.fps());

        Score sfinalSynthetic;
        computeSceneScore(sfinalSynthetic, 5, "Large tris FLAT Z   ", SCENE_BIG_TRIANGLES, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(2));
        computeSceneScore(sfinalSynthetic, 5, "Grid small FLAT     ", SCENE_SMALL_TRI_GRID, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(2));
        computeSceneScore(sfinalSynthetic, 5, "Grid small TEX      ", SCENE_SMALL_TRI_GRID, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, benchFrames(2));
        computeSceneScore(sfinalSynthetic, 3, "Grid near clip TEX  ", SCENE_SMALL_TRI_CLIPPED, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, benchFrames(3));
        computeSceneScore(sfinalSynthetic, 5, "Cube direct FLAT    ", SCENE_DIRECT_CUBE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(2));
#if TGX_BENCHMARK_FULL_PROFILE
        computeSceneScore(sfinalSynthetic, 5, "Cube direct TEX     ", SCENE_DIRECT_TEX_CUBE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP, benchFrames(2));
#else
        computeSceneScore(sfinalSynthetic, 5, "Cube direct TEX     ", SCENE_DIRECT_TEX_CUBE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, benchFrames(2));
#endif
        computeSceneScore(sfinalSynthetic, 4, "Sphere direct GOUR  ", SCENE_DIRECT_SPHERE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, benchFrames(3));
        computeSceneScore(sfinalSynthetic, 4, "Sphere direct TEX   ", SCENE_DIRECT_TEX_SPHERE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, benchFrames(3));
        computeSceneScore(sfinalSynthetic, 3, "Vertex color direct ", SCENE_VERTEX_COLOR, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, benchFrames(2));
        computeSceneScore(sfinalSynthetic, 2, "Point cloud pixels  ", SCENE_PIXELS, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(2));
        computeSceneScore(sfinalSynthetic, 2, "Point cloud dots    ", SCENE_DOTS, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(3));
        computeSceneScore(sfinalSynthetic, 2, "Wire cube thick     ", SCENE_WIRE_CUBE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(3));
        computeSceneScore(sfinalSynthetic, 2, "Wire sphere thick   ", SCENE_WIRE_SPHERE, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(4));
        computeSceneScore(sfinalSynthetic, 1, "Wire bunny mesh     ", SCENE_WIRE_BUNNY, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, benchFrames(4));
        Serial.print("total: "); sfinalSynthetic.print();
        Serial.printf("-> SYNTHETIC SCORE : %.2f fps\n\n", sfinalSynthetic.fps());

        Score sfinal;
#if TGX_BENCHMARK_FULL_PROFILE
        sfinal.add(sfinalBuddha, 1);
        sfinal.add(sfinalR2D2, 1);
#endif
        sfinal.add(sfinalBunny, 1);
        sfinal.add(sfinalSynthetic, 1);
        Serial.printf("*** BENCHMARK FINAL SCORE: %0.2f fps ***\n\n", sfinal.fps());
        }

    const float t = (millis() - ems) / 1000.0f;
    Serial.printf("\n\nBenchmark completed in %0.2f seconds\n\n", t);
#ifdef TGX_BENCHMARK_CPU
    std::fflush(stdout);
#else
    while (1) { delay(1000); }
#endif
    }





void loop()
    {
    }

#ifdef TGX_BENCHMARK_CPU
int main()
    {
    setup();
    return 0;
    }
#endif




/** end of file */
