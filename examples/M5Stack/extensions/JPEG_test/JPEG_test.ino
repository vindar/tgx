/********************************************************************
* tgx library: JPEGDEC extension example for M5Stack Core2/CoreS3.
*
* Install JPEGDEC and LovyanGFX from Arduino's library manager.
********************************************************************/

#include <Arduino.h>
#include <esp_heap_caps.h>

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>
#include <JPEGDEC.h>

#include "batman.h"

using namespace tgx;

LGFX lcd;
JPEGDEC jpeg;

uint16_t* fb = nullptr;
Image<RGB565> im;
int screen_lx = 0;
int screen_ly = 0;


const char* boardName()
    {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    return "CoreS3";
#else
    return "Core2";
#endif
    }


uint32_t hashFramebuffer()
    {
    uint32_t h = 2166136261u;
    const int n = screen_lx * screen_ly;
    const int step = (n > 4096) ? (n / 4096) : 1;
    for (int i = 0; i < n; i += step)
        {
        const uint16_t v = fb[i];
        h ^= (uint8_t)(v & 0xffu);
        h *= 16777619u;
        h ^= (uint8_t)(v >> 8);
        h *= 16777619u;
        }
    return h;
    }


bool allocateFramebuffer()
    {
    const size_t bytes = (size_t)screen_lx * (size_t)screen_ly * sizeof(uint16_t);
    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (fb == nullptr) fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    if (fb == nullptr) return false;
    im.set(fb, screen_lx, screen_ly);
    return true;
    }


void pushFramebuffer()
    {
    lcd.pushImage(0, 0, screen_lx, screen_ly, fb);
    }


void setup()
    {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 3000;
    while (!Serial && millis() < serialDeadline) delay(10);
#if defined(ESP32)
    delay(5000);
#else
    delay(100);
#endif

    Serial.println("TGX_EXTENSION_EXAMPLE_BEGIN");
    Serial.println("DECODER=JPEG");
    Serial.print("BOARD=");
    Serial.println(boardName());
    Serial.flush();

    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    lcd.fillScreen(TFT_BLACK);

    screen_lx = lcd.width();
    screen_ly = lcd.height();

    if (!allocateFramebuffer())
        {
        Serial.println("RESULT=ALLOC_FAILED");
        Serial.println("TGX_EXTENSION_EXAMPLE_END");
        return;
        }

    im.fillScreenHGradient(RGB565_Blue, RGB565_Red);

    const int openResult = jpeg.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    int decodeResult = -1;
    uint32_t decodeUs = 0;
    int drawX = 0;
    int drawY = 0;

    if (openResult)
        {
        const int drawW = jpeg.getWidth() / 2;
        const int drawH = jpeg.getHeight() / 2;
        drawX = (screen_lx > drawW) ? ((screen_lx - drawW) / 2) : 0;
        drawY = (screen_ly > drawH) ? ((screen_ly - drawH) / 2) : 0;

        const uint32_t startUs = micros();
        decodeResult = im.JPEGDecode(jpeg, { drawX, drawY }, JPEG_SCALE_HALF, 0.5f);
        decodeUs = micros() - startUs;
        jpeg.close();
        }

    pushFramebuffer();

    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    Serial.print("RESULT=");
    Serial.println(decodeResult);
    Serial.print("SCREEN_W=");
    Serial.println(screen_lx);
    Serial.print("SCREEN_H=");
    Serial.println(screen_ly);
    Serial.print("DRAW_X=");
    Serial.println(drawX);
    Serial.print("DRAW_Y=");
    Serial.println(drawY);
    Serial.print("TIME_US=");
    Serial.println(decodeUs);
    Serial.print("FB_HASH=");
    Serial.println(hashFramebuffer(), HEX);
    Serial.println("TGX_EXTENSION_EXAMPLE_END");
    Serial.flush();
    }


void loop()
    {
    delay(1000);
    }
