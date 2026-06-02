/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Dynamic textured cube. The texture is modified every frame, then TGX
* renders the cube into a centered 320x240 RGB565 framebuffer.
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_borg_cube";

static constexpr int SLX = 320;
static constexpr int SLY = 240;
static constexpr float RATIO = ((float)SLX) / SLY;
static constexpr int TEX_SIZE = 128;

tgx_p4::Display display;

uint16_t* fb = nullptr;
uint16_t* zbuf = nullptr;
RGB565 texture_data[TEX_SIZE * TEX_SIZE];
Image<RGB565> im;
Image<RGB565> texture(texture_data, TEX_SIZE, TEX_SIZE);

const Shader LOADED_SHADERS = SHADER_ORTHO | SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


int randInt(int limit)
    {
    if (limit <= 0) return 0;
    return (int)(esp_random() % (uint32_t)limit);
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


void initRenderer()
    {
    fb = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX framebuffer"));
    zbuf = static_cast<uint16_t*>(allocPreferInternal(SLX * SLY * sizeof(uint16_t), "TGX z-buffer"));

    im.set(fb, SLX, SLY);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_BILINEAR);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE);
    renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);

    texture.fillScreen(RGB565_Blue);
    printf("[TGX scene] perspective\n");
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


void splash()
    {
    static int count = 0;
    static RGB565 color = RGB565_Green;

    if (count == 0)
        {
        const int palette = randInt(6);
        if (palette == 0) color = RGB565(randInt(32), randInt(16), randInt(8));
        else if (palette == 1) color = RGB565(randInt(8), randInt(32), randInt(16));
        else if (palette == 2) color = RGB565(randInt(16), randInt(8), randInt(32));
        else if (palette == 3) color = RGB565(randInt(32), randInt(32), randInt(4));
        else if (palette == 4) color = RGB565(randInt(8), randInt(32), randInt(32));
        else color = RGB565(randInt(32), randInt(8), randInt(32));
        }
    count = (count + 1) % 220;

    const iVec2 pos(randInt(TEX_SIZE), randInt(TEX_SIZE));
    const int r = 2 + randInt(8);
    texture.fillRect(iBox2(pos.x - r, pos.x + r, pos.y - r, pos.y + r), color);
    texture.drawFastHLine({ 0, randInt(TEX_SIZE) }, TEX_SIZE, RGB565(18, 62, 25));
    texture.drawFastVLine({ randInt(TEX_SIZE), 0 }, TEX_SIZE, RGB565(18, 62, 25));
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    uint64_t start = tgx_p4::millis64();
    int frame = 0;
    int projtype = 1;

    while (true)
        {
        const uint64_t now = tgx_p4::millis64();
        const float em = (float)(now - start);

        fMat4 M;
        M.setRotate(em / 11.0f, { 0, 1, 0 });
        M.multRotate(em / 23.0f, { 1, 0, 0 });
        M.multRotate(em / 41.0f, { 0, 0, 1 });
        M.multTranslate({ 0, 0, -5 });

        im.fillScreen(projtype ? RGB565_Black : RGB565_Gray);
        renderer.clearZbuffer();
        renderer.setModelMatrix(M);
        renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);

        im.drawText(projtype ? "Perspective projection" : "Orthographic projection",
                    { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
        drawFPS();
        display.presentCenteredRGB565(fb, SLX, SLY, 0x0014);

        splash();

        if (++frame % 1000 == 0)
            {
            projtype = 1 - projtype;
            if (projtype)
                {
                renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);
                printf("[TGX scene] perspective\n");
                }
            else
                {
                renderer.setOrtho(-1.8f * RATIO, 1.8f * RATIO, -1.8f, 1.8f, 1.0f, 100.0f);
                printf("[TGX scene] orthographic\n");
                }
            }
        }
    }
