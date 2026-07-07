/********************************************************************
* TGX example: headless texture and primitive test.
*
* This ESP-IDF example renders several textured 3D primitives into an
* off-screen framebuffer and reports telemetry on the serial port.
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
                                     SHADER_ZBUFFER |
                                     SHADER_FLAT |
                                     SHADER_GOURAUD |
                                     SHADER_NOTEXTURE |
                                     SHADER_TEXTURE |
                                     SHADER_TEXTURE_NEAREST |
                                     SHADER_TEXTURE_BILINEAR |
                                     SHADER_TEXTURE_WRAP_POW2;

Image<RGB565> im;
Image<RGB565> texture;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;
Telemetry telemetry;

RGB565* fb = nullptr;
uint16_t* zbuf = nullptr;
RGB565* texture_buf = nullptr;

void initializeTexture()
    {
    texture.set(texture_buf, TEX_SIZE, TEX_SIZE);
    for (int y = 0; y < TEX_SIZE; ++y)
        {
        for (int x = 0; x < TEX_SIZE; ++x)
            {
            const bool checker = (((x >> 4) ^ (y >> 4)) & 1) != 0;
            const bool stripe = ((x + y) & 15) < 3;
            RGB565 c = checker ? rgb(235, 235, 245) : rgb(35, 88, 190);
            if (stripe) c = checker ? rgb(255, 205, 80) : rgb(20, 200, 170);
            texture_buf[y * TEX_SIZE + x] = c;
            }
        }
    }

void setupRenderer()
    {
    im.set(fb, LX, LY);
    renderer.setViewportSize(LX, LY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45.0f, (float)LX / (float)LY, 1.0f, 100.0f);
    renderer.setCulling(1);
    renderer.setMaterial(RGBf(0.85f, 0.85f, 0.85f), 0.14f, 0.76f, 0.35f, 12);
    renderer.setLightDirection({ 0.35f, -0.45f, -0.9f });
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    }

void setCommonModel(uint32_t t, float scale, float z)
    {
    fMat4 M;
    M.setScale(scale, scale, scale);
    M.multRotate(t / 18.0f, { 0, 1, 0 });
    M.multRotate(t / 43.0f, { 1, 0, 0 });
    M.multTranslate({ 0, 0, z });
    renderer.setModelMatrix(M);
    }

const char* drawFrame()
    {
    const uint32_t t = millis();
    const int scene = (int)((t / 5000) & 3);

    im.fillScreen(rgb(5, 7, 14));
    renderer.clearZbuffer();

    if (scene == 0)
        {
        renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST);
        renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
        setCommonModel(t, 1.35f, -5.2f);
        renderer.drawSphere(28, 14, &texture);
        return "sphere_nearest";
        }

    if (scene == 1)
        {
        renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR);
        renderer.setTextureQuality(SHADER_TEXTURE_BILINEAR);
        setCommonModel(t, 1.35f, -5.2f);
        renderer.drawSphere(28, 14, &texture);
        return "sphere_bilinear";
        }

    if (scene == 2)
        {
        renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST);
        renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
        setCommonModel(t, 1.15f, -5.0f);
        renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);
        return "cube_flat_texture";
        }

    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    setCommonModel(t, 1.05f, -5.6f);
    renderer.drawTruncatedCone(34, 0.85f, 0.28f, &texture, &texture, &texture);
    return "truncated_cone_texture";
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
    telemetry.begin("test-texture");
    delayMs(300);

    if (!allocateBuffers())
        {
        printf("TGX_EXAMPLE_ERROR board=esp32p4 example=test-texture message=allocation_failed\n");
        fflush(stdout);
        while (true) delayMs(1000);
        }

    initializeTexture();
    setupRenderer();
    printf("TGX_EXAMPLE_READY board=esp32p4 example=test-texture width=%d height=%d texture=%d\n", LX, LY, TEX_SIZE);
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
