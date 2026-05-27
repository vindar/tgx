/********************************************************************
* tgx library example : animated textured Borg cube.
*
*              EXAMPLE FOR M5STACK Core2 and CoreS3
*
* This sketch renders a rotating cube with a texture that is modified in
* real time.  It draws into a TGX framebuffer, then sends that framebuffer to
* the display with LovyanGFX.
*
* Instructions:
*
* 1. download and install LovyanGFX via Arduino's library manager or directly
*    from: https://github.com/lovyan03/LovyanGFX
*
* 2. Select the board model and serial port and upload the sketch.
*
* ---
* This example was tested with M5Stack Core2 and M5Stack CoreS3.
********************************************************************/

/**************** DMA NOTE ****************
* LovyanGFX DMA transfers can improve display upload speed on M5Stack boards.
* DMA is enabled by default here.  The sketch first tries to allocate a
* DMA-capable transfer buffer; if that fails, it automatically falls back to
* the blocking pushImage() path.  Uncomment the line below to force that
* non-DMA path.
*******************************************/
// #define DISABLE_DMA

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

using namespace tgx;

// The sketch renders into a smaller off-screen framebuffer, then centers it
// on the physical display.  This keeps RAM use reasonable on Core2/CoreS3.
static const int MAX_LX = 240;
static const int MAX_LY = 180;

// A 64x64 texture is small, but it is enough for this procedural cube demo.
static const int TEX_SIZE = 64;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
uint16_t texture_buf[TEX_SIZE * TEX_SIZE];
bool use_dma = false;

Image<RGB565> im;
Image<RGB565> texture;

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;
bool perspective_mode = true;
uint32_t last_projection_switch_ms = 0;

LGFX lcd;

// Only the shader paths used by this sketch are compiled in.
const Shader LOADED_SHADERS = SHADER_ORTHO | SHADER_PERSPECTIVE |
                              SHADER_ZBUFFER | SHADER_FLAT |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


void initializeTexture()
    {
    texture.set(texture_buf, TEX_SIZE, TEX_SIZE);
    texture.fillScreen(rgb888(4, 18, 10));

    // A simple grid gives the cube a technological texture before the random
    // animated details are added.
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

    // Fade the texture slightly so old details disappear instead of turning
    // the whole cube into one uniform color.
    texture.fillRect({ 0, TEX_SIZE - 1, 0, TEX_SIZE - 1 }, rgb888(4, 18, 10), 0.05f);

    // Add a few colored panels.  The palette stays limited so the result keeps
    // a readable "circuit board" look instead of becoming pure noise.
    if ((frame_count & 1) == 0)
        {
        for (int i = 0; i < 2; i++)
            {
            int x = random(TEX_SIZE);
            int y = random(TEX_SIZE);
            int r = 2 + random(6);
            int choice = random(5);
            RGB565 c = RGB565_Cyan;
            if (choice == 0) c = rgb888(30, 180, 80);
            if (choice == 1) c = rgb888(190, 45, 35);
            if (choice == 2) c = rgb888(35, 90, 210);
            if (choice == 3) c = rgb888(220, 160, 35);
            if (choice == 4) c = rgb888(160, 80, 210);
            texture.fillRect({ x - r, x + r, y - r, y + r }, c, 0.62f);
            }
        }

    // Repaint one bright circuit trace.  These lines keep the texture structured
    // even while colors slowly change.
    int y = random(TEX_SIZE);
    texture.drawFastHLine({ 0, y }, TEX_SIZE, rgb888(80, 220, 130), 0.55f);
    int x = random(TEX_SIZE);
    texture.drawFastVLine({ x, 0 }, TEX_SIZE, rgb888(70, 150, 230), 0.45f);

    // Keep a bright border in the texture so the cube silhouette stays visible
    // even when a face is mostly dark.
    RGB565 edge_color = rgb888(120, 220, 230);
    texture.drawRect({ 0, TEX_SIZE - 1, 0, TEX_SIZE - 1 }, edge_color, 0.88f);
    texture.drawRect({ 1, TEX_SIZE - 2, 1, TEX_SIZE - 2 }, edge_color, 0.55f);
    }


