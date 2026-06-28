struct SphereCase;

/*
 * TGX drawSphere() optimization benchmark.
 *
 * This sketch renders a fixed set of sphere cases into a RAM framebuffer and
 * prints serial timing rows. It deliberately does not upload pixels to a
 * display; the measured time is TGX rendering time only.
 */

#include <Arduino.h>
#include <tgx.h>

#if defined(ESP32)
    #include <esp_heap_caps.h>
#endif

using namespace tgx;


static const int FB_W = 160;
static const int FB_H = 120;
static const int TEX_W = 64;
static const int TEX_H = 64;
static const uint32_t CASE_TARGET_US = 650000;
static const int CASE_MAX_FRAMES = 240;


#if defined(TGX_BENCHMARK_T4)
    #define DEV_NAME "teensy41"
    uint16_t fb_storage[FB_W * FB_H];
    DMAMEM uint16_t zbuf_storage[FB_W * FB_H];
    uint16_t* fb = fb_storage;
    uint16_t* zbuf = zbuf_storage;
    void allocateBuffers() {}
#elif defined(TGX_BENCHMARK_ESP32)
    #define DEV_NAME "core2"
    uint16_t fb_storage[FB_W * FB_H];
    uint16_t* fb = fb_storage;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(FB_W * FB_H * sizeof(uint16_t), MALLOC_CAP_DMA);
        if (!zbuf) zbuf = (uint16_t*)malloc(FB_W * FB_H * sizeof(uint16_t));
        }
#elif defined(TGX_BENCHMARK_ESP32S3)
    #define DEV_NAME "cores3"
    uint16_t fb_storage[FB_W * FB_H];
    uint16_t* fb = fb_storage;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(FB_W * FB_H * sizeof(uint16_t), MALLOC_CAP_DMA);
        if (!zbuf) zbuf = (uint16_t*)malloc(FB_W * FB_H * sizeof(uint16_t));
        }
#elif defined(TGX_BENCHMARK_RP2350)
    #define DEV_NAME "pico2"
    uint16_t fb_storage[FB_W * FB_H];
    uint16_t zbuf_storage[FB_W * FB_H];
    uint16_t* fb = fb_storage;
    uint16_t* zbuf = zbuf_storage;
    void allocateBuffers() {}
#else
    #define DEV_NAME "generic"
    uint16_t fb_storage[FB_W * FB_H];
    uint16_t zbuf_storage[FB_W * FB_H];
    uint16_t* fb = fb_storage;
    uint16_t* zbuf = zbuf_storage;
    void allocateBuffers() {}
#endif


Image<RGB565> im;
RGB565 texture_data[TEX_W * TEX_H];
Image<RGB565> sphere_texture(texture_data, TEX_W, TEX_H);

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer;


enum ShadingMode
    {
    SHADE_FLAT,
    SHADE_GOURAUD
    };


enum TextureMode
    {
    TEX_NONE,
    TEX_CHECKER
    };


enum LightMode
    {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT
    };


struct SphereCase
    {
    const char* name;
    int sectors;
    int stacks;
    float scale;
    ShadingMode shading;
    TextureMode texture;
    LightMode light;
    bool specular;
    };


const SphereCase CASES[] =
    {
        { "small_low_flat_dir",          10,  6, 0.62f, SHADE_FLAT,    TEX_NONE,    LIGHT_DIRECTIONAL, false },
        { "small_low_gouraud_dir",       10,  6, 0.62f, SHADE_GOURAUD, TEX_NONE,    LIGHT_DIRECTIONAL, false },
        { "small_low_gouraud_tex_dir",   10,  6, 0.62f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_DIRECTIONAL, true  },
        { "medium_flat_tex_dir",         18, 10, 0.92f, SHADE_FLAT,    TEX_CHECKER, LIGHT_DIRECTIONAL, true  },
        { "medium_gouraud_point",        18, 10, 0.92f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_POINT,       false },
        { "medium_gouraud_point_spec",   18, 10, 0.92f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_POINT,       true  },
        { "high_small_gouraud_dir",      48, 24, 0.58f, SHADE_GOURAUD, TEX_NONE,    LIGHT_DIRECTIONAL, true  },
        { "high_small_gouraud_tex_dir",  48, 24, 0.58f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_DIRECTIONAL, true  },
        { "high_small_gouraud_point",    48, 24, 0.58f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_POINT,       true  },
        { "high_small_gouraud_spot",     48, 24, 0.58f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_SPOT,        true  },
        { "high_large_flat_tex_spot",    48, 24, 1.28f, SHADE_FLAT,    TEX_CHECKER, LIGHT_SPOT,        true  },
        { "high_large_gouraud_tex_spot", 48, 24, 1.28f, SHADE_GOURAUD, TEX_CHECKER, LIGHT_SPOT,        true  },
    };


