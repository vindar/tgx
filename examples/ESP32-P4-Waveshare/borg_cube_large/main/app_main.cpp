/********************************************************************
* TGX example: headless animated textured Borg cube.
*
* This ESP-IDF example renders into an off-screen TGX framebuffer and
* reports telemetry on the serial port. It does not require a display
* driver, so it is useful as a first ESP32-P4/Waveshare smoke test.
********************************************************************/

#include "tgx_p4_example.h"

using namespace tgx;
using namespace tgx_p4_example;

namespace
{

static const int LX = 320;
static const int LY = 240;
static const int TEX_SIZE = 128;

static const Shader LOADED_SHADERS = SHADER_PERSPECTIVE |
                                     SHADER_ORTHO |
                                     SHADER_ZBUFFER |
                                     SHADER_FLAT |
                                     SHADER_TEXTURE |
                                     SHADER_TEXTURE_NEAREST |
                                     SHADER_TEXTURE_WRAP_POW2;

Image<RGB565> im;
Image<RGB565> texture;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;
Telemetry telemetry;

RGB565* fb = nullptr;
uint16_t* zbuf = nullptr;
RGB565* texture_buf = nullptr;

uint32_t rng_state = 0x31415926u;
bool perspective_mode = true;
uint32_t last_projection_switch_ms = 0;

uint32_t nextRandom()
    {
    rng_state = 1664525u * rng_state + 1013904223u;
    return rng_state;
    }

void initializeTexture()
    {
    texture.set(texture_buf, TEX_SIZE, TEX_SIZE);

    for (int y = 0; y < TEX_SIZE; ++y)
        {
        for (int x = 0; x < TEX_SIZE; ++x)
            {
            const bool grid = ((x & 15) == 0) || ((y & 15) == 0);
            texture_buf[y * TEX_SIZE + x] = grid ? rgb(18, 95, 54) : rgb(3, 19, 12);
            }
        }

    for (int i = 0; i < TEX_SIZE; ++i)
        {
        texture_buf[i] = rgb(90, 190, 210);
        texture_buf[(TEX_SIZE - 1) * TEX_SIZE + i] = rgb(90, 190, 210);
        texture_buf[i * TEX_SIZE] = rgb(90, 190, 210);
        texture_buf[i * TEX_SIZE + TEX_SIZE - 1] = rgb(90, 190, 210);
        }
    }

void updateTexture()
    {
    for (int i = 0; i < TEX_SIZE * TEX_SIZE; ++i)
        {
        RGB565 c = texture_buf[i];
        const int r = (int)c.R;
        const int g = (int)c.G;
        const int b = (int)c.B;
        texture_buf[i] = RGB565((r * 31) >> 5, (g * 31) >> 5, (b * 31) >> 5);
        }

    for (int n = 0; n < 12; ++n)
        {
        const int x = (int)(nextRandom() & (TEX_SIZE - 1));
        const int y = (int)(nextRandom() & (TEX_SIZE - 1));
        const int radius = 2 + (int)(nextRandom() & 7);
        const int choice = (int)(nextRandom() % 5);
        RGB565 c = rgb(70, 220, 130);
        if (choice == 1) c = rgb(220, 80, 55);
        if (choice == 2) c = rgb(80, 140, 230);
        if (choice == 3) c = rgb(230, 180, 55);
        if (choice == 4) c = rgb(150, 90, 220);

        for (int yy = y - radius; yy <= y + radius; ++yy)
            {
            if ((yy < 1) || (yy >= TEX_SIZE - 1)) continue;
            for (int xx = x - radius; xx <= x + radius; ++xx)
                {
                if ((xx < 1) || (xx >= TEX_SIZE - 1)) continue;
                texture_buf[yy * TEX_SIZE + xx] = c;
                }
            }
        }

    const int line_y = (int)(nextRandom() & (TEX_SIZE - 1));
    const int line_x = (int)(nextRandom() & (TEX_SIZE - 1));
    for (int i = 0; i < TEX_SIZE; ++i)
        {
        texture_buf[line_y * TEX_SIZE + i] = rgb(100, 240, 160);
        texture_buf[i * TEX_SIZE + line_x] = rgb(70, 180, 250);
        }
    }

void setupRenderer()
    {
    im.set(fb, LX, LY);
    renderer.setViewportSize(LX, LY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE);
    renderer.setPerspective(45.0f, (float)LX / (float)LY, 1.0f, 100.0f);
    }

void updateProjection(uint32_t now_ms)
    {
    if (now_ms - last_projection_switch_ms < 7000) return;
    last_projection_switch_ms = now_ms;
    perspective_mode = !perspective_mode;

    if (perspective_mode)
        {
        renderer.setPerspective(45.0f, (float)LX / (float)LY, 1.0f, 100.0f);
        }
    else
        {
        renderer.setOrtho(-1.9f * (float)LX / (float)LY, 1.9f * (float)LX / (float)LY,
                          -1.9f, 1.9f, 1.0f, 100.0f);
        }
    }

const char* drawFrame()
    {
    const uint32_t t = millis();
    updateProjection(t);
    updateTexture();

    im.fillScreen(perspective_mode ? rgb(2, 5, 10) : rgb(24, 25, 31));
    renderer.clearZbuffer();

    fMat4 M;
    M.setRotate(t / 13.0f, { 0, 1, 0 });
    M.multRotate(t / 27.0f, { 1, 0, 0 });
    M.multRotate(t / 43.0f, { 0, 0, 1 });
    M.multTranslate({ 0, 0, -5 });
    renderer.setModelMatrix(M);
    renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);

    return perspective_mode ? "cube_perspective" : "cube_ortho";
    }

bool allocateBuffers()
    {
    fb = allocBuffer<RGB565>(LX * LY);
    zbuf = allocBuffer<uint16_t>(LX * LY);
    texture_buf = allocBuffer<RGB565>(TEX_SIZE * TEX_SIZE);
    return fb && zbuf && texture_buf;
    }

} // namespace

extern "C" void app_main(void)
    {
    telemetry.begin("borg_cube_large");
    delayMs(300);

    if (!allocateBuffers())
        {
        printf("TGX_EXAMPLE_ERROR board=esp32p4 example=borg_cube_large message=allocation_failed\n");
        fflush(stdout);
        while (true) delayMs(1000);
        }

    initializeTexture();
    setupRenderer();
    printf("TGX_EXAMPLE_READY board=esp32p4 example=borg_cube_large width=%d height=%d texture=%d\n", LX, LY, TEX_SIZE);
    fflush(stdout);

    while (true)
        {
        const uint32_t start = micros();
        const char* scene = drawFrame();
        const uint32_t render_us = micros() - start;
        telemetry.frame(scene, render_us, framebufferChecksum(fb, LX * LY));
        delayMs(1);
        }
    }
