/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Texture-mapping test using the same Mesh3Dv2 assets as the Teensy 4
* example. TGX renders 320x240 and the MIPI-DSI helper centers it on
* the 720x720 LCD.
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
#include "spot.h"
#include "bob.h"
#include "blub.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_test_texture";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr size_t MESH_CACHE_SIZE = 355000;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> im;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;
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
    renderer.setMaterial(RGBf(0.75f, 0.75f, 0.75f), 0.15f, 0.7f, 0.7f, 32);
    renderer.setCulling(1);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
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
    im.drawText(buf, { SLX - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawInfo(Shader shader, const Mesh3Dv2<RGB565>& mesh, int nb_triangles)
    {
    char buf[96];
    im.drawText(mesh.name ? mesh.name : "[unnamed mesh]", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%d triangles", nb_triangles);
    im.drawText(buf, { 4, SLY - 22 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%s%s",
             (shader & SHADER_GOURAUD) ? "Gouraud shading" : "Flat shading",
             (shader & SHADER_TEXTURE_BILINEAR) ? " / texture bilinear" :
             ((shader & SHADER_TEXTURE_NEAREST) ? " / texture nearest" : ""));
    im.drawText(buf, { 4, SLY - 6 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawModel(const Mesh3Dv2<RGB565>* mesh, int nb_triangles, float scale,
               const char* scene_gouraud, const char* scene_nearest, const char* scene_bilinear)
    {
    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* cached_mesh = tgx::cacheMesh(mesh, mesh_cache, MESH_CACHE_SIZE, "MIPL", &cache_used);
    ESP_LOGI(TAG, "mesh=%s cache_used=%u", cached_mesh->name ? cached_mesh->name : "[unnamed]", (unsigned)cache_used);

    const uint64_t max_ms = 18000;
    const uint64_t start = tgx_p4::millis64();
    int prev_part = -1;

    while (tgx_p4::millis64() - start < max_ms)
        {
        const uint64_t now = tgx_p4::millis64();
        const float em = (float)(now - start);

        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();

        fMat4 M;
        M.setScale(scale, scale, scale);
        M.multRotate((1440.0f * em) / (float)max_ms, { 0, 1, 0 });
        M.multRotate((800.0f * em) / (float)max_ms, { 1, 0, 0 });
        M.multTranslate({ 0, 0, -40 });
        renderer.setModelMatrix(M);

        const int part = (int)(((now - start) * 3) / max_ms);
        const Shader shader = (part == 0) ? SHADER_GOURAUD :
                              ((part == 1) ? (SHADER_GOURAUD | SHADER_TEXTURE_NEAREST) :
                                             (SHADER_GOURAUD | SHADER_TEXTURE_BILINEAR));

        if (part != prev_part)
            {
            prev_part = part;
            printf("[TGX scene] %s\n", (part == 0) ? scene_gouraud : ((part == 1) ? scene_nearest : scene_bilinear));
            }

        renderer.setShaders(shader);
        renderer.drawMesh(cached_mesh, false);
        drawInfo(shader, *cached_mesh, nb_triangles);
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
        drawModel(&spot, 5856, 13, "spot_gouraud", "spot_tex_nearest", "spot_tex_bilinear");
        drawModel(&bob, 10688, 15, "bob_gouraud", "bob_tex_nearest", "bob_tex_bilinear");
        drawModel(&blub, 14208, 15, "blub_gouraud", "blub_tex_nearest", "blub_tex_bilinear");

        const float angle = (float)M_PI * (float)(esp_random() % 360) / 180.0f;
        renderer.setLightDirection({ cosf(angle), sinf(angle), -0.3f });
        }
    }
