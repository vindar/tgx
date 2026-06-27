/********************************************************************
* tgx library example: moving point light on textured meshes.
*
*                    EXAMPLE FOR RP2040 / RP2350
*
* The sketch renders into a TGX framebuffer, then sends it to the TFT
* screen with TFT_eSPI.
********************************************************************/

#include <TFT_eSPI.h>
#include <tgx.h>

using namespace tgx;

#if defined(ARDUINO_ARCH_RP2350) || defined(PICO_RP2350) || defined(__ARM_ARCH_8M_MAIN__)
static const int MAX_LX = 320;
static const int MAX_LY = 220;
#else
static const int MAX_LX = 180;
static const int MAX_LY = 135;
#endif

uint16_t fb[MAX_LX * MAX_LY];
uint16_t fb2[MAX_LX * MAX_LY];
uint16_t zbuf[MAX_LX * MAX_LY];

Image<RGB565> im;
TFT_eSPI tft = TFT_eSPI();

const uint16_t SCREEN_BORDER_COLOR = TFT_BLACK;

#if defined(TGX_RUN_ON_RP2040)
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

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;
bool use_dma = false;
int draw_buffer_index = 0;

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
        Serial.print("pointlight_textured_meshes Pico fps=");
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


void setupDMA()
    {
    use_dma = tft.initDMA();
    if (use_dma)
        {
        tft.startWrite();
        Serial.println("pointlight_textured_meshes Pico display DMA enabled");
        }
    else
        {
        Serial.println("pointlight_textured_meshes Pico display DMA unavailable, using pushImage");
        }
    }


void pushFrame()
    {
    const int x = (tft.width() - render_lx) / 2;
    const int y = (tft.height() - render_ly) / 2;
    if (use_dma)
        {
        uint16_t* current = (draw_buffer_index == 0) ? fb : fb2;
        tft.pushImageDMA(x, y, render_lx, render_ly, current);
        draw_buffer_index = 1 - draw_buffer_index;
        im.set((draw_buffer_index == 0) ? fb : fb2, render_lx, render_ly);
        renderer.setImage(&im);
        }
    else
        {
        tft.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(SCREEN_BORDER_COLOR);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    setupDMA();

    im.set(fb, render_lx, render_ly);
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
