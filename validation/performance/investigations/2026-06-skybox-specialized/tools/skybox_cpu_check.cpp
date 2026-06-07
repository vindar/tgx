#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include <tgx.h>

using namespace tgx;

namespace
{

constexpr int W = 320;
constexpr int H = 240;
constexpr int TEX = 32;
constexpr int ITER = 240;

const Shader SKYBOX_SHADERS = SHADER_PERSPECTIVE |
                              SHADER_ZBUFFER |
                              SHADER_UNLIT |
                              SHADER_FLAT |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_WRAP_POW2;

uint64_t fnv1a(const RGB565* data, size_t count)
    {
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < count; ++i)
        {
        const uint16_t v = data[i].val;
        h ^= uint8_t(v & 255);
        h *= 1099511628211ull;
        h ^= uint8_t(v >> 8);
        h *= 1099511628211ull;
        }
    return h;
    }

RGB565 makeColor(int face, int x, int y)
    {
    const uint8_t r = uint8_t((face * 37 + x * 5 + y * 3) & 255);
    const uint8_t g = uint8_t((face * 61 + x * 2 + y * 7) & 255);
    const uint8_t b = uint8_t((face * 83 + x * 11 + y) & 255);
    return RGB565(RGB32(r, g, b));
    }

void fillTexture(Image<RGB565>& im, int face)
    {
    for (int y = 0; y < im.ly(); ++y)
        {
        for (int x = 0; x < im.lx(); ++x)
            {
            im(x, y) = makeColor(face, x, y);
            }
        }
    }

void configure(Renderer3D<RGB565, SKYBOX_SHADERS, uint16_t>& r, Image<RGB565>& im, uint16_t* zbuf)
    {
    r.setViewportSize(W, H);
    r.setOffset(0, 0);
    r.setImage(&im);
    r.setZbuffer(zbuf);
    r.setPerspective(55.0f, float(W) / float(H), 1.0f, 100.0f);
    r.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    r.setTextureQuality(SHADER_TEXTURE_NEAREST);
    r.setCulling(0);
    r.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f }, { 50.0f, 50.0f, 50.0f }, 180.0f, { 0.0f, 1.0f, 0.0f });
    }

template<typename Fn>
double timeDraw(Fn&& fn, std::vector<uint16_t>& zbuf)
    {
    uint64_t total_ns = 0;
    for (int i = 0; i < ITER; ++i)
        {
        std::fill(zbuf.begin(), zbuf.end(), 0);
        const auto t0 = std::chrono::high_resolution_clock::now();
        fn();
        const auto t1 = std::chrono::high_resolution_clock::now();
        total_ns += uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }
    return double(total_ns) / double(ITER) / 1000.0;
    }

}

int main()
    {
    std::vector<RGB565> fb_old(W * H);
    std::vector<RGB565> fb_new(W * H);
    std::vector<uint16_t> z_old(W * H);
    std::vector<uint16_t> z_new(W * H);

    std::vector<RGB565> tex_storage[6];
    Image<RGB565> tex[6];
    for (int i = 0; i < 6; ++i)
        {
        tex_storage[i].resize(TEX * TEX);
        tex[i] = Image<RGB565>(tex_storage[i].data(), TEX, TEX);
        fillTexture(tex[i], i);
        }

    Image<RGB565> im_old(fb_old.data(), W, H);
    Image<RGB565> im_new(fb_new.data(), W, H);
    Renderer3D<RGB565, SKYBOX_SHADERS, uint16_t> old_renderer;
    Renderer3D<RGB565, SKYBOX_SHADERS, uint16_t> new_renderer;
    configure(old_renderer, im_old, z_old.data());
    configure(new_renderer, im_new, z_new.data());

    std::fill(fb_old.begin(), fb_old.end(), RGB565(RGB32_Black));
    std::fill(z_old.begin(), z_old.end(), 0);
    old_renderer.setShaders(SHADER_UNLIT | SHADER_TEXTURE_NEAREST);
    old_renderer.drawCube(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);

    std::fill(fb_new.begin(), fb_new.end(), RGB565(RGB32_Black));
    std::fill(z_new.begin(), z_new.end(), 0);
    new_renderer.drawSkyBox(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);

    int mismatches = 0;
    for (size_t i = 0; i < fb_old.size(); ++i)
        {
        if (fb_old[i] != fb_new[i]) ++mismatches;
        }

    const uint64_t hash_old = fnv1a(fb_old.data(), fb_old.size());
    const uint64_t hash_new = fnv1a(fb_new.data(), fb_new.size());

    const double old_us = timeDraw([&]() {
        old_renderer.drawCube(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);
        }, z_old);

    const double new_us = timeDraw([&]() {
        new_renderer.drawSkyBox(&tex[0], &tex[1], &tex[2], &tex[3], &tex[4], &tex[5]);
        }, z_new);

    std::cout << "SKYBOX_CPU_CHECK_BEGIN\n";
    std::cout << "WIDTH=" << W << " HEIGHT=" << H << " ITER=" << ITER << "\n";
    std::cout << "HASH_OLD=" << hash_old << " HASH_NEW=" << hash_new << "\n";
    std::cout << "MISMATCHES=" << mismatches << "\n";
    std::cout << "TIME_OLD_US=" << old_us << " TIME_NEW_US=" << new_us << "\n";
    std::cout << "SPEEDUP=" << ((old_us / new_us) - 1.0) * 100.0 << "\n";
    std::cout << "SKYBOX_CPU_CHECK_END\n";

    return mismatches == 0 ? 0 : 2;
    }
