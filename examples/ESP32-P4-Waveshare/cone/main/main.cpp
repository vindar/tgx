/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Large tile-rendered cone example. TGX renders a 640x480
* virtual viewport in five 640x96 internal-RAM tiles. While TGX draws
* one tile, the previous tile is copied asynchronously to the 720x720
* MIPI-DSI backbuffer.
*
********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_cone";

static constexpr int VIEW_W = 640;
static constexpr int VIEW_H = 480;
static constexpr int TILE_H = 96;
static constexpr float RATIO = ((float)VIEW_W) / VIEW_H;

tgx_p4::Display display;

uint16_t* tile_zbuf = nullptr;
uint16_t* tile_fb[2] = { nullptr, nullptr };
Image<RGB565> tile_image;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_NOTEXTURE;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 2> renderer;


struct TimingStats
    {
    uint32_t frames = 0;
    uint64_t frame_us = 0;
    uint64_t update_us = 0;
    uint64_t begin_us = 0;
    uint64_t draw_us = 0;
    uint64_t copy_us = 0;
    uint64_t present_us = 0;
    };

TimingStats timing_stats;


uint64_t nowUs()
    {
    return (uint64_t)esp_timer_get_time();
    }


void addTiming(uint64_t frame_us, uint64_t update_us, uint64_t begin_us,
               uint64_t draw_us, uint64_t copy_us, uint64_t present_us)
    {
    timing_stats.frames++;
    timing_stats.frame_us += frame_us;
    timing_stats.update_us += update_us;
    timing_stats.begin_us += begin_us;
    timing_stats.draw_us += draw_us;
    timing_stats.copy_us += copy_us;
    timing_stats.present_us += present_us;

    if (timing_stats.frames < 30) return;

    const uint32_t n = timing_stats.frames;
    printf("[TGX timing] frames=%u frame=%.2fms update=%.2fms begin_wait=%.2fms draw_tiles=%.2fms copy_tiles=%.2fms present=%.2fms\n",
           (unsigned)n,
           (double)timing_stats.frame_us / (double)n / 1000.0,
           (double)timing_stats.update_us / (double)n / 1000.0,
           (double)timing_stats.begin_us / (double)n / 1000.0,
           (double)timing_stats.draw_us / (double)n / 1000.0,
           (double)timing_stats.copy_us / (double)n / 1000.0,
           (double)timing_stats.present_us / (double)n / 1000.0);

    timing_stats = TimingStats{};
    }


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


void* allocPreferInternal(size_t bytes, const char* name)
    {
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (p)
        {
        ESP_LOGI(TAG, "%s allocated in internal RAM (%u bytes)", name, (unsigned)bytes);
        return p;
        }

    p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (p)
        {
        ESP_LOGW(TAG, "%s allocated in PSRAM (%u bytes)", name, (unsigned)bytes);
        return p;
        }

    ESP_LOGE(TAG, "%s allocation failed (%u bytes)", name, (unsigned)bytes);
    abort();
    }


int currentFPS()
    {
    static uint64_t last_ms = 0;
    static int frame_count = 0;
    static int fps = 0;

    frame_count++;
    const uint64_t now = tgx_p4::millis64();
    if (now - last_ms >= 1000)
        {
        fps = frame_count;
        frame_count = 0;
        last_ms = now;
        printf("[TGX telemetry] fps=%d\n", fps);
        }
    return fps;
    }


fVec3 spotDirectionToObject(const fVec3& position)
    {
    return fVec3(0.0f, 0.0f, -5.0f) - position;
    }