void setupRenderer()
    {
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE);
    renderer.setPerspective(45, render_ratio, 1.0f, 100.0f);
    }


void updateProjection()
    {
    uint32_t now = millis();
    if (now - last_projection_switch_ms < 7000) return;

    last_projection_switch_ms = now;
    perspective_mode = !perspective_mode;

    if (perspective_mode)
        renderer.setPerspective(45, render_ratio, 1.0f, 100.0f);
    else
        // In orthographic mode there is no perspective shrinking.  The view
        // volume is widened with the screen ratio so the cube keeps its shape.
        renderer.setOrtho(-1.7f * render_ratio, 1.7f * render_ratio,
                          -1.7f, 1.7f, 1.0f, 100.0f);
    }


void drawFrame()
    {
    uint32_t t = millis();

    // Compose the model matrix by rotating the cube on several axes, then move
    // it in front of the camera.
    fMat4 M;
    M.setRotate(t / 13.0f, { 0, 1, 0 });
    M.multRotate(t / 27.0f, { 1, 0, 0 });
    M.multRotate(t / 43.0f, { 0, 0, 1 });
    M.multTranslate({ 0, 0, -5 });

    im.fillScreen(perspective_mode ? rgb888(2, 5, 10) : rgb888(26, 28, 32));
    renderer.clearZbuffer();
    renderer.setModelMatrix(M);
    renderer.drawCube(&texture, &texture, &texture, &texture, &texture, &texture);

    im.drawText(perspective_mode ? "Perspective" : "Orthographic",
                { 4, 13 }, font_tgx_OpenSans_Bold_10, rgb888(235, 190, 70));
    }


void updateFPS()
    {
    fps_frames++;
    uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("borg_cube M5Stack fps=");
        Serial.println(fps_frames);
        Serial.print("borg_cube M5Stack display=");
        Serial.println(use_dma ? "DMA" : "pushImage");
        fps_frames = 0;
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
        Serial.println("borg_cube M5Stack DMA buffer released to keep framebuffer/zbuffer");
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
        lcd.initDMA();
        lcd.startWrite();
        use_dma = true;
        Serial.println("borg_cube M5Stack display DMA enabled");
        }
    else
        {
        Serial.println("borg_cube M5Stack display DMA unavailable, using pushImage");
        }
#else
    Serial.println("borg_cube M5Stack display DMA disabled, using pushImage");
#endif

    return true;
    }


void copySwapBuffer()
    {
    const int n = render_lx * render_ly;
    for (int i = 0; i < n; i++)
        {
        const uint16_t a = fb[i];
        fb2[i] = (a << 8) | (a >> 8);
        }
    }


void pushFrame()
    {
    const int x = (lcd.width() - render_lx) / 2;
    const int y = (lcd.height() - render_ly) / 2;
    if (use_dma)
        {
        lcd.waitDMA();
        copySwapBuffer();
        lcd.pushImageDMA(x, y, render_lx, render_ly, fb2);
        }
    else
        {
        lcd.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    lcd.fillScreen(TFT_BLACK);

    render_lx = lcd.width();
    render_ly = lcd.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    size_t pixel_count = (size_t)render_lx * (size_t)render_ly;

    // Framebuffer and zbuffer are allocated at run time because their final
    // size depends on the actual M5Stack display detected by LovyanGFX.
    while (!allocateRenderBuffers(pixel_count))
        {
        Serial.println("Error: cannot allocate framebuffer/zbuffer");
        delay(1000);
        }
    lcd.setSwapBytes(!use_dma);

    im.set(fb, render_lx, render_ly);
    initializeTexture();
    setupRenderer();

    randomSeed(micros());
    fps_last_ms = millis();
    last_projection_switch_ms = millis();
    }


void loop()
    {
    updateProjection();
    updateTexture();
    drawFrame();
    pushFrame();
    updateFPS();
    }
