/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Tile-rendered texture test. The ESP32-P4 display is 720x720, but TGX
* does not need a full-size framebuffer. Instead, TGX renders a 640x480
* virtual viewport in five 640x96 tiles. While TGX draws one tile, the
* previous tile is copied asynchronously to the MIPI-DSI backbuffer.
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
#include "donkeykong_small_v2.h"
#include "naruto_v2.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_test_texture";

static constexpr int VIEW_W = 640;
static constexpr int VIEW_H = 480;
static constexpr int TILE_H = 96;
static constexpr float RATIO = ((float)VIEW_W) / VIEW_H;
static constexpr float TWO_PI = 6.28318530718f;
static constexpr uint64_t SCENE_DURATION_MS = 9000;
static constexpr size_t MESH_CACHE_SIZE = 768 * 1024;

tgx_p4::Display display;

uint16_t* tile_zbuf = nullptr;
uint16_t* tile_fb[2] = { nullptr, nullptr };
char* mesh_cache = nullptr;
size_t mesh_cache_used = 0;
Image<RGB565> tile_image;

// Keep the compile-time shader set minimal, but include both textured and
// untextured paths because this example switches between them at runtime.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


struct Scene
    {
    const Mesh3Dv2<RGB565>* mesh;
    const char* label;
    int triangles;
    float scale;
    float y;
    float z;
    float base_angle;
    bool textured;
    };


const Scene scenes[] =
    {
        { &donkeykong_small, "donkey_gouraud", 3200, 6.6f, -0.2f, -23.0f, 20.0f, false },
        { &donkeykong_small, "donkey_texture", 3200, 6.6f, -0.2f, -23.0f, 20.0f, true  },
        { &naruto_v2,        "naruto_gouraud", 2472, 9.0f, -0.4f, -24.0f,  0.0f, false },
        { &naruto_v2,        "naruto_texture", 2472, 9.0f, -0.4f, -24.0f,  0.0f, true  },
    };

static constexpr int SCENE_COUNT = (int)(sizeof(scenes) / sizeof(scenes[0]));

int current_scene = -1;
const Mesh3Dv2<RGB565>* cached_mesh = nullptr;
const Mesh3Dv2<RGB565>* cached_donkey = nullptr;
const Mesh3Dv2<RGB565>* cached_naruto = nullptr;


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


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
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


