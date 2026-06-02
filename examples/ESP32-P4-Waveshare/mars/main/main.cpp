/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Mars / skybox demo adapted from the Teensy 4 version. This ESP-IDF
* port keeps the same TGX rendering ideas: skybox cube, textured base,
* textured sphere and Mesh3Dv2 Falcon, rendered into a centered 320x240
* framebuffer on the 720x720 MIPI-DSI LCD.
*
********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"

#include "earth_small.h"
#include "marble.h"
#include "mars_front.h"
#include "mars_back.h"
#include "mars_left.h"
#include "mars_right.h"
#include "mars_bottom.h"
#include "mars_top_neb.h"
#include "../../../Teensy4/3D/mars/falcon/falcon_vs_v2.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_mars";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr size_t MESH_CACHE_SIZE = 512 * 1024;
static constexpr float SKYBOX_SIZE = 4000.0f;
static constexpr float BASE_WIDTH = 45.0f;
static constexpr float BASE_HEIGHT = 8.0f;
static constexpr float BASE_Y = -50.0f;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
char* mesh_cache = nullptr;
Image<RGB565> im;

const Mesh3Dv2<RGB565>* falcon_mesh = &falcon_vs_v2;
const Image<RGB565>* falcon_engine_off_texture = &Engine01_texture;
bool falcon_materials_writable = false;
static constexpr uint8_t FALCON_ENGINE_MATERIAL = 6;

const Shader LOADED_SHADER = SHADER_ZBUFFER | SHADER_PERSPECTIVE | SHADER_GOURAUD |
                             SHADER_FLAT | SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST |
                             SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADER, uint16_t> renderer;


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


float smooth01(float t)
    {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return 0.5f - 0.5f * cosf((float)M_PI * t);
    }


void drawSkyBox()
    {
    renderer.setCulling(0);
    renderer.setMaterialAmbiantStrength(1.8f);
    renderer.setMaterialDiffuseStrength(0.0f);
    renderer.setMaterialSpecularStrength(0.0f);
    renderer.setModelPosScaleRot({ 0, 0, 0 }, { SKYBOX_SIZE, SKYBOX_SIZE, SKYBOX_SIZE }, 180, { 0, 1, 0 });
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE_NEAREST);
    renderer.drawCube(&mars_front, &mars_back, &mars_top_neb, &mars_bottom, &mars_left, &mars_right);
    }


void drawBase()
    {
    renderer.setCulling(1);
    renderer.setMaterialSpecularExponent(6);
    renderer.setMaterialAmbiantStrength(0.2f);
    renderer.setMaterialDiffuseStrength(0.9f);
    renderer.setMaterialSpecularStrength(0.4f);
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2);

    const float aa = -0.05f;
    const float ee = 0.01f;
    const fVec2 v_front[4] = { { 0.2f + aa, 0.8f - aa }, { 0.2f + aa, 1.0f - ee }, { 0.8f - aa, 1.0f - ee }, { 0.8f - aa, 0.8f - aa } };
    const fVec2 v_back[4] =  { { 0.8f - aa, 0.2f + aa }, { 0.8f - aa, 0.0f + ee }, { 0.2f + aa, 0.0f + ee }, { 0.2f + aa, 0.2f + aa } };
    const fVec2 v_top[4] =   { { 0.2f + aa, 0.2f + aa }, { 0.2f + aa, 0.8f - aa }, { 0.8f - aa, 0.8f - aa }, { 0.8f - aa, 0.2f + aa } };
    const fVec2 v_bottom[4] = { { 0.2f + aa, 0.2f + aa }, { 0.2f + aa, 0.8f - aa }, { 0.8f - aa, 0.8f - aa }, { 0.8f - aa, 0.2f + aa } };
    const fVec2 v_left[4] =  { { 0.2f + aa, 0.2f + aa }, { 0.0f + ee, 0.2f + aa }, { 0.0f + ee, 0.8f - aa }, { 0.2f + aa, 0.8f - aa } };
    const fVec2 v_right[4] = { { 0.8f - aa, 0.8f - aa }, { 1.0f - ee, 0.8f - aa }, { 1.0f - ee, 0.2f + aa }, { 0.8f - aa, 0.2f + aa } };

    renderer.drawCube(v_front, &marble, v_back, &marble, v_top, &marble,
                      v_bottom, &marble, v_left, &marble, v_right, &marble);
    }


