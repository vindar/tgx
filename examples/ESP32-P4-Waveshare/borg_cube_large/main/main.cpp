/********************************************************************
*
* TGX example for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B.
*
* Large tile-rendered version of borg_cube. TGX renders a 640x480
* virtual viewport in four 640x120 tiles, then copies each tile into
* the 720x720 MIPI-DSI framebuffer before presenting the full frame.
*
********************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "tgx_p4_display.h"

using namespace tgx;

namespace
{

static const char* TAG = "tgx_p4_borg_cube_large";

static constexpr int VIEW_W = 640;
static constexpr int VIEW_H = 480;
static constexpr int TILE_H = 120;
static constexpr int TEX_SIZE = 128;
static constexpr float RATIO = ((float)VIEW_W) / VIEW_H;

tgx_p4::Display display;

uint16_t* tile_fb = nullptr;
uint16_t* tile_zbuf = nullptr;
RGB565 texture_data[TEX_SIZE * TEX_SIZE];
Image<RGB565> tile_image;
Image<RGB565> texture(texture_data, TEX_SIZE, TEX_SIZE);

bool perspective_mode = true;

const Shader LOADED_SHADERS = SHADER_ORTHO | SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


int randInt(int limit)
    {
    if (limit <= 0) return 0;
    return (int)(esp_random() % (uint32_t)limit);
    }


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


void* allocPreferInternal(size_t bytes, const char* name)
    {
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (p)
        {
        ESP_LOGI(TAG, "%s allocated in internal RAM (%u bytes)", name, (unsigned)bytes);
        return p;
        }

    p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (p)
        {
        ESP_LOGW(TAG, "%s allocated in PSRAM (%u bytes)", name, (unsigned)bytes);
        return p;
        }

    ESP_LOGE(TAG, "%s allocation failed (%u bytes)", name, (unsigned)bytes);
    abort();
    }


void initializeTexture()
    {
    texture.fillScreen(rgb888(4, 18, 10));
    for (int i = 0; i < TEX_SIZE; i += 8)
        {
        texture.drawFastVLine({ i, 0 }, TEX_SIZE, rgb888(20, 90, 42));
        texture.drawFastHLine({ 0, i }, TEX_SIZE, rgb888(20, 90, 42));
        }
    }


void updateTexture()
    {
    static uint32_t frame_count = 0;
    frame_count++;

    texture.fillRect({ 0, TEX_SIZE - 1, 0, TEX_SIZE - 1 }, rgb888(4, 18, 10), 0.05f);

    if ((frame_count & 1) == 0)
        {
        for (int i = 0; i < 2; i++)
            {
            const int x = randInt(TEX_SIZE);
            const int y = randInt(TEX_SIZE);
            const int r = 2 + randInt(6);
            const int choice = randInt(5);
            RGB565 c = RGB565_Cyan;
            if (choice == 0) c = rgb888(30, 180, 80);
            if (choice == 1) c = rgb888(190, 45, 35);
            if (choice == 2) c = rgb888(35, 90, 210);
            if (choice == 3) c = rgb888(220, 160, 35);
            if (choice == 4) c = rgb888(160, 80, 210);
            texture.fillRect({ x - r, x + r, y - r, y + r }, c, 0.62f);
            }
        }

    texture.drawFastHLine({ 0, randInt(TEX_SIZE) }, TEX_SIZE, rgb888(80, 220, 130), 0.55f);
    texture.drawFastVLine({ randInt(TEX_SIZE), 0 }, TEX_SIZE, rgb888(70, 150, 230), 0.45f);
    const RGB565 edge_color = rgb888(120, 220, 230);
    texture.drawRect({ 0, TEX_SIZE - 1, 0, TEX_SIZE - 1 }, edge_color, 0.88f);
    texture.drawRect({ 1, TEX_SIZE - 2, 1, TEX_SIZE - 2 }, edge_color, 0.55f);
    }


int currentFPS()
    {
    static uint64_t last_ms = 0;
    static int frame_count = 0;
    static int fps = 0;

    frame_count++;
    const uint64_t now = tgx_p4::millis64();
    if (now - last_ms >= 1000)
        {
        fps = frame_count;
        frame_count = 0;
        last_ms = now;
        printf("[TGX telemetry] fps=%d\n", fps);
        }
    return fps;
    }


void drawTile(int tile_y, const fMat4& model, int fps)
    {
    renderer.setOffset(0, tile_y);
    tile_image.fillScreen(perspective_mode ? rgb888(2, 5, 10) : rgb888(26, 28, 32));
    renderer.clearZbuffer();
    renderer.setModelMatrix(model);
    renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);

    if (tile_y == 0)
        {
        char buf[48];
        snprintf(buf, sizeof(buf), "%d FPS", fps);
        tile_image.drawText("TGX large tile rendering", { 4, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
        tile_image.drawText(buf, { VIEW_W - 54, 14 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
        }
    }


void initRenderer()
    {
    tile_fb = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile framebuffer"));
    tile_zbuf = static_cast<uint16_t*>(allocPreferInternal(VIEW_W * TILE_H * sizeof(uint16_t), "TGX tile z-buffer"));

    tile_image.set(tile_fb, VIEW_W, TILE_H);
    renderer.setViewportSize(VIEW_W, VIEW_H);
    renderer.setImage(&tile_image);
    renderer.setZbuffer(tile_zbuf);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_BILINEAR);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE);
    renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);

    initializeTexture();
    printf("[TGX scene] borg_cube_large\n");
    }


void updateProjection()
    {
    static uint64_t last_ms = 0;
    const uint64_t now = tgx_p4::millis64();
    if (now - last_ms < 7000) return;

    last_ms = now;
    perspective_mode = !perspective_mode;
    if (perspective_mode)
        renderer.setPerspective(45.0f, RATIO, 1.0f, 100.0f);
    else
        renderer.setOrtho(-1.7f * RATIO, 1.7f * RATIO, -1.7f, 1.7f, 1.0f, 100.0f);
    }

} // namespace


extern "C" void app_main(void)
    {
    ESP_ERROR_CHECK(display.begin());
    initRenderer();

    while (true)
        {
        const uint64_t now = tgx_p4::millis64();
        updateProjection();
        updateTexture();

        fMat4 M;
        M.setRotate((float)now / 13.0f, { 0, 1, 0 });
        M.multRotate((float)now / 27.0f, { 1, 0, 0 });
        M.multRotate((float)now / 43.0f, { 0, 0, 1 });
        M.multTranslate({ 0, 0, -5 });

        const int fps = currentFPS();
        display.beginFrame(0x0014);
        const int ox = (display.width() - VIEW_W) / 2;
        const int oy = (display.height() - VIEW_H) / 2;
        for (int tile_y = 0; tile_y < VIEW_H; tile_y += TILE_H)
            {
            drawTile(tile_y, M, fps);
            display.copyTileRGB565(tile_fb, VIEW_W, TILE_H, ox, oy + tile_y);
            }
        display.presentFrame();
        }
    }