void* allocPreferInternal(size_t bytes, const char* name)
    {
    // Tile framebuffers and z-buffer are hot; internal RAM is preferable.
    // The mesh cache is larger and may fall back to PSRAM.
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


size_t align4(size_t value)
    {
    return (value + 3) & ~(size_t)3;
    }


const Mesh3Dv2<RGB565>* cacheOneMesh(const Mesh3Dv2<RGB565>* mesh)
    {
    if (!mesh || !mesh_cache || mesh_cache_used >= MESH_CACHE_SIZE) return mesh;

    // Copy meshlets/materials/payload/textures once so the render loop does not
    // repeatedly fetch large static arrays from flash.
    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* cached = tgx::cacheMesh(mesh,
                                                    mesh_cache + mesh_cache_used,
                                                    MESH_CACHE_SIZE - mesh_cache_used,
                                                    "MIPL",
                                                    &cache_used);
    if (cache_used > 0)
        {
        mesh_cache_used += align4(cache_used);
        }

    const Mesh3Dv2<RGB565>* result = cached ? cached : mesh;
    ESP_LOGI(TAG, "mesh=%s cache_used=%u total_cache_used=%u",
             result->name ? result->name : "[unnamed]",
             (unsigned)cache_used,
             (unsigned)mesh_cache_used);
    return result;
    }


void cacheMeshes()
    {
    cached_donkey = cacheOneMesh(&donkeykong_small);
    cached_naruto = cacheOneMesh(&naruto_v2);
    }


const Mesh3Dv2<RGB565>* cachedSceneMesh(const Scene& scene)
    {
    if (scene.mesh == &donkeykong_small) return cached_donkey ? cached_donkey : scene.mesh;
    if (scene.mesh == &naruto_v2) return cached_naruto ? cached_naruto : scene.mesh;
    return scene.mesh;
    }


void setScene(int index)
    {
    if (index == current_scene) return;

    current_scene = index;
    const Scene& scene = scenes[current_scene];
    cached_mesh = cachedSceneMesh(scene);

    const Shader shader = scene.textured ?
        (SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2) :
        (SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setShaders(shader);
    timing_stats = TimingStats{};

    printf("[TGX scene] %s\n", scene.label);
    }


void updateSceneAndModel(uint64_t now_ms)
    {
    const int scene_index = (int)((now_ms / SCENE_DURATION_MS) % SCENE_COUNT);
    setScene(scene_index);

    const Scene& scene = scenes[current_scene];
    const float local_ms = (float)(now_ms % SCENE_DURATION_MS);
    const float turn = 360.0f * local_ms / (float)SCENE_DURATION_MS;
    const float bob = 0.25f * sinf(TWO_PI * local_ms / (float)SCENE_DURATION_MS);

    fMat4 M;
    M.setScale({ scene.scale, scene.scale, scene.scale });
    M.multRotate(scene.base_angle + turn, { 0, 1, 0 });
    M.multRotate(9.0f * sinf(TWO_PI * local_ms / ((float)SCENE_DURATION_MS * 0.72f)), { 1, 0, 0 });
    M.multTranslate({ 0.0f, scene.y + bob, scene.z });
    renderer.setModelMatrix(M);
    }


void drawOverlay(const Scene& scene, int fps)
    {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d FPS", fps);
    tile_image.drawText(buf, { VIEW_W - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    tile_image.drawText("TGX tiled texture mesh", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    tile_image.drawText(scene.mesh->name ? scene.mesh->name : "[unnamed mesh]", { 4, 32 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    snprintf(buf, sizeof(buf), "%d triangles / %s", scene.triangles, scene.textured ? "Gouraud + texture" : "Gouraud");
    tile_image.drawText(buf, { 4, 50 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void drawTile(int tile_buffer_index, int tile_y, int fps)
    {
    tile_image.set(tile_fb[tile_buffer_index], VIEW_W, TILE_H);

    // setOffset() tells TGX that this small framebuffer represents one band
    // of the larger 640x480 virtual viewport.
    renderer.setOffset(0, tile_y);
    tile_image.fillScreen(rgb888(1, 3, 8));
    renderer.clearZbuffer();
    renderer.drawMesh(cached_mesh);

    if (tile_y == 0)
        {
        drawOverlay(scenes[current_scene], fps);
        }
    }


void initRenderer()
    {
    tile_fb[0] = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer A"));
    tile_fb[1] = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer B"));
    tile_zbuf = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile z-buffer"));
    mesh_cache = static_cast<char*>(heap_caps_malloc(MESH_CACHE_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!mesh_cache) mesh_cache = static_cast<char*>(allocPreferInternal(MESH_CACHE_SIZE, "mesh cache"));
    cacheMeshes();

    tile_image.set(tile_fb[0], VIEW_W, TILE_H);
    renderer.setViewportSize(VIEW_W, VIEW_H);
    renderer.setImage(&tile_image);
    renderer.setZbuffer(tile_zbuf);
    renderer.setOffset(0, 0);
    renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setLight({ -0.35f, -0.65f, -0.55f },
                      RGBf(0.18f, 0.18f, 0.20f),
                      RGBf(0.82f, 0.82f, 0.82f),
                      RGBf(0.15f, 0.15f, 0.15f));

    setScene(0);
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
        const uint64_t now_ms = tgx_p4::millis64();

        const uint64_t update_start_us = nowUs();
        updateSceneAndModel(now_ms);
        const uint64_t update_us = nowUs() - update_start_us;

        const int fps = currentFPS();
        const int ox = (display.width() - VIEW_W) / 2;
        const int oy = (display.height() - VIEW_H) / 2;
        uint64_t draw_us = 0;
        uint64_t copy_us = 0;

        const uint64_t begin_start_us = nowUs();
        display.beginFrame();
        const uint64_t begin_us = nowUs() - begin_start_us;

        // Double-buffer the TGX tiles: draw into one tile buffer while the
        // previous tile is copied to the LCD backbuffer by the ESP32-P4 PPA.
        int tile_buffer = 0;
        for (int tile_y = 0; tile_y < VIEW_H; tile_y += TILE_H)
            {
            const uint64_t draw_start_us = nowUs();
            drawTile(tile_buffer, tile_y, fps);
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
