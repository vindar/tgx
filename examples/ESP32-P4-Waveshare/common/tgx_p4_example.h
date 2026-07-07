#ifndef TGX_P4_EXAMPLE_H
#define TGX_P4_EXAMPLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <tgx.h>

namespace tgx_p4_example
{

inline tgx::RGB565 rgb(int r, int g, int b)
    {
    return tgx::RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }

inline uint32_t millis()
    {
    return (uint32_t)(esp_timer_get_time() / 1000);
    }

inline uint32_t micros()
    {
    return (uint32_t)esp_timer_get_time();
    }

inline void delayMs(uint32_t ms)
    {
    TickType_t ticks = pdMS_TO_TICKS(ms);
    if ((ms > 0) && (ticks == 0)) ticks = 1;
    vTaskDelay(ticks);
    }

inline void* allocBufferBytes(size_t bytes)
    {
    void* ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr)
        {
        ptr = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        }
    if (!ptr)
        {
        ptr = malloc(bytes);
        }
    if (ptr)
        {
        memset(ptr, 0, bytes);
        }
    return ptr;
    }

template<typename T>
inline T* allocBuffer(size_t count)
    {
    return static_cast<T*>(allocBufferBytes(count * sizeof(T)));
    }

inline uint32_t framebufferChecksum(const tgx::RGB565* pixels, size_t count)
    {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < count; ++i)
        {
        h ^= pixels[i].val;
        h *= 16777619u;
        }
    return h;
    }

struct Telemetry
    {
    const char* example;
    uint32_t last_ms;
    uint32_t frames;
    uint64_t render_sum_us;
    uint32_t render_min_us;
    uint32_t render_max_us;

    void begin(const char* name)
        {
        example = name;
        last_ms = millis();
        frames = 0;
        render_sum_us = 0;
        render_min_us = 0xffffffffu;
        render_max_us = 0;
        printf("TGX_EXAMPLE_BEGIN board=esp32p4 example=%s\n", example);
        fflush(stdout);
        }

    void frame(const char* scene, uint32_t render_us, uint32_t checksum)
        {
        ++frames;
        render_sum_us += render_us;
        if (render_us < render_min_us) render_min_us = render_us;
        if (render_us > render_max_us) render_max_us = render_us;

        const uint32_t now = millis();
        if (now - last_ms >= 1000)
            {
            const float fps = (render_sum_us > 0) ? (1000000.0f * (float)frames / (float)render_sum_us) : 0.0f;
            const uint32_t avg_us = frames ? (uint32_t)(render_sum_us / frames) : 0;
            printf("TGX_EXAMPLE_FRAME board=esp32p4 example=%s scene=%s frames=%lu fps=%.2f render_avg_us=%lu render_min_us=%lu render_max_us=%lu checksum=%08lx\n",
                   example,
                   scene,
                   (unsigned long)frames,
                   fps,
                   (unsigned long)avg_us,
                   (unsigned long)render_min_us,
                   (unsigned long)render_max_us,
                   (unsigned long)checksum);
            fflush(stdout);

            last_ms = now;
            frames = 0;
            render_sum_us = 0;
            render_min_us = 0xffffffffu;
            render_max_us = 0;
            }
        }
    };

} // namespace tgx_p4_example

#endif
