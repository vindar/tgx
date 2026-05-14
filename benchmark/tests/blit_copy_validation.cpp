// CPU validation tests for TGX blit/copy paths.
//
// The same scenarios are exercised with RGB565, RGB32 and RGBf because these
// are the main formats we care about for MCU and CPU rendering.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/tgx.h"

namespace
{

struct TestState
{
    int failures = 0;

    void check(bool condition, const std::string& label)
    {
        if (!condition)
            {
            failures++;
            std::cout << "FAIL: " << label << "\n";
            }
    }
};

template<typename color_t>
struct ColorMaker;

template<>
struct ColorMaker<tgx::RGB565>
{
    static tgx::RGB565 make(int i)
    {
        return tgx::RGB565((i * 3 + 1) & 31, (i * 5 + 2) & 63, (i * 7 + 3) & 31);
    }

    static tgx::RGB565 transparent()
    {
        return tgx::RGB565(31, 0, 31);
    }
};

template<>
struct ColorMaker<tgx::RGB32>
{
    static tgx::RGB32 make(int i)
    {
        return tgx::RGB32((i * 17 + 1) & 255, (i * 29 + 2) & 255, (i * 43 + 3) & 255);
    }

    static tgx::RGB32 transparent()
    {
        return tgx::RGB32(255, 0, 255);
    }
};

template<>
struct ColorMaker<tgx::RGBf>
{
    static tgx::RGBf make(int i)
    {
        return tgx::RGBf(0.001f * float(i + 1), 0.002f * float(i + 2), 0.003f * float(i + 3));
    }

    static tgx::RGBf transparent()
    {
        return tgx::RGBf(1.0f, 0.0f, 1.0f);
    }
};

template<typename color_t>
std::vector<color_t> makePixels(int w, int h, int seed)
{
    std::vector<color_t> pixels(w * h);
    for (int i = 0; i < w * h; i++) pixels[i] = ColorMaker<color_t>::make(seed + i);
    return pixels;
}

template<typename color_t>
std::vector<color_t> snapshot(const tgx::Image<color_t>& im)
{
    std::vector<color_t> pixels(im.lx() * im.ly());
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            pixels[x + y * im.lx()] = im(x, y);
            }
        }
    return pixels;
}

template<typename color_t>
void pasteNoBlend(std::vector<color_t>& dst, int dst_w, int dst_h,
                  const std::vector<color_t>& src, int src_w, int src_h,
                  int dest_x, int dest_y)
{
    int src_x = 0;
    int src_y = 0;
    int sx = src_w;
    int sy = src_h;

    if (dest_x < 0) { src_x = -dest_x; sx += dest_x; dest_x = 0; }
    if (dest_y < 0) { src_y = -dest_y; sy += dest_y; dest_y = 0; }
    if ((dest_x >= dst_w) || (dest_y >= dst_h) || (src_x >= src_w) || (src_y >= src_h)) return;

    sx -= tgx::max(0, dest_x + sx - dst_w);
    sy -= tgx::max(0, dest_y + sy - dst_h);
    sx -= tgx::max(0, src_x + sx - src_w);
    sy -= tgx::max(0, src_y + sy - src_h);
    if ((sx <= 0) || (sy <= 0)) return;

    for (int y = 0; y < sy; y++)
        {
        for (int x = 0; x < sx; x++)
            {
            dst[(dest_x + x) + (dest_y + y) * dst_w] = src[(src_x + x) + (src_y + y) * src_w];
            }
        }
}

template<typename color_t>
void pasteMasked(std::vector<color_t>& dst, int dst_w, int dst_h,
                 const std::vector<color_t>& src, int src_w, int src_h,
                 int dest_x, int dest_y, color_t transparent)
{
    int src_x = 0;
    int src_y = 0;
    int sx = src_w;
    int sy = src_h;

    if (dest_x < 0) { src_x = -dest_x; sx += dest_x; dest_x = 0; }
    if (dest_y < 0) { src_y = -dest_y; sy += dest_y; dest_y = 0; }
    if ((dest_x >= dst_w) || (dest_y >= dst_h) || (src_x >= src_w) || (src_y >= src_h)) return;

    sx -= tgx::max(0, dest_x + sx - dst_w);
    sy -= tgx::max(0, dest_y + sy - dst_h);
    sx -= tgx::max(0, src_x + sx - src_w);
    sy -= tgx::max(0, src_y + sy - src_h);
    if ((sx <= 0) || (sy <= 0)) return;

    for (int y = 0; y < sy; y++)
        {
        for (int x = 0; x < sx; x++)
            {
            const color_t c = src[(src_x + x) + (src_y + y) * src_w];
            if (c != transparent) dst[(dest_x + x) + (dest_y + y) * dst_w] = c;
            }
        }
}

template<typename color_t>
bool imageEquals(const tgx::Image<color_t>& im, const std::vector<color_t>& expected)
{
    if (int(expected.size()) != im.lx() * im.ly()) return false;
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            if (im(x, y) != expected[x + y * im.lx()]) return false;
            }
        }
    return true;
}

template<typename color_t>
void testExternalBlitClipping(TestState& t, const char* color_name)
{
    constexpr int DW = 8;
    constexpr int DH = 6;
    constexpr int SW = 4;
    constexpr int SH = 3;

    auto dst = makePixels<color_t>(DW, DH, 100);
    auto sprite_pixels = makePixels<color_t>(SW, SH, 500);
    auto expected = dst;

    tgx::Image<color_t> im(dst.data(), DW, DH);
    tgx::Image<color_t> sprite(sprite_pixels.data(), SW, SH);

    im.blit(sprite, {-2, 2}, -1.0f);
    pasteNoBlend(expected, DW, DH, sprite_pixels, SW, SH, -2, 2);
    t.check(imageEquals(im, expected), std::string(color_name) + " external blit clipped left");

    im.blit(sprite, {6, 4}, -1.0f);
    pasteNoBlend(expected, DW, DH, sprite_pixels, SW, SH, 6, 4);
    t.check(imageEquals(im, expected), std::string(color_name) + " external blit clipped bottom-right");
}

