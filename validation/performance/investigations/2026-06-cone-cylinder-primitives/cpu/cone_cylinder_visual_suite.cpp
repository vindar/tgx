#define cimg_display 0

#include <algorithm>
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


static const int W = 360;
static const int H = 270;
static const int SIDE_TEX_W = 128;
static const int SIDE_TEX_H = 64;
static const int CAP_TEX_W = 96;
static const int CAP_TEX_H = 96;

RGB565 fb[W * H];
uint16_t zbuf[W * H];
RGB565 sideTextureData[SIDE_TEX_W * SIDE_TEX_H];
RGB565 bottomTextureData[CAP_TEX_W * CAP_TEX_H];
RGB565 topTextureData[CAP_TEX_W * CAP_TEX_H];

Image<RGB565> im(fb, W, H);
Image<RGB565> sideTexture(sideTextureData, SIDE_TEX_W, SIDE_TEX_H);
Image<RGB565> bottomTexture(bottomTextureData, CAP_TEX_W, CAP_TEX_H);
Image<RGB565> topTexture(topTextureData, CAP_TEX_W, CAP_TEX_H);

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE |
                              SHADER_TEXTURE_AFFINE |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_WRAP_POW2 | SHADER_TEXTURE_CLAMP;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 1> renderer;


enum ShapeKind
    {
    SHAPE_CYLINDER,
    SHAPE_CONE,
    SHAPE_TRUNCATED
    };


enum ShadingMode
    {
    SHADE_FLAT,
    SHADE_GOURAUD
    };


enum TextureMode
    {
    TEX_NONE,
    TEX_SIDE,
    TEX_ALL,
    TEX_SIDE_ONLY_OPEN
    };


struct VisualCase
    {
    const char* name;
    ShapeKind shape;
    int sectors;
    float bottomRadius;
    float topRadius;
    bool bottomCap;
    bool topCap;
    ShadingMode shading;
    TextureMode texture;
    bool affine;
    fVec3 cameraPos;
    fVec3 cameraTarget;
    float yaw;
    float pitch;
    float scale;
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
            const int band = x / 16;
            const bool checker = (((x >> 4) ^ (y >> 4)) & 1) != 0;
            const bool seam = (x < 3) || (x > SIDE_TEX_W - 4);
            const bool hline = ((y % 16) == 0);

            if (seam) sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(245, 245, 245);
            else if (hline) sideTextureData[y * SIDE_TEX_W + x] = colorFromBytes(25, 25, 35);
            else if ((band & 1) == 0) sideTextureData[y * SIDE_TEX_W + x] = checker ? colorFromBytes(35, 108, 220) : colorFromBytes(45, 155, 230);
            else sideTextureData[y * SIDE_TEX_W + x] = checker ? colorFromBytes(222, 158, 42) : colorFromBytes(238, 196, 82);
            }
        }

    for (int y = 0; y < CAP_TEX_H; y++)
        {
        for (int x = 0; x < CAP_TEX_W; x++)
            {
            const float fx = ((float)x + 0.5f) / CAP_TEX_W - 0.5f;
            const float fy = ((float)y + 0.5f) / CAP_TEX_H - 0.5f;
            const float r2 = fx * fx + fy * fy;
            const bool ring = (((int)(r2 * 180.0f)) & 1) != 0;
            const bool cross = (std::abs(x - CAP_TEX_W / 2) < 2) || (std::abs(y - CAP_TEX_H / 2) < 2);

            if (cross)
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(250, 250, 250);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(250, 250, 250);
                }
            else if (ring)
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(190, 55, 70);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(60, 180, 135);
                }
            else
                {
                bottomTextureData[y * CAP_TEX_W + x] = colorFromBytes(105, 34, 45);
                topTextureData[y * CAP_TEX_W + x] = colorFromBytes(25, 95, 85);
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
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setViewportSize(W, H);
    renderer.setOffset(0, 0);
    renderer.setPerspective(47.0f, ((float)W) / H, 1.0f, 80.0f);
    renderer.setCulling(1);
    renderer.setLight({ -0.35f, -0.78f, -0.52f },
                      RGBf(0.10f, 0.10f, 0.11f),
                      RGBf(0.80f, 0.82f, 0.80f),
                      RGBf(0.55f, 0.52f, 0.46f));
    renderer.setSpotLightCount(1);
    renderer.setSpotLight(0, { 1.8f, 1.1f, 2.4f }, 4.8f,
                          RGBf(0.72f, 0.62f, 0.46f),
                          RGBf(0.45f, 0.38f, 0.32f));
    }


static void configureCase(const VisualCase& c)
    {
    Shader shader = SHADER_PERSPECTIVE | SHADER_ZBUFFER;
    shader |= (c.shading == SHADE_GOURAUD) ? SHADER_GOURAUD : SHADER_FLAT;
    if (c.texture != TEX_NONE)
        {
        shader |= c.affine ? SHADER_TEXTURE_AFFINE : SHADER_TEXTURE;
        shader |= SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP;
        }
    else
        {
        shader |= SHADER_NOTEXTURE;
        }
    renderer.setShaders(shader);
    renderer.setCulling(1);
    renderer.setLookAt(c.cameraPos, c.cameraTarget, { 0.0f, 1.0f, 0.0f });
    renderer.setMaterial(RGBf(0.78f, 0.70f, 0.58f), 0.12f, 0.88f, 0.35f, 24);
    renderer.setModelPosScaleRot({ 0.0f, -0.04f, 0.0f },
                                 { c.scale, c.scale, c.scale },
                                 c.yaw, { 0.18f + 0.01f * c.pitch, 1.0f, 0.08f });
    }