void drawTile(int tile_buffer_index, int tile_y, const fMat4& model, int fps)
    {
    tile_image.set(tile_fb[tile_buffer_index], VIEW_W, TILE_H);
    renderer.setOffset(0, tile_y);
    tile_image.fillScreen(rgb888(2, 5, 10));
    renderer.clearZbuffer();
    renderer.setModelMatrix(model);
    renderer.drawCone(36, true);

    if (tile_y == 0)
        {
        char buf[48];
        snprintf(buf, sizeof(buf), "%d FPS", fps);
        tile_image.drawText("TGX cone / two spotlights", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
        tile_image.drawText(buf, { VIEW_W - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
        }
    }


void initRenderer()
    {
    tile_fb[0] = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer A"));
    tile_fb[1] = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer B"));
    tile_zbuf = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile z-buffer"));

    tile_image.set(tile_fb[0], VIEW_W, TILE_H);
    renderer.setViewportSize(VIEW_W, VIEW_H);
    renderer.setImage(&tile_image);
    renderer.setZbuffer(tile_zbuf);
    renderer.setCulling(1);
    renderer.setShaders(SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);
    renderer.setMaterial(RGBf(0.72f, 0.73f, 0.76f), 0.030f, 0.98f, 0.45f, 24);

    // Keep the directional light weak so the two local spotlights shape the cone.
    renderer.setLight({ -0.25f, -0.80f, -0.35f },
                      RGBf(0.010f, 0.011f, 0.014f),
                      RGBf(0.028f, 0.030f, 0.034f),
                      RGBf(0.0f, 0.0f, 0.0f));

    const fVec3 warm_pos = { -2.95f, 1.85f, -3.15f };
    const fVec3 cool_pos = { 2.80f, -1.65f, -3.85f };

    renderer.setSpotLightCount(2);
    renderer.setSpotLight(0, warm_pos, spotDirectionToObject(warm_pos),
                          6.6f, 62.0f, 34.0f,
                          RGBf(1.45f, 0.70f, 0.18f),
                          RGBf(0.55f, 0.25f, 0.08f));
    renderer.setSpotLight(1, cool_pos, spotDirectionToObject(cool_pos),
                          6.8f, 62.0f, 34.0f,
                          RGBf(0.12f, 0.56f, 1.55f),
                          RGBf(0.06f, 0.24f, 0.58f));

    printf("[TGX scene] cone_large_spotlights\n");
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();
    display.clear(0x0014);

    while (true)
        {
        const uint64_t frame_start_us = nowUs();
        const uint64_t now = tgx_p4::millis64();

        const uint64_t update_start_us = nowUs();
        const uint64_t update_us = nowUs() - update_start_us;

        fMat4 M;
        M.setRotate((float)now / 16.7f, { 0, 1, 0 });
        M.multRotate((float)now / 41.7f, { 1, 0, 0 });
        M.multRotate((float)now / 61.0f, { 0, 0, 1 });
        M.multTranslate({ 0, 0, -5 });

        const int fps = currentFPS();
        const int ox = (display.width() - VIEW_W) / 2;
        const int oy = (display.height() - VIEW_H) / 2;
        uint64_t draw_us = 0;
        uint64_t copy_us = 0;

        const uint64_t begin_start_us = nowUs();
        display.beginFrame();
        const uint64_t begin_us = nowUs() - begin_start_us;

        int tile_buffer = 0;
        for (int tile_y = 0; tile_y < VIEW_H; tile_y += TILE_H)
            {
            const uint64_t draw_start_us = nowUs();
            drawTile(tile_buffer, tile_y, M, fps);
            draw_us += nowUs() - draw_start_us;

            const uint64_t copy_start_us = nowUs();
            display.startCopyTileRGB565Async(tile_fb[tile_buffer], VIEW_W, TILE_H, ox, oy + tile_y);
            copy_us += nowUs() - copy_start_us;
            tile_buffer = 1 - tile_buffer;
            }

        const uint64_t final_copy_start_us = nowUs();
        display.waitCopyDone();
        copy_us += nowUs() - final_copy_start_us;

        const uint64_t present_start_us = nowUs();
        display.presentFrame();
        const uint64_t present_us = nowUs() - present_start_us;
        const uint64_t frame_us = nowUs() - frame_start_us;
        addTiming(frame_us, update_us, begin_us, draw_us, copy_us, present_us);
        }
    }
