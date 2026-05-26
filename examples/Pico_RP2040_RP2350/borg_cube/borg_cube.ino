/********************************************************************
* tgx library example : animated textured Borg cube.
*
*                    EXAMPLE FOR RP2040 / RP2350
*
* This sketch renders a rotating cube with a texture that is modified in
* real time.  It draws into a TGX framebuffer, then sends that framebuffer to
* the TFT screen with TFT_eSPI.
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
*
* ---
* This example was tested with:
*    - boards:  Raspberry Pico W (RP2040) and Raspberry Pico 2 (RP2350)
*    - screen:  ILI9341 (320x240)
********************************************************************/

#include <TFT_eSPI.h>

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

using namespace tgx;

// RP2040 RAM is tight once framebuffer + zbuffer + texture are all present.
// Pico 2 has more RAM, so it can render a larger viewport.
#if defined(ARDUINO_ARCH_RP2350) || defined(PICO_RP2350) || defined(__ARM_ARCH_8M_MAIN__)
static const int MAX_LX = 240;
static const int MAX_LY = 180;
#else
static const int MAX_LX = 160;
static const int MAX_LY = 120;
#endif

// A 64x64 texture is small, but it is enough for this procedural cube demo.
static const int TEX_SIZE = 64;

uint16_t fb[MAX_LX * MAX_LY];
uint16_t zbuf[MAX_LX * MAX_LY];
uint16_t texture_buf[TEX_SIZE * TEX_SIZE];

Image<RGB565> im;
Image<RGB565> texture;

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;
bool perspective_mode = true;
uint32_t last_projection_switch_ms = 0;

TFT_eSPI tft = TFT_eSPI();

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
        Serial.print("borg_cube Pico fps=");
        Serial.println(fps_frames);
        fps_frames = 0;
        fps_last_ms = now;
        }
    }


void setup()
    {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

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

    tft.pushImage((tft.width() - render_lx) / 2,
                  (tft.height() - render_ly) / 2,
                  render_lx, render_ly, fb);

    updateFPS();
    }
