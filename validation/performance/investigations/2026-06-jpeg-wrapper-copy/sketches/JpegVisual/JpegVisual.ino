#include <Arduino.h>
#include <tgx.h>
#include <JPEGDEC.h>

#include "../../../../../examples/Teensy4/2D/JPEG_test/batman.h"

#if defined(ARDUINO_TEENSY41)
    #include <ILI9341_T4.h>
#elif defined(ESP32)
    #include <esp_heap_caps.h>
    #define LGFX_AUTODETECT
    #include <LovyanGFX.hpp>
#else
    #include <TFT_eSPI.h>
#endif

using namespace tgx;

static const int FB_W = 320;
static const int FB_H = 240;
static const int JPEG_X = 40;
static const int JPEG_Y = 0;

static uint16_t* fb = nullptr;
static JPEGDEC jpeg;
static int displayW = FB_W;
static int displayH = FB_H;
static int pushX = 0;
static int pushY = 0;
#if !defined(ARDUINO_TEENSY41) && !defined(ESP32)
static bool useDma = false;
#endif

#if defined(ARDUINO_TEENSY41)
static const int PIN_SCK = 13;
static const int PIN_MISO = 12;
static const int PIN_MOSI = 11;
static const int PIN_DC = 10;
static const int PIN_CS = 9;
static const int PIN_RESET = 6;
static const int PIN_BACKLIGHT = 255;
static const int PIN_TOUCH_IRQ = 255;
static const int PIN_TOUCH_CS = 255;
static const int SPI_SPEED = 30000000;
static ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
#elif defined(ESP32)
static LGFX lcd;
#else
static TFT_eSPI tft = TFT_eSPI();
#endif

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

void waitForCaptureKick()
    {
    Serial.begin(115200);
    const uint32_t serialStart = millis();
    while (!Serial && (millis() - serialStart < 3000)) {}

    delay(9000);
    while (Serial.available() > 0) Serial.read();

    const uint32_t kickStart = millis();
    while ((millis() - kickStart) < 15000)
        {
        if (Serial.available() > 0)
            {
            while (Serial.available() > 0) Serial.read();
            break;
            }
        delay(10);
        }
    }

void initDisplay()
    {
#if defined(ARDUINO_TEENSY41)
    tft.output(&Serial);
    while (!tft.begin(SPI_SPEED)) {}
    tft.setRotation(3);
    displayW = FB_W;
    displayH = FB_H;
    pushX = 0;
    pushY = 0;
    if (PIN_BACKLIGHT != 255)
        {
        pinMode(PIN_BACKLIGHT, OUTPUT);
        digitalWrite(PIN_BACKLIGHT, HIGH);
        }
#elif defined(ESP32)
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    lcd.fillScreen(0);
    displayW = lcd.width();
    displayH = lcd.height();
    pushX = (displayW > FB_W) ? ((displayW - FB_W) / 2) : 0;
    pushY = (displayH > FB_H) ? ((displayH - FB_H) / 2) : 0;
#else
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(0);
    displayW = tft.width();
    displayH = tft.height();
    pushX = (displayW > FB_W) ? ((displayW - FB_W) / 2) : 0;
    pushY = (displayH > FB_H) ? ((displayH - FB_H) / 2) : 0;
    useDma = tft.initDMA();
    if (useDma) tft.startWrite();
#endif
    }

void pushFramebuffer()
    {
#if defined(ARDUINO_TEENSY41)
    tft.update(fb);
#elif defined(ESP32)
    lcd.pushImage(pushX, pushY, FB_W, FB_H, fb);
#else
    if (useDma)
        {
        tft.pushImageDMA(pushX, pushY, FB_W, FB_H, fb);
        tft.dmaWait();
        }
    else
        {
        tft.pushImage(pushX, pushY, FB_W, FB_H, fb);
        }
#endif
    }

void setup()
    {
    waitForCaptureKick();

    Serial.println("TGX_JPEG_VISUAL_BEGIN");
    Serial.print("BOARD=");
    Serial.println(boardName());
    Serial.print("SKETCH=");
    Serial.println("JpegVisual");

    fb = allocateFramebuffer();
    if (!fb)
        {
        Serial.println("RESULT=ALLOC_FAILED");
        Serial.println("IMAGE_W=0");
        Serial.println("IMAGE_H=0");
        Serial.println("TIME_US=0");
        Serial.println("FB_HASH=0");
        Serial.println("TGX_JPEG_VISUAL_END");
        return;
        }

    Image<RGB565> im(fb, FB_W, FB_H);
    im.clear(RGB565_Black);
    initDisplay();

    const int openResult = jpeg.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    uint32_t decodeUs = 0;
    int decodeResult = -1000;
    if (openResult)
        {
        const uint32_t startUs = micros();
        decodeResult = im.JPEGDecode(jpeg, { JPEG_X, JPEG_Y }, 0, 1.0f);
        decodeUs = micros() - startUs;
        jpeg.close();
        }

    pushFramebuffer();

    Serial.print("RESULT=");
    Serial.println(decodeResult);
    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    Serial.print("IMAGE_W=");
    Serial.println(FB_W);
    Serial.print("IMAGE_H=");
    Serial.println(FB_H);
    Serial.print("DRAW_X=");
    Serial.println(JPEG_X);
    Serial.print("DRAW_Y=");
    Serial.println(JPEG_Y);
    Serial.print("DISPLAY_W=");
    Serial.println(displayW);
    Serial.print("DISPLAY_H=");
    Serial.println(displayH);
    Serial.print("PUSH_X=");
    Serial.println(pushX);
    Serial.print("PUSH_Y=");
    Serial.println(pushY);
#if !defined(ARDUINO_TEENSY41) && !defined(ESP32)
    Serial.print("DISPLAY_MODE=");
    Serial.println(useDma ? "DMA" : "pushImage");
#endif
    Serial.print("TIME_US=");
    Serial.println(decodeUs);
    Serial.print("FB_HASH=");
    Serial.println(hashFramebuffer(fb, FB_W * FB_H), HEX);
    Serial.print("NONZERO=");
    Serial.println(countNonZero(fb, FB_W * FB_H));
    Serial.println("TGX_JPEG_VISUAL_END");
    }

void loop()
    {
    delay(1000);
    }
