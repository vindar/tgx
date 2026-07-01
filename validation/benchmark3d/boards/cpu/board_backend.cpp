#include "../../common/tgx_bench.h"

#include <chrono>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

namespace tgxbench
{

namespace
{

const auto g_start_time = std::chrono::steady_clock::now();

void* alignedAlloc(size_t bytes, size_t alignment)
{
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
    const size_t extra = alignment - 1 + sizeof(void*);
    void* raw = std::malloc(bytes + extra);
    if (!raw) return nullptr;
    uintptr_t aligned = ((uintptr_t)raw + sizeof(void*) + alignment - 1) & ~(uintptr_t)(alignment - 1);
    ((void**)aligned)[-1] = raw;
    memset((void*)aligned, 0, bytes);
    return (void*)aligned;
}

} // namespace

BenchCaps benchGetCaps()
{
    BenchCaps caps;
    caps.board_name = TGX_BENCH_BOARD_NAME;
    caps.profile_name = TGX_BENCH_PROFILE_NAME;
    caps.power_class = TGX_BENCH_POWER_CLASS;
    caps.default_width = TGX_BENCH_DEFAULT_WIDTH;
    caps.default_height = TGX_BENCH_DEFAULT_HEIGHT;
    caps.min_frames = TGX_BENCH_MIN_FRAMES;
    caps.max_frames = TGX_BENCH_MAX_FRAMES;
    caps.target_us = TGX_BENCH_TARGET_US;
    caps.fast_arena_bytes = TGX_BENCH_FAST_ARENA_BYTES;
    caps.bulk_arena_bytes = TGX_BENCH_BULK_ARENA_BYTES;
    caps.supports_heavy_meshes = TGX_BENCH_SUPPORT_HEAVY_MESHES != 0;
    caps.supports_two_spotlights = TGX_BENCH_SUPPORT_TWO_SPOTLIGHTS != 0;
    return caps;
}

bool benchBackendSetup()
{
    return true;
}

void benchDelayMs(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void benchYield()
{
    std::this_thread::yield();
}

uint32_t benchMicros()
{
    return (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - g_start_time).count();
}

uint32_t benchMillis()
{
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_start_time).count();
}

void benchPrint(const char* text)
{
    fputs(text, stdout);
}

void benchPrintln(const char* text)
{
    fputs(text, stdout);
    fputc('\n', stdout);
}

void benchPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void benchResetAllocators()
{
}

void* benchAlloc(BenchMemoryKind, size_t bytes, size_t alignment)
{
    return alignedAlloc(bytes, alignment);
}

void benchFree(BenchMemoryKind, void* ptr)
{
    if (ptr) std::free(((void**)ptr)[-1]);
}

const char* benchMemoryBackendName()
{
    return "cpu_aligned_malloc";
}

} // namespace tgxbench
