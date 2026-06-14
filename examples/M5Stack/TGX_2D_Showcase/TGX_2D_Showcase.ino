/********************************************************************
* tgx library example : 2D drawing showcase.
*
*              EXAMPLE FOR M5STACK Core2 and CoreS3
*
* This sketch draws everything into a TGX framebuffer, then sends the
* framebuffer to the display with LovyanGFX.  The small sprite is generated
* at startup with TGX drawing commands.
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

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

using namespace tgx;

static const int MAX_LX = 320;
static const int MAX_LY = 240;
static const int SPRITE_LX = 56;
static const int SPRITE_LY = 56;

uint16_t* fb = nullptr;
uint16_t sprite_buf[SPRITE_LX * SPRITE_LY];

Image<RGB565> im;
Image<RGB565> sprite;

int screen_lx = 0;
int screen_ly = 0;

LGFX lcd;

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;
uint32_t fps_value = 0;
uint32_t fps_render_sum_us = 0;


RGB565 rgb888(int r, int g, int b)
    {
    // RGB565(int,int,int) expects 5/6/5-bit components.  This helper accepts
    // the usual 0..255 RGB values, which is more convenient in an example.
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


RGB565 colorWheel(int value)
    {
    // Return a bright color that smoothly cycles through red, green and blue.
    value = value % 768;
    if (value < 0) value += 768;

    if (value < 256) return rgb888(255 - value, value, 40);
    if (value < 512) return rgb888(40, 511 - value, value - 256);
    return rgb888(value - 512, 40, 767 - value);
    }


RGB565 softColorWheel(int value)
    {
    // A softer palette for the background.  Keeping a little of every color
    // channel avoids harsh transitions and nearly flat single-color screens.
    RGB565 c = colorWheel(value);
    int r8 = (c.R * 255) / 31;
    int g8 = (c.G * 255) / 63;
    int b8 = (c.B * 255) / 31;
    return rgb888(18 + (r8 * 90) / 255,
                  22 + (g8 * 72) / 255,
                  38 + (b8 * 92) / 255);
    }


void buildSprite()
    {
    // The sprite is just another TGX image backed by its own small buffer.
    // Drawing into it is exactly like drawing into the main framebuffer.
    sprite.set(sprite_buf, SPRITE_LX, SPRITE_LY);
    sprite.fillScreen(RGB565_Magenta);
    sprite.fillCircleAA({ 28.0f, 28.0f }, 24.0f, rgb888(250, 210, 35));
    sprite.drawCircleAA({ 28.0f, 28.0f }, 24.0f, RGB565_White, 0.65f);
    sprite.fillCircleAA({ 20.0f, 22.0f }, 4.0f, RGB565_Black);
    sprite.fillCircleAA({ 36.0f, 22.0f }, 4.0f, RGB565_Black);
    sprite.drawThickLineAA({ 18.0f, 38.0f }, { 38.0f, 38.0f }, 4.0f,
                           END_ROUNDED, END_ROUNDED, RGB565_Black);
    }


void drawBackground(uint32_t t)
    {
    // Draw the animated background first.  Everything else is drawn on top of
    // it, so this also clears the previous frame.
    int phase = (int)(t / 23);
    RGB565 left = softColorWheel(phase);
    RGB565 right = softColorWheel(phase + 260);

    im.fillScreenHGradient(left, right);

    for (int x = 0; x < screen_lx; x += 20)
        im.drawFastVLine({ x, 0 }, screen_ly, RGB565_White, 0.14f);
    for (int y = 0; y < screen_ly; y += 20)
        im.drawFastHLine({ 0, y }, screen_lx, RGB565_White, 0.14f);
    }


void drawHeader(uint32_t t)
    {
    // A small translucent header with text.  The title is shortened on small
    // displays so it does not collide with the FPS counter.
    im.fillRoundRectAA({ 6.0f, (float)screen_lx - 6.0f, 6.0f, 32.0f },
                       7.0f, rgb888(8, 12, 28), 0.78f);
    im.drawRoundRectAA({ 6.0f, (float)screen_lx - 6.0f, 6.0f, 32.0f },
                       7.0f, RGB565_White, 0.25f);
    if (screen_lx >= 230)
        im.drawText("TGX 2D showcase", { 14, 25 }, font_tgx_OpenSans_Bold_14,
                    RGB565_White);
    else
        im.drawText("TGX 2D", { 14, 25 }, font_tgx_OpenSans_Bold_14,
                    RGB565_White);

    char buf[24];
    snprintf(buf, sizeof(buf), "%lu fps", (unsigned long)fps_value);
    im.drawTextEx(buf, { screen_lx - 12, 25 }, font_tgx_OpenSans_Bold_10,
                  Anchor::BOTTOMRIGHT, false, false, RGB565_Cyan);

    float x = 20.0f + ((screen_lx - 40.0f) * ((t % 2400) / 2400.0f));
    im.fillCircleAA({ x, 32.0f }, 3.0f, RGB565_Yellow);
    }


void drawVectorShapes(uint32_t t)
    {
    // This central group demonstrates anti-aliased polygons, thick lines,
    // wedge lines, circles and ellipses.
    int cx = screen_lx / 2;
    int cy = 44 + (screen_ly - 78) / 2;
    int radius = (screen_lx < screen_ly ? screen_lx : screen_ly) / 4;
    if (radius < 28) radius = 28;

    float a = t * 0.055f;
    float ca = cosf(a * 0.0174532925f);
    float sa = sinf(a * 0.0174532925f);

    fVec2 poly[6];
    for (int i = 0; i < 6; i++)
        {
        float ang = (a + i * 60.0f) * 0.0174532925f;
        float r = radius * (0.75f + 0.12f * sinf((t * 0.004f) + i));
        poly[i] = { cx + r * cosf(ang), cy + r * sinf(ang) };
        }
    im.fillPolygonAA(6, poly, rgb888(40, 90, 170), 0.42f);
    im.drawThickPolygonAA(6, poly, 3.0f, RGB565_Cyan, 0.80f);

    im.drawThickLineAA({ cx - radius * ca, cy - radius * sa },
                       { cx + radius * ca, cy + radius * sa },
                       5.0f, END_ROUNDED, END_ARROW_1, RGB565_Orange, 0.75f);
    im.drawWedgeLineAA({ cx - radius * sa, cy + radius * ca },
                       { cx + radius * sa, cy - radius * ca },
                       2.0f, END_ROUNDED, 8.0f, END_ROUNDED,
                       RGB565_Lime, 0.70f);

    im.fillCircleAA({ (float)cx, (float)cy }, radius * 0.28f,
                    RGB565_Yellow, 0.35f);
    im.drawCircleAA({ (float)cx, (float)cy }, radius * 0.55f,
                    RGB565_White, 0.45f);
    im.drawEllipseAA({ (float)cx, (float)cy },
                     { radius * 0.95f, radius * 0.42f },
                     RGB565_Magenta, 0.55f);
    }


void drawPieDemo(uint32_t t)
    {
    // A small animated pie/arc display fills the right side of the screen and
    // demonstrates TGX circle-sector primitives.
    float cx = screen_lx * 0.78f;
    float cy = screen_ly * 0.48f;
    float r = (screen_lx < screen_ly ? screen_lx : screen_ly) * 0.145f;
    if (r < 22.0f) r = 22.0f;

    float phase = t * 0.035f;
    float sweep = 85.0f + 55.0f * sinf(t * 0.0025f);

    im.fillCircleAA({ cx, cy }, r + 6.0f, rgb888(4, 10, 22), 0.46f);
    im.fillCircleSectorAA({ cx, cy }, r, phase, phase + sweep,
                          RGB565_Orange, 0.82f);
    im.fillCircleSectorAA({ cx, cy }, r * 0.82f, phase + 125.0f,
                          phase + 225.0f, RGB565_Cyan, 0.64f);
    im.fillThickCircleSectorAA({ cx, cy }, r + 8.0f, phase + 210.0f,
                               phase + 335.0f, 6.0f,
                               rgb888(20, 180, 90), RGB565_White, 0.72f);
    im.drawCircleArcAA({ cx, cy }, r + 12.0f, phase + 20.0f,
                       phase + 300.0f, RGB565_White, 0.45f);
    im.fillCircleAA({ cx, cy }, r * 0.24f, rgb888(8, 12, 28), 0.95f);
    im.drawTextEx("pies", { (int)cx, (int)(cy + 4.0f) },
                  font_tgx_OpenSans_Bold_9, Anchor::CENTER, false, false,
                  RGB565_White);
    }


void drawWavePanel(uint32_t t)
    {
    // A sub-image is a view into a rectangular part of another image.  Drawing
    // on 'panel' below modifies the corresponding pixels in 'im'.
    int panel_h = 34;
    int y0 = screen_ly - panel_h - 4;
    if (y0 < 44) return;

    Image<RGB565> panel = im(6, screen_lx - 6, y0, screen_ly - 5);
    panel.fillScreenHGradient(rgb888(12, 18, 36), rgb888(0, 44, 48));

    fVec2 pts[12];
    int w = panel.lx();
    int h = panel.ly();
    for (int i = 0; i < 12; i++)
        {
        float x = 6.0f + i * ((w - 12.0f) / 11.0f);
        float y = h * 0.5f + sinf(t * 0.006f + i * 0.72f) * (h * 0.30f);
        pts[i] = { x, y };
        }
    panel.drawThickPolylineAA(12, pts, 3.0f, END_ROUNDED, END_ROUNDED,
                              RGB565_Yellow, 0.90f);
    panel.drawText("sub-images, text, AA primitives, sprites",
                   { 8, h - 7 }, font_tgx_OpenSans_Bold_9, RGB565_White);
    }


void drawSprite(uint32_t t)
    {
    // Blit the sprite with scaling, rotation and a transparent key color.
    float x = screen_lx * 0.20f + screen_lx * 0.11f * sinf(t * 0.0032f);
    float y = screen_ly * 0.48f + screen_ly * 0.17f * cosf(t * 0.0027f);
    float scale = 0.75f + 0.20f * sinf(t * 0.004f);
    float angle = (t * 0.090f);

    im.blitScaledRotatedMasked(sprite, RGB565_Magenta,
                               { SPRITE_LX / 2.0f, SPRITE_LY / 2.0f },
                               { x, y }, scale, angle);
    }


void drawFrame()
    {
    // The order matters: background first, then shapes, then UI on top.
    uint32_t t = millis();

    drawBackground(t);
    drawVectorShapes(t);
    drawPieDemo(t);
    drawSprite(t);
    drawWavePanel(t);
    drawHeader(t);
    }


void updateFPS(uint32_t render_us)
    {
    // The FPS counter is intentionally simple: one line per second on Serial.
    fps_frames++;
    fps_render_sum_us += render_us;
    uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        fps_value = fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0;
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;

        Serial.print("TGX_2D_Showcase M5Stack fps=");
        Serial.println(fps_value);
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

    screen_lx = lcd.width();
    screen_ly = lcd.height();
    if (screen_lx > MAX_LX) screen_lx = MAX_LX;
    if (screen_ly > MAX_LY) screen_ly = MAX_LY;

    // Allocate the framebuffer after the display is initialized.  On Core2 this
    // usually uses PSRAM; on CoreS3 it may use internal RAM.  Either way, TGX
    // simply receives a pointer to the memory buffer.
    fb = (uint16_t*)heap_caps_malloc(screen_lx * screen_ly * sizeof(uint16_t),
                                     MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (fb == nullptr)
        fb = (uint16_t*)heap_caps_malloc(screen_lx * screen_ly * sizeof(uint16_t),
                                         MALLOC_CAP_8BIT);
    while (fb == nullptr)
        {
        Serial.println("Error: cannot allocate framebuffer");
        delay(1000);
        }

    im.set(fb, screen_lx, screen_ly);
    buildSprite();

    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawFrame();
    const uint32_t render_us = micros() - render_start_us;

    // Blocking upload.  This keeps the example short and avoids a second
    // full-size framebuffer.
    lcd.pushImage((lcd.width() - screen_lx) / 2,
                  (lcd.height() - screen_ly) / 2,
                  screen_lx, screen_ly, fb);

    updateFPS(render_us);
    }
