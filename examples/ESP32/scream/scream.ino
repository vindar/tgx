/********************************************************************
* tgx library example : dynamic 3D geometry.
*
*                    EXAMPLE FOR ESP32 / ESP32-S2 / ESP32-S3
*
* This sketch draws a textured sheet and changes its vertices every frame.
* It is a compact version of the Teensy4 scream example, tuned for TFT_eSPI
* displays and smaller ESP32 boards.
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
* older TFT_eSPI / Espressif ESP32 core combinations have had DMA issues.
* Use a recent TFT_eSPI release and keep the Espressif ESP32 board package up
* to date.  DMA is enabled by default here; if the DMA buffer cannot be
* allocated, the sketch automatically falls back to the blocking pushImage()
* path.  Uncomment the line below to force that non-DMA path.
*******************************************/
// #define DISABLE_DMA

#include <TFT_eSPI.h>

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "scream_texture.h"

using namespace tgx;

// Render into an off-screen TGX image, then copy that image to the display.
// MAX_LX/MAX_LY are only upper limits: on a smaller TFT, the actual render
// size is clamped to the screen size at run time.
static const int MAX_LX = 200;
static const int MAX_LY = 150;

// The sheet is a regular grid.  More cells gives smoother deformation, but
// also more vertices, normals and quads to update every frame.
static const int N = 22;
static const int M = 22;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

Image<RGB565> im;
TFT_eSPI tft = TFT_eSPI();

// Only the shader paths used below are compiled in.  The sheet is textured and
// lit with Gouraud shading, and needs a zbuffer because it folds in 3D.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

const uint16_t SCREEN_BORDER_COLOR = TFT_DARKGREY;

fVec3 vertices[(N + 1) * (M + 1)];
fVec3 normals[(N + 1) * (M + 1)];
fVec2 texcoords[(N + 1) * (M + 1)];

// One triangle strip covers the whole grid.  Degenerate triangles connect
// adjacent rows without drawing visible geometry.
static const int STRIP_INDEX_COUNT = 2 * (N + 1) + (M - 1) * (2 * (N + 1) + 2);
uint16_t strip[STRIP_INDEX_COUNT];

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;
uint32_t fps_render_sum_us = 0;
uint32_t explosion_start_ms = 0;
uint32_t explosion_cycle = 0;

fVec2 explosion_center(0, 0);
float explosion_h = 0.34f;
float explosion_w = 10.0f;
float explosion_s = 0.44f;
float explosion_delay = 1300.0f;


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


float randFloat(float a, float b)
    {
    return a + (b - a) * (random(1000000) / 1000000.0f);
    }


float sinc(float x)
    {
    if (fabsf(x) < 0.01f) return 1.0f;
    return sinf(x) / x;
    }


void initSheet()
    {
    const float dx = 2.0f / N;
    const float dy = 2.0f / M;

    // The grid lives in the XZ plane.  The Y coordinate is updated every frame
    // to create the explosion wave.
    for (int j = 0; j <= M; j++)
        {
        for (int i = 0; i <= N; i++)
            {
            int k = (N + 1) * j + i;
            vertices[k] = fVec3(i * dx - 1.0f, 0.0f, j * dy - 1.0f);
            texcoords[k] = fVec2((float)i / N, (float)j / M);
            }
        }

    int k = 0;
    for (int j = 0; j < M; j++)
        {
        if (j > 0)
            {
            const uint16_t first = (uint16_t)(j * (N + 1));
            strip[k] = strip[k - 1]; k++; // degenerate connector
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
            int a = (N + 1) * j + i;
            int b = (N + 1) * (j + 1) + i;
            int c = (N + 1) * (j + 1) + i + 1;
            int d = (N + 1) * j + i + 1;
            addFaceNormal(a, b, d);
            addFaceNormal(b, c, d);
            }
        }

    for (int k = 0; k < (N + 1) * (M + 1); k++) normals[k].normalize();
    }


void startExplosion(bool first)
    {
    explosion_start_ms = millis();
    explosion_cycle++;

    if (first)
        {
        explosion_center = fVec2(0.0f, 0.0f);
        explosion_h = 0.34f;
        explosion_w = 10.0f;
        explosion_s = 0.44f;
        explosion_delay = 1300.0f;
        }
    else
        {
        explosion_center = fVec2(randFloat(-0.65f, 0.65f), randFloat(-0.65f, 0.65f));
        explosion_h = randFloat(0.15f, 0.32f);
        explosion_w = randFloat(5.0f, 15.0f);
        explosion_s = randFloat(0.35f, 1.5f);
        explosion_delay = 0.0f;
        }

    Serial.print("[scream] explosion ");
    Serial.println(explosion_cycle);
    }


