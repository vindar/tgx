/********************************************************************
* tgx library example : displaying the Donkey Kong 3D mesh.
*
*                    EXAMPLE FOR ESP32 / ESP32-S2 / ESP32-S3
*
* This sketch is the TFT_eSPI version of the M5Stack Donkey Kong mesh
* example.  It draws into a TGX framebuffer and uploads the framebuffer to an
* ILI9341/ST77xx-style TFT configured through TFT_eSPI.
*
* Instructions:
*
* 1. download and install Bodmer's TFT_eSPI library via Arduino's library
*    manager or directly from: https://github.com/Bodmer/TFT_eSPI
*
* 2. Configure the TFT_eSPI library for the screen used
*    (customize "TFT_eSPI/User_Setup.h" and/or
*    "TFT_eSPI/User_Setup_Select.h")
*
* 3. Select the board model and serial port and upload the sketch.
********************************************************************/

/**************** DMA NOTE ****************
* TFT_eSPI DMA transfers can improve display upload speed on ESP32 boards, but
* the DMA buffer is optional.  If it cannot be allocated, the sketch
* automatically falls back to the blocking pushImage() path.  Uncomment the
* line below to force that non-DMA path.
*******************************************/
// #define DISABLE_DMA

#include <TFT_eSPI.h>

#include <tgx.h>

#include "donkeykong_small_v2.h"
#include "tgx_example_telemetry.h"

using namespace tgx;

// Keep the viewport moderate so ESP32-S2/S3 boards with smaller internal RAM
// can still allocate the framebuffer and zbuffer.
static const int MAX_LX = 240;
static const int MAX_LY = 180;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

Image<RGB565> imfb;
TFT_eSPI tft = TFT_eSPI();

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE |
                              SHADER_ZBUFFER |
                              SHADER_FLAT |
                              SHADER_GOURAUD |
                              SHADER_NOTEXTURE |
                              SHADER_TEXTURE |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


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


void freeRenderBuffers()
    {
    if (fb != nullptr)
        {
        free(fb);
        fb = nullptr;
        }
    if (zbuf != nullptr)
        {
        free(zbuf);
        zbuf = nullptr;
        }
    if (fb2 != nullptr)
        {
        free(fb2);
        fb2 = nullptr;
        }
    use_dma = false;
    }


bool allocateRenderBuffers()
    {
    const size_t pixel_count = (size_t)render_lx * (size_t)render_ly;
    const size_t bytes = pixel_count * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
        Serial.println("donkeykong ESP32 DMA buffer released to keep framebuffer/zbuffer");
        freeRenderBuffers();
        fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        }

    if (fb == nullptr || zbuf == nullptr)
        {
        freeRenderBuffers();
        return false;
        }

#if !defined(DISABLE_DMA)
    if (fb2 != nullptr)
        {
        tft.initDMA();
        tft.startWrite();
        use_dma = true;
        Serial.println("donkeykong ESP32 display DMA enabled");
        }
    else
        {
        Serial.println("donkeykong ESP32 display DMA unavailable, using pushImage");
        }
#else
    Serial.println("donkeykong ESP32 display DMA disabled, using pushImage");
#endif

    return true;
    }


void setupRenderer()
    {
    imfb.set(fb, render_lx, render_ly);
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&imfb);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, render_ratio, 1.0f, 100.0f);
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    }


tgx::fMat4 moveModel(int& loopnumber)
    {
    const float end1 = 6000;
    const float end2 = 2000;
    const float end3 = 6000;
    const float end4 = 2000;

    const int tot = (int)(end1 + end2 + end3 + end4);
    int m = millis();

    loopnumber = m / tot;
    float t = m % tot;
    const float roty = 360 * (t / 4000);
    float tz, ty;
    const float dilat = 6.5;
    const float nearz = 13;
    const float upy = 2.8;
    if (t < end1)
        {
        tz = -25;
        ty = 0;
        }
    else
        {
        t -= end1;
        if (t < end2)
            {
            t /= end2;
            tz = -25 + (25 - nearz) * t;
            ty = -upy * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                {
                tz = -nearz;
                ty = -upy;
                }
            else
                {
                t -= end3;
                t /= end4;
                tz = -nearz - (25 - nearz) * t;
                ty = upy * (t - 1);
                }
            }
        }
    fMat4 M;
    M.setScale({ dilat, dilat, dilat });
    M.multRotate(-roty, { 0, 1, 0 });
    M.multTranslate({ 0, ty, tz });
    return M;
    }


void updateSceneLabel(int loopnumber)
    {
    static int prev_loopnumber = -1;
    if (prev_loopnumber == loopnumber) return;

    prev_loopnumber = loopnumber;
    if (use_dma) tft.dmaWait();
    tft.setTextDatum(BL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_DARKGREY, true);
    switch (loopnumber % 4)
        {
        case 0: tft.drawString("Gouraud/texture", 0, tft.height() - 1); telemetrySetScene("gouraud_texture"); break;
        case 1: tft.drawString("Wireframe      ", 0, tft.height() - 1); telemetrySetScene("wireframe"); break;
        case 2: tft.drawString("Flat Shading   ", 0, tft.height() - 1); telemetrySetScene("flat"); break;
        case 3: tft.drawString("Gouraud shading", 0, tft.height() - 1); telemetrySetScene("gouraud"); break;
        }
    }


void pushFrame()
    {
    const int x = (tft.width() - render_lx) / 2;
    const int y = (tft.height() - render_ly) / 2;
    if (use_dma)
        tft.pushImageDMA(x, y, render_lx, render_ly, fb, fb2);
    else
        tft.pushImage(x, y, render_lx, render_ly, fb);
    }


void setup()
    {
    Serial.begin(115200);
    telemetryBegin();

    enableBoardDisplayPower();

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_DARKGREY);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    const int available_y = (tft.height() > 20) ? (tft.height() - 20) : tft.height();
    if (render_ly > available_y) render_ly = available_y;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    while (!allocateRenderBuffers())
        {
        Serial.println("Error: cannot allocate framebuffer/zbuffer");
        delay(1000);
        }

    setupRenderer();
    }


int loopnumber = 0;


void loop()
    {
    telemetryStartFrame();

    fMat4 M = moveModel(loopnumber);
    renderer.setModelMatrix(M);

    imfb.fillScreen(RGB565_Cyan);
    renderer.clearZbuffer();

    switch (loopnumber % 4)
        {
        case 0:
            renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        case 1:
            renderer.drawWireFrameMeshAA(&donkeykong_small);
            break;
        case 2:
            renderer.setShaders(SHADER_FLAT);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        case 3:
            renderer.setShaders(SHADER_GOURAUD);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        }

    updateSceneLabel(loopnumber);
    telemetryEndFrame();

    pushFrame();
    }