void setFalconEngineTexture(const Image<RGB565>* texture)
    {
    if (!falcon_materials_writable) return;
    MeshMaterial3Dv2<RGB565>* materials = const_cast<MeshMaterial3Dv2<RGB565>*>(falcon_mesh->materials);
    materials[FALCON_ENGINE_MATERIAL].texture = texture;
    }


void drawFalcon(float t)
    {
    renderer.setCulling(1);
    renderer.setMaterialAmbiantStrength(0.2f);
    renderer.setMaterialDiffuseStrength(0.75f);
    renderer.setMaterialSpecularStrength(0.5f);
    renderer.setMaterialSpecularExponent(32);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2);

    const float x = -75.0f + 150.0f * smooth01(t);
    const float y = -12.0f + 9.0f * sinf(2.0f * (float)M_PI * t);
    const float z = -170.0f + 70.0f * sinf((float)M_PI * t);
    const float roll = 20.0f * sinf(2.0f * (float)M_PI * t);
    const float yaw = -35.0f + 70.0f * t;

    renderer.setModelPosScaleRot({ x, y, z }, { 12.0f, 12.0f, 12.0f }, yaw, { 0, 1, 0 });
    fMat4 M = renderer.getModelMatrix();
    M.multRotate(roll, { 0, 0, 1 });
    renderer.setModelMatrix(M);
    setFalconEngineTexture((t > 0.55f) ? &Engine02_texture : falcon_engine_off_texture);
    renderer.drawMesh(falcon_mesh);
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
    renderer.setPerspective(50.0f, ((float)SLX) / SLY, 10.0f, 8000.0f);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setLightDirection({ -0.40f, -0.30f, 1.0f });
    renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f });

    size_t cache_used = 0;
    const Mesh3Dv2<RGB565>* cached = cacheMesh(&falcon_vs_v2, mesh_cache, MESH_CACHE_SIZE, "LMP", &cache_used);
    falcon_mesh = cached ? cached : &falcon_vs_v2;
    falcon_materials_writable = (cached != nullptr);
    falcon_engine_off_texture = falcon_mesh->materials[FALCON_ENGINE_MATERIAL].texture;
    ESP_LOGI(TAG, "falcon cache_used=%u", (unsigned)cache_used);
    printf("[TGX scene] mars_demo\n");
    }


void drawScene(float t)
    {
    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();

    const float angle = 2.0f * (float)M_PI * t;
    const fVec3 eye = { 210.0f * sinf(angle * 0.35f), -12.0f + 28.0f * sinf(angle * 0.2f), -210.0f + 55.0f * cosf(angle * 0.5f) };
    const fVec3 at = { 15.0f * sinf(angle * 0.7f), -20.0f + 12.0f * sinf(angle * 0.3f), 0.0f };
    renderer.setLookAt(eye, at, { 0, 1, 0 });

    drawSkyBox();

    const fVec3 base_pos = { 0, BASE_Y, 0 };
    renderer.setModelPosScaleRot(base_pos, { BASE_WIDTH, BASE_HEIGHT, BASE_WIDTH }, 15.0f * sinf(angle), { 0, 1, 0 });
    drawBase();

    const float sphere_r = 55.0f + 6.0f * sinf(angle * 0.8f);
    renderer.setModelPosScaleRot({ 0, BASE_Y + 70.0f, 0 }, { sphere_r, sphere_r, sphere_r }, 360.0f * t, { 0, 1, 0 });
    renderer.setCulling(1);
    renderer.setMaterialAmbiantStrength(0.25f);
    renderer.setMaterialDiffuseStrength(0.85f);
    renderer.setMaterialSpecularStrength(0.7f);
    renderer.setMaterialSpecularExponent(24);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2);
    renderer.drawAdaptativeSphere(&earth_small, 3.0f);

    drawFalcon(t);

    im.drawText("Mars demo / Mesh3Dv2 + skybox", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    drawFPS();
    display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    const uint64_t period_ms = 22000;
    const uint64_t start = tgx_p4::millis64();

    while (true)
        {
        const uint64_t now = tgx_p4::millis64();
        const float t = (float)((now - start) % period_ms) / (float)period_ms;
        drawScene(t);
        }
    }
