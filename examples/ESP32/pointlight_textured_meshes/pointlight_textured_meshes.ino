/********************************************************************
* tgx library example: moving point light on textured meshes.
*
*           EXAMPLE FOR ESP32 FAMILY (ESP32, ESP32-S2, ESP32-S3 ...)
*
* The sketch renders into a TGX framebuffer, then sends it to the display
* with TFT_eSPI.
********************************************************************/

// #define DISABLE_DMA

#include <TFT_eSPI.h>

#include <tgx.h>

using namespace tgx;

static const int MAX_LX = 240;
static const int MAX_LY = 180;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

Image<RGB565> im;
TFT_eSPI tft = TFT_eSPI();

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;

// Affine texturing is faster on ESP32-S2; other ESP32 variants keep perspective-correct texturing.
#if defined(TGX_RUN_ON_ESP32S2)
static constexpr Shader TEXTURE_SHADER = SHADER_TEXTURE_AFFINE;
#else
static constexpr Shader TEXTURE_SHADER = SHADER_TEXTURE;
#endif

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_UNLIT |
                              SHADER_NOTEXTURE |
                              TEXTURE_SHADER |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer;

#include "pointlight_textured_scene.h"

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
        Serial.print("pointlight_textured_meshes ESP32 fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        Serial.print("pointlight_textured_meshes scene=");
        Serial.println(pointlight_mesh_scenes[pointlightSceneIndex()].name);
        Serial.print("pointlight_textured_meshes display=");
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
        Serial.println("pointlight_textured_meshes ESP32 DMA buffer released to keep framebuffer/zbuffer");
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


void enableBoardDisplayPower()
    {
    // Some Adafruit TFT Feather boards switch display power and backlight with
    // GPIOs.  These defines are present only on those boards.
#if defined(TFT_I2C_POWER)
    pinMode(TFT_I2C_POWER, OUTPUT);
    digitalWrite(TFT_I2C_POWER, HIGH);
    delay(10);
#endif
#if defined(TFT_BACKLITE)
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
#endif
#if defined(TFT_BL)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
    }


void pushFrame()
    {
    const int x = (tft.width() - render_lx) / 2;
    const int y = (tft.height() - render_ly) / 2;
    if (use_dma)
        {
        tft.pushImageDMA(x, y, render_lx, render_ly, fb, fb2);
        }
    else
        {
        tft.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    enableBoardDisplayPower();

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    if (!allocateRenderBuffers((size_t)render_lx * (size_t)render_ly))
        {
        Serial.println("pointlight_textured_meshes ESP32 buffer allocation failed");
        while (true) delay(1000);
        }

    im.set(fb, render_lx, render_ly);

    if (use_dma)
        {
        tft.initDMA();
        tft.startWrite();
        Serial.println("pointlight_textured_meshes ESP32 display DMA enabled");
        }
    else
        {
        Serial.println("pointlight_textured_meshes ESP32 display DMA unavailable, using pushImage");
        }

    setupPointlightTexturedScene(render_ratio);
    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawPointlightTexturedFrame();
    const uint32_t render_us = micros() - render_start_us;
    pushFrame();
    updateFPS(render_us);
    }
