/********************************************************************
* tgx library: PNGdec extension example for M5Stack Core2/CoreS3.
*
* Install PNGdec and LovyanGFX from Arduino's library manager.
********************************************************************/

#include <Arduino.h>
#include <esp_heap_caps.h>

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>
#include <PNGdec.h>

#include "octocat_4bpp.h"

using namespace tgx;

LGFX lcd;
PNG png;

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
    Serial.println("DECODER=PNG");
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

    const int openResult = png.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    int result = -1;
    int decodeCount = 0;
    uint32_t decodeUs = 0;

    if (openResult == PNG_SUCCESS)
        {
        const uint32_t startUs = micros();
        float op = 1.0f;
        for (int j = 0; j < 3; j++)
            {
            for (int i = 0; i < 4; i++)
                {
                const int x = i * 80 - 20;
                const int y = j * 100 - 30;
                result = im.PNGDecode(png, { x, y }, op);
                decodeCount++;
                op -= 1.0f / 13.0f;
                }
            }
        decodeUs = micros() - startUs;
        png.close();
        }

    lcd.pushImage(0, 0, screen_lx, screen_ly, fb);

    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    Serial.print("RESULT=");
    Serial.println(result);
    Serial.print("DECODE_COUNT=");
    Serial.println(decodeCount);
    Serial.print("SCREEN_W=");
    Serial.println(screen_lx);
    Serial.print("SCREEN_H=");
    Serial.println(screen_ly);
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
