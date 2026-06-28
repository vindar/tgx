#define cimg_display 0

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <direct.h>
#include <fstream>
#include <string>

#include <tgx.h>
#include "CImg.h"

using namespace tgx;
using cimg_library::CImg;


static const int W = 320;
static const int H = 240;
static const int TEX_W = 96;
static const int TEX_H = 64;

RGB565 fb[W * H];
uint16_t zbuf[W * H];
RGB565 textureData[TEX_W * TEX_H];

Image<RGB565> im(fb, W, H);
Image<RGB565> sphereTexture(textureData, TEX_W, TEX_H);

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


struct CompareStats
    {
    uint64_t pixels = 0;
    uint64_t changed = 0;
    uint64_t absSum = 0;
    uint64_t sqSum = 0;
    int maxChannel = 0;
    int maxPixelL1 = 0;
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


static void ensureDir(const std::string& path)
    {
    _mkdir(path.c_str());
    }


static void buildTexture()
    {
    for (int y = 0; y < TEX_H; y++)
        {
        for (int x = 0; x < TEX_W; x++)
            {
            const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
            const bool stripe = ((x + 2 * y) % 17) < 3;
            if (stripe) textureData[y * TEX_W + x] = colorFromBytes(235, 235, 245);
            else if (checker) textureData[y * TEX_W + x] = colorFromBytes(34, 97, 205);
            else textureData[y * TEX_W + x] = colorFromBytes(196, 149, 45);
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


static void setupRenderer()
    {
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setViewportSize(W, H);
    renderer.setOffset(0, 0);
    renderer.setPerspective(48.0f, ((float)W) / H, 1.0f, 80.0f);
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


static void configureCase(const SphereCase& c)
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


static void renderCase(const SphereCase& c)
    {
    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    configureCase(c);
    if (c.texture == TEX_CHECKER) renderer.drawSphere(c.sectors, c.stacks, &sphereTexture);
    else renderer.drawSphere(c.sectors, c.stacks);
    }


static bool fileExists(const std::string& path)
    {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
    }


static CompareStats compareImages(const CImg<unsigned char>& ref, const CImg<unsigned char>& cand, CImg<unsigned char>& diff)
    {
    CompareStats stats;
    diff.assign(cand.width(), cand.height(), 1, 3, 0);

    for (int y = 0; y < cand.height(); y++)
        {
        for (int x = 0; x < cand.width(); x++)
            {
            int l1 = 0;
            bool changed = false;
            for (int c = 0; c < 3; c++)
                {
                const int d = std::abs((int)cand(x, y, 0, c) - (int)ref(x, y, 0, c));
                stats.absSum += d;
                stats.sqSum += (uint64_t)d * d;
                stats.maxChannel = std::max(stats.maxChannel, d);
                l1 += d;
                diff(x, y, 0, c) = (unsigned char)std::min(255, d * 32);
                if (d) changed = true;
                }
            stats.maxPixelL1 = std::max(stats.maxPixelL1, l1);
            stats.changed += changed ? 1 : 0;
            stats.pixels++;
            }
        }
    return stats;
    }


static void saveSideBySide(const CImg<unsigned char>& ref, const CImg<unsigned char>& cand, const std::string& path)
    {
    const int gap = 6;
    const int labelH = 22;
    const unsigned char bg[] = { 12, 12, 12 };
    const unsigned char fg[] = { 255, 255, 255 };
    const unsigned char sep[] = { 128, 128, 128 };
    CImg<unsigned char> out(ref.width() + gap + cand.width(), labelH + ref.height(), 1, 3, 0);
    out.draw_rectangle(0, 0, out.width() - 1, out.height() - 1, bg);
    out.draw_text(4, 4, "reference", fg, bg, 1.0f, 13);
    out.draw_text(ref.width() + gap + 4, 4, "candidate", fg, bg, 1.0f, 13);
    out.draw_image(0, labelH, ref);
    out.draw_image(ref.width() + gap, labelH, cand);
    out.draw_line(ref.width() + gap / 2, 0, ref.width() + gap / 2, out.height() - 1, sep);
    out.save_bmp(path.c_str());
    }


int main(int argc, char** argv)
    {
    if (argc < 2)
        {
        std::fprintf(stderr, "usage: %s <output_dir> [reference_dir]\n", argv[0]);
        return 2;
        }

    const std::string outDir = argv[1];
    const std::string refDir = (argc >= 3) ? argv[2] : "";
    const std::string diffDir = outDir + "\\diff";
    const std::string sideDir = outDir + "\\side_by_side";
    ensureDir(outDir);
    if (!refDir.empty())
        {
        ensureDir(diffDir);
        ensureDir(sideDir);
        }

    buildTexture();
    setupRenderer();

    std::ofstream summary(outDir + "\\summary.csv");
    summary << "case,sectors,stacks,scale,shading,texture,light,specular,hash";
    if (!refDir.empty())
        {
        summary << ",changed_pixels,total_pixels,changed_ratio,mean_channel_abs,max_channel_abs,max_pixel_l1,rms_channel";
        }
    summary << "\n";

    for (const SphereCase& c : CASES)
        {
        renderCase(c);
        CImg<unsigned char> img = framebufferToImage();
        const std::string name = std::string(c.name) + ".bmp";
        const std::string outPath = outDir + "\\" + name;
        img.save_bmp(outPath.c_str());

        summary << c.name << ','
                << c.sectors << ','
                << c.stacks << ','
                << c.scale << ','
                << ((c.shading == SHADE_GOURAUD) ? "gouraud" : "flat") << ','
                << ((c.texture == TEX_CHECKER) ? "checker" : "none") << ','
                << ((c.light == LIGHT_DIRECTIONAL) ? "directional" : ((c.light == LIGHT_POINT) ? "point" : "spot")) << ','
                << (c.specular ? "on" : "off") << ','
                << framebufferHash();

        if (!refDir.empty())
            {
            const std::string refPath = refDir + "\\" + name;
            if (fileExists(refPath))
                {
                CImg<unsigned char> ref(refPath.c_str());
                CImg<unsigned char> diff;
                CompareStats stats = compareImages(ref, img, diff);
                diff.save_bmp((diffDir + "\\" + std::string(c.name) + "_diff_x32.bmp").c_str());
                saveSideBySide(ref, img, sideDir + "\\" + name);

                const double denom = (double)stats.pixels * 3.0;
                const double changedRatio = stats.pixels ? ((double)stats.changed / stats.pixels) : 0.0;
                const double meanAbs = denom ? ((double)stats.absSum / denom) : 0.0;
                const double rms = denom ? std::sqrt((double)stats.sqSum / denom) : 0.0;
                summary << ',' << stats.changed
                        << ',' << stats.pixels
                        << ',' << changedRatio
                        << ',' << meanAbs
                        << ',' << stats.maxChannel
                        << ',' << stats.maxPixelL1
                        << ',' << rms;
                }
            else
                {
                summary << ",,,,,,,";
                }
            }

        summary << "\n";
        std::printf("rendered %s\n", c.name);
        }

    std::printf("wrote %s\n", outDir.c_str());
    return 0;
    }

