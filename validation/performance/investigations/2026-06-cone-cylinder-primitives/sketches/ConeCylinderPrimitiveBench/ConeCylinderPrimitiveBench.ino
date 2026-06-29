#include <Arduino.h>
#include <tgx.h>

using namespace tgx;

static const int W = 160;
static const int H = 120;
static const int SIDE_TEX_W = 64;
static const int SIDE_TEX_H = 32;
static const int CAP_TEX_W = 48;
static const int CAP_TEX_H = 48;
static const int FRAMES_PER_CASE = 40;

static RGB565 fb[W * H];
static uint16_t zbuf[W * H];
static RGB565 sideTextureData[SIDE_TEX_W * SIDE_TEX_H];
static RGB565 bottomTextureData[CAP_TEX_W * CAP_TEX_H];
static RGB565 topTextureData[CAP_TEX_W * CAP_TEX_H];

static Image<RGB565> im(fb, W, H);
static Image<RGB565> sideTexture(sideTextureData, SIDE_TEX_W, SIDE_TEX_H);
static Image<RGB565> bottomTexture(bottomTextureData, CAP_TEX_W, CAP_TEX_H);
static Image<RGB565> topTexture(topTextureData, CAP_TEX_W, CAP_TEX_H);

static const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                                     SHADER_FLAT | SHADER_GOURAUD |
                                     SHADER_NOTEXTURE | SHADER_TEXTURE |
                                     SHADER_TEXTURE_AFFINE |
                                     SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP;

static Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer({ W, H }, &im, zbuf);

enum ShapeKind
    {
    SHAPE_CYLINDER,
    SHAPE_CONE,
    SHAPE_TRUNCATED
    };

struct BenchCase
    {
    const char* name;
    ShapeKind shape;
    int sectors;
    float bottomRadius;
    float topRadius;
    bool bottomCap;
    bool topCap;
    bool gouraud;
    bool texture;
    bool pointLight;
    float scale;
    };

static const BenchCase CASES[] =
    {
    { "cylinder_gouraud_closed", SHAPE_CYLINDER, 32, 1.0f, 1.0f, true, true, true, false, false, 1.0f },
    { "cylinder_textured_all", SHAPE_CYLINDER, 40, 1.0f, 1.0f, true, true, true, true, false, 1.0f },
    { "cylinder_open_side_tex", SHAPE_CYLINDER, 40, 1.0f, 1.0f, false, false, false, true, false, 1.0f },
    { "cone_gouraud_textured_point", SHAPE_CONE, 40, 1.0f, 0.0f, true, true, true, true, true, 1.0f },
    { "truncated_gouraud_textured_point", SHAPE_TRUNCATED, 48, 1.0f, 0.45f, true, true, true, true, true, 1.0f },
    { "truncated_flat_textured", SHAPE_TRUNCATED, 16, 1.0f, 0.55f, true, true, false, true, false, 1.05f },
    };


static RGB565 colorFromBytes(int r, int g, int b)
    {
    return RGB565(r / 8, g / 4, b / 8);
    }


static void buildTextures()
    {
    for (int y = 0; y < SIDE_TEX_H; y++)
        {
        for (int x = 0; x < SIDE_TEX_W; x++)
            {
            const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
            const bool seam = (x < 2) || (x > SIDE_TEX_W - 3);
            if (seam) sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(245, 245, 245);
            else if (checker) sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(40, 120, 220);
            else sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(220, 162, 42);
            }
        }

    for (int y = 0; y < CAP_TEX_H; y++)
        {
        for (int x = 0; x < CAP_TEX_W; x++)
            {
            const bool cross = (abs(x - CAP_TEX_W / 2) < 2) || (abs(y - CAP_TEX_H / 2) < 2);
            const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
            if (cross)
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(245, 245, 245);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(245, 245, 245);
                }
            else if (checker)
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(185, 55, 70);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(55, 172, 128);
                }
            else
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(95, 35, 48);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(22, 90, 82);
                }
            }
        }
    }


