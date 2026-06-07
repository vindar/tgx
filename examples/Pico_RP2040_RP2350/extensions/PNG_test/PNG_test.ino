/********************************************************************
* tgx library: PNGdec extension example for RP2040/RP2350 TFT_eSPI
* boards.
*
* Install PNGdec and TFT_eSPI from Arduino's library manager. Configure
* TFT_eSPI/User_Setup_Select.h for the connected board and screen.
********************************************************************/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <tgx.h>
#include <PNGdec.h>

#include "octocat_4bpp.h"

using namespace tgx;

TFT_eSPI tft = TFT_eSPI();
PNG png;

uint16_t* fb = nullptr;
Image<RGB565> im;
int screen_lx = 0;
int screen_ly = 0;


const char* boardName()
    {
#if defined(ARDUINO_ARCH_RP2040) && defined(__ARM_ARCH_8M_MAIN__)
    return "Pico2";
#elif defined(ARDUINO_ARCH_RP2040)
    return "Pico";
#else
    return "RP2040/RP2350";
#endif
    }


void enableBacklight()
    {
#if defined(TFT_BL)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
    }


bool allocateFramebuffer()
    {
    const size_t bytes = (size_t)screen_lx * (size_t)screen_ly * sizeof(uint16_t);
    fb = (uint16_t*)malloc(bytes);
    if (fb == nullptr) return false;
    im.set(fb, screen_lx, screen_ly);
    return true;
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

    enableBacklight();
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    screen_lx = tft.width();
    screen_ly = tft.height();

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

    tft.pushImage(0, 0, screen_lx, screen_ly, fb);

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
    static uint32_t lastTelemetry = 0;
    if (millis() - lastTelemetry >= 2000)
        {
        lastTelemetry = millis();
        Serial.println("TGX_EXTENSION_EXAMPLE_BEGIN");
        Serial.println("DECODER=PNG");
        Serial.print("BOARD=");
        Serial.println(boardName());
        Serial.println("REPEAT=1");
        Serial.print("SCREEN_W=");
        Serial.println(screen_lx);
        Serial.print("SCREEN_H=");
        Serial.println(screen_ly);
        Serial.print("FB_HASH=");
        Serial.println(fb ? hashFramebuffer() : 0, HEX);
        Serial.println("TGX_EXTENSION_EXAMPLE_END");
        Serial.flush();
        }
    delay(1000);
    }
