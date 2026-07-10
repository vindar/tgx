/********************************************************************
*
* tgx library example: texture mask color.
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
static const int TEX_W = 384;
static const int TEX_H = 384;

uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
uint16_t zbuf[SLX * SLY];
DMAMEM RGB565 texture_pixels[TEX_W * TEX_H];

Image<RGB565> im(fb, SLX, SLY);
Image<RGB565> mask_texture(texture_pixels, TEX_W, TEX_H);

const RGB565 MASK_COLOR = RGB565(31, 0, 31);

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_UNLIT |
                              SHADER_NOTEXTURE | SHADER_TEXTURE |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_CLAMP |
                              SHADER_TEXTURE_NOMASK | SHADER_TEXTURE_MASK;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


uint32_t telemetry_last_ms = 0;
uint32_t telemetry_frame_start_us = 0;
uint32_t telemetry_frames = 0;
uint32_t telemetry_sum_us = 0;
uint32_t telemetry_min_us = 0xFFFFFFFFu;
uint32_t telemetry_max_us = 0;
uint32_t telemetry_cycle = 0;
const char* telemetry_scene = "startup";


void telemetryBegin()
    {
    telemetry_last_ms = millis();
    telemetry_frames = 0;
    telemetry_sum_us = 0;
    telemetry_min_us = 0xFFFFFFFFu;
    telemetry_max_us = 0;
    }


void telemetryStartFrame()
    {
    if (telemetry_frames == 0) telemetry_last_ms = millis();
    telemetry_frame_start_us = micros();
    }


void telemetryPrintScene()
    {
    for (const char* p = telemetry_scene; *p != 0; p++)
        {
        const char c = *p;
        Serial.print((c <= ' ' || c == '=') ? '_' : c);
        }
    }


void telemetrySetScene(const char* scene)
    {
    if (scene == nullptr) scene = "unnamed";
    telemetry_scene = scene;
    telemetry_cycle++;
    telemetry_last_ms = millis();
    telemetry_frames = 0;
    telemetry_sum_us = 0;
    telemetry_min_us = 0xFFFFFFFFu;
    telemetry_max_us = 0;
    Serial.print("\n[TGX scene] cycle=");
    Serial.print(telemetry_cycle);
    Serial.print(" scene=");
    telemetryPrintScene();
    Serial.println();
    }


void telemetryEndFrame()
    {
    const uint32_t dt = micros() - telemetry_frame_start_us;
    telemetry_frames++;
    telemetry_sum_us += dt;
    if (dt < telemetry_min_us) telemetry_min_us = dt;
    if (dt > telemetry_max_us) telemetry_max_us = dt;

    const uint32_t now = millis();
    const uint32_t elapsed_ms = now - telemetry_last_ms;
    if (elapsed_ms >= 1000)
        {
        Serial.print("\n[TGX telemetry] cycle=");
        Serial.print(telemetry_cycle);
        Serial.print(" scene=");
        telemetryPrintScene();
        Serial.print(" fps=");
        Serial.print((1000.0f * telemetry_frames) / elapsed_ms, 2);
        Serial.print(" frame_avg_us=");
        Serial.print(((float)telemetry_sum_us) / telemetry_frames, 1);
        Serial.print(" frame_min_us=");
        Serial.print(telemetry_min_us);
        Serial.print(" frame_max_us=");
        Serial.print(telemetry_max_us);
        Serial.print(" frames=");
        Serial.println(telemetry_frames);

        telemetry_last_ms = now;
        telemetry_frames = 0;
        telemetry_sum_us = 0;
        telemetry_min_us = 0xFFFFFFFFu;
        telemetry_max_us = 0;
        }
    }


void buildMaskTexture()
    {
    const int sx = TEX_W / 64;
    const int sy = TEX_H / 64;
    const int hole0_x = 18 * sx;
    const int hole0_y = 20 * sy;
    const int hole1_x = 45 * sx;
    const int hole1_y = 42 * sy;
    const int hole0_r2 = 120 * sx * sx;
    const int hole1_r2 = 150 * sx * sx;
    const int slash_width = 3 * sx;
    const int checker_size = 8 * sx;

    for (int y = 0; y < TEX_H; y++)
        {
        for (int x = 0; x < TEX_W; x++)
            {
            const int dx0 = x - hole0_x;
            const int dy0 = y - hole0_y;
            const int dx1 = x - hole1_x;
            const int dy1 = y - hole1_y;
            const bool hole0 = (dx0 * dx0 + dy0 * dy0) < hole0_r2;
            const bool hole1 = (dx1 * dx1 + dy1 * dy1) < hole1_r2;
            const bool slash = (x > 10 * sx) && (x < 56 * sx) && (abs((x - y) - 7 * sx) < slash_width);

            if (hole0 || hole1 || slash)
                {
                texture_pixels[x + y * TEX_W] = MASK_COLOR;
                }
            else
                {
                const bool checker = (((x / checker_size) ^ (y / checker_size)) & 1) != 0;
                texture_pixels[x + y * TEX_W] = checker ? RGB565(30, 47, 5) : RGB565(3, 38, 29);
                }
            }
        }
    }


void drawBackground(uint32_t t)
    {
    im.fillScreen(RGB565(1, 2, 4));
    const int shift = (int)((t / 40) % 32);
    for (int y = 0; y < SLY; y += 16)
        {
        const RGB565 c = ((y / 16) & 1) ? RGB565(2, 7, 12) : RGB565(3, 10, 16);
        im.drawFastHLine({ 0, y }, SLX, c);
        }
    for (int x = -shift; x < SLX; x += 32)
        {
        im.drawFastVLine({ x, 0 }, SLY, RGB565(2, 8, 13));
        }
    }


void drawObjectsBehindPanel(float a)
    {
    renderer.setShaders((Shader)(SHADER_GOURAUD | SHADER_NOTEXTURE));

    renderer.setMaterial(RGBf(1.00f, 0.08f, 0.04f), 0.16f, 0.82f, 0.45f, 20);
    fMat4 M;
    M.setScale(0.82f, 0.82f, 0.82f);
    M.multRotate(a * 1.2f, { 0, 1, 0 });
    M.multRotate(a * 0.7f, { 1, 0, 0 });
    M.multTranslate({ -0.85f + 0.28f * sinf(a * 0.020f), 0.0f, -6.1f });
    renderer.setModelMatrix(M);
    renderer.drawSphere(22, 12);

    renderer.setMaterial(RGBf(0.05f, 0.95f, 0.18f), 0.16f, 0.82f, 0.45f, 20);
    M.setScale(0.72f, 1.05f, 0.72f);
    M.multRotate(22.0f + 12.0f * sinf(a * 0.018f), { 1, 0, 0 });
    M.multRotate(18.0f * sinf(a * 0.014f), { 0, 0, 1 });
    M.multTranslate({ 0.95f + 0.25f * cosf(a * 0.018f),
                      -0.05f + 0.10f * sinf(a * 0.015f),
                      -6.4f + 0.20f * cosf(a * 0.012f) });
    renderer.setModelMatrix(M);
    renderer.drawCone(22, true);
    }


void drawTexturedPanel(bool mask_enabled, Shader quality)
    {
    fMat4 M;
    M.setIdentity();
    renderer.setModelMatrix(M);

    renderer.setShaders((Shader)(SHADER_UNLIT | SHADER_TEXTURE | quality | SHADER_TEXTURE_CLAMP |
                                 (mask_enabled ? SHADER_TEXTURE_MASK : SHADER_TEXTURE_NOMASK)));
    renderer.enableTextureMaskColor(mask_enabled);

    const fVec3 P0(-1.75f, -1.18f, -4.1f);
    const fVec3 P1( 1.75f, -1.18f, -4.1f);
    const fVec3 P2( 1.75f,  1.18f, -4.1f);
    const fVec3 P3(-1.75f,  1.18f, -4.1f);

    const fVec2 T0(0.0f, 0.0f);
    const fVec2 T1(1.0f, 0.0f);
    const fVec2 T2(1.0f, 1.0f);
    const fVec2 T3(0.0f, 1.0f);

    renderer.drawQuad(P0, P1, P2, P3,
                      nullptr, nullptr, nullptr, nullptr,
                      &T0, &T1, &T2, &T3,
                      &mask_texture);
    }


void setup()
    {
    Serial.begin(9600);
    telemetryBegin();

    buildMaskTexture();

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
    renderer.setPerspective(45.0f, ((float)SLX) / SLY, 1.0f, 100.0f);
    renderer.setCulling(0);
    renderer.setLight({ -0.35f, -0.95f, -1.0f },
                      RGBf(0.10f, 0.10f, 0.12f),
                      RGBf(0.95f, 0.93f, 0.86f),
                      RGBf(0.75f, 0.70f, 0.65f));
    renderer.setTextureWrappingMode(SHADER_TEXTURE_CLAMP);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureMaskColor(MASK_COLOR);
    }


void loop()
    {
    const uint32_t now = millis();
    const uint32_t phase = (now / 5000) % 3;
    const bool mask_enabled = (phase != 0);
    const Shader quality = (phase == 2) ? SHADER_TEXTURE_BILINEAR : SHADER_TEXTURE_NEAREST;
    const char* scene = (phase == 0) ? "mask_off_nearest" : ((phase == 1) ? "mask_on_nearest" : "mask_on_bilinear");
    static uint32_t prev_phase = 9999;

    if (phase != prev_phase)
        {
        prev_phase = phase;
        telemetrySetScene(scene);
        }

    telemetryStartFrame();

    drawBackground(now);
    renderer.clearZbuffer();
    const float a = now * 0.055f;
    drawObjectsBehindPanel(a);
    drawTexturedPanel(mask_enabled, quality);

    tft.overlayFPS(fb);

    telemetryEndFrame();
    tft.update(fb);
    }