template<typename color_t>
void testInPlaceOverlap(TestState& t, const char* color_name)
{
    constexpr int W = 8;
    constexpr int H = 8;

    {
        auto pixels = makePixels<color_t>(W, H, 1000);
        auto expected = pixels;
        tgx::Image<color_t> im(pixels.data(), W, H);
        auto src = im(0, 4, 0, 4);
        const auto src_snapshot = snapshot(src);

        im.blit(src, {2, 2}, -1.0f);
        pasteNoBlend(expected, W, H, src_snapshot, src.lx(), src.ly(), 2, 2);
        t.check(imageEquals(im, expected), std::string(color_name) + " inplace overlap down-right");
    }

    {
        auto pixels = makePixels<color_t>(W, H, 2000);
        auto expected = pixels;
        tgx::Image<color_t> im(pixels.data(), W, H);
        auto src = im(2, 6, 2, 6);
        const auto src_snapshot = snapshot(src);

        im.blit(src, {0, 0}, -1.0f);
        pasteNoBlend(expected, W, H, src_snapshot, src.lx(), src.ly(), 0, 0);
        t.check(imageEquals(im, expected), std::string(color_name) + " inplace overlap up-left");
    }

    {
        auto pixels = makePixels<color_t>(W, H, 3000);
        auto expected = pixels;
        tgx::Image<color_t> im(pixels.data(), W, H);
        auto src = im(0, 5, 2, 3);
        const auto src_snapshot = snapshot(src);

        im.blit(src, {2, 2}, -1.0f);
        pasteNoBlend(expected, W, H, src_snapshot, src.lx(), src.ly(), 2, 2);
        t.check(imageEquals(im, expected), std::string(color_name) + " inplace horizontal overlap right");
    }

    {
        auto pixels = makePixels<color_t>(W, H, 4000);
        auto expected = pixels;
        tgx::Image<color_t> im(pixels.data(), W, H);
        auto src = im(2, 7, 2, 3);
        const auto src_snapshot = snapshot(src);

        im.blit(src, {0, 2}, -1.0f);
        pasteNoBlend(expected, W, H, src_snapshot, src.lx(), src.ly(), 0, 2);
        t.check(imageEquals(im, expected), std::string(color_name) + " inplace horizontal overlap left");
    }
}

template<typename color_t>
void testMaskedBlit(TestState& t, const char* color_name)
{
    constexpr int DW = 7;
    constexpr int DH = 5;
    constexpr int SW = 5;
    constexpr int SH = 4;

    auto dst = makePixels<color_t>(DW, DH, 6000);
    auto sprite_pixels = makePixels<color_t>(SW, SH, 7000);
    const color_t transparent = ColorMaker<color_t>::transparent();
    sprite_pixels[1 + 0 * SW] = transparent;
    sprite_pixels[3 + 1 * SW] = transparent;
    sprite_pixels[0 + 3 * SW] = transparent;

    auto expected = dst;
    tgx::Image<color_t> im(dst.data(), DW, DH);
    tgx::Image<color_t> sprite(sprite_pixels.data(), SW, SH);

    im.blitMasked(sprite, transparent, {-1, 1}, -1.0f);
    pasteMasked(expected, DW, DH, sprite_pixels, SW, SH, -1, 1, transparent);
    t.check(imageEquals(im, expected), std::string(color_name) + " masked blit clipped no-blend");
}

template<typename dst_color_t, typename src_color_t>
void testCrossColorBlendOperator(TestState& t, const char* label)
{
    constexpr int DW = 6;
    constexpr int DH = 4;
    constexpr int SW = 3;
    constexpr int SH = 2;

    auto dst = makePixels<dst_color_t>(DW, DH, 8000);
    auto src = makePixels<src_color_t>(SW, SH, 9000);
    auto expected = dst;

    tgx::Image<dst_color_t> im(dst.data(), DW, DH);
    tgx::Image<src_color_t> sprite(src.data(), SW, SH);
    const auto op = [](src_color_t s, dst_color_t) { return dst_color_t(s); };

    im.blit(sprite, {2, 1}, op);
    for (int y = 0; y < SH; y++)
        {
        for (int x = 0; x < SW; x++)
            {
            expected[(2 + x) + (1 + y) * DW] = dst_color_t(src[x + y * SW]);
            }
        }
    t.check(imageEquals(im, expected), label);
}

template<typename color_t>
void runColorTests(TestState& t, const char* color_name)
{
    testExternalBlitClipping<color_t>(t, color_name);
    testInPlaceOverlap<color_t>(t, color_name);
    testMaskedBlit<color_t>(t, color_name);
}

} // namespace

int main()
{
    TestState t;
    runColorTests<tgx::RGB565>(t, "RGB565");
    runColorTests<tgx::RGB32>(t, "RGB32");
    runColorTests<tgx::RGBf>(t, "RGBf");

    testCrossColorBlendOperator<tgx::RGB565, tgx::RGB32>(t, "RGB565<-RGB32 custom blit");
    testCrossColorBlendOperator<tgx::RGB32, tgx::RGB565>(t, "RGB32<-RGB565 custom blit");
    testCrossColorBlendOperator<tgx::RGBf, tgx::RGB32>(t, "RGBf<-RGB32 custom blit");

    if (t.failures != 0)
        {
        std::cout << t.failures << " blit/copy validation failure(s)\n";
        return 1;
        }

    std::cout << "blit/copy validation OK\n";
    return 0;
}

