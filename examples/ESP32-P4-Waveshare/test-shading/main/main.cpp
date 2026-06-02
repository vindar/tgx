/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* This ESP-IDF example renders the same dynamic shading test as the
* Teensy 4 version, but draws into a 320x240 TGX framebuffer centered
* on the 720x720 MIPI-DSI LCD.
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"

#include "teapot.h"
#include "skull.h"
#include "suzanne.h"
#include "bunny.h"
#include "dragon.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_test_shading";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr size_t MESH_CACHE_SIZE = 360000;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> im;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD;
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
    const iBox2 B = im.measureText(buf, { 0, 0 }, font_tgx_OpenSans_Bold_10);
    im.drawText(buf, { SLX - B.lx() - 4, 13 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawInfo(int mode, const char* mesh_name, int nb_triangles)
    {
    char buf[80];
    im.drawText(mesh_name ? mesh_name : "[unnamed mesh]", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%d triangles", nb_triangles);
    im.drawText(buf, { 4, SLY - 22 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%s", (mode == 0) ? "Wireframe AA" : ((mode == 1) ? "Flat shading" : "Gouraud shading"));
    im.drawText(buf, { 4, SLY - 6 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
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
    renderer.setCulling(1);
    }


void drawModel(const Mesh3Dv2<RGB565>* mesh, int nb_triangles, float scale,
               const char* scene_wireframe, const char* scene_flat, const char* scene_gouraud,
               float tilt = 0.0f)
    {
    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* cached_mesh = tgx::cacheMesh(mesh, mesh_cache, MESH_CACHE_SIZE, "LMP", &cache_used);
    ESP_LOGI(TAG, "scene %s/%s/%s mesh=%s cache_used=%u", scene_wireframe, scene_flat, scene_gouraud,
             cached_mesh->name ? cached_mesh->name : "[unnamed]", (unsigned)cache_used);

    const uint64_t max_ms = 12000;
    const uint64_t start = tgx_p4::millis64();
    int prev_mode = -1;

    while (tgx_p4::millis64() - start < max_ms)
        {
        const uint64_t now = tgx_p4::millis64();
        const float em = (float)(now - start);

        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();

        fMat4 M;
        M.setScale(scale, scale, scale);
        M.multRotate(tilt, { 0, 0, 1 });
        M.multRotate((1440.0f * em) / (float)max_ms, { 0, 1, 0 });
        M.multTranslate({ 0, 0, -40 });
        renderer.setModelMatrix(M);

        const int mode = (int)(((now - start) * 3) / max_ms) % 3;
        if (mode != prev_mode)
            {
            prev_mode = mode;
            printf("[TGX scene] %s\n", (mode == 0) ? scene_wireframe : ((mode == 1) ? scene_flat : scene_gouraud));
            }

        if (mode == 0)
            {
            renderer.drawWireFrameMeshAA(cached_mesh);
            }
        else if (mode == 1)
            {
            renderer.setShaders(SHADER_FLAT);
            renderer.drawMesh(cached_mesh, false);
            }
        else
            {
            renderer.setShaders(SHADER_GOURAUD);
            renderer.drawMesh(cached_mesh, false);
            }

        drawInfo(mode, cached_mesh->name, nb_triangles);
        drawFPS();
        display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);
        }
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    while (true)
        {
        renderer.setMaterial(RGBf(0.15f, 0.7f, 0.39f), 0.2f, 0.8f, 0.5f, 8);
        drawModel(&teapot, 2256, 15, "teapot_wireframe_aa", "teapot_flat", "teapot_gouraud", 30);

        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.15f, 0.7f, 0.8f, 48);
        drawModel(&bunny, 4968, 12, "bunny_wireframe_aa", "bunny_flat", "bunny_gouraud");

        renderer.setMaterial(RGBf(166 / 256.0f, 130 / 256.0f, 110.0f / 256.0f), 0.15f, 0.7f, 0.4f, 16);
        drawModel(&skull_1, 9535, 12, "skull_wireframe_aa", "skull_flat", "skull_gouraud");

        renderer.setLightAmbiant({ 0, 0, 1.0f });
        renderer.setLightDiffuse({ 1.0f, 0, 0 });
        renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f });

        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.8f, 0.8f, 32);
        drawModel(&suzanne, 15744, 13, "suzanne_wireframe_aa", "suzanne_flat", "suzanne_gouraud");

        renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f });
        renderer.setLightDiffuse({ 1.0f, 1.0f, 1.0f });
        renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f });

        renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
        drawModel(&dragon, 23000, 15, "dragon_wireframe_aa", "dragon_flat", "dragon_gouraud");

        const float angle = (float)M_PI * (float)(esp_random() % 360) / 180.0f;
        renderer.setLightDirection({ cosf(angle), sinf(angle), -0.3f });
        }
    }

