/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Mesh3Dv2 character viewer adapted from the Teensy 4.1 example.
* The ESP32-P4 has enough flash/PSRAM to keep the full Teensy 4.1
* model list while rendering into a centered 320x240 TGX framebuffer.
*
********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"
#include "nanosuit_v2.h"
#include "R2D2_v2.h"
#include "elementalist_v2.h"
#include "sinbad_v2.h"
#include "cyborg_v2.h"
#include "dennis_v2.h"
#include "manga3_v2.h"
#include "naruto_v2.h"
#include "stormtrooper_v2.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_characters";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr size_t MESH_CACHE_SIZE = 2 * 1024 * 1024;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> im;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

const Shader shader = SHADER_GOURAUD | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

const Mesh3Dv2<RGB565>* meshes[] = {
    &nanosuit_v2,
    &elementalist_v2,
    &sinbad_v2,
    &cyborg_v2,
    &naruto_v2,
    &manga3_v2,
    &dennis_v2,
    &R2D2_v2,
    &stormtrooper_v2
    };

const int mesh_triangles[] = { 8807, 5013, 7610, 5549, 2472, 13427, 7657, 8035, 6518 };

int mesh_index = 0;
const Mesh3Dv2<RGB565>* cached_mesh = nullptr;
uint64_t mesh_start_ms = 0;


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
    im.drawText(buf, { SLX - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawInfo(const Mesh3Dv2<RGB565>* mesh, int nb_triangles)
    {
    char buf[96];
    im.drawText(mesh->name ? mesh->name : "[unnamed mesh]", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%d triangles", nb_triangles);
    im.drawText(buf, { 4, SLY - 22 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    im.drawText("Gouraud shading / texturing", { 4, SLY - 6 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


bool moveModel()
    {
    const float end1 = 6000.0f;
    const float end2 = 2000.0f;
    const float end3 = 6000.0f;
    const float end4 = 2000.0f;
    const float total = end1 + end2 + end3 + end4;

    const uint64_t now = tgx_p4::millis64();
    float t = (float)(now - mesh_start_ms);
    bool change = false;
    if (t > total)
        {
        mesh_start_ms = now;
        t = 0.0f;
        change = true;
        }

    const float dilat = 9.0f;
    const float roty = 360.0f * (t / 4000.0f);
    float tz = -25.0f;
    float ty = 0.0f;

    if (t >= end1)
        {
        t -= end1;
        if (t < end2)
            {
            t /= end2;
            tz = -25.0f + 15.0f * t;
            ty = -6.0f * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                {
                tz = -10.0f;
                ty = -6.0f;
                }
            else
                {
                t -= end3;
                t /= end4;
                tz = -10.0f - 15.0f * t;
                ty = -6.0f + 6.0f * t;
                }
            }
        }

    fMat4 M;
    M.setScale({ dilat, dilat, dilat });
    M.multRotate(-roty, { 0, 1, 0 });
    M.multTranslate({ 0, ty, tz });
    renderer.setModelMatrix(M);
    return change;
    }


void cacheCurrentMesh()
    {
    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* source = meshes[mesh_index];
    const Mesh3Dv2<RGB565>* cached = tgx::cacheMesh(source, mesh_cache, MESH_CACHE_SIZE, "MIPL", &cache_used);
    cached_mesh = cached ? cached : source;
    mesh_start_ms = tgx_p4::millis64();
    printf("[TGX scene] %s\n", cached_mesh->name ? cached_mesh->name : "[unnamed mesh]");
    ESP_LOGI(TAG, "mesh=%s cache_used=%u", cached_mesh->name ? cached_mesh->name : "[unnamed]", (unsigned)cache_used);
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
    renderer.setShaders(shader);

    cacheCurrentMesh();
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    while (true)
        {
        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();

        if (moveModel())
            {
            mesh_index = (mesh_index + 1) % (int)(sizeof(meshes) / sizeof(meshes[0]));
            cacheCurrentMesh();
            }

        renderer.drawMesh(cached_mesh);
        drawInfo(cached_mesh, mesh_triangles[mesh_index]);
        drawFPS();
        display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);
        }
    }
