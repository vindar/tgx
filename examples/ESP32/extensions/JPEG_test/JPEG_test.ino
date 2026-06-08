/********************************************************************
* tgx library: JPEGDEC extension example for ESP32-family TFT_eSPI boards.
*
* Install JPEGDEC and TFT_eSPI from Arduino's library manager. Configure
* TFT_eSPI/User_Setup_Select.h for the connected board and screen.
********************************************************************/

#include <Arduino.h>
#if defined(ESP32)
#include <FS.h>
#endif
#include <TFT_eSPI.h>
#include <tgx.h>
#include <JPEGDEC.h>

#if defined(ESP32)
#include <esp_heap_caps.h>
#endif

#include "batman.h"

using namespace tgx;

TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;

uint16_t* fb = nullptr;
Image<RGB565> im;
int screen_lx = 0;
int screen_ly = 0;


const char* boardName()
    {
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    return "ESP32-S3";
#elif defined(ESP32)
    return "ESP32";
#else
    return "TFT_eSPI";
#endif
    }


void enableBacklight()
    {
#if defined(TFT_I2C_POWER)
    pinMode(TFT_I2C_POWER, OUTPUT);
    digitalWrite(TFT_I2C_POWER, HIGH);
    delay(10);
#endif
#if defined(TFT_BACKLITE)
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
#endif
#if defined(TFT_BL)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
    }


bool allocateFramebuffer()
    {
    const size_t bytes = (size_t)screen_lx * (size_t)screen_ly * sizeof(uint16_t);
#if defined(ESP32)
    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (fb == nullptr) fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
#else
    fb = (uint16_t*)malloc(bytes);
#endif
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
    Serial.println("DECODER=JPEG");
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

    tft.pushImage(0, 0, screen_lx, screen_ly, fb);

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
