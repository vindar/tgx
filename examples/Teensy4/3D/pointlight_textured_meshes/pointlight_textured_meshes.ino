/********************************************************************
*
* tgx library example: moving point light on textured meshes.
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

#include <ILI9341_T4.h>
#include <tgx.h>

using namespace tgx;


#define PIN_SCK     13
#define PIN_MISO    12
#define PIN_MOSI    11
#define PIN_DC      10
#define PIN_CS      9
#define PIN_RESET   6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255

#define SPI_SPEED 30000000

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;

static const int SLX = 320;
static const int SLY = 240;
static const int render_lx = SLX;
static const int render_ly = SLY;

DMAMEM uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
DMAMEM uint16_t zbuf[SLX * SLY];

Image<RGB565> im(fb, SLX, SLY);

static constexpr Shader TEXTURE_SHADER = SHADER_TEXTURE;

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
        Serial.print("pointlight_textured_meshes Teensy4 fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        Serial.print("pointlight_textured_meshes scene=");
        Serial.println(pointlight_mesh_scenes[pointlightSceneIndex()].name);
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;
        }
    }


void setup()
    {
    Serial.begin(115200);

    while (!tft.begin(SPI_SPEED));

    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setVSyncSpacing(0);
    tft.setRefreshRate(140);

    setupPointlightTexturedScene((float)SLX / (float)SLY);
    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawPointlightTexturedFrame();
    const uint32_t render_us = micros() - render_start_us;
    tft.update(fb);
    updateFPS(render_us);
    }
