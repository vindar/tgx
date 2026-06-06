#include <Arduino.h>
#include <tgx.h>

static constexpr int W = 32;
static constexpr int H = 24;
static tgx::RGB565 framebuffer[W * H];

static void waitForSerial()
{
    const uint32_t start = millis();
    while (!Serial && (millis() - start < 5000)) {
        delay(10);
    }
    delay(200);
}

static uint32_t fnv1a(const uint8_t* data, size_t len)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

void setup()
{
    Serial.begin(9600);
    waitForSerial();
    Serial.println("TGX_RENDER_HASH_BEGIN");
    tgx::Image<tgx::RGB565> im(framebuffer, W, H);
    im.fillScreen(tgx::RGB565(0, 0, 0));
    im.fillRect(tgx::iBox2(3, 12, 4, 16), tgx::RGB565(20, 120, 210));
    im.drawLine(tgx::iVec2(0, 0), tgx::iVec2(W - 1, H - 1), tgx::RGB565(255, 60, 0));
    im.drawLine(tgx::iVec2(W - 1, 0), tgx::iVec2(0, H - 1), tgx::RGB565(0, 255, 80));
    for (int y = 0; y < H; ++y) {
        im.drawPixel(tgx::iVec2((y * 7) % W, y), tgx::RGB565(255, 255, 255));
    }
    uint32_t nonzero = 0;
    for (int i = 0; i < W * H; ++i) {
        if ((uint16_t)framebuffer[i] != 0) {
            ++nonzero;
        }
    }
    const uint32_t hash = fnv1a(reinterpret_cast<const uint8_t*>(framebuffer), sizeof(framebuffer));
    Serial.print("WIDTH=");
    Serial.println(W);
    Serial.print("HEIGHT=");
    Serial.println(H);
    Serial.print("NONZERO=");
    Serial.println(nonzero);
    Serial.print("HASH=0x");
    Serial.println(hash, HEX);
    Serial.println("TGX_RENDER_HASH_OK");
    Serial.println("TGX_RENDER_HASH_END");
}

void loop()
{
    delay(1000);
}