static void configureRenderer()
    {
    renderer.setViewportSize(W, H);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(47.0f, ((float)W) / H, 1.0f, 80.0f);
    renderer.setLookAt({ 3.1f, 1.9f, 5.2f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    renderer.setCulling(1);
    renderer.setLight({ -0.35f, -0.78f, -0.52f },
                      RGBf(0.10f, 0.10f, 0.11f),
                      RGBf(0.82f, 0.82f, 0.78f),
                      RGBf(0.45f, 0.42f, 0.36f));
    renderer.setSpotLightCount(0);
    renderer.setMaterial(RGBf(0.78f, 0.70f, 0.58f), 0.12f, 0.88f, 0.35f, 24);
    }


static void configureCase(const BenchCase& c, float angle)
    {
    Shader shader = SHADER_PERSPECTIVE | SHADER_ZBUFFER;
    shader |= c.gouraud ? SHADER_GOURAUD : SHADER_FLAT;
    shader |= c.texture ? (SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP) : SHADER_NOTEXTURE;
    renderer.setShaders(shader);
    renderer.setCulling(1);

    if (c.pointLight)
        {
        renderer.setSpotLightCount(1);
        renderer.setLight({ -0.35f, -0.78f, -0.52f },
                          RGBf(0.05f, 0.05f, 0.05f),
                          RGBf(0.16f, 0.15f, 0.13f),
                          RGBf(0.0f, 0.0f, 0.0f));
        renderer.setSpotLight(0, { 1.8f, 1.0f, 2.3f }, 4.8f,
                              RGBf(0.95f, 0.70f, 0.42f),
                              RGBf(0.45f, 0.36f, 0.28f));
        }
    else
        {
        renderer.setSpotLightCount(0);
        renderer.setLight({ -0.35f, -0.78f, -0.52f },
                          RGBf(0.10f, 0.10f, 0.11f),
                          RGBf(0.82f, 0.82f, 0.78f),
                          RGBf(0.45f, 0.42f, 0.36f));
        }

    renderer.setModelPosScaleRot({ 0.0f, -0.04f, 0.0f },
                                 { c.scale, c.scale, c.scale },
                                 angle, { 0.18f, 1.0f, 0.08f });
    }


static void drawCase(const BenchCase& c)
    {
    if (!c.texture)
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawCylinder(c.sectors, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawCone(c.sectors, c.bottomCap);
        else renderer.drawTruncatedCone(c.sectors, c.bottomRadius, c.topRadius, c.bottomCap, c.topCap);
        }
    else
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawCylinder(c.sectors, &sideTexture, &bottomTexture, &topTexture, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawCone(c.sectors, &sideTexture, &bottomTexture, c.bottomCap);
        else renderer.drawTruncatedCone(c.sectors, c.bottomRadius, c.topRadius, &sideTexture, &bottomTexture, &topTexture, c.bottomCap, c.topCap);
        }
    }


static void runCase(const BenchCase& c, int cycle)
    {
    uint32_t minUs = 0xffffffffu;
    uint32_t maxUs = 0;
    uint64_t totalUs = 0;

    for (int frame = 0; frame < FRAMES_PER_CASE; frame++)
        {
        const float angle = 360.0f * ((float)frame / FRAMES_PER_CASE) + cycle * 11.0f;
        const uint32_t start = micros();
        im.fillScreen(RGB565_Black);
        renderer.clearZbuffer();
        configureCase(c, angle);
        drawCase(c);
        const uint32_t dt = micros() - start;
        if (dt < minUs) minUs = dt;
        if (dt > maxUs) maxUs = dt;
        totalUs += dt;
        }

    const float avgUs = (float)totalUs / FRAMES_PER_CASE;
    const float fps = 1000000.0f / avgUs;
    Serial.print("[TGX telemetry] cycle=");
    Serial.print(cycle);
    Serial.print(" scene=");
    Serial.print(c.name);
    Serial.print(" fps=");
    Serial.print(fps, 2);
    Serial.print(" frame_avg_us=");
    Serial.print(avgUs, 1);
    Serial.print(" frame_min_us=");
    Serial.print(minUs);
    Serial.print(" frame_max_us=");
    Serial.print(maxUs);
    Serial.print(" frames=");
    Serial.println(FRAMES_PER_CASE);
    }


void setup()
    {
    Serial.begin(115200);
    delay(1200);
    buildTextures();
    configureRenderer();
    Serial.println("[TGX] ConeCylinderPrimitiveBench start");
    }


void loop()
    {
    static int cycle = 0;
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); i++)
        {
        runCase(CASES[i], cycle);
        delay(20);
        }
    cycle++;
    }
