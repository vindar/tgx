// CPU validation tests for TGX text rendering.
//
// These tests intentionally exercise the same text paths with the three most
// important color types: RGB565, RGB32 and RGBf.

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/tgx.h"
#include "src/font_tgx_OpenSans.h"

namespace
{

uint8_t gfx_bitmap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // A: full 8x8 block
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // B: full 8x8 block
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  // C: full 8x8 block
};

GFXglyph gfx_glyphs[] = {
    { 0,  8, 8, 8, 0, -8 },
    { 8,  8, 8, 8, 0, -8 },
    { 16, 8, 8, 8, 0, -8 }
};

GFXfont gfx_font = {
    gfx_bitmap,
    gfx_glyphs,
    'A',
    'C',
    10
};

template<typename color_t>
struct TestColors;

template<>
struct TestColors<tgx::RGB565>
{
    static tgx::RGB565 background() { return tgx::RGB565_Black; }
    static tgx::RGB565 foreground() { return tgx::RGB565_Red; }
};

template<>
struct TestColors<tgx::RGB32>
{
    static tgx::RGB32 background() { return tgx::RGB32_Black; }
    static tgx::RGB32 foreground() { return tgx::RGB32_Red; }
};

template<>
struct TestColors<tgx::RGBf>
{
    static tgx::RGBf background() { return tgx::RGBf(0.0f, 0.0f, 0.0f); }
    static tgx::RGBf foreground() { return tgx::RGBf(1.0f, 0.0f, 0.0f); }
};

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

std::ostream& operator<<(std::ostream& os, const tgx::iVec2& v)
{
    os << "(" << v.x << "," << v.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const tgx::iBox2& b)
{
    if (b.isEmpty()) return os << "(empty)";
    os << "[" << b.minX << "," << b.maxX << "]x[" << b.minY << "," << b.maxY << "]";
    return os;
}

std::string boxToString(const tgx::iBox2& b)
{
    std::ostringstream oss;
    oss << b;
    return oss.str();
}

template<typename color_t>
tgx::iBox2 paintedBounds(const tgx::Image<color_t>& im, color_t background)
{
    tgx::iBox2 b;
    b.empty();
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            if (im(x, y) != background)
                {
                b |= tgx::iVec2(x, y);
                }
            }
        }
    return b;
}

template<typename color_t>
bool samePixels(const tgx::Image<color_t>& a, const tgx::Image<color_t>& b)
{
    if ((a.lx() != b.lx()) || (a.ly() != b.ly())) return false;
    for (int y = 0; y < a.ly(); y++)
        {
        for (int x = 0; x < a.lx(); x++)
            {
            if (a(x, y) != b(x, y)) return false;
            }
        }
    return true;
}

template<typename color_t>
void fill(std::vector<color_t>& pixels, color_t color)
{
    for (auto& p : pixels) p = color;
}

template<typename color_t>
tgx::iVec2 expectedDefaultStartForAnchor(const tgx::Image<color_t>& im,
                                         const char* text,
                                         const GFXfont& font,
                                         tgx::iVec2 anchor_pos,
                                         tgx::Anchor anchor)
{
    tgx::iBox2 b = im.measureText(text, anchor_pos, font, tgx::DEFAULT_TEXT_ANCHOR, false, false);
    tgx::iVec2 anchor_in_box = b.getAnchor(anchor);
    if (anchor & tgx::BASELINE) anchor_in_box.y = anchor_pos.y;
    return anchor_pos + (anchor_pos - anchor_in_box);
}

template<typename color_t>
tgx::iVec2 expectedDefaultStartForAnchor(const tgx::Image<color_t>& im,
                                         const char* text,
                                         const ILI9341_t3_font_t& font,
                                         tgx::iVec2 anchor_pos,
                                         tgx::Anchor anchor)
{
    tgx::iBox2 b = im.measureText(text, anchor_pos, font, tgx::DEFAULT_TEXT_ANCHOR, false, false);
    tgx::iVec2 anchor_in_box = b.getAnchor(anchor);
    if (anchor & tgx::BASELINE) anchor_in_box.y = anchor_pos.y;
    return anchor_pos + (anchor_pos - anchor_in_box);
}

