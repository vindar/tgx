#include "board_config.h"

#include "../../common/tgx_bench.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace tgxbench
{

namespace
{

struct HeapAllocRecord
    {
    void* raw;
    };

static HeapAllocRecord g_heap_allocs[96];
static size_t g_heap_alloc_count = 0;

uint32_t heapCapsForKind(BenchMemoryKind kind)
{
    if (kind == BenchMemoryKind::ScratchFast)
        {
        return MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
        }
    return MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
}

void* heapAllocAligned(BenchMemoryKind kind, size_t bytes, size_t alignment)
{
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
    const size_t extra = alignment - 1 + sizeof(void*);

    void* raw = heap_caps_malloc(bytes + extra, heapCapsForKind(kind));
    if (!raw)
        {
        raw = heap_caps_malloc(bytes + extra, MALLOC_CAP_8BIT);
        }
    if ((!raw) || (g_heap_alloc_count >= (sizeof(g_heap_allocs) / sizeof(g_heap_allocs[0]))))
        {
        if (raw) heap_caps_free(raw);
        return nullptr;
        }

    const uintptr_t aligned = ((uintptr_t)raw + sizeof(void*) + alignment - 1) & ~(uintptr_t)(alignment - 1);
    ((void**)aligned)[-1] = raw;
    memset((void*)aligned, 0, bytes);
    g_heap_allocs[g_heap_alloc_count++].raw = raw;
    return (void*)aligned;
}

void heapFreeAligned(void* ptr)
{
    if (!ptr) return;
    void* raw = ((void**)ptr)[-1];
    for (size_t i = 0; i < g_heap_alloc_count; ++i)
        {
        if (g_heap_allocs[i].raw == raw)
            {
            g_heap_allocs[i] = g_heap_allocs[g_heap_alloc_count - 1];
            --g_heap_alloc_count;
            heap_caps_free(raw);
            return;
            }
        }
    heap_caps_free(raw);
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
    benchDelayMs(TGX_BENCH_SERIAL_STARTUP_DELAY_MS);
    return true;
}

void benchDelayMs(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void benchYield()
{
    vTaskDelay(1);
}

uint32_t benchMicros()
{
    return (uint32_t)esp_timer_get_time();
}

uint32_t benchMillis()
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void benchPrint(const char* text)
{
    fputs(text, stdout);
    fflush(stdout);
}

void benchPrintln(const char* text)
{
    fputs(text, stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

void benchPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

void benchResetAllocators()
{
    for (size_t i = 0; i < g_heap_alloc_count; ++i)
        {
        heap_caps_free(g_heap_allocs[i].raw);
        g_heap_allocs[i].raw = nullptr;
        }
    g_heap_alloc_count = 0;
}

void* benchAlloc(BenchMemoryKind kind, size_t bytes, size_t alignment)
{
    return heapAllocAligned(kind, bytes, alignment);
}

void benchFree(BenchMemoryKind, void* ptr)
{
    heapFreeAligned(ptr);
}

const char* benchMemoryBackendName()
{
    return "espidf_heap_caps";
}

} // namespace tgxbench
