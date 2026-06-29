#define cimg_display 0

#include <cstdint>
#include <cstdio>
#include <direct.h>
#include <fstream>
#include <string>
#include <vector>

#include <tgx.h>
#include "examples/CPU/buddhaOnCPU/CImg.h"

using namespace tgx;
using cimg_library::CImg;

static const int W = 240;
static const int H = 180;
static const int SIDE_TEX_W = 64;
static const int SIDE_TEX_H = 64;
static const int CAP_TEX_W = 64;
static const int CAP_TEX_H = 64;

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
                                     SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR |
                                     SHADER_TEXTURE_WRAP_POW2 | SHADER_TEXTURE_CLAMP;

static Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer;

enum ShapeKind
    {
    SHAPE_CYLINDER,
    SHAPE_CONE,
    SHAPE_TRUNCATED
    };

enum DrawMode
    {
    DRAW_SOLID_TEXTURED,
    DRAW_WIREFRAME_FAST,
    DRAW_WIREFRAME_AA,
    DRAW_WIREFRAME_THICK
    };

struct ClipCase
    {
    const char* name;
    ShapeKind shape;
    DrawMode mode;
    fVec3 pos;
    fVec3 scale;
    float yaw;
    bool bottomCap;
    bool topCap;
    bool culling;
    };

struct Snapshot
    {
    std::string name;
    CImg<unsigned char> image;
    uint32_t hash;
    uint32_t litPixels;
    };


static RGB565 colorFromBytes(int r, int g, int b)
    {
    return RGB565(r / 8, g / 4, b / 8);
    }


static void ensureDir(const std::string& path)
    {
    _mkdir(path.c_str());
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
            else if (checker) sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(35, 115, 230);
            else sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(230, 155, 35);
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
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(185, 45, 70);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(40, 170, 120);
                }
            else
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(82, 30, 42);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(18, 80, 70);
                }
            }
        }
    }


static void rgb565ToBytes(RGB565 c, unsigned char& r, unsigned char& g, unsigned char& b)
    {
    const uint16_t v = c.val;
    r = (unsigned char)((((v >> 11) & 31) * 255 + 15) / 31);
    g = (unsigned char)((((v >> 5) & 63) * 255 + 31) / 63);
    b = (unsigned char)(((v & 31) * 255 + 15) / 31);
    }


static CImg<unsigned char> framebufferToImage()
    {
    CImg<unsigned char> out(W, H, 1, 3, 0);
    for (int y = 0; y < H; y++)
        {
        for (int x = 0; x < W; x++)
            {
            unsigned char r, g, b;
            rgb565ToBytes(fb[y * W + x], r, g, b);
            out(x, y, 0, 0) = r;
            out(x, y, 0, 1) = g;
            out(x, y, 0, 2) = b;
            }
        }
    return out;
    }


static uint32_t framebufferHash()
    {
    uint32_t h = 2166136261u;
    for (int i = 0; i < W * H; i++)
        {
        h ^= fb[i].val;
        h *= 16777619u;
        }
    return h;
    }


static uint32_t countLitPixels()
    {
    uint32_t n = 0;
    for (int i = 0; i < W * H; i++)
        {
        if (fb[i].val != RGB565_Black.val) n++;
        }
    return n;
    }


static void setupRenderer()
    {
    renderer.setViewportSize(W, H);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(46.0f, ((float)W) / H, 0.5f, 40.0f);
    renderer.setLookAt({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f });
    renderer.setCulling(1);
    renderer.setLight({ -0.35f, -0.78f, -0.52f },
                      RGBf(0.10f, 0.10f, 0.10f),
                      RGBf(0.82f, 0.78f, 0.70f),
                      RGBf(0.10f, 0.10f, 0.10f));
    renderer.setMaterial(RGBf(0.78f, 0.74f, 0.62f), 0.14f, 0.86f, 0.20f, 18);
    }


static void configureCase(const ClipCase& c)
    {
    renderer.setCulling(c.culling ? 1 : 0);
    renderer.setModelPosScaleRot(c.pos, c.scale, c.yaw, { 0.25f, 1.0f, 0.15f });

    if (c.mode == DRAW_SOLID_TEXTURED)
        {
        renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                            SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP);
        }
    else
        {
        renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_NOTEXTURE);
        }
    }


