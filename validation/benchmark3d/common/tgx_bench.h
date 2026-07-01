#ifndef TGX_BENCHMARK3D_TGX_BENCH_H
#define TGX_BENCHMARK3D_TGX_BENCH_H

#include <stddef.h>
#include <stdint.h>

#include <tgx.h>

#ifndef TGX_BENCH_BOARD_NAME
#define TGX_BENCH_BOARD_NAME "unknown"
#endif

#ifndef TGX_BENCH_PROFILE_NAME
#define TGX_BENCH_PROFILE_NAME "default"
#endif

#ifndef TGX_BENCH_DEFAULT_WIDTH
#define TGX_BENCH_DEFAULT_WIDTH 160
#endif

#ifndef TGX_BENCH_DEFAULT_HEIGHT
#define TGX_BENCH_DEFAULT_HEIGHT 120
#endif

#ifndef TGX_BENCH_MIN_FRAMES
#define TGX_BENCH_MIN_FRAMES 3
#endif

#ifndef TGX_BENCH_MAX_FRAMES
#define TGX_BENCH_MAX_FRAMES 20
#endif

#ifndef TGX_BENCH_TARGET_US
#define TGX_BENCH_TARGET_US 200000UL
#endif

#ifndef TGX_BENCH_VISUAL_FRAME_COUNT
#define TGX_BENCH_VISUAL_FRAME_COUNT 96
#endif

#if (!defined(TGX_BENCH_MODULE_CORE_RASTER)) && \
    (!defined(TGX_BENCH_MODULE_TEXTURE_RASTER)) && \
    (!defined(TGX_BENCH_MODULE_LIGHTING_SHADING)) && \
    (!defined(TGX_BENCH_MODULE_PRIMITIVES)) && \
    (!defined(TGX_BENCH_MODULE_MESH)) && \
    (!defined(TGX_BENCH_MODULE_WIRE_POINTS)) && \
    (!defined(TGX_BENCH_MODULE_SKYBOX_NOZ)) && \
    (!defined(TGX_BENCH_MODULE_INTEGRATED)) && \
    (!defined(TGX_BENCH_MODULE_MIXED_STRESS))
#define TGX_BENCH_MODULE_CORE_RASTER 1
#endif

#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP)
#endif
#ifndef TGX_BENCH_MAX_DIRECTIONAL_LIGHTS
#define TGX_BENCH_MAX_DIRECTIONAL_LIGHTS 2
#endif
#ifndef TGX_BENCH_MAX_SPOT_LIGHTS
#define TGX_BENCH_MAX_SPOT_LIGHTS 2
#endif
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2)
#endif
#ifndef TGX_BENCH_MAX_DIRECTIONAL_LIGHTS
#define TGX_BENCH_MAX_DIRECTIONAL_LIGHTS 2
#endif
#ifndef TGX_BENCH_MAX_SPOT_LIGHTS
#define TGX_BENCH_MAX_SPOT_LIGHTS 2
#endif
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP)
#endif
#elif defined(TGX_BENCH_MODULE_MESH)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP)
#endif
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE)
#endif
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2)
#endif
#ifndef TGX_BENCH_MAX_DIRECTIONAL_LIGHTS
#define TGX_BENCH_MAX_DIRECTIONAL_LIGHTS 2
#endif
#ifndef TGX_BENCH_MAX_SPOT_LIGHTS
#define TGX_BENCH_MAX_SPOT_LIGHTS 2
#endif
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP)
#endif
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP)
#endif
#else
#ifndef TGX_BENCH_LOADED_SHADERS
#define TGX_BENCH_LOADED_SHADERS (tgx::SHADER_PERSPECTIVE | tgx::SHADER_ORTHO | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)
#endif
#endif

#ifndef TGX_BENCH_MAX_DIRECTIONAL_LIGHTS
#define TGX_BENCH_MAX_DIRECTIONAL_LIGHTS 1
#endif

#ifndef TGX_BENCH_MAX_SPOT_LIGHTS
#define TGX_BENCH_MAX_SPOT_LIGHTS 0
#endif

namespace tgxbench
{

enum class BenchMemoryKind
    {
    Framebuffer,
    ZBuffer,
    ScratchFast,
    AssetCache,
    Bulk
    };

struct BenchCaps
    {
    const char* board_name;
    const char* profile_name;
    int power_class;
    int default_width;
    int default_height;
    uint32_t min_frames;
    uint32_t max_frames;
    uint32_t target_us;
    size_t fast_arena_bytes;
    size_t bulk_arena_bytes;
    bool supports_heavy_meshes;
    bool supports_two_spotlights;
    };

using BenchRenderer = tgx::Renderer3D<tgx::RGB565, TGX_BENCH_LOADED_SHADERS, uint16_t, TGX_BENCH_MAX_DIRECTIONAL_LIGHTS, TGX_BENCH_MAX_SPOT_LIGHTS>;

struct BenchContext
    {
    BenchCaps caps;
    int width;
    int height;
    tgx::RGB565* framebuffer;
    uint16_t* zbuffer;
    tgx::Image<tgx::RGB565> image;
    BenchRenderer renderer;
    };

typedef bool (*BenchSetupFn)(BenchContext& ctx);
typedef void (*BenchRenderFn)(BenchContext& ctx, uint32_t frame_index);
typedef void (*BenchTeardownFn)(BenchContext& ctx);

struct BenchTest
    {
    const char* id;
    const char* name;
    tgx::Shader shaders;
    uint32_t min_frames;
    uint32_t max_frames;
    uint32_t target_us;
    BenchSetupFn setup;
    BenchRenderFn render;
    BenchTeardownFn teardown;
    };

struct BenchModule
    {
    const char* id;
    const char* name;
    const BenchTest* tests;
    uint32_t test_count;
    };

BenchCaps benchGetCaps();
bool benchBackendSetup();
void benchDelayMs(uint32_t ms);
void benchYield();
uint32_t benchMicros();
uint32_t benchMillis();
void benchPrint(const char* text);
void benchPrintln(const char* text);
void benchPrintf(const char* format, ...);

void benchResetAllocators();
void* benchAlloc(BenchMemoryKind kind, size_t bytes, size_t alignment);
void benchFree(BenchMemoryKind kind, void* ptr);
const char* benchMemoryKindName(BenchMemoryKind kind);
const char* benchMemoryBackendName();

bool benchInitContext(BenchContext& ctx);
void benchClearTarget(BenchContext& ctx);
uint64_t benchFramebufferChecksum(const BenchContext& ctx);
int benchRunModule(const BenchModule& module);

} // namespace tgxbench

#endif
