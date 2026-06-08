/********************************************************************
* tgx library: AnimatedGIF extension example for M5Stack Core2/CoreS3.
*
* Install AnimatedGIF and LovyanGFX from Arduino's library manager.
********************************************************************/

#include <Arduino.h>
#include <esp_heap_caps.h>

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>
#include <AnimatedGIF.h>

#include "earth_128x128.h"

using namespace tgx;

LGFX lcd;
AnimatedGIF gif;

uint16_t* fb = nullptr;
Image<RGB565> im;
int screen_lx = 0;
int screen_ly = 0;
int gif_x = 0;
int gif_y = 0;
bool firstTelemetryPrinted = false;


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
    Serial.println("DECODER=GIF");
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
    gif_x = (screen_lx > 128) ? ((screen_lx - 128) / 2) : 0;
    gif_y = (screen_ly > 128) ? ((screen_ly - 128) / 2) : 0;

    if (!allocateFramebuffer())
        {
        Serial.println("RESULT=ALLOC_FAILED");
        Serial.println("TGX_EXTENSION_EXAMPLE_END");
        return;
        }

    im.clear(RGB565_Black);
    const int openResult = gif.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    }


void loop()
    {
    const uint32_t startUs = micros();
    int rc = im.GIFplayFrame(gif, { gif_x, gif_y });
    const uint32_t frameUs = micros() - startUs;

    lcd.pushImage(0, 0, screen_lx, screen_ly, fb);

    if (!firstTelemetryPrinted)
        {
        Serial.print("RESULT=");
        Serial.println(rc);
        Serial.print("SCREEN_W=");
        Serial.println(screen_lx);
        Serial.print("SCREEN_H=");
        Serial.println(screen_ly);
        Serial.print("DRAW_X=");
        Serial.println(gif_x);
        Serial.print("DRAW_Y=");
        Serial.println(gif_y);
        Serial.print("TIME_US=");
        Serial.println(frameUs);
        Serial.print("FB_HASH=");
        Serial.println(hashFramebuffer(), HEX);
        Serial.println("TGX_EXTENSION_EXAMPLE_END");
        Serial.flush();
        firstTelemetryPrinted = true;
        }

    if (!rc) gif.reset();
    delay(30);
    }