static void drawCase(const ClipCase& c)
    {
    if (c.mode == DRAW_SOLID_TEXTURED)
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawCylinder(32, &sideTexture, &bottomTexture, &topTexture, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawCone(32, &sideTexture, &bottomTexture, c.bottomCap);
        else renderer.drawTruncatedCone(32, 1.0f, 0.45f, &sideTexture, &bottomTexture, &topTexture, c.bottomCap, c.topCap);
        }
    else if (c.mode == DRAW_WIREFRAME_FAST)
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawWireFrameCylinder(32, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawWireFrameCone(32, c.bottomCap);
        else renderer.drawWireFrameTruncatedCone(32, 1.0f, 0.45f, c.bottomCap, c.topCap);
        }
    else if (c.mode == DRAW_WIREFRAME_AA)
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawWireFrameCylinderAA(32, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawWireFrameConeAA(32, c.bottomCap);
        else renderer.drawWireFrameTruncatedConeAA(32, 1.0f, 0.45f, c.bottomCap, c.topCap);
        }
    else
        {
        const RGB565 cyan = colorFromBytes(80, 210, 255);
        if (c.shape == SHAPE_CYLINDER) renderer.drawWireFrameCylinder(24, c.bottomCap, c.topCap, 2.0f, cyan, 0.88f);
        else if (c.shape == SHAPE_CONE) renderer.drawWireFrameCone(24, c.bottomCap, 2.0f, cyan, 0.88f);
        else renderer.drawWireFrameTruncatedCone(24, 1.0f, 0.45f, c.bottomCap, c.topCap, 2.0f, cyan, 0.88f);
        }
    }


static Snapshot renderCase(const ClipCase& c)
    {
    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    configureCase(c);
    drawCase(c);
    return { c.name, framebufferToImage(), framebufferHash(), countLitPixels() };
    }


static void writeOverview(const std::string& path, const std::vector<Snapshot>& shots)
    {
    const int cols = 3;
    const int rows = (int)((shots.size() + cols - 1) / cols);
    const int labelH = 18;
    CImg<unsigned char> sheet(cols * W, rows * (H + labelH), 1, 3, 16);
    const unsigned char white[3] = { 235, 235, 235 };

    for (size_t i = 0; i < shots.size(); i++)
        {
        const int col = (int)(i % cols);
        const int row = (int)(i / cols);
        const int x0 = col * W;
        const int y0 = row * (H + labelH);
        sheet.draw_text(x0 + 4, y0 + 3, shots[i].name.c_str(), white, 0, 1.0f, 13);
        sheet.draw_image(x0, y0 + labelH, shots[i].image);
        }

    sheet.save_bmp(path.c_str());
    }


int main(int argc, char** argv)
    {
    const std::string outDir = (argc > 1) ? argv[1] : "validation\\performance\\investigations\\2026-06-cone-cylinder-primitives\\cpu\\clipping";
    ensureDir(outDir);
    buildTextures();
    setupRenderer();

    const ClipCase cases[] =
        {
        { "tex cylinder near", SHAPE_CYLINDER, DRAW_SOLID_TEXTURED, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, 24.0f, true, true, false },
        { "tex cone near", SHAPE_CONE, DRAW_SOLID_TEXTURED, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, -20.0f, true, true, false },
        { "tex trunc near", SHAPE_TRUNCATED, DRAW_SOLID_TEXTURED, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, 18.0f, true, true, false },

        { "tex cylinder side", SHAPE_CYLINDER, DRAW_SOLID_TEXTURED, { 1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, 26.0f, true, true, true },
        { "tex cone side", SHAPE_CONE, DRAW_SOLID_TEXTURED, { -1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, -24.0f, true, true, true },
        { "tex trunc side", SHAPE_TRUNCATED, DRAW_SOLID_TEXTURED, { 1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, 18.0f, true, true, true },

        { "wf cyl near", SHAPE_CYLINDER, DRAW_WIREFRAME_FAST, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, 24.0f, true, true, false },
        { "wf cone near aa", SHAPE_CONE, DRAW_WIREFRAME_AA, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, -20.0f, true, true, false },
        { "wf trunc near thick", SHAPE_TRUNCATED, DRAW_WIREFRAME_THICK, { 0.0f, 0.0f, -0.95f }, { 0.70f, 0.70f, 0.70f }, 18.0f, true, true, false },

        { "wf cyl side aa", SHAPE_CYLINDER, DRAW_WIREFRAME_AA, { 1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, 26.0f, true, true, true },
        { "wf cone side", SHAPE_CONE, DRAW_WIREFRAME_FAST, { -1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, -24.0f, true, true, true },
        { "wf trunc side thick", SHAPE_TRUNCATED, DRAW_WIREFRAME_THICK, { 1.02f, 0.0f, -3.0f }, { 0.95f, 0.95f, 0.95f }, 18.0f, true, true, true },
        };

    std::vector<Snapshot> shots;
    std::ofstream csv(outDir + "\\summary.csv");
    csv << "name,hash,lit_pixels\n";

    for (const auto& c : cases)
        {
        Snapshot shot = renderCase(c);
        csv << shot.name << ",0x" << std::hex << shot.hash << std::dec << "," << shot.litPixels << "\n";
        shots.push_back(shot);
        std::printf("%s hash=0x%08x lit=%u\n", shot.name.c_str(), shot.hash, shot.litPixels);
        }

    writeOverview(outDir + "\\overview.bmp", shots);
    std::printf("overview=%s\\overview.bmp\n", outDir.c_str());
    return 0;
    }
