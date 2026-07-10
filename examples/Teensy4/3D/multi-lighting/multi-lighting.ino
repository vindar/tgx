/********************************************************************
*
* tgx library example: multiple directional lights.
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

// This example runs on Teensy 4.0/4.1 with ILI9341 via SPI.
// The screen driver library: https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h>

// The tgx library.
#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

using namespace tgx;


//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK     13
#define PIN_MISO    12
#define PIN_MOSI    11
#define PIN_DC      10

#define PIN_CS      9
#define PIN_RESET   6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255


// 30 MHz SPI, we can go higher with short wires.
#define SPI_SPEED       30000000


ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;

static const int SLX = 320;
static const int SLY = 240;

DMAMEM uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
DMAMEM uint16_t zbuf[SLX * SLY];

Image<RGB565> im(fb, SLX, SLY);


// No extra shader flag is needed for multiple directional lights.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE;

// The last template argument gives this renderer four directional-light slots.
Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 4> renderer;


#include "tgx_example_telemetry.h"


bool multi_light_mode = false;


void setOneDirectionalLight()
    {
    // The simple API still controls directional light 0.
    renderer.setLight({ -0.35f, -0.55f, -1.0f },
                      RGBf(0.18f, 0.18f, 0.20f),
                      RGBf(0.85f, 0.82f, 0.76f),
                      RGBf(0.50f, 0.50f, 0.55f));
    renderer.setDirectionalLightCount(1);
    }


void setFourDirectionalLights()
    {
    renderer.setDirectionalLightCount(4);
    renderer.setDirectionalLightAmbiant(RGBf(0.07f, 0.08f, 0.11f));
    renderer.setDirectionalLight(0, { -0.55f, -0.55f, -1.0f }, RGBf(0.54f, 0.48f, 0.40f), RGBf(0.22f, 0.20f, 0.18f));
    renderer.setDirectionalLight(1, {  0.85f, -0.20f, -0.55f }, RGBf(0.18f, 0.34f, 0.95f), RGBf(0.05f, 0.10f, 0.28f));
    renderer.setDirectionalLight(2, { -0.35f,  0.72f, -0.45f }, RGBf(0.88f, 0.20f, 0.18f), RGBf(0.24f, 0.06f, 0.05f));
    renderer.setDirectionalLight(3, {  0.18f,  0.35f, -0.90f }, RGBf(0.18f, 0.78f, 0.38f), RGBf(0.06f, 0.18f, 0.09f));
    }


void setup()
    {
    Serial.begin(9600);
    telemetryBegin();

    while (!tft.begin(SPI_SPEED));

    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setVSyncSpacing(0);
    tft.setRefreshRate(140);

    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, ((float)SLX) / SLY, 1.0f, 100.0f);
    renderer.setCulling(1);
    setOneDirectionalLight();
    telemetrySetScene("one_directional_light");
    }


void drawLabel(const char* text)
    {
    im.drawText(text, { 5, 13 }, font_tgx_OpenSans_Bold_10, RGB565_White);
    im.drawText("Renderer3D<..., 4>", { 5, SLY - 8 }, font_tgx_OpenSans_Bold_10, RGB565_White);
    }


void loop()
    {
    const bool use_multi = ((millis() / 4500) & 1) != 0;
    if (use_multi != multi_light_mode)
        {
        multi_light_mode = use_multi;
        if (multi_light_mode) setFourDirectionalLights();
        else setOneDirectionalLight();
        telemetrySetScene(multi_light_mode ? "four_directional_lights" : "one_directional_light");
        }

    const float a = millis() * 0.055f;

    telemetryStartFrame();

    im.fillScreen(RGB565(8, 10, 18));
    renderer.clearZbuffer();

    renderer.setShaders(SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setMaterial(RGBf(0.88f, 0.78f, 0.62f), 0.15f, 0.82f, 0.62f, 36);
    renderer.setModelPosScaleRot({ -1.45f, 0.05f, -7.8f }, { 1.55f, 1.55f, 1.55f }, a, { 0.15f, 1.0f, 0.10f });
    renderer.drawSphere(28, 15);

    renderer.setShaders(SHADER_FLAT | SHADER_NOTEXTURE);
    renderer.setMaterial(RGBf(0.62f, 0.74f, 0.98f), 0.14f, 0.76f, 0.50f, 24);
    renderer.setModelPosScaleRot({ 1.45f, 0.0f, -7.8f }, { 1.18f, 1.18f, 1.18f }, -a * 1.3f, { 0.35f, 1.0f, 0.2f });
    renderer.drawCube();

    drawLabel(multi_light_mode ? "4 colored directional lights" : "simple one-light API");
    tft.overlayFPS(fb);
    telemetryEndFrame();
    tft.update(fb);
    }