static void drawCase(const VisualCase& c)
    {
    const Image<RGB565>* side = nullptr;
    const Image<RGB565>* bottom = nullptr;
    const Image<RGB565>* top = nullptr;

    if (c.texture == TEX_SIDE)
        {
        side = &sideTexture;
        }
    else if (c.texture == TEX_ALL)
        {
        side = &sideTexture;
        bottom = &bottomTexture;
        top = &topTexture;
        }
    else if (c.texture == TEX_SIDE_ONLY_OPEN)
        {
        side = &sideTexture;
        }

    if (c.texture == TEX_NONE)
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawCylinder(c.sectors, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawCone(c.sectors, c.bottomCap);
        else renderer.drawTruncatedCone(c.sectors, c.bottomRadius, c.topRadius, c.bottomCap, c.topCap);
        }
    else
        {
        if (c.shape == SHAPE_CYLINDER) renderer.drawCylinder(c.sectors, side, bottom, top, c.bottomCap, c.topCap);
        else if (c.shape == SHAPE_CONE) renderer.drawCone(c.sectors, side, bottom, c.bottomCap);
        else renderer.drawTruncatedCone(c.sectors, c.bottomRadius, c.topRadius, side, bottom, top, c.bottomCap, c.topCap);
        }
    }


static Snapshot renderCase(const VisualCase& c)
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
    CImg<unsigned char> sheet(cols * W, rows * (H + labelH), 1, 3, 18);
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
    const std::string outDir = (argc > 1) ? argv[1] : "validation\\performance\\investigations\\2026-06-cone-cylinder-primitives\\cpu\\current";
    ensureDir(outDir);
    buildTextures();
    setupRenderer();

    const VisualCase cases[] =
        {
        { "cylinder_gouraud_closed", SHAPE_CYLINDER, 32, 1.0f, 1.0f, true, true, SHADE_GOURAUD, TEX_NONE, false, { 3.2f, 1.9f, 5.0f }, { 0, 0, 0 }, 24.0f, 0.0f, 1.0f },
        { "cylinder_open_top_culling", SHAPE_CYLINDER, 32, 1.0f, 1.0f, true, false, SHADE_GOURAUD, TEX_NONE, false, { 2.2f, 4.2f, 3.2f }, { 0, -0.15f, 0 }, 18.0f, 0.0f, 1.0f },
        { "cylinder_textured_all", SHAPE_CYLINDER, 40, 1.0f, 1.0f, true, true, SHADE_GOURAUD, TEX_ALL, false, { 3.0f, 2.0f, 5.0f }, { 0, 0, 0 }, -22.0f, 0.0f, 1.0f },
        { "cylinder_side_texture_open", SHAPE_CYLINDER, 40, 1.0f, 1.0f, false, false, SHADE_FLAT, TEX_SIDE_ONLY_OPEN, true, { 3.3f, 1.5f, 5.1f }, { 0, 0, 0 }, 38.0f, 0.0f, 1.0f },
        { "cone_flat_closed", SHAPE_CONE, 28, 1.0f, 0.0f, true, true, SHADE_FLAT, TEX_NONE, false, { 3.0f, 1.7f, 5.1f }, { 0, 0, 0 }, -26.0f, 0.0f, 1.0f },
        { "cone_gouraud_textured", SHAPE_CONE, 40, 1.0f, 0.0f, true, true, SHADE_GOURAUD, TEX_ALL, false, { 3.2f, 2.0f, 5.2f }, { 0, 0, 0 }, 16.0f, 0.0f, 1.0f },
        { "cone_open_bottom_culling", SHAPE_CONE, 36, 1.0f, 0.0f, false, true, SHADE_GOURAUD, TEX_SIDE, false, { 2.6f, -2.8f, 4.0f }, { 0, -0.15f, 0 }, 15.0f, 0.0f, 1.0f },
        { "truncated_textured_all", SHAPE_TRUNCATED, 48, 1.0f, 0.45f, true, true, SHADE_GOURAUD, TEX_ALL, false, { 3.0f, 1.9f, 5.2f }, { 0, 0, 0 }, -30.0f, 0.0f, 1.0f },
        { "inverted_truncated_textured", SHAPE_TRUNCATED, 48, 0.35f, 1.0f, true, true, SHADE_GOURAUD, TEX_ALL, false, { 3.0f, 1.9f, 5.2f }, { 0, 0, 0 }, 34.0f, 0.0f, 1.0f },
        { "low_sector_faceted_caps", SHAPE_TRUNCATED, 8, 1.0f, 0.55f, true, true, SHADE_FLAT, TEX_ALL, false, { 3.0f, 2.1f, 5.0f }, { 0, 0, 0 }, 20.0f, 0.0f, 1.05f },
        };

    std::vector<Snapshot> shots;
    std::ofstream csv(outDir + "\\summary.csv");
    csv << "name,hash,lit_pixels\n";

    for (const auto& c : cases)
        {
        Snapshot shot = renderCase(c);
        const std::string bmpPath = outDir + "\\" + shot.name + ".bmp";
        shot.image.save_bmp(bmpPath.c_str());
        csv << shot.name << ",0x" << std::hex << shot.hash << std::dec << "," << shot.litPixels << "\n";
        shots.push_back(shot);
        std::printf("%s hash=0x%08x lit=%u\n", shot.name.c_str(), shot.hash, shot.litPixels);
        }

    writeOverview(outDir + "\\overview.bmp", shots);
    std::printf("overview=%s\\overview.bmp\n", outDir.c_str());
    return 0;
    }