void updateSheet()
    {
    float t = (float)(millis() - explosion_start_ms);
    const float t0 = explosion_delay;
    const float t1 = explosion_delay + 1800.0f;
    const float t2 = explosion_delay + 4800.0f;

    if (t > t2)
        {
        startExplosion(false);
        t = 0.0f;
        }

    float alpha = 0.0f;
    if (t > t0 && t < t2)
        alpha = (t < t1) ? ((t - t0) / (t1 - t0)) : (1.0f - (t - t1) / (t2 - t1));

    const float dx = 2.0f / N;
    const float dy = 2.0f / M;
    const float wave_time = (t > t1) ? (t - t1) : 0.0f;
    const float wave = wave_time * explosion_s / 1000.0f;

    for (int j = 0; j <= M; j++)
        {
        for (int i = 0; i <= N; i++)
            {
            const float x = dx * i - 1.0f - explosion_center.x;
            const float y = dy * j - 1.0f - explosion_center.y;
            const float r = sqrtf(x * x + y * y);
            vertices[(N + 1) * j + i].y = alpha * explosion_h * sinc(explosion_w * (r - wave));
            }
        }

    updateNormals();
    }


fVec3 cameraPosition()
    {
    const float t = millis() * 0.001f;
    const float d = 1.75f + 0.20f * sinf(t * 0.35f);
    const float w = t * 0.18f + 1.9f;
    return fVec3(d * sinf(w), 1.12f, d * cosf(w));
    }


void drawFrame()
    {
    updateSheet();

    im.fillScreen(rgb888(3, 5, 8));
    renderer.clearZbuffer();

    // The camera slowly orbits around the sheet.  The model matrix stays
    // identity because the sheet vertices are already in model space.
    renderer.setLookAt(cameraPosition(), { 0, 0, 0 }, { 0, 1, 0 });
    renderer.drawTriangleStrip(STRIP_INDEX_COUNT, strip, vertices, strip, normals, strip, texcoords, &scream_texture);

    }


void updateFPS(uint32_t render_us)
    {
    fps_frames++;
    fps_render_sum_us += render_us;
    uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("scream ESP32 fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        Serial.print("scream ESP32 display=");
        Serial.println(use_dma ? "DMA" : "pushImage");
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;
        }
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


bool allocateRenderBuffers(size_t pixel_count)
    {
    const size_t bytes = pixel_count * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    // DMA needs a contiguous DMA-capable internal RAM block.  Allocate it
    // first, then fall back cleanly if keeping it prevents fb/zbuf allocation.
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
        Serial.println("scream ESP32 DMA buffer released to keep framebuffer/zbuffer");
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
        Serial.println("scream ESP32 display DMA enabled");
        }
    else
        {
        Serial.println("scream ESP32 display DMA unavailable, using pushImage");
        }
#else
    Serial.println("scream ESP32 display DMA disabled, using pushImage");
#endif

    return true;
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


void drawScreenLabel()
    {
    if (use_dma) tft.dmaWait();
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, SCREEN_BORDER_COLOR, true);
    tft.drawString("Dynamic geometry", 0, 0);
    }


void setup()
    {
    Serial.begin(115200);

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

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(SCREEN_BORDER_COLOR);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    const int available_y = (tft.height() > 20) ? (tft.height() - 20) : tft.height();
    if (render_ly > available_y) render_ly = available_y;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    size_t pixel_count = (size_t)render_lx * (size_t)render_ly;

    // The final viewport depends on the physical TFT size, so allocate the
    // framebuffer/zbuffer once that size is known.
    while (!allocateRenderBuffers(pixel_count))
        {
        Serial.println("Error: cannot allocate framebuffer/zbuffer");
        delay(1000);
        }
    drawScreenLabel();

    im.set(fb, render_lx, render_ly);
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, render_ratio, 0.1f, 50.0f);
    renderer.setCulling(0);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE);

    fMat4 M0;
    M0.setIdentity();
    renderer.setModelMatrix(M0);

    initSheet();
    randomSeed(0);
    startExplosion(true);
    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawFrame();
    const uint32_t render_us = micros() - render_start_us;
    pushFrame();
    updateFPS(render_us);
    }
