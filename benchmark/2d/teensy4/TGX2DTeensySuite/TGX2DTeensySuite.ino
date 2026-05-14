/********************************************************************
*
* TGX 2D validation and benchmark suite.
*
* EXAMPLE FOR TEENSY 4 / 4.1
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

#include <ILI9341_T4.h>
#include <tgx.h>
#include <font_tgx_Arial.h>
#include <font_tgx_OpenSans.h>
#include <font_tgx_OpenSans_Bold.h>

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

#define SPI_SPEED 40000000

static const int SLX = 320;
static const int SLY = 240;

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff2;

DMAMEM uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
Image<RGB565> im(fb, SLX, SLY);

static const int SW = 52;
static const int SH = 36;
RGB565 spriteBuf[SW * SH];
RGB565 spriteMaskedBuf[SW * SH];
Image<RGB565> sprite(spriteBuf, SW, SH);
Image<RGB565> spriteMasked(spriteMaskedBuf, SW, SH);
const RGB565 maskColor = RGB565(31, 0, 31);
const char* RESULT_PLATFORM = "teensy4";
const char* RESULT_COLOR = "RGB565";

uint8_t gfx_bitmap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x81, 0xbd, 0xa5, 0xa5, 0xbd, 0x81, 0xff, 0x00,
    0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24, 0x24, 0x66
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

struct Bench
{
    const char* name;
    uint32_t iterations;
    uint32_t total_us;
    uint32_t hash;
    uint32_t changed;
    bool ok;
};

Bench results[40];
int resultCount = 0;

struct IPointSource
{
    const iVec2* points;
    int count;
    int index;

    bool operator()(iVec2& p)
    {
        if (count <= 0) return false;
        if (index == count) index = 0;
        p = points[index++];
        return index < count;
    }
};

struct FPointSource
{
    const fVec2* points;
    int count;
    int index;

    bool operator()(fVec2& p)
    {
        if (count <= 0) return false;
        if (index == count) index = 0;
        p = points[index++];
        return index < count;
    }
};

RGB565 stateBuf[8 * 5];
RGB565 srcBuf[96 * 72];
RGB565 dstBuf[96 * 72];
RGB565 smallBuf[48 * 36];
RGB32 convertBuf32[7 * 5];
RGBf convertBufF[4 * 3];

uint32_t fnv1a(uint32_t h, uint8_t b)
{
    h ^= b;
    h *= 16777619u;
    return h;
}

uint32_t imageHash()
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < SLX * SLY; i++)
        {
        const uint16_t v = fb[i];
        h = fnv1a(h, (uint8_t)(v & 255));
        h = fnv1a(h, (uint8_t)(v >> 8));
        }
    return h;
}

uint32_t countChanged(RGB565 bg)
{
    uint32_t n = 0;
    for (int i = 0; i < SLX * SLY; i++) if (fb[i] != bg.val) n++;
    return n;
}

uint32_t countChangedImage(const Image<RGB565>& image, RGB565 bg)
{
    uint32_t n = 0;
    for (int y = 0; y < image.ly(); y++)
        {
        for (int x = 0; x < image.lx(); x++)
            {
            if (image(x, y) != bg) n++;
            }
        }
    return n;
}

bool imageEquals(const Image<RGB565>& a, const Image<RGB565>& b)
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

void title(const char* text)
{
    im.fillRect(iBox2(0, 319, 0, 22), RGB565(2, 6, 11));
    im.drawTextEx(text, { 160, 17 }, font_tgx_OpenSans_Bold_14, CENTER, false, false, RGB565_White);
}

void panel(iBox2 box, const char* label)
{
    im.fillRect(box, RGB565(3, 7, 14));
    im.drawRect(box, RGB565(12, 25, 31));
    im.drawTextEx(label, { box.minX + 5, box.minY + 13 }, font_tgx_OpenSans_8, BASELINE | LEFT, false, false, RGB565(28, 45, 31));
}

void buildSprites()
{
    sprite.fillScreenHGradient(RGB565(4, 22, 31), RGB565(31, 45, 5));
    sprite.fillCircleAA({ 16.0f, 18.0f }, 10.0f, RGB565(31, 12, 2), 0.9f);
    sprite.drawThickRectAA(fBox2(1.0f, SW - 2.0f, 1.0f, SH - 2.0f), 2.0f, RGB565_White, 0.8f);
    sprite.drawTextEx("TGX", { SW / 2, 23 }, font_tgx_OpenSans_Bold_10, CENTER, false, false, RGB565_Black, 0.8f);

    spriteMasked.copyFrom(sprite);
    spriteMasked.fillCircleAA({ 8.0f, 8.0f }, 6.0f, maskColor);
    spriteMasked.drawThickLineAA({ 6.0f, SH - 7.0f }, { SW - 7.0f, 6.0f }, 5.0f, END_ROUNDED, END_ROUNDED, maskColor);
}

bool sampleValidation()
{
    im.clear(RGB565_Black);
    im.drawPixel({ 3, 3 }, RGB565_Red);
    if (im(3, 3) != RGB565_Red) return false;
    im.fillRect(iBox2(20, 34, 20, 34), RGB565_Green);
    if (im(20, 20) != RGB565_Green) return false;
    if (im(34, 34) != RGB565_Green) return false;
    im.blit(sprite, { 50, 50 }, -1.0f);
    return im(50, 50) == sprite(0, 0);
}

bool imageStateValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    for (int i = 0; i < 8 * 5; i++) stateBuf[i] = bg;

    Image<RGB565> invalid;
    if (invalid.isValid() || invalid.lx() != 0 || invalid.ly() != 0 || invalid.stride() != 0) return false;

    Image<RGB565> img;
    img.set(stateBuf, 6, 4, 8);
    if (!img.isValid() || img.lx() != 6 || img.ly() != 4 || img.stride() != 8) return false;
    const auto box = img.imageBox();
    if (img.width() != 6 || img.height() != 4 || img.dim() != iVec2{ 6, 4 } ||
        box.minX != 0 || box.maxX != 5 || box.minY != 0 || box.maxY != 3) return false;
    if (img.data() != stateBuf || static_cast<const Image<RGB565>&>(img).data() != stateBuf) return false;

    img.drawPixel({ 2, 1 }, RGB565_Red);
    if (img.readPixel({ 2, 1 }) != RGB565_Red) return false;
    if (img.readPixel({ -1, 1 }, RGB565_Yellow) != RGB565_Yellow) return false;

    img.drawPixelf({ 4.4f, 2.4f }, RGB565_Green);
    if (img.readPixelf({ 4.4f, 2.4f }) != RGB565_Green) return false;

    auto sub = img(1, 4, 1, 3);
    if (!sub.isValid() || sub.lx() != 4 || sub.ly() != 3 || sub.stride() != 8) return false;
    sub.fillScreen(RGB565_Blue);
    if (img(1, 1) != RGB565_Blue || img(4, 3) != RGB565_Blue || img(0, 1) != bg) return false;

    auto cropped = img.getCrop(iBox2(-3, 2, -2, 2));
    if (!cropped.isValid() || cropped.lx() != 3 || cropped.ly() != 3 || cropped.stride() != 8) return false;
    cropped.clear(RGB565_Green);
    if (img(0, 0) != RGB565_Green || img(2, 2) != RGB565_Green) return false;

    Image<RGB565> croppedInPlace(stateBuf, 6, 4, 8);
    croppedInPlace.crop(iBox2(2, 5, 1, 2));
    if (!croppedInPlace.isValid() || croppedInPlace.lx() != 4 || croppedInPlace.ly() != 2 || croppedInPlace.stride() != 8) return false;
    croppedInPlace.clear(RGB565_Red);
    if (img(2, 1) != RGB565_Red || img(5, 2) != RGB565_Red) return false;

    int visited = 0;
    img.iterate([&](iVec2, RGB565& c)
        {
        c = bg;
        visited++;
        return true;
        }, iBox2(1, 3, 1, 2));
    if (visited != 6 || img(1, 1) != bg || img(3, 2) != bg) return false;
    const Image<RGB565>& cimg = img;
    int constVisited = 0;
    cimg.iterate([&](iVec2, const RGB565& c)
        {
        (void)c;
        constVisited++;
        return constVisited < 5;
        }, iBox2(1, 3, 1, 2));
    if (constVisited != 5) return false;

    img.setInvalid();
    return (!img.isValid() && img.lx() == 0 && img.ly() == 0 && img.stride() == 0);
}

bool transferFillValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 48, 36);
    img.clear(bg);
    img.drawRect(iBox2(6, 25, 5, 21), RGB565_White);
    const int fillStack = img.fill<1024>({ 10, 10 }, RGB565_White, RGB565_Red);
    if (fillStack < 0 || img(10, 10) != RGB565_Red || img(6, 5) != RGB565_White || img(3, 3) != bg) return false;
    const int fillStack2 = img.fill<1024>({ 0, 0 }, RGB565_Blue);
    if (fillStack2 < 0 || img(0, 0) != RGB565_Blue || img(10, 10) != RGB565_Red) return false;

    Image<RGB565> src(srcBuf, 12, 8);
    src.fillScreenHGradient(RGB565_Red, RGB565_Green);
    src.fillRect(iBox2(3, 8, 2, 5), RGB565_Blue);

    Image<RGB565> reduced(smallBuf, 6, 4);
    reduced.clear(bg);
    auto reducedView = reduced.copyReduceHalf(src);
    if (!reducedView.isValid() || reducedView.lx() != 6 || reducedView.ly() != 4) return false;
    if (countChangedImage(reduced, bg) == 0) return false;

    Image<RGB565> inplace(srcBuf, 12, 8);
    auto half = inplace.reduceHalf();
    if (!half.isValid() || half.lx() != 6 || half.ly() != 4) return false;

    Image<RGB565> back(smallBuf, 5, 4);
    back.clear(bg);
    src.blitBackward(back, { 2, 3 });
    if (back(0, 0) != src(2, 3) || back(4, 3) != src(6, 6)) return false;

    for (int i = 0; i < 7 * 5; i++) convertBuf32[i] = RGB32(40, 170, 210);
    Image<RGB32> src32(convertBuf32, 7, 5);
    Image<RGB565> copy(dstBuf, 14, 10);
    copy.clear(bg);
    copy.copyFrom(src32, -1.0f);
    if (countChangedImage(copy, bg) == 0) return false;

    copy.clear(bg);
    copy.blit(src32, { 2, 2 }, [](RGB32, RGB565) { return RGB565(24, 8, 24); });
    return copy(2, 2) == RGB565(24, 8, 24);
}

bool convertValidation()
{
    for (int i = 0; i < 5 * 4; i++) convertBuf32[i] = RGB32(12, 80, 220);
    Image<RGB32> im32(convertBuf32, 5, 4);
    auto im565 = im32.convert<RGB565>();
    if (!im565.isValid() || im565.lx() != 5 || im565.ly() != 4 || im565.stride() != 5) return false;
    if (im565(0, 0) != RGB565(RGB32(12, 80, 220))) return false;

    for (int i = 0; i < 4 * 3; i++) convertBufF[i] = RGBf(0.25f, 0.5f, 0.75f);
    Image<RGBf> imf(convertBufF, 4, 3);
    auto imf32 = imf.convert<RGB32>();
    return imf32.isValid() && imf32.lx() == 4 && imf32.ly() == 3 && imf32.stride() == 4;
}

RGB565 idColor(int id)
{
    return RGB565(RGB32((id * 53 + 31) & 255, (id * 97 + 43) & 255, (id * 151 + 59) & 255));
}

bool rotatedBlitValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> src(srcBuf, 4, 3);
    for (int y = 0; y < 3; y++)
        {
        for (int x = 0; x < 4; x++) src(x, y) = idColor(1 + x + 4 * y);
        }

    for (int angleIndex = 0; angleIndex < 4; angleIndex++)
        {
        const int angle = angleIndex * 90;
        Image<RGB565> dst(dstBuf, 8, 8);
        dst.clear(bg);
        dst.blitRotated(src, { 2, 2 }, angle, -1.0f);

        if (angle == 0)
            {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 4; x++)
                    if (dst(2 + x, 2 + y) != src(x, y)) return false;
            }
        else if (angle == 90)
            {
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 3; x++)
                    if (dst(2 + x, 2 + y) != src(y, 2 - x)) return false;
            }
        else if (angle == 180)
            {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 4; x++)
                    if (dst(2 + x, 2 + y) != src(3 - x, 2 - y)) return false;
            }
        else
            {
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 3; x++)
                    if (dst(2 + x, 2 + y) != src(3 - y, x)) return false;
            }

        if (dst(0, 0) != bg || dst(7, 7) != bg) return false;
        }

    for (int i = 0; i < 4 * 3; i++) convertBuf32[i] = RGB32(10, 20, 30);
    Image<RGB32> src32(convertBuf32, 4, 3);
    Image<RGB565> dst(dstBuf, 8, 8);
    dst.clear(bg);
    const RGB565 custom = RGB565(24, 8, 24);
    dst.blitRotated(src32, { 2, 2 }, 90, [&](RGB32, RGB565) { return custom; });
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 3; x++)
            if (dst(2 + x, 2 + y) != custom) return false;

    return true;
}

bool textLayoutValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> txt(dstBuf, 96, 70);
    txt.clear(bg);
    const char* text = "TGX text";

    const auto left = txt.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false);
    const auto center = txt.measureText(text, { 48, 24 }, font_tgx_OpenSans_10, BASELINE | CENTER, false, false);
    const auto right = txt.measureText(text, { 82, 24 }, font_tgx_OpenSans_10, BASELINE | RIGHT, false, false);
    if (left.minX != 14 || center.minX >= 48 || center.maxX <= 48 || right.maxX != 82) return false;

    const iVec2 anchorPos{ 48, 34 };
    const auto topLeft = txt.measureText(text, anchorPos, font_tgx_OpenSans_10, TOPLEFT, false, false);
    const auto topRight = txt.measureText(text, anchorPos, font_tgx_OpenSans_10, TOPRIGHT, false, false);
    const auto bottomRight = txt.measureText(text, anchorPos, font_tgx_OpenSans_10, BOTTOMRIGHT, false, false);
    const auto centered = txt.measureText(text, anchorPos, font_tgx_OpenSans_10, CENTER, false, false);
    if (topLeft.minX != anchorPos.x || topLeft.minY != anchorPos.y ||
        topRight.maxX != anchorPos.x || topRight.minY != anchorPos.y ||
        bottomRight.maxX != anchorPos.x || bottomRight.maxY != anchorPos.y ||
        centered.minX >= anchorPos.x || centered.maxX <= anchorPos.x ||
        centered.minY >= anchorPos.y || centered.maxY <= anchorPos.y) return false;

    const auto nowrap = txt.measureText("wrap wrap wrap wrap", { 70, 14 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false);
    const auto wrap = txt.measureText("wrap wrap wrap wrap", { 70, 14 }, font_tgx_OpenSans_10, BASELINE | LEFT, true, false);
    if ((wrap.maxY - wrap.minY) <= (nowrap.maxY - nowrap.minY)) return false;

    int lineAdvance = 0;
    txt.measureChar('A', { 14, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, &lineAdvance);
    const int lineHeight = txt.fontHeight(font_tgx_OpenSans_10);
    if (lineAdvance <= 0 || lineHeight <= 0) return false;

    const auto multilineLocal = txt.measureText("A\nA", { 14, 24 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false);
    const auto multilineZero = txt.measureText("A\nA", { 14, 24 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, true);
    if ((multilineLocal.maxY - multilineLocal.minY) < lineHeight ||
        multilineZero.minX != 0 || multilineZero.maxY != multilineLocal.maxY) return false;

    txt.clear(bg);
    auto cursorLocal = txt.drawTextEx("A\nA", { 14, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, false, false, RGB565_White, -1.0f);
    auto cursorZero = txt.drawTextEx("A\nA", { 14, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, false, true, RGB565_White, -1.0f);
    if (cursorLocal.x != 14 + lineAdvance || cursorLocal.y != 24 + lineHeight ||
        cursorZero.x != lineAdvance || cursorZero.y != cursorLocal.y) return false;

    int wideAdvance = 0;
    txt.measureChar('W', { 0, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, &wideAdvance);
    if (wideAdvance <= 0) return false;
    const int wrapStartX = txt.lx() - (wideAdvance / 2);
    auto wrapCursor = txt.drawTextEx("W", { wrapStartX, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, true, true, RGB565_White, -1.0f);
    if (wrapCursor.x != wideAdvance || wrapCursor.y != 24 + lineHeight) return false;

    const uint32_t before = countChangedImage(txt, bg);
    auto cursor = txt.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false, RGB565_White, -1.0f);
    const uint32_t after = countChangedImage(txt, bg);
    if (after <= before || cursor.x <= 14) return false;

    const auto deprecatedBox = txt.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, false);
    const auto modernBox = txt.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, false, false);
    if (deprecatedBox.minX != modernBox.minX || deprecatedBox.maxX != modernBox.maxX ||
        deprecatedBox.minY != modernBox.minY || deprecatedBox.maxY != modernBox.maxY) return false;

    txt.clear(bg);
    cursor = txt.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false, RGB565_White, 0.0f);
    if (countChangedImage(txt, bg) != 0 || cursor.x <= 14) return false;

    Image<RGB565> modern(srcBuf, 96, 70);
    Image<RGB565> legacy(dstBuf, 96, 70);
    modern.clear(bg);
    legacy.clear(bg);
    const auto modernCursor = modern.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, DEFAULT_TEXT_ANCHOR, false, false, RGB565_White, -1.0f);
    const auto legacyCursor = legacy.drawText(text, { 14, 24 }, RGB565_White, font_tgx_OpenSans_10, false, -1.0f);
    if (modernCursor != legacyCursor) return false;
    for (int y = 0; y < modern.ly(); y++)
        for (int x = 0; x < modern.lx(); x++)
            if (modern(x, y) != legacy(x, y)) return false;

    return true;
}

bool primitiveExactValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 16, 16);
    img.clear(bg);

    img.drawPixel({ -1, 2 }, RGB565_Red);
    img.drawPixel({ 16, 2 }, RGB565_Red);
    if (countChangedImage(img, bg) != 0) return false;

    img.drawFastHLine({ 2, 5 }, 5, RGB565_Red);
    if (countChangedImage(img, bg) != 5 || img(2, 5) != RGB565_Red || img(6, 5) != RGB565_Red || img(7, 5) != bg) return false;

    img.clear(bg);
    img.drawFastVLine({ 7, 1 }, 4, RGB565_Green);
    if (countChangedImage(img, bg) != 4 || img(7, 1) != RGB565_Green || img(7, 4) != RGB565_Green || img(7, 5) != bg) return false;

    img.clear(bg);
    img.drawLine({ 1, 1 }, { 6, 1 }, RGB565_Blue);
    if (countChangedImage(img, bg) != 6 || img(1, 1) != RGB565_Blue || img(6, 1) != RGB565_Blue) return false;

    img.clear(bg);
    img.drawSegment({ 1, 3 }, false, { 6, 3 }, false, RGB565_Yellow);
    if (countChangedImage(img, bg) != 4 || img(1, 3) != bg || img(2, 3) != RGB565_Yellow ||
        img(5, 3) != RGB565_Yellow || img(6, 3) != bg) return false;

    img.clear(bg);
    img.drawRect(iBox2(2, 6, 3, 7), RGB565_Red);
    if (countChangedImage(img, bg) != 16 || img(2, 3) != RGB565_Red || img(6, 7) != RGB565_Red || img(4, 5) != bg) return false;

    img.clear(bg);
    img.fillRect(iBox2(-2, 3, -1, 2), RGB565_Green);
    if (countChangedImage(img, bg) != 12 || img(0, 0) != RGB565_Green || img(3, 2) != RGB565_Green || img(4, 2) != bg) return false;

    img.clear(bg);
    img.fillThickRect(iBox2(2, 9, 2, 9), 2, RGB565_Blue, RGB565_Red);
    return img(2, 2) == RGB565_Red && img(3, 3) == RGB565_Red && img(4, 4) == RGB565_Blue && img(9, 9) == RGB565_Red;
}

bool clipBlitValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 10, 8);
    img.clear(bg);

    img.drawFastHLine({ -5, 2 }, 8, RGB565_Red);
    if (countChangedImage(img, bg) != 3 || img(0, 2) != RGB565_Red || img(2, 2) != RGB565_Red || img(3, 2) != bg) return false;

    img.clear(bg);
    img.drawFastVLine({ 4, -4 }, 7, RGB565_Green);
    if (countChangedImage(img, bg) != 3 || img(4, 0) != RGB565_Green || img(4, 2) != RGB565_Green || img(4, 3) != bg) return false;

    Image<RGB565> src(srcBuf, 4, 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++)
            src(x, y) = idColor(1 + x + 4 * y);

    img.clear(bg);
    img.blit(src, { -2, -1 }, -1.0f);
    if (countChangedImage(img, bg) != 4 || img(0, 0) != src(2, 1) || img(1, 0) != src(3, 1) ||
        img(0, 1) != src(2, 2) || img(1, 1) != src(3, 2) || img(2, 0) != bg) return false;

    Image<RGB565> masked(smallBuf, 3, 2);
    masked(0, 0) = RGB565_Red;
    masked(1, 0) = maskColor;
    masked(2, 0) = RGB565_Blue;
    masked(0, 1) = maskColor;
    masked(1, 1) = RGB565_Green;
    masked(2, 1) = RGB565_Red;

    img.clear(bg);
    img.blitMasked(masked, maskColor, { 8, 7 }, -1.0f);
    if (countChangedImage(img, bg) != 1 || img(8, 7) != RGB565_Red || img(9, 7) != bg) return false;

    img.clear(bg);
    const RGB565 custom = RGB565(2, 8, 12);
    img.blit(src, { 2, 2 }, [custom](RGB565, RGB565) { return custom; });
    return countChangedImage(img, bg) == 12 && img(2, 2) == custom && img(5, 4) == custom;
}

bool imageEdgeValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> nullBuffer(static_cast<RGB565*>(nullptr), 4, 4);
    if (nullBuffer.isValid()) return false;

    Image<RGB565> invalidStride(stateBuf, 8, 4, 7);
    if (invalidStride.isValid()) return false;

    Image<RGB565> img(dstBuf, 10, 8, 12);
    img.clear(bg);
    auto outsideCrop = img.getCrop(iBox2(20, 25, 1, 3));
    if (outsideCrop.isValid()) return false;

    auto crop1 = img.getCrop(iBox2(2, 8, 1, 6));
    auto crop2 = crop1.getCrop(iBox2(1, 3, 2, 4));
    if (!crop2.isValid() || crop2.lx() != 3 || crop2.ly() != 3 || crop2.stride() != 12) return false;
    crop2.clear(RGB565_Red);
    if (img(3, 3) != RGB565_Red || img(5, 5) != RGB565_Red || img(2, 3) != bg || img(6, 5) != bg) return false;

    int visited = 0;
    img.iterate([&](iVec2, RGB565& c)
        {
        c = RGB565_Green;
        visited++;
        return true;
        }, iBox2(-2, 2, -3, 1));
    if (visited != 6 || img(0, 0) != RGB565_Green || img(2, 1) != RGB565_Green) return false;

    Image<RGB565> srcOdd(srcBuf, 5, 3);
    srcOdd.clear(RGB565_Blue);
    Image<RGB565> dstOdd(smallBuf, 2, 1);
    dstOdd.clear(bg);
    auto reducedOdd = dstOdd.copyReduceHalf(srcOdd);
    if (!reducedOdd.isValid() || reducedOdd.lx() != 2 || reducedOdd.ly() != 1 ||
        dstOdd(0, 0) != RGB565_Blue || dstOdd(1, 0) != RGB565_Blue) return false;

    Image<RGB565> tooSmall(stateBuf, 1, 1);
    tooSmall.clear(bg);
    if (tooSmall.copyReduceHalf(srcOdd).isValid()) return false;

    Image<RGB565> oneCol(srcBuf, 1, 5);
    oneCol.clear(RGB565_Red);
    Image<RGB565> oneColDst(smallBuf, 1, 2);
    oneColDst.clear(bg);
    auto oneColReduced = oneColDst.copyReduceHalf(oneCol);
    if (!oneColReduced.isValid() || oneColReduced.lx() != 1 || oneColReduced.ly() != 2 ||
        oneColDst(0, 0) != RGB565_Red || oneColDst(0, 1) != RGB565_Red) return false;

    Image<RGB565> oneRow(srcBuf, 5, 1);
    oneRow.clear(RGB565_Green);
    Image<RGB565> oneRowDst(smallBuf, 2, 1);
    oneRowDst.clear(bg);
    auto oneRowReduced = oneRowDst.copyReduceHalf(oneRow);
    return oneRowReduced.isValid() && oneRowReduced.lx() == 2 && oneRowReduced.ly() == 1 &&
           oneRowDst(0, 0) == RGB565_Green && oneRowDst(1, 0) == RGB565_Green;
}

bool gradientOpacityValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 8, 6);
    img.clear(bg);

    img.fillScreenHGradient(RGB565_Red, RGB565_Blue);
    if (img(0, 0) != RGB565_Red || img(7, 0) != RGB565_Blue || img(0, 5) != RGB565_Red || img(7, 5) != RGB565_Blue) return false;

    img.clear(bg);
    img.fillScreenVGradient(RGB565_Green, RGB565_Blue);
    if (img(0, 0) != RGB565_Green || img(7, 0) != RGB565_Green || img(0, 5) != RGB565_Blue || img(7, 5) != RGB565_Blue) return false;

    img.clear(bg);
    img.fillRectHGradient(iBox2(-2, 3, 1, 2), RGB565_Red, RGB565_Green);
    if (img(0, 1) != RGB565_Red || img(3, 2) != RGB565_Green || img(4, 1) != bg) return false;

    img.clear(bg);
    img.fillRectVGradient(iBox2(2, 5, -1, 3), RGB565_Red, RGB565_Blue);
    if (img(2, 0) != RGB565_Red || img(5, 3) != RGB565_Blue || img(1, 0) != bg) return false;

    img.clear(bg);
    img.drawFastHLine({ 1, 1 }, 4, RGB565_Red, 0.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.drawFastVLine({ 2, 0 }, 4, RGB565_Green, 1.0f);
    if (countChangedImage(img, bg) != 4 || img(2, 0) != RGB565_Green || img(2, 3) != RGB565_Green) return false;

    img.fillRect(iBox2(0, 3, 4, 5), RGB565_Blue, 0.0f);
    return countChangedImage(img, bg) == 4;
}

bool lineRectExactValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 16, 16);
    img.clear(bg);

    img.drawLine({ 1, 1 }, { 6, 6 }, RGB565_Red);
    if (countChangedImage(img, bg) != 6) return false;
    for (int i = 1; i <= 6; i++)
        {
        if (img(i, i) != RGB565_Red) return false;
        }
    if (img(2, 3) != bg || img(6, 5) != bg) return false;

    img.clear(bg);
    img.drawLine({ 6, 6 }, { 1, 1 }, RGB565_Red);
    if (countChangedImage(img, bg) != 6 || img(1, 1) != RGB565_Red || img(6, 6) != RGB565_Red) return false;

    img.clear(bg);
    img.drawLine({ 4, 4 }, { 4, 4 }, RGB565_Green);
    if (countChangedImage(img, bg) != 1 || img(4, 4) != RGB565_Green) return false;

    img.clear(bg);
    img.drawSegment({ 1, 1 }, false, { 6, 6 }, true, RGB565_Blue);
    if (countChangedImage(img, bg) != 5 || img(1, 1) != bg || img(2, 2) != RGB565_Blue || img(6, 6) != RGB565_Blue) return false;

    img.clear(bg);
    img.drawLine({ -3, 7 }, { 3, 7 }, RGB565_Red);
    if (countChangedImage(img, bg) != 4 || img(0, 7) != RGB565_Red || img(3, 7) != RGB565_Red || img(4, 7) != bg) return false;

    img.clear(bg);
    img.drawRect(iBox2(2, 7, 4, 4), RGB565_Green);
    if (countChangedImage(img, bg) != 6 || img(2, 4) != RGB565_Green || img(7, 4) != RGB565_Green || img(8, 4) != bg) return false;

    img.clear(bg);
    img.drawRect(iBox2(5, 5, 2, 8), RGB565_Blue);
    if (countChangedImage(img, bg) != 7 || img(5, 2) != RGB565_Blue || img(5, 8) != RGB565_Blue || img(6, 5) != bg) return false;

    img.clear(bg);
    img.drawThickRect(iBox2(2, 3, 2, 6), 3, RGB565_Red);
    if (countChangedImage(img, bg) != 10 || img(2, 2) != RGB565_Red || img(3, 6) != RGB565_Red) return false;

    img.clear(bg);
    img.drawLine({ 1, 1 }, { 6, 6 }, RGB565_Red, 0.0f);
    return countChangedImage(img, bg) == 0;
}

bool triangleQuadValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 16, 16);
    img.clear(bg);

    img.fillQuad({ 2, 3 }, { 8, 3 }, { 8, 7 }, { 2, 7 }, RGB565_Red);
    if (countChangedImage(img, bg) != 35 || img(2, 3) != RGB565_Red || img(8, 7) != RGB565_Red ||
        img(5, 5) != RGB565_Red || img(9, 5) != bg || img(5, 8) != bg) return false;

    img.clear(bg);
    img.drawQuad({ 2, 3 }, { 8, 3 }, { 8, 7 }, { 2, 7 }, RGB565_Green);
    if (img(2, 3) != RGB565_Green || img(8, 3) != RGB565_Green || img(8, 7) != RGB565_Green ||
        img(2, 7) != RGB565_Green || img(5, 5) != bg) return false;

    img.clear(bg);
    img.fillTriangle({ 2, 2 }, { 9, 2 }, { 2, 9 }, RGB565_Blue, RGB565_Yellow);
    if (img(2, 2) != RGB565_Yellow || img(9, 2) != RGB565_Yellow || img(2, 9) != RGB565_Yellow ||
        img(3, 4) != RGB565_Blue || img(10, 9) != bg) return false;

    const uint32_t triHash = imageHash();
    img.clear(bg);
    img.fillTriangle({ 2, 2 }, { 2, 9 }, { 9, 2 }, RGB565_Blue, RGB565_Yellow);
    if (imageHash() != triHash) return false;

    img.clear(bg);
    img.drawTriangle({ 2, 2 }, { 9, 2 }, { 2, 9 }, RGB565_Red);
    if (img(2, 2) != RGB565_Red || img(9, 2) != RGB565_Red || img(2, 9) != RGB565_Red ||
        img(3, 4) != bg || countChangedImage(img, bg) == 0) return false;

    img.clear(bg);
    img.fillQuad({ -3, -2 }, { 4, -2 }, { 4, 4 }, { -3, 4 }, RGB565_Green);
    return countChangedImage(img, bg) == 25 && img(0, 0) == RGB565_Green && img(4, 4) == RGB565_Green && img(5, 4) == bg;
}

bool circleRoundValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 20, 18);
    img.clear(bg);

    img.drawCircle({ 5, 5 }, 1, RGB565_Red);
    if (countChangedImage(img, bg) != 4 || img(4, 5) != RGB565_Red || img(6, 5) != RGB565_Red ||
        img(5, 4) != RGB565_Red || img(5, 6) != RGB565_Red || img(5, 5) != bg) return false;

    img.clear(bg);
    img.fillCircle({ 5, 5 }, 1, RGB565_Green, RGB565_Red);
    if (countChangedImage(img, bg) != 5 || img(5, 5) != RGB565_Green || img(4, 5) != RGB565_Red ||
        img(6, 5) != RGB565_Red || img(5, 4) != RGB565_Red || img(5, 6) != RGB565_Red) return false;

    img.clear(bg);
    img.drawCircle({ 8, 8 }, 4, RGB565_Blue);
    if (img(4, 8) != RGB565_Blue || img(12, 8) != RGB565_Blue ||
        img(8, 4) != RGB565_Blue || img(8, 12) != RGB565_Blue ||
        img(8, 8) != bg || img(3, 8) != bg) return false;

    img.clear(bg);
    img.fillCircle({ 8, 8 }, 4, RGB565_Green, RGB565_Red);
    if (img(8, 8) != RGB565_Green || img(4, 8) != RGB565_Red || img(12, 8) != RGB565_Red ||
        img(8, 4) != RGB565_Red || img(8, 12) != RGB565_Red || img(3, 8) != bg) return false;

    img.clear(bg);
    img.drawCircle({ 1, 1 }, 4, RGB565_Blue);
    if (img(1, 5) != RGB565_Blue || img(5, 1) != RGB565_Blue || img(6, 1) != bg) return false;

    img.clear(bg);
    img.drawEllipse({ 9, 8 }, { 5, 3 }, RGB565_Yellow);
    if (img(4, 8) != RGB565_Yellow || img(14, 8) != RGB565_Yellow ||
        img(9, 5) != RGB565_Yellow || img(9, 11) != RGB565_Yellow ||
        img(9, 8) != bg) return false;

    img.clear(bg);
    img.fillEllipse({ 9, 8 }, { 5, 3 }, RGB565_Green, RGB565_Red);
    if (img(9, 8) != RGB565_Green || img(4, 8) != RGB565_Red || img(14, 8) != RGB565_Red ||
        img(9, 5) != RGB565_Red || img(9, 11) != RGB565_Red || img(3, 8) != bg) return false;

    img.clear(bg);
    img.drawRoundRect(iBox2(2, 15, 2, 13), 3, RGB565_Blue);
    if (img(5, 2) != RGB565_Blue || img(12, 2) != RGB565_Blue ||
        img(2, 5) != RGB565_Blue || img(15, 10) != RGB565_Blue ||
        img(8, 8) != bg || img(2, 2) != bg) return false;

    img.clear(bg);
    img.fillRoundRect(iBox2(2, 15, 2, 13), 3, RGB565_Green);
    if (img(8, 8) != RGB565_Green || img(5, 2) != RGB565_Green ||
        img(2, 5) != RGB565_Green || img(2, 2) != bg) return false;

    img.clear(bg);
    img.fillCircle({ 8, 8 }, 4, RGB565_Red, RGB565_Blue, 0.0f);
    img.fillEllipse({ 9, 8 }, { 5, 3 }, RGB565_Red, RGB565_Blue, 0.0f);
    img.fillRoundRect(iBox2(2, 15, 2, 13), 3, RGB565_Red, 0.0f);
    return countChangedImage(img, bg) == 0;
}

bool aaPrimitivesValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 48, 48);
    img.clear(bg);

    img.drawLineAA({ 5.2f, 5.4f }, { 40.7f, 30.8f }, RGB565_Red);
    if (countChangedImage(img, bg) < 25 || img(0, 0) != bg || img(47, 47) != bg) return false;

    img.clear(bg);
    img.drawLineAA({ -12.5f, 10.2f }, { 20.2f, 10.2f }, RGB565_Green);
    if (countChangedImage(img, bg) < 15 || img(47, 10) != bg) return false;

    img.clear(bg);
    img.drawLineAA({ 5.2f, 5.4f }, { 40.7f, 30.8f }, RGB565_Red, 0.0f);
    img.drawThickLineAA({ 8.0f, 10.0f }, { 42.0f, 10.0f }, 6.0f, END_ROUNDED, END_ROUNDED, RGB565_Red, 0.0f);
    img.drawWedgeLineAA({ 8.0f, 20.0f }, { 42.0f, 24.0f }, 2.0f, END_ROUNDED, 8.0f, END_ROUNDED, RGB565_Red, 0.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.clear(bg);
    img.drawThickLineAA({ 7.0f, 12.0f }, { 41.0f, 12.0f }, 6.0f, END_ROUNDED, END_ARROW_2, RGB565_Blue);
    if (countChangedImage(img, bg) < 150 || img(24, 12) != RGB565_Blue || img(24, 24) != bg) return false;

    img.clear(bg);
    img.drawWedgeLineAA({ 7.0f, 24.0f }, { 41.0f, 28.0f }, 2.0f, END_ROUNDED, 10.0f, END_ROUNDED, RGB565_Cyan);
    if (countChangedImage(img, bg) < 120 || img(0, 0) != bg || img(47, 47) != bg) return false;

    img.clear(bg);
    img.fillRectAA(fBox2(8.35f, 28.75f, 6.25f, 19.6f), RGB565_Green);
    if (countChangedImage(img, bg) < 200 || img(18, 12) != RGB565_Green || img(6, 12) != bg || img(31, 12) != bg) return false;

    img.clear(bg);
    img.drawThickRectAA(fBox2(8.0f, 40.0f, 6.0f, 20.0f), 3.0f, RGB565_Yellow);
    if (countChangedImage(img, bg) < 80 || img(4, 13) != bg || img(44, 13) != bg) return false;

    img.clear(bg);
    img.fillThickRectAA(fBox2(8.0f, 40.0f, 6.0f, 20.0f), 3.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 300 || img(24, 13) != RGB565_Blue || img(4, 13) != bg) return false;

    img.clear(bg);
    img.drawRoundRectAA(fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, RGB565_Cyan);
    if (countChangedImage(img, bg) < 90 || img(8, 5) != bg || img(24, 15) != bg) return false;

    img.clear(bg);
    img.fillRoundRectAA(fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, RGB565_Green);
    if (countChangedImage(img, bg) < 450 || img(24, 15) != RGB565_Green || img(8, 5) != bg) return false;

    img.clear(bg);
    img.drawThickRoundRectAA(fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, 3.0f, RGB565_Yellow);
    if (countChangedImage(img, bg) < 170 || img(24, 15) != bg) return false;

    img.clear(bg);
    img.fillThickRoundRectAA(fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, 3.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 450 || img(24, 15) != RGB565_Blue) return false;

    img.clear(bg);
    img.drawCircleAA({ 24.0f, 24.0f }, 10.0f, RGB565_Red);
    if (countChangedImage(img, bg) < 55 || img(24, 24) != bg) return false;

    img.clear(bg);
    img.fillCircleAA({ 24.0f, 24.0f }, 10.0f, RGB565_Green);
    if (countChangedImage(img, bg) < 300 || img(24, 24) != RGB565_Green || img(12, 24) != bg) return false;

    img.clear(bg);
    img.fillThickCircleAA({ 24.0f, 24.0f }, 12.0f, 4.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 350 || img(24, 24) != RGB565_Blue) return false;

    img.clear(bg);
    img.drawEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, RGB565_Yellow);
    if (countChangedImage(img, bg) < 60 || img(24, 24) != bg) return false;

    img.clear(bg);
    img.fillEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, RGB565_Cyan);
    if (countChangedImage(img, bg) < 280 || img(24, 24) != RGB565_Cyan || img(8, 24) != bg) return false;

    img.clear(bg);
    img.fillThickEllipseAA({ 24.0f, 24.0f }, { 15.0f, 8.0f }, 3.0f, RGB565_Green, RGB565_Red);
    if (countChangedImage(img, bg) < 300 || img(24, 24) != RGB565_Green) return false;

    img.clear(bg);
    img.drawTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, RGB565_Red);
    if (countChangedImage(img, bg) < 80 || img(23, 28) != bg) return false;

    img.clear(bg);
    img.fillTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, RGB565_Green);
    if (countChangedImage(img, bg) < 450 || img(23, 28) != RGB565_Green || img(3, 3) != bg) return false;

    img.clear(bg);
    img.fillThickTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, 3.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 450 || img(23, 28) != RGB565_Blue) return false;

    img.clear(bg);
    img.drawQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, RGB565_Yellow);
    if (countChangedImage(img, bg) < 100 || img(24, 24) != bg) return false;

    img.clear(bg);
    img.fillQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, RGB565_Cyan);
    if (countChangedImage(img, bg) < 700 || img(24, 24) != RGB565_Cyan || img(3, 3) != bg) return false;

    img.clear(bg);
    img.fillThickQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, 3.0f, RGB565_Green, RGB565_Red);
    if (countChangedImage(img, bg) < 700 || img(24, 24) != RGB565_Green || img(3, 3) != bg) return false;

    img.clear(bg);
    img.fillRectAA(fBox2(8.35f, 28.75f, 6.25f, 19.6f), RGB565_Red, 0.0f);
    img.fillRoundRectAA(fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, RGB565_Red, 0.0f);
    img.fillCircleAA({ 24.0f, 24.0f }, 10.0f, RGB565_Red, 0.0f);
    img.fillEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, RGB565_Red, 0.0f);
    img.fillTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, RGB565_Red, 0.0f);
    img.fillQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, RGB565_Red, 0.0f);
    return countChangedImage(img, bg) == 0;
}

bool curvePolygonValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> img(dstBuf, 72, 72);
    Image<RGB565> ref(srcBuf, 72, 72);
    img.clear(bg);
    ref.clear(bg);

    fVec2 polyline[] = { { 8.5f, 55.0f }, { 18.0f, 14.5f }, { 34.0f, 50.5f }, { 49.5f, 15.0f }, { 63.0f, 57.0f } };
    img.drawPolylineAA(5, polyline, RGB565_Red);
    FPointSource polylineSrc{ polyline, 5, 0 };
    ref.drawPolylineAA(polylineSrc, RGB565_Red);
    if (countChangedImage(img, bg) < 60 || img(0, 0) != bg || img(71, 71) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.drawThickPolylineAA(5, polyline, 5.0f, END_ROUNDED, END_ARROW_2, RGB565_Blue);
    FPointSource thickPolylineSrc{ polyline, 5, 0 };
    ref.drawThickPolylineAA(thickPolylineSrc, 5.0f, END_ROUNDED, END_ARROW_2, RGB565_Blue);
    if (countChangedImage(img, bg) < 320 || img(18, 15) != RGB565_Blue || img(0, 0) != bg || !imageEquals(img, ref)) return false;

    fVec2 polygon[] = { { 10.0f, 11.0f }, { 58.0f, 10.0f }, { 63.0f, 42.0f }, { 36.0f, 61.0f }, { 8.0f, 43.0f } };
    img.clear(bg);
    ref.clear(bg);
    img.drawPolygonAA(5, polygon, RGB565_Yellow);
    FPointSource polygonSrc{ polygon, 5, 0 };
    ref.drawPolygonAA(polygonSrc, RGB565_Yellow);
    if (countChangedImage(img, bg) < 110 || img(35, 34) != bg || img(1, 1) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.fillPolygonAA(5, polygon, RGB565_Green);
    FPointSource fillPolygonSrc{ polygon, 5, 0 };
    ref.fillPolygonAA(fillPolygonSrc, RGB565_Green);
    if (countChangedImage(img, bg) < 1600 || img(35, 34) != RGB565_Green || img(1, 1) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.drawThickPolygonAA(5, polygon, 4.0f, RGB565_Cyan);
    FPointSource thickPolygonSrc{ polygon, 5, 0 };
    ref.drawThickPolygonAA(thickPolygonSrc, 4.0f, RGB565_Cyan);
    if (countChangedImage(img, bg) < 400 || img(35, 34) != bg || img(1, 1) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.fillThickPolygonAA(5, polygon, 4.0f, RGB565_Blue, RGB565_Red);
    FPointSource fillThickPolygonSrc{ polygon, 5, 0 };
    ref.fillThickPolygonAA(fillThickPolygonSrc, 4.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 1700 || img(35, 34) != RGB565_Blue || img(1, 1) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    img.drawQuadBezier({ 6, 54 }, { 66, 54 }, { 36, 4 }, 1.0f, true, RGB565_Red);
    if (countChangedImage(img, bg) < 55 || img(0, 0) != bg || img(36, 54) != bg) return false;

    img.clear(bg);
    img.drawCubicBezier({ 6, 52 }, { 66, 52 }, { 10, 2 }, { 62, 70 }, true, RGB565_Yellow);
    if (countChangedImage(img, bg) < 55 || img(0, 0) != bg) return false;

    iVec2 splineI[] = { { 8, 58 }, { 20, 16 }, { 36, 10 }, { 58, 26 }, { 52, 56 }, { 30, 62 } };
    img.clear(bg);
    img.drawQuadSpline(6, splineI, true, RGB565_Green);
    if (countChangedImage(img, bg) < 70 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawCubicSpline(6, splineI, true, RGB565_Blue);
    if (countChangedImage(img, bg) < 70 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawClosedSpline(6, splineI, RGB565_Cyan);
    if (countChangedImage(img, bg) < 70 || img(0, 0) != bg) return false;

    fVec2 splineF[] = { { 8.0f, 58.0f }, { 20.0f, 16.0f }, { 36.0f, 10.0f }, { 58.0f, 26.0f }, { 52.0f, 56.0f }, { 30.0f, 62.0f } };
    img.clear(bg);
    img.drawThickQuadBezierAA({ 6.0f, 54.0f }, { 66.0f, 54.0f }, { 36.0f, 4.0f }, 1.0f, 5.0f, END_ROUNDED, END_ROUNDED, RGB565_Red);
    if (countChangedImage(img, bg) < 300 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawThickCubicBezierAA({ 6.0f, 52.0f }, { 66.0f, 52.0f }, { 10.0f, 2.0f }, { 62.0f, 70.0f }, 5.0f, END_ROUNDED, END_ROUNDED, RGB565_Yellow);
    if (countChangedImage(img, bg) < 300 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawThickQuadSplineAA(6, splineF, 4.0f, END_ROUNDED, END_ARROW_2, RGB565_Green);
    if (countChangedImage(img, bg) < 400 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawThickCubicSplineAA(6, splineF, 4.0f, END_ROUNDED, END_ROUNDED, RGB565_Blue);
    if (countChangedImage(img, bg) < 400 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawThickClosedSplineAA(6, splineF, 4.0f, RGB565_Cyan);
    if (countChangedImage(img, bg) < 400 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.fillClosedSplineAA(6, splineF, RGB565_Green);
    if (countChangedImage(img, bg) < 850 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.fillThickClosedSplineAA(6, splineF, 5.0f, RGB565_Blue, RGB565_Red);
    if (countChangedImage(img, bg) < 1000 || img(0, 0) != bg) return false;

    img.clear(bg);
    img.drawPolylineAA(5, polyline, RGB565_Red, 0.0f);
    img.drawThickPolylineAA(5, polyline, 5.0f, END_ROUNDED, END_ROUNDED, RGB565_Red, 0.0f);
    img.fillPolygonAA(5, polygon, RGB565_Red, 0.0f);
    img.fillThickPolygonAA(5, polygon, 4.0f, RGB565_Red, RGB565_Blue, 0.0f);
    img.drawThickQuadBezierAA({ 6.0f, 54.0f }, { 66.0f, 54.0f }, { 36.0f, 4.0f }, 1.0f, 5.0f, END_ROUNDED, END_ROUNDED, RGB565_Red, 0.0f);
    img.fillClosedSplineAA(6, splineF, RGB565_Red, 0.0f);
    return countChangedImage(img, bg) == 0;
}

void fillTexture(Image<RGB565>& tex, RGB565 color)
{
    for (int y = 0; y < tex.ly(); y++)
        {
        for (int x = 0; x < tex.lx(); x++) tex(x, y) = color;
        }
}

bool texturedEdgeValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    const RGB565 red = RGB565_Red;
    const RGB565 green = RGB565_Green;
    const RGB565 blue = RGB565_Blue;
    const RGB565 yellow = RGB565_Yellow;
    const RGB565 magenta = RGB565(31, 0, 31);
    const RGB565 custom = RGB565(15, 11, 24);

    Image<RGB565> img(dstBuf, 64, 64);
    Image<RGB565> ref(srcBuf, 64, 64);
    Image<RGB565> tex(smallBuf, 8, 8);
    img.clear(bg);
    ref.clear(bg);
    fillTexture(tex, red);

    img.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    const uint32_t plainTriangleCount = countChangedImage(img, bg);
    if (plainTriangleCount < 600 || img(12, 12) != red || img(0, 0) != bg || img(63, 63) != bg) return false;

    img.clear(bg);
    img.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { -20, 10 }, { 30, 8 }, { 4, 58 }, -1.0f);
    if (countChangedImage(img, bg) < 400 || img(63, 0) != bg || img(63, 63) != bg) return false;

    img.clear(bg);
    img.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 },
        [custom](RGB565, RGB565) { return custom; });
    if (countChangedImage(img, bg) < 600 || img(12, 12) != custom || img(0, 0) != bg) return false;

    img.clear(bg);
    fillTexture(tex, magenta);
    img.drawTexturedMaskedTriangle(tex, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.clear(bg);
    fillTexture(tex, green);
    for (int y = 2; y < 6; y++)
        {
        for (int x = 2; x < 6; x++) tex(x, y) = magenta;
        }
    img.drawTexturedMaskedTriangle(tex, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    const uint32_t mixedMaskCount = countChangedImage(img, bg);
    if (mixedMaskCount == 0 || mixedMaskCount >= plainTriangleCount || img(0, 0) != bg) return false;

    img.clear(bg);
    fillTexture(tex, red);
    img.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, -1.0f);
    if (countChangedImage(img, bg) < 600 || img(0, 0) != bg) return false;

    img.clear(bg);
    fillTexture(tex, magenta);
    img.drawTexturedGradientMaskedTriangle(tex, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, -1.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.clear(bg);
    ref.clear(bg);
    fillTexture(tex, red);
    img.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, -1.0f);
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if (countChangedImage(img, bg) < 1100 || img(20, 20) != red || img(4, 4) != bg || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 },
        [custom](RGB565, RGB565) { return custom; });
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 },
        [custom](RGB565, RGB565) { return custom; });
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 },
        [custom](RGB565, RGB565) { return custom; });
    if (countChangedImage(img, bg) < 1100 || img(20, 20) != custom || !imageEquals(img, ref)) return false;

    img.clear(bg);
    ref.clear(bg);
    img.drawTexturedGradientQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, -1.0f);
    ref.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, red, green, blue, -1.0f);
    ref.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 }, red, blue, yellow, -1.0f);
    if (countChangedImage(img, bg) < 1100 || !imageEquals(img, ref)) return false;

    img.clear(bg);
    fillTexture(tex, magenta);
    img.drawTexturedMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.clear(bg);
    img.drawTexturedGradientMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, -1.0f);
    if (countChangedImage(img, bg) != 0) return false;

    Image<RGB565> invalidSrc;
    img.clear(bg);
    img.drawTexturedTriangle(invalidSrc, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    img.drawTexturedQuad(invalidSrc, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if (countChangedImage(img, bg) != 0) return false;

    img.clear(bg);
    fillTexture(tex, red);
    img.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, 0.0f);
    img.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, 0.0f);
    img.drawTexturedMaskedTriangle(tex, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, 0.0f);
    img.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, 0.0f);
    img.drawTexturedGradientQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, 0.0f);
    img.drawTexturedMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, 0.0f);
    img.drawTexturedGradientMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, 0.0f);
    return countChangedImage(img, bg) == 0;
}

bool fillFailureValidation()
{
    Image<RGB565> img(dstBuf, 12, 8);
    img.clear(RGB565(1, 2, 4));
    return img.fill<6>({ 0, 0 }, RGB565_Red) == -1;
}

bool gfxTextValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> txt(dstBuf, 96, 70);
    txt.clear(bg);
    if (txt.fontHeight(gfx_font) != 10) return false;

    int advance = 0;
    const auto box = txt.measureChar('A', { 8, 20 }, gfx_font, BASELINE | LEFT, &advance);
    if (advance != 8 || box.minX != 8 || box.maxX != 15 || box.minY != 12 || box.maxY != 19) return false;

    auto cursor = txt.drawChar('A', { 8, 20 }, gfx_font, RGB565_Red, -1.0f);
    if (cursor.x != 16 || countChangedImage(txt, bg) != 64) return false;

    const auto textBox = txt.measureText("ABC", { 48, 38 }, gfx_font, BASELINE | CENTER, false, false);
    if ((textBox.maxX - textBox.minX + 1) != 24 || textBox.minX > 48 || textBox.maxX < 48) return false;

    const uint32_t before = countChangedImage(txt, bg);
    cursor = txt.drawTextEx("ABC", { 48, 38 }, gfx_font, BASELINE | CENTER, false, false, RGB565_Green, 0.7f);
    if (countChangedImage(txt, bg) <= before || cursor.x <= 48) return false;

    const auto deprecatedCursor = txt.drawText("ABC", { 8, 60 }, RGB565_Yellow, gfx_font, false, -1.0f);
    const auto modernCursor = txt.drawText("ABC", { 8, 60 }, gfx_font, RGB565_Yellow, -1.0f);
    return deprecatedCursor == modernCursor;
}

bool textStressValidation()
{
    const RGB565 bg = RGB565(1, 2, 4);
    Image<RGB565> txt(dstBuf, 96, 72);
    Image<RGB565> zero(srcBuf, 96, 72);
    txt.clear(bg);
    zero.clear(bg);

    const char* paragraph = "TGX wraps text near edges.\nSecond line keeps going.";
    const auto nowrapBox = txt.measureText(paragraph, { 8, 16 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false);
    const auto wrapBox = txt.measureText(paragraph, { 8, 16 }, font_tgx_OpenSans_10, BASELINE | LEFT, true, true);
    if ((wrapBox.maxY - wrapBox.minY) <= (nowrapBox.maxY - nowrapBox.minY) || wrapBox.minX != 0) return false;

    const auto cursor = txt.drawTextEx(paragraph, { 8, 16 }, font_tgx_OpenSans_10, BASELINE | LEFT, true, true, RGB565_White, -1.0f);
    const uint32_t wrappedCount = countChangedImage(txt, bg);
    if (wrappedCount < 200 || cursor.y <= 16 || cursor.x < 0 || cursor.x >= txt.lx()) return false;

    const auto zeroCursor = zero.drawTextEx(paragraph, { 8, 16 }, font_tgx_OpenSans_10, BASELINE | LEFT, true, true, RGB565_White, 0.0f);
    if (zeroCursor != cursor || countChangedImage(zero, bg) != 0) return false;

    const uint32_t beforeEdge = countChangedImage(txt, bg);
    const auto edgeBox = txt.measureText("EDGE", { 94, 70 }, font_tgx_OpenSans_Bold_14, BOTTOMRIGHT, false, false);
    if (edgeBox.maxX != 94 || edgeBox.maxY != 70 || edgeBox.minX < 0 || edgeBox.minY < 0) return false;
    txt.drawTextEx("EDGE", { 94, 70 }, font_tgx_OpenSans_Bold_14, BOTTOMRIGHT, false, false, RGB565_Cyan, 0.75f);
    if (countChangedImage(txt, bg) <= beforeEdge) return false;

    const char* gfxText = "ABC\nBCA\nCAB";
    const auto gfxBox = txt.measureText(gfxText, { 48, 48 }, gfx_font, CENTER, false, false);
    if (gfxBox.minX >= 48 || gfxBox.maxX <= 48 || gfxBox.minY >= 48 || gfxBox.maxY <= 48) return false;

    const uint32_t beforeGfx = countChangedImage(txt, bg);
    const auto gfxCursor = txt.drawTextEx(gfxText, { 48, 48 }, gfx_font, CENTER, false, false, RGB565_Yellow, -1.0f);
    if (countChangedImage(txt, bg) <= beforeGfx + 120 || gfxCursor.y <= 48) return false;

    zero.clear(bg);
    const auto gfxZeroCursor = zero.drawTextEx(gfxText, { 48, 48 }, gfx_font, CENTER, false, false, RGB565_Yellow, 0.0f);
    if (gfxZeroCursor != gfxCursor || countChangedImage(zero, bg) != 0) return false;

    const uint32_t beforeMix = countChangedImage(txt, bg);
    txt.drawTextEx("ABC", { 4, 68 }, gfx_font, BASELINE | LEFT, false, false, RGB565_Red, 0.55f);
    txt.drawTextEx("mix", { 34, 68 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false, RGB565_Cyan, 0.65f);
    return countChangedImage(txt, bg) > beforeMix;
}

void drawPixels()
{
    title("pixels / subimages");
    for (int i = 0; i < 120; i++)
        {
        im.drawPixel({ 8 + (i % 80), 35 + (i % 45) }, RGB565(31, 7, 5));
        im.drawPixelf({ 120.4f + float(i % 82), 35.6f + float(i / 3) }, RGB565(5, 38, 31), 0.65f);
        }
    auto sub = im(34, 126, 128, 214);
    sub.fillScreenVGradient(RGB565_Blue, RGB565_Orange);
    sub.drawRect(sub.imageBox(), RGB565_White);
}

void drawLinesRects()
{
    title("lines / rects");
    im.drawFastHLine({ 12, 36 }, 280, RGB565_Yellow);
    im.drawFastVLine({ 20, 42 }, 170, RGB565_Cyan);
    im.drawLine({ 12, 218 }, { 306, 42 }, RGB565_Red);
    im.drawLineAA({ 18.2f, 150.4f }, { 304.7f, 82.2f }, RGB565_White, 0.75f);
    im.drawThickLineAA({ 36.0f, 198.0f }, { 284.0f, 202.0f }, 8.0f, END_ROUNDED, END_ARROW_2, RGB565_Orange, 0.85f);
    im.drawWedgeLineAA({ 54.0f, 82.0f }, { 260.0f, 128.0f }, 2.0f, END_ROUNDED, 13.0f, END_ROUNDED, RGB565_Purple, 0.7f);
    im.fillRectHGradient(iBox2(54, 188, 96, 135), RGB565_Blue, RGB565_Yellow, 0.75f);
    im.fillRectVGradient(iBox2(210, 250, 142, 178), RGB565_Red, RGB565_Blue, 0.7f);
    im.drawThickRect(iBox2(204, 296, 62, 134), 4, RGB565_White, 0.8f);
    im.fillRectAA(fBox2(60.4f, 182.7f, 148.3f, 178.4f), RGB565_Green, 0.45f);
    im.fillThickRect(iBox2(120, 176, 154, 184), 3, RGB565_Blue, RGB565_White, 0.55f);
    im.drawThickRectAA(fBox2(118.0f, 184.0f, 190.0f, 220.0f), 3.0f, RGB565_Yellow, 0.75f);
    im.fillThickRectAA(fBox2(190.0f, 244.0f, 190.0f, 220.0f), 3.0f, RGB565_Green, RGB565_White, 0.55f);
    im.drawWideLine({ 34.0f, 62.0f }, { 104.0f, 62.0f }, 4.0f, RGB565_White, 0.7f);
    im.drawWedgeLine({ 214.0f, 202.0f }, { 292.0f, 186.0f }, 2.0f, 8.0f, RGB565_Cyan, 0.7f);
    im.drawSpot({ 284.0f, 164.0f }, 11.0f, RGB565_Red, 0.65f);
}

void drawCircles()
{
    title("circles / ellipses / arcs");
    im.drawCircle({ 56, 72 }, 36, RGB565_Yellow);
    im.fillCircle({ 146, 72 }, 34, RGB565_Cyan, RGB565_White, 0.8f);
    im.drawThickCircleAA({ 236.0f, 72.0f }, 36.0f, 8.0f, RGB565_Green, 0.85f);
    im.fillThickCircleAA({ 74.0f, 168.0f }, 39.0f, 11.0f, RGB565_Red, RGB565_White, 0.75f);
    im.drawEllipse({ 170, 168 }, { 54, 24 }, RGB565_Orange);
    im.fillEllipse({ 170, 168 }, { 36, 15 }, RGB565_Cyan, RGB565_White, 0.45f);
    im.drawEllipseAA({ 112.0f, 116.0f }, { 37.0f, 18.0f }, RGB565_Yellow, 0.8f);
    im.drawThickEllipseAA({ 226.0f, 116.0f }, { 38.0f, 20.0f }, 5.0f, RGB565_Green, 0.8f);
    im.fillEllipseAA({ 250.0f, 168.0f }, { 48.0f, 25.0f }, RGB565_Purple, 0.6f);
    im.fillThickEllipseAA({ 64.0f, 116.0f }, { 32.0f, 17.0f }, 6.0f, RGB565_Blue, RGB565_White, 0.65f);
    im.drawCircleArcAA({ 112.0f, 168.0f }, 42.0f, 20.0f, 250.0f, RGB565_White, 0.75f);
    im.drawThickCircleArcAA({ 166.0f, 130.0f }, 60.0f, 200.0f, 338.0f, 7.0f, RGB565_White, 0.65f);
    im.fillCircleSectorAA({ 286.0f, 76.0f }, 28.0f, 20.0f, 290.0f, RGB565_Orange, 0.65f);
    im.fillThickCircleSectorAA({ 292.0f, 190.0f }, 31.0f, 210.0f, 510.0f, 8.0f, RGB565_Blue, RGB565_White, 0.65f);
}

void drawTrianglesPolygons()
{
    title("triangles / polygons");
    im.drawTriangle({ 16, 220 }, { 72, 40 }, { 130, 220 }, RGB565_Yellow, 0.8f);
    im.fillTriangle({ 102, 214 }, { 166, 42 }, { 222, 214 }, RGB565_Cyan, RGB565_White, 0.6f);
    im.drawTriangleAA({ 22.0f, 78.0f }, { 80.0f, 48.0f }, { 116.0f, 94.0f }, RGB565_Red, 0.8f);
    im.drawThickTriangleAA({ 28.0f, 104.0f }, { 92.0f, 104.0f }, { 60.0f, 142.0f }, 4.0f, RGB565_White, 0.75f);
    im.fillTriangleAA({ 128.0f, 92.0f }, { 188.0f, 94.0f }, { 158.0f, 140.0f }, RGB565_Green, 0.55f);
    im.fillThickTriangleAA({ 216.0f, 174.0f }, { 306.0f, 182.0f }, { 262.0f, 222.0f }, 5.0f, RGB565_Blue, RGB565_White, 0.65f);
    im.drawGradientTriangle({ 198.0f, 54.0f }, { 306.0f, 64.0f }, { 260.0f, 170.0f }, RGB565_Red, RGB565_Green, RGB565_Blue);
    im.drawQuad({ 22, 38 }, { 84, 34 }, { 96, 58 }, { 28, 62 }, RGB565_White, 0.8f);
    im.fillQuad({ 222, 34 }, { 306, 40 }, { 292, 58 }, { 232, 58 }, RGB565_Orange, 0.45f);
    im.drawQuadAA({ 202.0f, 180.0f }, { 306.0f, 186.0f }, { 292.0f, 204.0f }, { 208.0f, 202.0f }, RGB565_White, 0.7f);
    im.drawThickQuadAA({ 206.0f, 62.0f }, { 306.0f, 68.0f }, { 292.0f, 88.0f }, { 212.0f, 84.0f }, 4.0f, RGB565_Red, 0.75f);
    im.fillQuadAA({ 206.0f, 94.0f }, { 306.0f, 100.0f }, { 292.0f, 124.0f }, { 212.0f, 120.0f }, RGB565_Yellow, 0.55f);
    im.fillThickQuadAA({ 206.0f, 130.0f }, { 306.0f, 136.0f }, { 292.0f, 158.0f }, { 212.0f, 154.0f }, 4.0f, RGB565_Green, RGB565_White, 0.6f);
    im.drawGradientQuad({ 30.0f, 150.0f }, { 118.0f, 154.0f }, { 104.0f, 180.0f }, { 34.0f, 176.0f }, RGB565_Red, RGB565_Green, RGB565_Blue, RGB565_Yellow, 0.75f);
    fVec2 p[] = { { 22.0f, 166.0f }, { 70.0f, 110.0f }, { 126.0f, 162.0f }, { 178.0f, 104.0f }, { 236.0f, 182.0f }, { 292.0f, 126.0f } };
    im.fillPolygonAA(6, p, RGB565(4, 22, 31), 0.35f);
    im.drawThickPolylineAA(6, p, 5.0f, END_ROUNDED, END_ARROW_3, RGB565(31, 10, 12), 0.85f);
    im.drawPolygonAA(6, p, RGB565_White, 0.55f);
    im.drawThickPolygonAA(6, p, 3.0f, RGB565_Orange, 0.5f);
}

void drawBlits()
{
    title("blits / transforms");
    im.blit(sprite, { 18, 40 }, -1.0f);
    im.blit(sprite, { 96, 40 }, 0.45f);
    im.blitMasked(spriteMasked, maskColor, { 174, 38 }, -1.0f);
    im.blitRotated(sprite, { 244, 34 }, 90, 0.8f);
    im.blitScaledRotated(sprite, { SW / 2.0f, SH / 2.0f }, { 82.0f, 164.0f }, 1.25f, -28.0f, 0.8f);
    im.blitScaledRotatedMasked(spriteMasked, maskColor, { SW / 2.0f, SH / 2.0f }, { 222.0f, 166.0f }, 1.45f, 31.0f, -1.0f);
}

void drawText()
{
    title("text / fonts");
    im.drawText("OpenSans 18", { 20, 62 }, font_tgx_OpenSans_18, RGB565_White, -1.0f);
    im.drawText("opacity 0.5", { 20, 96 }, font_tgx_OpenSans_Bold_20, RGB565_Orange, 0.5f);
    im.drawTextEx("CENTER", { 160, 142 }, font_tgx_OpenSans_24, CENTER, false, false, RGB565_Cyan, 0.8f);
    im.drawFastHLine({ 32, 142 }, 256, RGB565(12, 25, 31), 0.8f);
    auto box = im.measureText("CENTER", { 160, 142 }, font_tgx_OpenSans_24, CENTER, false, false);
    im.drawRect(box, RGB565_White, 0.45f);
    im.drawTextEx("RIGHT", { 300, 210 }, font_tgx_Arial_20, BASELINE | RIGHT, false, false, RGB565_Red, -1.0f);
}

void drawRoundedAndSplines()
{
    title("rounded / splines");
    im.drawRoundRect(iBox2(18, 132, 40, 96), 14, RGB565_Yellow, 0.85f);
    im.fillRoundRect(iBox2(174, 302, 40, 96), 18, RGB565_Cyan, 0.55f);
    im.drawRoundRectAA(fBox2(22.0f, 140.0f, 122.0f, 210.0f), 20.0f, RGB565_Red, 0.8f);
    im.drawThickRoundRectAA(fBox2(176.0f, 304.0f, 122.0f, 210.0f), 22.0f, 8.0f, RGB565_White, 0.75f);
    im.fillRoundRectAA(fBox2(42.0f, 140.0f, 70.0f, 102.0f), 12.0f, RGB565_Purple, 0.45f);
    im.fillThickRoundRectAA(fBox2(176.0f, 304.0f, 122.0f, 210.0f), 22.0f, 8.0f, RGB565_Green, RGB565_White, 0.75f);
    iVec2 s[] = { { 26, 214 }, { 82, 162 }, { 134, 214 }, { 188, 160 }, { 292, 216 } };
    im.drawQuadSpline(5, s, true, RGB565_Red, 0.65f);
    im.drawCubicSpline(5, s, true, RGB565_Orange, 0.85f);
    im.drawClosedSpline(5, s, RGB565_Cyan, 0.55f);
    im.drawQuadBezier({ 26, 150 }, { 292, 150 }, { 160, 92 }, 1.0f, true, RGB565_White, 0.75f);
    im.drawCubicBezier({ 26, 184 }, { 292, 184 }, { 72, 92 }, { 250, 250 }, true, RGB565_Yellow, 0.75f);
}

void drawTextured()
{
    title("textured triangles");
    im.drawTexturedTriangle(sprite, { 0, 0 }, { SW, 0 }, { SW / 2.0f, SH }, { 28, 60 }, { 144, 48 }, { 84, 186 }, 0.85f);
    im.drawTexturedGradientTriangle(sprite, { 0, 0 }, { SW, 0 }, { SW / 2.0f, SH }, { 20, 36 }, { 126, 34 }, { 76, 104 }, RGB565_Red, RGB565_Green, RGB565_Blue, 0.7f);
    im.drawTexturedMaskedTriangle(spriteMasked, maskColor, { 0, 0 }, { SW, 0 }, { SW / 2.0f, SH }, { 170, 62 }, { 304, 56 }, { 236, 190 }, -1.0f);
    im.drawTexturedGradientMaskedTriangle(spriteMasked, maskColor, { 0, 0 }, { SW, 0 }, { SW / 2.0f, SH }, { 172, 34 }, { 304, 36 }, { 238, 106 }, RGB565_Red, RGB565_Green, RGB565_Blue, 0.7f);
    im.drawTexturedQuad(sprite, { 0, 0 }, { SW, 0 }, { SW, SH }, { 0, SH }, { 164, 198 }, { 224, 204 }, { 218, 232 }, { 160, 226 }, 0.65f);
    im.drawTexturedMaskedQuad(spriteMasked, maskColor, { 0, 0 }, { SW, 0 }, { SW, SH }, { 0, SH }, { 238, 198 }, { 310, 204 }, { 302, 232 }, { 236, 226 }, -1.0f);
    im.drawTexturedGradientQuad(sprite, { 0, 0 }, { SW, 0 }, { SW, SH }, { 0, SH }, { 38, 196 }, { 150, 204 }, { 146, 232 }, { 42, 224 },
                                RGB565_Red, RGB565_Green, RGB565_Blue, RGB565_Yellow, 0.8f);
    im.drawTexturedGradientMaskedQuad(spriteMasked, maskColor, { 0, 0 }, { SW, 0 }, { SW, SH }, { 0, SH }, { 210, 126 }, { 310, 132 }, { 302, 174 }, { 214, 168 },
                                      RGB565_Red, RGB565_Green, RGB565_Blue, RGB565_Yellow, 0.75f);
}

void drawImageOps()
{
    title("image / copy / fill");
    im.fillRect(iBox2(18, 118, 42, 134), RGB565(2, 4, 8));
    im.drawRect(iBox2(30, 104, 54, 118), RGB565_White);
    im.fill<2048>({ 44, 72 }, RGB565_White, RGB565_Red);
    im.fill<2048>({ 20, 44 }, RGB565_Blue);

    Image<RGB565> src(srcBuf, 52, 36);
    src.fillScreenHGradient(RGB565_Blue, RGB565_Yellow);
    src.fillCircleAA({ 18.0f, 18.0f }, 11.0f, RGB565_Red, 0.9f);
    src.drawTextEx("TGX", { 26, 24 }, font_tgx_OpenSans_Bold_10, CENTER, false, false, RGB565_Black, 0.8f);

    auto target = im(142, 238, 44, 134);
    target.copyFrom(src, 0.85f);

    Image<RGB565> half(smallBuf, 26, 18);
    half.copyReduceHalf(src);
    im.blit(half, { 148, 154 }, -1.0f);

    Image<RGB565> back(dstBuf, 26, 18);
    src.blitBackward(back, { 10, 8 });
    im.blit(back, { 214, 154 }, -1.0f);

    src.reduceHalf();
    im.blit(src, { 264, 154 }, -1.0f);
}

void drawThickSplines()
{
    title("thick / filled splines");
    fVec2 pts[] = { { 28.0f, 180.0f }, { 80.0f, 64.0f }, { 142.0f, 178.0f }, { 210.0f, 62.0f }, { 292.0f, 182.0f } };
    im.drawThickQuadBezierAA({ 28.0f, 72.0f }, { 292.0f, 70.0f }, { 160.0f, 136.0f }, 1.0f, 5.0f, END_ROUNDED, END_ARROW_2, RGB565_Yellow, 0.85f);
    im.drawThickCubicBezierAA({ 28.0f, 112.0f }, { 292.0f, 112.0f }, { 70.0f, 12.0f }, { 250.0f, 222.0f }, 4.0f, END_ROUNDED, END_ROUNDED, RGB565_Cyan, 0.8f);
    im.drawThickQuadSplineAA(5, pts, 4.0f, END_ROUNDED, END_ARROW_3, RGB565_Red, 0.8f);
    im.drawThickCubicSplineAA(5, pts, 3.0f, END_ROUNDED, END_ROUNDED, RGB565_Green, 0.65f);
    im.drawThickClosedSplineAA(5, pts, 3.0f, RGB565_White, 0.5f);
    im.fillClosedSplineAA(5, pts, RGB565_Blue, 0.25f);
    im.fillThickClosedSplineAA(5, pts, 5.0f, RGB565_Green, RGB565_White, 0.35f);
}

void drawLayoutRotations()
{
    title("text layout / rotations");
    Image<RGB565> src(srcBuf, 42, 28);
    src.fillScreenHGradient(RGB565_Blue, RGB565_Yellow);
    src.fillCircleAA({ 16.0f, 14.0f }, 10.0f, RGB565_Red, 0.9f);
    src.drawTextEx("TGX", { 21, 19 }, font_tgx_OpenSans_Bold_10, CENTER, false, false, RGB565_Black, 0.8f);

    im.blitRotated(src, { 20, 48 }, 0, -1.0f);
    im.blitRotated(src, { 76, 44 }, 90, -1.0f);
    im.blitRotated(src, { 128, 48 }, 180, -1.0f);
    im.blitRotated(src, { 184, 44 }, 270, -1.0f);
    im.drawTextEx("LEFT", { 20, 122 }, font_tgx_OpenSans_10, BASELINE | LEFT, false, false, RGB565_White, -1.0f);
    im.drawTextEx("CENTER", { 160, 150 }, font_tgx_OpenSans_10, CENTER, false, false, RGB565_Cyan, 0.85f);
    im.drawTextEx("RIGHT", { 306, 174 }, font_tgx_OpenSans_10, BASELINE | RIGHT, false, false, RGB565_Orange, -1.0f);
    im.drawTextEx("wrap wrap wrap wrap", { 228, 120 }, font_tgx_OpenSans_10, BASELINE | LEFT, true, false, RGB565_Yellow, 0.85f);
}

void drawGfxFontPanel()
{
    title("GFXfont path");
    im.drawChar('A', { 28, 82 }, gfx_font, RGB565_Red, -1.0f);
    im.drawChar('B', { 58, 82 }, gfx_font, RGB565_Green, 0.75f);
    im.drawChar('C', { 88, 82 }, gfx_font, RGB565_Cyan, 0.55f);
    im.drawText("ABC ABC", { 28, 124 }, gfx_font, RGB565_Yellow, -1.0f);
    im.drawTextEx("ABC\nBCA", { 206, 134 }, gfx_font, CENTER, false, false, RGB565_White, 0.85f);
    auto box = im.measureText("ABC", { 28, 124 }, gfx_font, BASELINE | LEFT, false, false);
    im.drawRect(box, RGB565_White, 0.55f);
}

void drawBulkOps()
{
    title("bulk fill / copy");
    im.fillScreenHGradient(RGB565(2, 8, 18), RGB565(28, 42, 12));
    title("bulk fill / copy");
    im.fillRectVGradient(iBox2(18, 146, 42, 170), RGB565_Red, RGB565_Blue, 0.75f);
    im.fillRectHGradient(iBox2(18, 300, 184, 226), RGB565_Blue, RGB565_Yellow, 0.7f);

    Image<RGB565> src(srcBuf, 80, 50);
    src.fillScreenVGradient(RGB565_Blue, RGB565_Red);
    src.fillCircleAA({ 40.0f, 25.0f }, 18.0f, RGB565_Yellow, 0.85f);
    src.drawTextEx("copy", { 40, 31 }, font_tgx_OpenSans_10, CENTER, false, false, RGB565_Black, 0.85f);

    auto dst = im(170, 300, 54, 150);
    dst.copyFrom(src, 0.82f);
    im.blit(src, { 28, 78 }, 0.45f);
}

void displayPage()
{
    tft.update(fb);
    delay(900);
}

void printResultCsv(const char* name, const char* kind, uint32_t iterations, uint32_t total, uint32_t hash, uint32_t changed, bool ok)
{
    Serial.print("RESULT,1,");
    Serial.print(RESULT_PLATFORM);
    Serial.print(",");
    Serial.print(RESULT_COLOR);
    Serial.print(",");
    Serial.print(name);
    Serial.print(",");
    Serial.print(kind);
    Serial.print(",");
    Serial.print(iterations);
    Serial.print(",");
    Serial.print(total);
    Serial.print(",");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(",0x");
    Serial.print(hash, HEX);
    Serial.print(",");
    Serial.print(changed);
    Serial.print(",");
    Serial.print(ok ? "true" : "false");
    Serial.println(",");
}

template<typename DrawFn>
void runBench(const char* name, uint32_t iterations, DrawFn draw)
{
    const uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++)
        {
        im.clear(RGB565(1, 2, 4));
        draw();
        }
    const uint32_t total = micros() - t0;
    const uint32_t changed = countChanged(RGB565(1, 2, 4));
    const uint32_t hash = imageHash();
    const bool ok = changed > 0;
    results[resultCount++] = { name, iterations, total, hash, changed, ok };

    Serial.print(name);
    Serial.print(", iterations=");
    Serial.print(iterations);
    Serial.print(", total_us=");
    Serial.print(total);
    Serial.print(", per_iter_us=");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(", changed=");
    Serial.print(changed);
    Serial.print(", hash=0x");
    Serial.print(hash, HEX);
    Serial.print(", ok=");
    Serial.println(ok ? "true" : "false");
    printResultCsv(name, "benchmark", iterations, total, hash, changed, ok);
}

template<typename ValidateFn>
void runValidation(const char* name, uint32_t iterations, ValidateFn validate)
{
    bool ok = false;
    const uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++) ok = validate();
    const uint32_t total = micros() - t0;
    results[resultCount++] = { name, iterations, total, 0, 0, ok };

    Serial.print(name);
    Serial.print(", iterations=");
    Serial.print(iterations);
    Serial.print(", total_us=");
    Serial.print(total);
    Serial.print(", per_iter_us=");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(", ok=");
    Serial.println(ok ? "true" : "false");
    printResultCsv(name, "validation", iterations, total, 0, 0, ok);
}

void showSummary()
{
    im.clear(RGB565_Black);
    title("TGX 2D suite summary");
    int y = 40;
    for (int i = 0; i < resultCount && i < 8; i++)
        {
        char line[80];
        snprintf(line, sizeof(line), "%s  %.1fus", results[i].name, (double)results[i].total_us / (double)results[i].iterations);
        im.drawText(line, { 10, y }, font_tgx_OpenSans_10, results[i].ok ? RGB565_Green : RGB565_Red);
        y += 22;
        }
    tft.update(fb);
}

void waitForStart()
{
    im.clear(RGB565_Black);
    title("TGX 2D suite ready");
    im.drawTextEx("Open Serial Monitor", { 160, 92 }, font_tgx_OpenSans_Bold_14, CENTER, false, false, RGB565_White);
    im.drawTextEx("Send any character", { 160, 124 }, font_tgx_OpenSans_18, CENTER, false, false, RGB565_Cyan);
    im.drawTextEx("600MHz / Fastest / USB Serial", { 160, 168 }, font_tgx_OpenSans_10, CENTER, false, false, RGB565_Yellow);
    im.drawTextEx("Runs can be repeated", { 160, 194 }, font_tgx_OpenSans_10, CENTER, false, false, RGB565_Green);
    tft.update(fb);

    while (!Serial) delay(10);
    while (Serial.available()) Serial.read();

    Serial.println("TGX 2D Teensy validation/benchmark suite");
    Serial.println("Config: Teensy 4.1, 600MHz, Optimize Fastest, USB Serial");
    Serial.println("RESULT_SCHEMA,schema_version,platform,color_type,group,kind,iterations,total_us,per_iter_us,hash,changed_pixels,ok,note");
    Serial.println("Send any character to start.");

    while (!Serial.available()) delay(10);
    while (Serial.available()) Serial.read();
    Serial.println("Starting...");
}

void runSuite()
{
    resultCount = 0;

    Serial.print("sample_validation=");
    const bool sampleOk = sampleValidation();
    Serial.println(sampleOk ? "true" : "false");
    printResultCsv("sample_validation", "validation", 1, 0, 0, 0, sampleOk);
    runValidation("image_state", 200, imageStateValidation);
    runValidation("transfer_fill", 80, transferFillValidation);
    runValidation("convert", 200, convertValidation);
    runValidation("rotated_blit", 120, rotatedBlitValidation);
    runValidation("text_layout", 120, textLayoutValidation);
    runValidation("primitive_exact", 200, primitiveExactValidation);
    runValidation("clip_blit", 160, clipBlitValidation);
    runValidation("image_edge", 160, imageEdgeValidation);
    runValidation("gradient_opacity", 160, gradientOpacityValidation);
    runValidation("line_rect_exact", 200, lineRectExactValidation);
    runValidation("triangle_quad", 160, triangleQuadValidation);
    runValidation("circle_round", 160, circleRoundValidation);
    runValidation("aa_primitives", 80, aaPrimitivesValidation);
    runValidation("curve_polygon", 80, curvePolygonValidation);
    runValidation("textured_edge", 80, texturedEdgeValidation);
    runValidation("fill_failure", 120, fillFailureValidation);
    runValidation("gfx_text_val", 120, gfxTextValidation);
    runValidation("text_stress", 80, textStressValidation);

    im.clear(RGB565(1, 2, 4)); drawPixels(); displayPage(); runBench("pixels", 40, drawPixels);
    im.clear(RGB565(1, 2, 4)); drawLinesRects(); displayPage(); runBench("lines_rects", 25, drawLinesRects);
    im.clear(RGB565(1, 2, 4)); drawCircles(); displayPage(); runBench("circles", 20, drawCircles);
    im.clear(RGB565(1, 2, 4)); drawTrianglesPolygons(); displayPage(); runBench("tri_poly", 18, drawTrianglesPolygons);
    im.clear(RGB565(1, 2, 4)); drawBlits(); displayPage(); runBench("blits", 25, drawBlits);
    im.clear(RGB565(1, 2, 4)); drawText(); displayPage(); runBench("text", 20, drawText);
    im.clear(RGB565(1, 2, 4)); drawRoundedAndSplines(); displayPage(); runBench("round_spline", 15, drawRoundedAndSplines);
    im.clear(RGB565(1, 2, 4)); drawTextured(); displayPage(); runBench("textured", 15, drawTextured);
    im.clear(RGB565(1, 2, 4)); drawImageOps(); displayPage(); runBench("image_ops", 20, drawImageOps);
    im.clear(RGB565(1, 2, 4)); drawThickSplines(); displayPage(); runBench("thick_splines", 12, drawThickSplines);
    im.clear(RGB565(1, 2, 4)); drawLayoutRotations(); displayPage(); runBench("layout_rot", 20, drawLayoutRotations);
    im.clear(RGB565(1, 2, 4)); drawGfxFontPanel(); displayPage(); runBench("gfx_text", 30, drawGfxFontPanel);
    im.clear(RGB565(1, 2, 4)); drawBulkOps(); displayPage(); runBench("bulk_ops", 20, drawBulkOps);

    showSummary();
    Serial.println("TGX 2D suite done");
    delay(2500);
}

void setup()
{
    Serial.begin(115200);
    tft.output(&Serial);
    while (!tft.begin(SPI_SPEED));

    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setRefreshRate(140);
    tft.setVSyncSpacing(2);

    buildSprites();
    delay(250);
}

void loop()
{
    waitForStart();
    runSuite();
}