static RGB565 colorFromBytes(int r, int g, int b)
    {
    return RGB565(r / 8, g / 4, b / 8);
    }


void buildTexture()
    {
    for (int y = 0; y < TEX_H; y++)
        {
        for (int x = 0; x < TEX_W; x++)
            {
            const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
            const bool stripe = ((x + 2 * y) % 17) < 3;
            if (stripe) texture_data[y * TEX_W + x] = colorFromBytes(235, 235, 245);
            else if (checker) texture_data[y * TEX_W + x] = colorFromBytes(34, 97, 205);
            else texture_data[y * TEX_W + x] = colorFromBytes(196, 149, 45);
            }
        }
    }


void setupRenderer()
    {
    im.set(fb, FB_W, FB_H);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setViewportSize(FB_W, FB_H);
    renderer.setOffset(0, 0);
    renderer.setPerspective(48.0f, ((float)FB_W) / FB_H, 1.0f, 80.0f);
    renderer.setLookAt({ 0.0f, 0.28f, 5.8f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    renderer.setLight({ -0.35f, -0.72f, -0.45f },
                      RGBf(0.08f, 0.08f, 0.08f),
                      RGBf(0.85f, 0.83f, 0.78f),
                      RGBf(0.80f, 0.78f, 0.72f));
    renderer.setSpotLightCount(1);
    renderer.setSpotLight(0, { 1.45f, 0.95f, 1.95f }, 5.2f,
                          RGBf(1.05f, 0.78f, 0.45f),
                          RGBf(0.90f, 0.72f, 0.50f));
    }


void configureCase(const SphereCase& c)
    {
    Shader shader = SHADER_PERSPECTIVE | SHADER_ZBUFFER;
    shader |= (c.shading == SHADE_GOURAUD) ? SHADER_GOURAUD : SHADER_FLAT;
    if (c.texture == TEX_CHECKER) shader |= SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;
    else shader |= SHADER_NOTEXTURE;
    renderer.setShaders(shader);

    if (c.light == LIGHT_DIRECTIONAL)
        {
        renderer.setSpotLightCount(0);
        renderer.setLight({ -0.35f, -0.72f, -0.45f },
                          RGBf(0.08f, 0.08f, 0.08f),
                          RGBf(0.88f, 0.86f, 0.82f),
                          RGBf(0.88f, 0.84f, 0.76f));
        }
    else if (c.light == LIGHT_POINT)
        {
        renderer.setSpotLightCount(1);
        renderer.setLight({ -0.35f, -0.72f, -0.45f },
                          RGBf(0.045f, 0.045f, 0.045f),
                          RGBf(0.16f, 0.15f, 0.14f),
                          RGBf(0.0f, 0.0f, 0.0f));
        renderer.setSpotLight(0, { 1.35f, 0.85f, 1.65f }, 4.8f,
                              RGBf(1.15f, 0.84f, 0.42f),
                              c.specular ? RGBf(0.82f, 0.70f, 0.54f) : RGBf(0.0f, 0.0f, 0.0f));
        }
    else
        {
        renderer.setSpotLightCount(1);
        renderer.setLight({ -0.35f, -0.72f, -0.45f },
                          RGBf(0.035f, 0.035f, 0.035f),
                          RGBf(0.10f, 0.10f, 0.10f),
                          RGBf(0.0f, 0.0f, 0.0f));
        renderer.setSpotLight(0, { 1.65f, 1.10f, 2.15f },
                              { -0.68f, -0.38f, -0.64f },
                              4.9f, 32.0f, 17.0f,
                              RGBf(1.10f, 0.76f, 0.42f),
                              c.specular ? RGBf(0.86f, 0.70f, 0.50f) : RGBf(0.0f, 0.0f, 0.0f));
        }

    const float spec = c.specular ? 0.82f : 0.0f;
    renderer.setMaterial(RGBf(0.82f, 0.70f, 0.48f), 0.11f, 0.86f, spec, 28);
    renderer.setModelPosScaleRot({ 0.0f, -0.04f, 0.0f }, { c.scale, c.scale, c.scale },
                                 24.0f, { 0.24f, 1.0f, 0.09f });
    }


void renderCase(const SphereCase& c)
    {
    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    configureCase(c);
    if (c.texture == TEX_CHECKER) renderer.drawSphere(c.sectors, c.stacks, &sphere_texture);
    else renderer.drawSphere(c.sectors, c.stacks);
    }


uint16_t frameHash()
    {
    uint32_t h = 2166136261u;
    for (int i = 0; i < FB_W * FB_H; i++)
        {
        h ^= fb[i];
        h *= 16777619u;
        }
    return (uint16_t)((h >> 16) ^ h);
    }


void benchCase(const SphereCase& c)
    {
    renderCase(c);
    const uint16_t hash = frameHash();

    uint32_t frames = 0;
    const uint32_t start = micros();
    uint32_t elapsed = 0;
    do
        {
        renderCase(c);
        frames++;
        elapsed = micros() - start;
        }
    while ((elapsed < CASE_TARGET_US) && (frames < CASE_MAX_FRAMES));

    const float avg_us = frames ? ((float)elapsed / frames) : 0.0f;
    const float fps = avg_us > 0.0f ? (1000000.0f / avg_us) : 0.0f;

    Serial.print("TGX_SPHERE_RESULT,board=");
    Serial.print(DEV_NAME);
    Serial.print(",case=");
    Serial.print(c.name);
    Serial.print(",sectors=");
    Serial.print(c.sectors);
    Serial.print(",stacks=");
    Serial.print(c.stacks);
    Serial.print(",scale=");
    Serial.print(c.scale, 3);
    Serial.print(",shading=");
    Serial.print((c.shading == SHADE_GOURAUD) ? "gouraud" : "flat");
    Serial.print(",texture=");
    Serial.print((c.texture == TEX_CHECKER) ? "checker" : "none");
    Serial.print(",light=");
    Serial.print((c.light == LIGHT_DIRECTIONAL) ? "directional" : ((c.light == LIGHT_POINT) ? "point" : "spot"));
    Serial.print(",specular=");
    Serial.print(c.specular ? "on" : "off");
    Serial.print(",frames=");
    Serial.print(frames);
    Serial.print(",avg_us=");
    Serial.print(avg_us, 2);
    Serial.print(",fps=");
    Serial.print(fps, 2);
    Serial.print(",hash=");
    Serial.println(hash);
    }


void runBench()
    {
    Serial.println("TGX_SPHERE_BENCH_BEGIN");
    Serial.print("TGX_SPHERE_INFO,board=");
    Serial.print(DEV_NAME);
    Serial.print(",fb=");
    Serial.print(FB_W);
    Serial.print("x");
    Serial.print(FB_H);
    Serial.print(",cases=");
    Serial.println((int)(sizeof(CASES) / sizeof(CASES[0])));

    for (unsigned int i = 0; i < sizeof(CASES) / sizeof(CASES[0]); i++)
        {
        benchCase(CASES[i]);
        delay(40);
        }

    Serial.println("TGX_SPHERE_BENCH_END");
    }


void setup()
    {
    Serial.begin(9600);
    const uint32_t start = millis();
    while ((!Serial) && (millis() - start < 3000)) {}
    delay(8000);

    allocateBuffers();
    if ((!fb) || (!zbuf))
        {
        while (true)
            {
            Serial.println("TGX_SPHERE_ERROR,allocation_failed");
            delay(2000);
            }
        }

    buildTexture();
    setupRenderer();
    runBench();
    }


void loop()
    {
    delay(1000);
    }
