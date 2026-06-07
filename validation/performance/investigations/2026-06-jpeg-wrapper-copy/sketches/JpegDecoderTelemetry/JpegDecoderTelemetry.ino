#include <Arduino.h>
#include <tgx.h>
#include <JPEGDEC.h>

#include "../../../../../examples/Teensy4/2D/extensions/JPEG_test/batman.h"

#if defined(ESP32)
    #include <esp_heap_caps.h>
#endif

using namespace tgx;

static const int FB_W = 320;
static const int FB_H = 240;
static uint16_t* fb = nullptr;
static JPEGDEC jpeg;

const char* boardName()
    {
#if defined(ARDUINO_TEENSY41)
    return "Teensy41";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    return "CoreS3";
#elif defined(ESP32)
    return "Core2";
#elif defined(ARDUINO_ARCH_RP2040) && defined(__ARM_ARCH_8M_MAIN__)
    return "Pico2";
#elif defined(ARDUINO_ARCH_RP2040)
    return "PicoRP2040";
#else
    return "Unknown";
#endif
    }

uint16_t* allocateFramebuffer()
    {
#if defined(ESP32)
    void* p = heap_caps_malloc((size_t)FB_W * FB_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = heap_caps_malloc((size_t)FB_W * FB_H * sizeof(uint16_t), MALLOC_CAP_8BIT);
    return (uint16_t*)p;
#else
    return (uint16_t*)malloc((size_t)FB_W * FB_H * sizeof(uint16_t));
#endif
    }

uint32_t hashFramebuffer(const uint16_t* p, int count)
    {
    uint32_t h = 2166136261u;
    for (int i = 0; i < count; i++)
        {
        const uint16_t v = p[i];
        h ^= (uint8_t)(v & 0xffu);
        h *= 16777619u;
        h ^= (uint8_t)(v >> 8);
        h *= 16777619u;
        }
    return h;
    }

int countNonZero(const uint16_t* p, int count)
    {
    int n = 0;
    for (int i = 0; i < count; i++)
        {
        if (p[i] != 0) n++;
        }
    return n;
    }

void setup()
    {
    Serial.begin(115200);
    const uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 3000)) {}
    delay(9000);
    while (Serial.available() > 0) Serial.read();
    const uint32_t kickStart = millis();
    while ((millis() - kickStart) < 15000)
        {
        if (Serial.available() > 0)
            {
            Serial.read();
            break;
            }
        delay(10);
        }

    Serial.println("TGX_DECODER_EXAMPLE_BEGIN");
    Serial.println("DECODER=JPEG");
    Serial.print("BOARD=");
    Serial.println(boardName());

    fb = allocateFramebuffer();
    if (!fb)
        {
        Serial.println("RESULT=ALLOC_FAILED");
        Serial.println("TGX_DECODER_EXAMPLE_END");
        return;
        }

    Image<RGB565> im(fb, FB_W, FB_H);
    im.clear(RGB565_Black);

    const int openResult = jpeg.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    uint32_t decodeUs = 0;
    int decodeResult = 1000;
    if (openResult)
        {
        const uint32_t t1 = micros();
        decodeResult = im.JPEGDecode(jpeg, { 0, 0 }, 0, 1.0f);
        decodeUs = micros() - t1;
        jpeg.close();
        }

    Serial.print("RESULT=");
    Serial.println(decodeResult);
    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    Serial.print("TIME_US=");
    Serial.println(decodeUs);
    Serial.print("FB_HASH=");
    Serial.println(hashFramebuffer(fb, FB_W * FB_H), HEX);
    Serial.print("NONZERO=");
    Serial.println(countNonZero(fb, FB_W * FB_H));
    Serial.print("IMAGE_W=");
    Serial.println(FB_W);
    Serial.print("IMAGE_H=");
    Serial.println(FB_H);
    Serial.println("TGX_DECODER_EXAMPLE_END");
    }

void loop()
    {
    delay(1000);
    }
