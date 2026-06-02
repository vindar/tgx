/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Minimal Mesh3Dv2 example. TGX renders a 320x240 RGB565 image and
* the MIPI-DSI helper centers it on the 720x720 LCD.
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"
#include "buddha.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_buddha";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr size_t MESH_CACHE_SIZE = 340000;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> im;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


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


void initRenderer()
    {
    fb = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX framebuffer"));
    zbuf = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX z-buffer"));
    mesh_cache = static_cast<char*>(heap_caps_malloc(MESH_CACHE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!mesh_cache) mesh_cache = static_cast<char*>(allocPreferInternal(MESH_CACHE_SIZE, "mesh cache"));

    im.set(fb, SLX, SLY);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45.0f, ((float)SLX) / SLY, 1.0f, 100.0f);
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    renderer.setShaders(SHADER_GOURAUD);
    renderer.setCulling(1);
    }


void drawFPS()
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

    char buf[24];
    snprintf(buf, sizeof(buf), "%d FPS", fps);
    im.drawText(buf, { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* buddha_cached = tgx::cacheMesh(&buddha, mesh_cache, MESH_CACHE_SIZE, "LMP", &cache_used);
    ESP_LOGI(TAG, "buddha cached, used=%u bytes", (unsigned)cache_used);
    printf("[TGX scene] buddha_rotation\n");

    float a = 0.0f;
    while (true)
        {
        im.fillScreen(RGB565_Blue);
        renderer.clearZbuffer();

        renderer.setModelPosScaleRot({ 0, 0.5f, -35 }, { 13, 13, 13 }, a);
        renderer.drawMesh(buddha_cached, false);

        drawFPS();
        display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);

        a += 3.0f;
        if (a >= 360.0f)
            {
            a -= 360.0f;
            printf("[TGX scene] buddha_rotation\n");
            }
        }
    }
