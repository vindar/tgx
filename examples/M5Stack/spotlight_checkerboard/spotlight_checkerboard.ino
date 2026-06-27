/********************************************************************
* tgx library example: animated spotlight on a checkerboard.
*
*                  EXAMPLE FOR M5STACK Core2 and CoreS3
*
* The sketch renders into a TGX framebuffer, then sends it to the display
* with LovyanGFX.
********************************************************************/

// #define DISABLE_DMA

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>

using namespace tgx;

static const int MAX_LX = 240;
static const int MAX_LY = 180;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

Image<RGB565> im;
LGFX lcd;

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_UNLIT |
                              SHADER_NOTEXTURE;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer;

#include "spotlight_checkerboard_scene.h"

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;
uint32_t fps_render_sum_us = 0;


void updateFPS(uint32_t render_us)
    {
    fps_frames++;
    fps_render_sum_us += render_us;
    const uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("spotlight_checkerboard M5Stack fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        Serial.print("spotlight_checkerboard display=");
        Serial.println(use_dma ? "DMA" : "pushImage");
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;
        }
    }


void freeRenderBuffers()
    {
    if (fb != nullptr) { free(fb); fb = nullptr; }
    if (zbuf != nullptr) { free(zbuf); zbuf = nullptr; }
    if (fb2 != nullptr) { free(fb2); fb2 = nullptr; }
    use_dma = false;
    }


bool allocateRenderBuffers(size_t pixel_count)
    {
    const size_t bytes = pixel_count * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
        freeRenderBuffers();
        fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        }

    if (fb == nullptr || zbuf == nullptr)
        {
        freeRenderBuffers();
        return false;
        }

    use_dma = (fb2 != nullptr);
    return true;
    }


void copySwapBuffer()
    {
    const int n = render_lx * render_ly;
    for (int i = 0; i < n; i++)
        {
        const uint16_t a = fb[i];
        fb2[i] = (a << 8) | (a >> 8);
        }
    }


void pushFrame()
    {
    const int x = (lcd.width() - render_lx) / 2;
    const int y = (lcd.height() - render_ly) / 2;
    if (use_dma)
        {
        lcd.waitDMA();
        copySwapBuffer();
        lcd.pushImageDMA(x, y, render_lx, render_ly, fb2);
        }
    else
        {
        lcd.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    lcd.fillScreen(TFT_BLACK);

    render_lx = lcd.width();
    render_ly = lcd.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    if (!allocateRenderBuffers((size_t)render_lx * (size_t)render_ly))
        {
        Serial.println("spotlight_checkerboard M5Stack buffer allocation failed");
        while (true) delay(1000);
        }

    im.set(fb, render_lx, render_ly);

    if (use_dma)
        {
        lcd.initDMA();
        lcd.startWrite();
        Serial.println("spotlight_checkerboard M5Stack display DMA enabled");
        }
    else
        {
        Serial.println("spotlight_checkerboard M5Stack display DMA unavailable, using pushImage");
        }

    lcd.setSwapBytes(!use_dma);

    setupSpotlightCheckerboardScene(render_ratio);
    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawSpotlightCheckerboardFrame();
    const uint32_t render_us = micros() - render_start_us;
    pushFrame();
    updateFPS(render_us);
    }