template<typename color_t>
void testGfxExactFitWrap(TestState& t, const char* color_name)
{
    constexpr int W = 24;
    constexpr int H = 32;
    const color_t bg = TestColors<color_t>::background();
    const color_t fg = TestColors<color_t>::foreground();
    std::vector<color_t> pixels(W * H);
    fill(pixels, bg);
    tgx::Image<color_t> im(pixels.data(), W, H);

    const tgx::iVec2 pos(0, 12);
    const tgx::iVec2 ret = im.drawTextEx("ABC", pos, gfx_font, tgx::DEFAULT_TEXT_ANCHOR, true, false, fg, -1.0f);
    const tgx::iBox2 measured = im.measureText("ABC", pos, gfx_font, tgx::DEFAULT_TEXT_ANCHOR, true, false);
    const tgx::iBox2 painted = paintedBounds(im, bg);
    const tgx::iBox2 expected(0, 23, 4, 11);

    t.check(ret == tgx::iVec2(24, 12), std::string(color_name) + " GFX wrap exact-fit return");
    t.check(measured == expected, std::string(color_name) + " GFX wrap exact-fit measured " + "got " + boxToString(measured));
    t.check(painted == expected, std::string(color_name) + " GFX wrap exact-fit painted");
}

template<typename color_t, typename font_t>
void testBaselineRightAnchor(TestState& t, const char* color_name, const char* font_name, const font_t& font)
{
    constexpr int W = 96;
    constexpr int H = 64;
    const color_t bg = TestColors<color_t>::background();
    const color_t fg = TestColors<color_t>::foreground();
    const char* text = "AB";
    const tgx::iVec2 anchor_pos(60, 32);
    const tgx::Anchor anchor = tgx::BASELINE | tgx::RIGHT;

    std::vector<color_t> anchor_pixels(W * H);
    std::vector<color_t> reference_pixels(W * H);
    fill(anchor_pixels, bg);
    fill(reference_pixels, bg);

    tgx::Image<color_t> anchor_im(anchor_pixels.data(), W, H);
    tgx::Image<color_t> reference_im(reference_pixels.data(), W, H);

    const tgx::iVec2 expected_start = expectedDefaultStartForAnchor(reference_im, text, font, anchor_pos, anchor);
    const tgx::iVec2 expected_ret = reference_im.drawTextEx(text, expected_start, font, tgx::DEFAULT_TEXT_ANCHOR, false, false, fg, -1.0f);
    const tgx::iVec2 ret = anchor_im.drawTextEx(text, anchor_pos, font, anchor, false, false, fg, -1.0f);

    const std::string prefix = std::string(color_name) + " " + font_name + " BASELINE|RIGHT ";
    t.check(ret == expected_ret, prefix + "return");
    t.check(paintedBounds(anchor_im, bg) == paintedBounds(reference_im, bg), prefix + "painted bounds");
    t.check(samePixels(anchor_im, reference_im), prefix + "pixels");
}

template<typename color_t>
void runColorTests(TestState& t, const char* color_name)
{
    testGfxExactFitWrap<color_t>(t, color_name);
    testBaselineRightAnchor<color_t>(t, color_name, "GFX", gfx_font);
    testBaselineRightAnchor<color_t>(t, color_name, "ILI", font_tgx_OpenSans_12);
}

} // namespace

int main()
{
    TestState t;
    runColorTests<tgx::RGB565>(t, "RGB565");
    runColorTests<tgx::RGB32>(t, "RGB32");
    runColorTests<tgx::RGBf>(t, "RGBf");

    if (t.failures != 0)
        {
        std::cout << t.failures << " text validation failure(s)\n";
        return 1;
        }

    std::cout << "text validation OK\n";
    return 0;
}
