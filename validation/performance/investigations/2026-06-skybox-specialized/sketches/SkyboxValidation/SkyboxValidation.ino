#include <ILI9341_T4.h>
#include <tgx.h>

using namespace tgx;

#define PIN_SCK     13
#define PIN_MISO    12
#define PIN_MOSI    11
#define PIN_DC      10
#define PIN_CS      9
#define PIN_RESET   6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255
#define SPI_SPEED   40000000

static const int SLX = 320;
static const int SLY = 240;
static const int TEX = 32;
static const int ITER = 80;

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
DMAMEM ILI9341_T4::DiffBuffStatic<10000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<10000> diff2;

uint16_t fb[SLX * SLY];
uint16_t internal_fb[SLX * SLY];
DMAMEM uint16_t ref_fb[SLX * SLY];
DMAMEM uint16_t zbuf[SLX * SLY];

RGB565 tex_data[6][TEX * TEX];
Image<RGB565> tex[6] = {
    Image<RGB565>(tex_data[0], TEX, TEX),
    Image<RGB565>(tex_data[1], TEX, TEX),
    Image<RGB565>(tex_data[2], TEX, TEX),
    Image<RGB565>(tex_data[3], TEX, TEX),
    Image<RGB565>(tex_data[4], TEX, TEX),
    Image<RGB565>(tex_data[5], TEX, TEX)
};

Image<RGB565> im(fb, SLX, SLY);

const Shader LOADED_SHADER = SHADER_ZBUFFER | SHADER_PERSPECTIVE |
                             SHADER_UNLIT | SHADER_FLAT |
                             SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR |
                             SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADER, uint16_t> renderer;

static RGB565 makeColor(int face, int x, int y)
    {
    const uint8_t r = uint8_t((face * 37 + x * 5 + y * 3) & 255);
    const uint8_t g = uint8_t((face * 61 + x * 2 + y * 7) & 255);
    const uint8_t b = uint8_t((face * 83 + x * 11 + y) & 255);
    return RGB565(RGB32(r, g, b));
    }

static void fillTextures()
    {
    for (int face = 0; face < 6; face++)
        {
        for (int y = 0; y < TEX; y++)
            {
            for (int x = 0; x < TEX; x++)
                {
                tex[face](x, y) = makeColor(face, x, y);
                }
            }
        }
    }

static uint64_t hashFrame(const uint16_t* data)
    {
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < SLX * SLY; i++)
        {
        const uint16_t v = data[i];
        h ^= uint8_t(v & 255);
        h *= 1099511628211ull;
        h ^= uint8_t(v >> 8);
        h *= 1099511628211ull;
        }
    return h;
    }

static void configureRenderer()
    {
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(55.0f, ((float)SLX) / SLY, 1.0f, 100.0f);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setCulling(0);
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f }, { 50.0f, 50.0f, 50.0f }, 180.0f, { 0.0f, 1.0f, 0.0f });
    }

static void clearFrame()
    {
    memset(fb, 0, sizeof(fb));
    memset(zbuf, 0, sizeof(zbuf));
    }

static void drawOld()
    {
    renderer.setShaders(SHADER_UNLIT | SHADER_TEXTURE_NEAREST);
    renderer.drawCube(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);
    }

static void drawNew()
    {
    renderer.drawSkyBox(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);
    }

static uint32_t timeOld()
    {
    uint32_t total = 0;
    for (int i = 0; i < ITER; i++)
        {
        clearFrame();
        const uint32_t t0 = micros();
        drawOld();
        total += micros() - t0;
        }
    return total / ITER;
    }

static uint32_t timeNew()
    {
    uint32_t total = 0;
    for (int i = 0; i < ITER; i++)
        {
        clearFrame();
        const uint32_t t0 = micros();
        drawNew();
        total += micros() - t0;
        }
    return total / ITER;
    }

void setup()
    {
    Serial.begin(9600);
    while (!Serial && millis() < 2500) {}

    Serial.println("TGX_SKYBOX_VALIDATION_BEGIN");
    Serial.println("BOARD=Teensy41");

    while (!tft.begin(SPI_SPEED)) {}
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);
    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(5);
    tft.setRefreshRate(60);

    fillTextures();
    configureRenderer();

    clearFrame();
    drawOld();
    memcpy(ref_fb, fb, sizeof(fb));
    const uint64_t old_hash = hashFrame(ref_fb);

    clearFrame();
    drawNew();
    const uint64_t new_hash = hashFrame(fb);

    int mismatches = 0;
    for (int i = 0; i < SLX * SLY; i++)
        {
        if (ref_fb[i] != fb[i]) mismatches++;
        }

    const uint32_t old_us = timeOld();
    const uint32_t new_us = timeNew();

    clearFrame();
    drawNew();
    tft.update(fb, true);

    Serial.print("HASH_OLD=");
    Serial.println((uint32_t)(old_hash ^ (old_hash >> 32)));
    Serial.print("HASH_NEW=");
    Serial.println((uint32_t)(new_hash ^ (new_hash >> 32)));
    Serial.print("MISMATCHES=");
    Serial.println(mismatches);
    Serial.print("TIME_OLD_US=");
    Serial.println(old_us);
    Serial.print("TIME_NEW_US=");
    Serial.println(new_us);
    Serial.print("SPEEDUP_PCT=");
    Serial.println(((100.0f * old_us) / new_us) - 100.0f, 2);
    Serial.println("TGX_SKYBOX_VALIDATION_END");
    }

void loop()
    {
    delay(1000);
    }
