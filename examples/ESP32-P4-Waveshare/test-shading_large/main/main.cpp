/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Large tile-rendered version of test-shading. TGX renders a 640x480
* virtual viewport in four 640x120 tiles, then copies the tiles into
* the 720x720 MIPI-DSI framebuffer before presenting the full frame.
*
********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"

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

static const char* TAG = "tgx_p4_test_shading_large";

static constexpr int VIEW_W = 640;
static constexpr int VIEW_H = 480;
static constexpr int TILE_H = 120;
static constexpr size_t MESH_CACHE_SIZE = 480000;

tgx_p4::Display display;

uint16_t* tile_fb = nullptr;
uint16_t* tile_zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> tile_image;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_GOURAUD;
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


void drawOverlay(int tile_y, int fps, int mode, const char* mesh_name, int nb_triangles)
    {
    if (tile_y != 0) return;

    char buf[80];
    tile_image.drawText(mesh_name ? mesh_name : "[unnamed mesh]", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);

    snprintf(buf, sizeof(buf), "%d FPS", fps);
    const iBox2 B = tile_image.measureText(buf, { 0, 0 }, font_tgx_OpenSans_Bold_10);
    tile_image.drawText(buf, { VIEW_W - B.lx() - 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);

    snprintf(buf, sizeof(buf), "%d triangles", nb_triangles);
    tile_image.drawText(buf, { 4, 32 }, font_tgx_OpenSans_Bold_10, RGB565_Red);

    snprintf(buf, sizeof(buf), "%s", (mode == 0) ? "Wireframe AA" : ((mode == 1) ? "Flat shading" : "Gouraud shading"));
    tile_image.drawText(buf, { 4, 50 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawTile(int tile_y, const Mesh3Dv2<RGB565>* mesh, const fMat4& model,
              int fps, int mode, int nb_triangles)
    {
    renderer.setOffset(0, tile_y);
    tile_image.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    renderer.setModelMatrix(model);

    if (mode == 0)
        {
        renderer.drawWireFrameMeshAA(mesh);
        }
    else if (mode == 1)
        {
        renderer.setShaders(SHADER_FLAT);
        renderer.drawMesh(mesh, false);
        }
    else
        {
        renderer.setShaders(SHADER_GOURAUD);
        renderer.drawMesh(mesh, false);
        }

    drawOverlay(tile_y, fps, mode, mesh->name, nb_triangles);
    }


void initRenderer()
    {
    tile_fb = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer"));
    tile_zbuf = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile z-buffer"));
    mesh_cache = static_cast<char*>(heap_caps_malloc(MESH_CACHE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!mesh_cache) mesh_cache = static_cast<char*>(allocPreferInternal(MESH_CACHE_SIZE, "mesh cache"));

    tile_image.set(tile_fb, VIEW_W, TILE_H);

    renderer.setViewportSize(VIEW_W, VIEW_H);
    renderer.setImage(&tile_image);
    renderer.setZbuffer(tile_zbuf);
    renderer.setPerspective(45.0f, ((float)VIEW_W) / VIEW_H, 1.0f, 100.0f);
    renderer.setCulling(1);
    }


void drawModel(const Mesh3Dv2<RGB565>* mesh, int nb_triangles, float scale,
               const char* scene_wireframe, const char* scene_flat, const char* scene_gouraud,
               float tilt = 0.0f)
    {
    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* cached_mesh = tgx::cacheMesh(mesh, mesh_cache, MESH_CACHE_SIZE, "LMP", &cache_used);
    if (!cached_mesh) cached_mesh = mesh;
    ESP_LOGI(TAG, "scene %s/%s/%s mesh=%s cache_used=%u", scene_wireframe, scene_flat, scene_gouraud,
             cached_mesh->name ? cached_mesh->name : "[unnamed]", (unsigned)cache_used);

    const uint64_t max_ms = 12000;
    const uint64_t start = tgx_p4::millis64();
    int prev_mode = -1;

    while (tgx_p4::millis64() - start < max_ms)
        {
        const uint64_t now = tgx_p4::millis64();
        const float em = (float)(now - start);

        fMat4 M;
        M.setScale(scale, scale, scale);
        M.multRotate(tilt, { 0, 0, 1 });
        M.multRotate((1440.0f * em) / (float)max_ms, { 0, 1, 0 });
        M.multTranslate({ 0, 0, -40 });

        const int mode = (int)(((now - start) * 3) / max_ms) % 3;
        if (mode != prev_mode)
            {
            prev_mode = mode;
            printf("[TGX scene] %s\n", (mode == 0) ? scene_wireframe : ((mode == 1) ? scene_flat : scene_gouraud));
            }

        const int fps = currentFPS();
        display.beginFrame(0x0014);
        const int ox = (display.width() - VIEW_W) / 2;
        const int oy = (display.height() - VIEW_H) / 2;
        for (int tile_y = 0; tile_y < VIEW_H; tile_y += TILE_H)
            {
            drawTile(tile_y, cached_mesh, M, fps, mode, nb_triangles);
            display.copyTileRGB565(tile_fb, VIEW_W, TILE_H, ox, oy + tile_y);
            }
        display.presentFrame();
        }
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    while (true)
        {
        renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f });
        renderer.setLightDiffuse({ 1.0f, 1.0f, 1.0f });
        renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f });
        renderer.setLightDirection({ 0.4f, -0.6f, -1.0f });

        renderer.setMaterial(RGBf(0.15f, 0.7f, 0.39f), 0.2f, 0.8f, 0.5f, 8);
        drawModel(&teapot, 2256, 16, "large_teapot_wireframe_aa", "large_teapot_flat", "large_teapot_gouraud", 30);

        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.15f, 0.7f, 0.8f, 48);
        drawModel(&bunny, 4968, 13, "large_bunny_wireframe_aa", "large_bunny_flat", "large_bunny_gouraud");

        renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
        drawModel(&dragon, 23000, 16, "large_dragon_wireframe_aa", "large_dragon_flat", "large_dragon_gouraud");

        const float angle = (float)M_PI * (float)(esp_random() % 360) / 180.0f;
        renderer.setLightDirection({ cosf(angle), sinf(angle), -0.3f });
        }
    }
