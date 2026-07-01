#include <Arduino.h>

#ifndef DMAMEM
#define DMAMEM
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

#include "../../common/tgx_bench.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace tgxbench
{

namespace
{

struct Arena
    {
    uint8_t* data;
    size_t size;
    size_t used;
    };

#if defined(ARDUINO_ARCH_ESP32)

struct HeapAllocRecord
    {
    void* raw;
    };

static HeapAllocRecord g_heap_allocs[64];
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
    uint32_t caps = heapCapsForKind(kind);
    void* raw = heap_caps_malloc(bytes + extra, caps);
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

#else

static uint8_t g_fast_arena[TGX_BENCH_FAST_ARENA_BYTES];

#if defined(__IMXRT1062__)
DMAMEM static uint8_t g_bulk_arena[TGX_BENCH_BULK_ARENA_BYTES];
#else
static uint8_t g_bulk_arena[TGX_BENCH_BULK_ARENA_BYTES];
#endif

static Arena g_fast = { g_fast_arena, TGX_BENCH_FAST_ARENA_BYTES, 0 };
static Arena g_bulk = { g_bulk_arena, TGX_BENCH_BULK_ARENA_BYTES, 0 };

Arena& arenaForKind(BenchMemoryKind kind)
{
    switch (kind)
        {
        case BenchMemoryKind::ScratchFast: return g_fast;
        case BenchMemoryKind::Framebuffer:
        case BenchMemoryKind::ZBuffer:
        case BenchMemoryKind::AssetCache:
        case BenchMemoryKind::Bulk:
        default: return g_bulk;
        }
}

void* arenaAlloc(Arena& arena, size_t bytes, size_t alignment)
{
    if (alignment == 0) alignment = 1;
    const uintptr_t base = (uintptr_t)arena.data;
    const uintptr_t current = base + arena.used;
    const uintptr_t aligned = (current + alignment - 1) & ~(uintptr_t)(alignment - 1);
    const size_t new_used = (size_t)(aligned - base) + bytes;
    if (new_used > arena.size) return nullptr;
    arena.used = new_used;
    void* ptr = (void*)aligned;
    memset(ptr, 0, bytes);
    return ptr;
}

#endif

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
    Serial.begin(115200);
    const uint32_t start = millis();
    while ((!Serial) && ((millis() - start) < 2500))
        {
        delay(10);
        }
    delay(TGX_BENCH_SERIAL_STARTUP_DELAY_MS);
    return true;
}

void benchDelayMs(uint32_t ms)
{
    delay(ms);
}

void benchYield()
{
    yield();
}

uint32_t benchMicros()
{
    return micros();
}

uint32_t benchMillis()
{
    return millis();
}

void benchPrint(const char* text)
{
    Serial.print(text);
}

void benchPrintln(const char* text)
{
    Serial.println(text);
}

void benchPrintf(const char* format, ...)
{
    char buffer[384];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}

void benchResetAllocators()
{
#if defined(ARDUINO_ARCH_ESP32)
    for (size_t i = 0; i < g_heap_alloc_count; ++i)
        {
        heap_caps_free(g_heap_allocs[i].raw);
        g_heap_allocs[i].raw = nullptr;
        }
    g_heap_alloc_count = 0;
#else
    g_fast.used = 0;
    g_bulk.used = 0;
#endif
}

void* benchAlloc(BenchMemoryKind kind, size_t bytes, size_t alignment)
{
#if defined(ARDUINO_ARCH_ESP32)
    return heapAllocAligned(kind, bytes, alignment);
#else
    return arenaAlloc(arenaForKind(kind), bytes, alignment);
#endif
}

void benchFree(BenchMemoryKind, void* ptr)
{
#if defined(ARDUINO_ARCH_ESP32)
    heapFreeAligned(ptr);
#else
    (void)ptr;
#endif
}

const char* benchMemoryBackendName()
{
#if defined(ARDUINO_ARCH_ESP32)
    return "esp32_heap_caps";
#else
#if defined(__IMXRT1062__)
    return "static_arena_fast_ram_bulk_dmamem";
#else
    return "static_arena";
#endif
#endif
}

} // namespace tgxbench
