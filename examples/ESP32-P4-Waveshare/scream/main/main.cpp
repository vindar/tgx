/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Dynamic geometry demo based on the Teensy 4 "scream" example.
* TGX renders the deforming textured sheet into a centered 320x240
* RGB565 framebuffer, then the ESP-IDF display helper presents it on
* the 720x720 MIPI-DSI panel.
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
#include "scream_texture.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_scream";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr int N = 45;
static constexpr int M = 45;
static constexpr float DX = 2.0f / N;
static constexpr float DY = 2.0f / M;
static constexpr int STRIP_INDEX_COUNT = 2 * (N + 1) + (M - 1) * (2 * (N + 1) + 2);

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
Image<RGB565> im;

fVec3 vertices[(N + 1) * (M + 1)];
fVec3 normals[(N + 1) * (M + 1)];
fVec2 texcoords[(N + 1) * (M + 1)];
uint16_t strip[STRIP_INDEX_COUNT];

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;
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
    im.drawText(buf, { SLX - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }


void initSheet()
    {
    for (int j = 0; j <= M; j++)
        {
        for (int i = 0; i <= N; i++)
            {
            vertices[(N + 1) * j + i] = fVec3(i * DX - 1.0f, 0.0f, j * DY - 1.0f);
            texcoords[(N + 1) * j + i] = fVec2(i * DX / 2.0f, j * DY / 2.0f);
            }
        }

    int k = 0;
    for (int j = 0; j < M; j++)
        {
        if (j > 0)
            {
            const uint16_t first = (uint16_t)(j * (N + 1));
            const uint16_t last = strip[k - 1];
            strip[k++] = last;
            strip[k++] = first;
            strip[k++] = first;
            strip[k++] = (uint16_t)((j + 1) * (N + 1));
            for (int i = 1; i <= N; i++)
                {
                strip[k++] = (uint16_t)(j * (N + 1) + i);
                strip[k++] = (uint16_t)((j + 1) * (N + 1) + i);
                }
            }
        else
            {
            for (int i = 0; i <= N; i++)
                {
                strip[k++] = (uint16_t)(j * (N + 1) + i);
                strip[k++] = (uint16_t)((j + 1) * (N + 1) + i);
                }
            }
        }
    }


void addFaceNormal(int i1, int i2, int i3)
    {
    const fVec3 P1 = vertices[i1];
    const fVec3 P2 = vertices[i2];
    const fVec3 P3 = vertices[i3];
    const fVec3 Nf = crossProduct(P1 - P3, P2 - P1);
    normals[i1] += Nf;
    normals[i2] += Nf;
    normals[i3] += Nf;
    }


void updateNormals()
    {
    for (int k = 0; k < (N + 1) * (M + 1); k++) normals[k] = fVec3(0, 0, 0);

    for (int j = 0; j < M; j++)
        {
        for (int i = 0; i < N; i++)
            {
            addFaceNormal((N + 1) * j + i, (N + 1) * (j + 1) + i, (N + 1) * j + i + 1);
            addFaceNormal((N + 1) * (j + 1) + i, (N + 1) * (j + 1) + i + 1, (N + 1) * j + i + 1);
            }
        }

    for (int k = 0; k < (N + 1) * (M + 1); k++) normals[k].normalize();
    }


float sinc(float x)
    {
    if (fabsf(x) < 0.01f) return 1.0f;
    return sinf(x) / x;
    }


float randomFloat(float a, float b)
    {
    const uint32_t r = esp_random() & 0x00ffffff;
    const float u = (float)r / (float)0x01000000;
    return a + (b - a) * u;
    }


fVec3 cameraPosition()
    {
    const uint64_t em = tgx_p4::millis64();
    const float T = 30.0f;
    const float d = 1.5f + 0.5f * sinf((float)em / 10000.0f);
    const float w = (float)em * 2.0f * (float)M_PI / (1000.0f * T) + (float)M_PI * 0.6f;
    return fVec3(d * sinf(w), d * 0.8f, d * cosf(w));
    }


void initRenderer()
    {
    fb = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX framebuffer"));
    zbuf = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX z-buffer"));

    im.set(fb, SLX, SLY);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45.0f, ((float)SLX) / SLY, 0.1f, 50.0f);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
    renderer.setCulling(0);

    fMat4 MV;
    MV.setIdentity();
    renderer.setModelMatrix(MV);

    initSheet();
    printf("[TGX scene] explosion\n");
    }


void explosion(fVec2 center, float h, float w, float s, float start_delay_ms)
    {
    s /= 1000.0f;
    const uint64_t start_ms = tgx_p4::millis64();
    const float t0 = start_delay_ms;
    const float t1 = start_delay_ms + 2000.0f;
    const float t2 = start_delay_ms + 5000.0f;

    while ((float)(tgx_p4::millis64() - start_ms) < t2)
        {
        const float t = (float)(tgx_p4::millis64() - start_ms);

        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();

        const float alpha = ((t <= t0) || (t >= t2)) ? 0.0f :
            ((t < t1) ? (t - t0) / (t1 - t0) : (1.0f - (t - t1) / (t2 - t1)));

        for (int j = 0; j <= M; j++)
            {
            for (int i = 0; i <= N; i++)
                {
                const float x = DX * i - 1.0f - center.x;
                const float y = DY * j - 1.0f - center.y;
                const float r = sqrtf(x * x + y * y);
                vertices[(N + 1) * j + i].y = alpha * h * sinc(w * (r - fmaxf(0.0f, t - t1) * s));
                }
            }

        updateNormals();

        renderer.setLookAt(cameraPosition(), { 0, 0, 0 }, { 0, 1, 0 });
        renderer.drawTriangleStrip(STRIP_INDEX_COUNT, strip, vertices, strip, normals, strip, texcoords, &scream_texture);

        drawFPS();
        display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);
        }
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    explosion({ 0.0f, 0.0f }, 0.4f, 10.0f, 0.4f, 2000.0f);

    while (true)
        {
        const float h = randomFloat(0.0f, 0.3f);
        const float w = randomFloat(3.0f, 18.0f);
        const float s = randomFloat(0.2f, 2.0f);
        const fVec2 center(randomFloat(-0.7f, 0.7f), randomFloat(-0.7f, 0.7f));
        explosion(center, h, w, s, 0.0f);
        }
    }
