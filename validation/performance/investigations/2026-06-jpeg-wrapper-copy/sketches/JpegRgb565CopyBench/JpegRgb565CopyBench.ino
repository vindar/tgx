#include <Arduino.h>
#include <string.h>
#include <tgx.h>

#if defined(ESP32)
    #include <esp_heap_caps.h>
#endif

using namespace tgx;

struct FakeJpegDraw
    {
    int x;
    int y;
    int iWidth;
    int iHeight;
    uint16_t* pPixels;
    };

static const int MAX_STRIDE = 352;
static const int MAX_HEIGHT = 96;
static const int MAX_SRC_PIXELS = 320 * 32;

static uint16_t* dst_ref = nullptr;
static uint16_t* dst_cand = nullptr;
static uint16_t* dst_prod = nullptr;
static uint16_t* src_pixels = nullptr;

static volatile uint32_t checksum_sink = 0;

const char* boardName()
    {
#if defined(ARDUINO_TEENSY41)
    return "Teensy41";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    return "CoreS3";
#elif defined(ESP32)
    return "Core2";
#elif defined(ARDUINO_ARCH_RP2040) && defined(__ARM_ARCH_8M_MAIN__)
    return "Pico2";
#elif defined(ARDUINO_ARCH_RP2040)
    return "PicoRP2040";
#else
    return "Unknown";
#endif
    }

uint16_t* allocatePixels(size_t count)
    {
#if defined(ESP32)
    void* p = heap_caps_malloc(count * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = heap_caps_malloc(count * sizeof(uint16_t), MALLOC_CAP_8BIT);
    return (uint16_t*)p;
#else
    return (uint16_t*)malloc(count * sizeof(uint16_t));
#endif
    }

bool allocateBuffers()
    {
    dst_ref = allocatePixels((size_t)MAX_STRIDE * MAX_HEIGHT);
    dst_cand = allocatePixels((size_t)MAX_STRIDE * MAX_HEIGHT);
    dst_prod = allocatePixels((size_t)MAX_STRIDE * MAX_HEIGHT);
    src_pixels = allocatePixels(MAX_SRC_PIXELS);
    return dst_ref && dst_cand && dst_prod && src_pixels;
    }

uint32_t lcg(uint32_t& state)
    {
    state = 1664525u * state + 1013904223u;
    return state;
    }

uint16_t random565(uint32_t& state)
    {
    return (uint16_t)((lcg(state) >> 8) ^ (lcg(state) >> 16));
    }

void fillBuffers(int stride, int height, int srcCount, uint32_t seed)
    {
    uint32_t s = seed;
    for (int i = 0; i < stride * height; i++)
        {
        const uint16_t v = random565(s);
        dst_ref[i] = v;
        dst_cand[i] = v;
        dst_prod[i] = v;
        }
    for (int i = 0; i < srcCount; i++)
        {
        src_pixels[i] = random565(s);
        }
    }

uint32_t checksumBuffer(const uint16_t* p, int count)
    {
    uint32_t h = 2166136261u;
    for (int i = 0; i < count; i++)
        {
        const uint16_t v = p[i];
        h ^= (uint8_t)(v & 0xffu);
        h *= 16777619u;
        h ^= (uint8_t)(v >> 8);
        h *= 16777619u;
        }
    return h;
    }

int countMismatches(const uint16_t* a, const uint16_t* b, int count)
    {
    int n = 0;
    for (int i = 0; i < count; i++)
        {
        if (a[i] != b[i]) n++;
        }
    return n;
    }

void oldPerPixelConvert565(int x, int y, FakeJpegDraw* pDraw, Image<RGB565>* im, float op)
    {
    x += pDraw->x;
    y += pDraw->y;
    uint16_t* p = pDraw->pPixels;
    if ((op >= 1.0f) && (((x >= 0) && (x + pDraw->iWidth <= im->width())) && ((y >= 0) && (y + pDraw->iHeight <= im->height()))))
        {
        uint16_t* fb = (uint16_t*)im->data() + x + (y * im->stride());
        const int str = im->stride();
        for (int j = 0; j < pDraw->iHeight; j++)
            {
            for (int i = 0; i < pDraw->iWidth; i++)
                {
                fb[i + str * j] = *(p++);
                }
            }
        }
    else
        {
        for (int j = 0; j < pDraw->iHeight; j++)
            {
            for (int i = 0; i < pDraw->iWidth; i++)
                {
                im->template drawPixel<true>({ x + i, y + j }, RGB565(*(p++)), op);
                }
            }
        }
    }

void rowMemcpyConvert565(int x, int y, FakeJpegDraw* pDraw, Image<RGB565>* im, float op)
    {
    x += pDraw->x;
    y += pDraw->y;
    uint16_t* p = pDraw->pPixels;
    if ((op >= 1.0f) && (((x >= 0) && (x + pDraw->iWidth <= im->width())) && ((y >= 0) && (y + pDraw->iHeight <= im->height()))))
        {
        uint16_t* fb = (uint16_t*)im->data() + x + (y * im->stride());
        const int str = im->stride();
        const size_t rowBytes = (size_t)pDraw->iWidth * sizeof(uint16_t);
        for (int j = 0; j < pDraw->iHeight; j++)
            {
            memcpy(fb + str * j, p, rowBytes);
            p += pDraw->iWidth;
            }
        }
    else
        {
        for (int j = 0; j < pDraw->iHeight; j++)
            {
            for (int i = 0; i < pDraw->iWidth; i++)
                {
                im->template drawPixel<true>({ x + i, y + j }, RGB565(*(p++)), op);
                }
            }
        }
    }

uint32_t timeVariant(int variant, int repeats, int baseX, int baseY, FakeJpegDraw* draw, Image<RGB565>* im, float op)
    {
    const uint32_t t0 = micros();
    for (int r = 0; r < repeats; r++)
        {
        if (variant == 1) rowMemcpyConvert565(baseX, baseY, draw, im, op);
        else if (variant == 2) tgx::_jpegdec_color_convert565(baseX, baseY, draw, im, op);
        else oldPerPixelConvert565(baseX, baseY, draw, im, op);
        }
    return micros() - t0;
    }

int chooseRepeats(int w, int h)
    {
    const int pixels = w * h;
    int repeats = 24000 / (pixels > 0 ? pixels : 1);
    if (repeats < 16) repeats = 16;
    if (repeats > 1500) repeats = 1500;
    return repeats;
    }

void runCase(const char* caseName, int imgW, int imgH, int stride, int baseX, int baseY, int blockX, int blockY, int w, int h, float op)
    {
    if (stride > MAX_STRIDE || imgH > MAX_HEIGHT || w * h > MAX_SRC_PIXELS) return;

    const int bufferCount = stride * imgH;
    fillBuffers(stride, imgH, w * h, 0x1234abcdU ^ (uint32_t)(w * 131 + h * 17 + stride * 7 + blockX * 3 + blockY));

    FakeJpegDraw draw;
    draw.x = blockX;
    draw.y = blockY;
    draw.iWidth = w;
    draw.iHeight = h;
    draw.pPixels = src_pixels;

    Image<RGB565> imRef(dst_ref, imgW, imgH, stride);
    Image<RGB565> imCand(dst_cand, imgW, imgH, stride);
    Image<RGB565> imProd(dst_prod, imgW, imgH, stride);

    oldPerPixelConvert565(baseX, baseY, &draw, &imRef, op);
    rowMemcpyConvert565(baseX, baseY, &draw, &imCand, op);
    tgx::_jpegdec_color_convert565(baseX, baseY, &draw, &imProd, op);

    const int mismatches = countMismatches(dst_ref, dst_cand, bufferCount);
    const int productionMismatches = countMismatches(dst_ref, dst_prod, bufferCount);
    const uint32_t candHash = checksumBuffer(dst_cand, bufferCount);
    const uint32_t productionHash = checksumBuffer(dst_prod, bufferCount);
    checksum_sink ^= candHash;
    checksum_sink ^= productionHash;

    const bool inBounds = (op >= 1.0f) &&
        ((baseX + blockX) >= 0) &&
        ((baseX + blockX + w) <= imgW) &&
        ((baseY + blockY) >= 0) &&
        ((baseY + blockY + h) <= imgH);

    const int repeats = chooseRepeats(w, h);

    fillBuffers(stride, imgH, w * h, 0x5678abcdU ^ (uint32_t)(w * 131 + h * 17 + stride * 7 + blockX * 3 + blockY));
    Image<RGB565> imRefTimed(dst_ref, imgW, imgH, stride);
    const uint32_t oldUs = timeVariant(0, repeats, baseX, baseY, &draw, &imRefTimed, op);
    const uint32_t oldHash = checksumBuffer(dst_ref, bufferCount);
    checksum_sink ^= oldHash;

    fillBuffers(stride, imgH, w * h, 0x5678abcdU ^ (uint32_t)(w * 131 + h * 17 + stride * 7 + blockX * 3 + blockY));
    Image<RGB565> imCandTimed(dst_cand, imgW, imgH, stride);
    const uint32_t memcpyUs = timeVariant(1, repeats, baseX, baseY, &draw, &imCandTimed, op);
    const uint32_t memcpyHash = checksumBuffer(dst_cand, bufferCount);
    checksum_sink ^= memcpyHash;

    fillBuffers(stride, imgH, w * h, 0x5678abcdU ^ (uint32_t)(w * 131 + h * 17 + stride * 7 + blockX * 3 + blockY));
    Image<RGB565> imProdTimed(dst_prod, imgW, imgH, stride);
    const uint32_t productionUs = timeVariant(2, repeats, baseX, baseY, &draw, &imProdTimed, op);
    const uint32_t productionTimedHash = checksumBuffer(dst_prod, bufferCount);
    checksum_sink ^= productionTimedHash;

    Serial.print("KERNEL=jpeg_rgb565_copy VARIANT=old_loop CASE=");
    Serial.print(caseName);
    Serial.print(" W=");
    Serial.print(w);
    Serial.print(" H=");
    Serial.print(h);
    Serial.print(" STRIDE=");
    Serial.print(stride);
    Serial.print(" OPACITY=");
    Serial.print(op, 3);
    Serial.print(" IN_BOUNDS=");
    Serial.print(inBounds ? 1 : 0);
    Serial.print(" CORRECT=1 MISMATCHES=0 REPEATS=");
    Serial.print(repeats);
    Serial.print(" TIME_US=");
    Serial.print(oldUs);
    Serial.print(" CHECKSUM=");
    Serial.println(oldHash, HEX);

    Serial.print("KERNEL=jpeg_rgb565_copy VARIANT=row_memcpy CASE=");
    Serial.print(caseName);
    Serial.print(" W=");
    Serial.print(w);
    Serial.print(" H=");
    Serial.print(h);
    Serial.print(" STRIDE=");
    Serial.print(stride);
    Serial.print(" OPACITY=");
    Serial.print(op, 3);
    Serial.print(" IN_BOUNDS=");
    Serial.print(inBounds ? 1 : 0);
    Serial.print(" CORRECT=");
    Serial.print(mismatches == 0 ? 1 : 0);
    Serial.print(" MISMATCHES=");
    Serial.print(mismatches);
    Serial.print(" REPEATS=");
    Serial.print(repeats);
    Serial.print(" TIME_US=");
    Serial.print(memcpyUs);
    Serial.print(" CHECKSUM=");
    Serial.println(memcpyHash, HEX);

    Serial.print("KERNEL=jpeg_rgb565_copy VARIANT=production CASE=");
    Serial.print(caseName);
    Serial.print(" W=");
    Serial.print(w);
    Serial.print(" H=");
    Serial.print(h);
    Serial.print(" STRIDE=");
    Serial.print(stride);
    Serial.print(" OPACITY=");
    Serial.print(op, 3);
    Serial.print(" IN_BOUNDS=");
    Serial.print(inBounds ? 1 : 0);
    Serial.print(" CORRECT=");
    Serial.print(productionMismatches == 0 ? 1 : 0);
    Serial.print(" MISMATCHES=");
    Serial.print(productionMismatches);
    Serial.print(" REPEATS=");
    Serial.print(repeats);
    Serial.print(" TIME_US=");
    Serial.print(productionUs);
    Serial.print(" CHECKSUM=");
    Serial.println(productionTimedHash, HEX);
    Serial.flush();
    delay(1);
    yield();
    }

void runBench()
    {
    const int widths[] = { 1, 2, 3, 4, 5, 8, 9, 16, 31, 32, 63, 64, 127, 128, 240, 319, 320 };
    const int heights[] = { 1, 2, 4, 8, 16, 32 };

    for (size_t wi = 0; wi < sizeof(widths) / sizeof(widths[0]); wi++)
        {
        for (size_t hi = 0; hi < sizeof(heights) / sizeof(heights[0]); hi++)
            {
            const int w = widths[wi];
            const int h = heights[hi];
            runCase("in_bounds_tight", w, h + 2, w, 0, 1, 0, 0, w, h, 1.0f);
            runCase("in_bounds_strided", 336, h + 8, 352, 5, 3, 4, 2, w, h, 1.0f);
            }
        }

    runCase("opacity_fallback", 336, 48, 352, 5, 3, 4, 2, 64, 8, 0.5f);
    runCase("left_clip_fallback", 64, 32, 80, 0, 5, -7, 0, 32, 8, 1.0f);
    runCase("top_clip_fallback", 64, 32, 80, 8, 0, 0, -3, 32, 8, 1.0f);
    runCase("right_clip_fallback", 64, 32, 80, 40, 5, 0, 0, 32, 8, 1.0f);
    runCase("bottom_clip_fallback", 64, 32, 80, 8, 28, 0, 0, 32, 8, 1.0f);
    runCase("edge_inside", 64, 32, 80, 32, 24, 0, 0, 32, 8, 1.0f);
    }

void setup()
    {
    Serial.begin(115200);
    const uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 3000)) {}
    delay(9000);
    delay(250);
    while (Serial.available() > 0) Serial.read();
    const uint32_t kickStart = millis();
    while ((millis() - kickStart) < 15000)
        {
        if (Serial.available() > 0)
            {
            Serial.read();
            break;
            }
        delay(10);
        }

    Serial.println("TGX_JPEG_RGB565_COPY_BENCH_BEGIN");
    Serial.print("BOARD=");
    Serial.println(boardName());
    Serial.println("CONFIG=display_free simulated_jpegdraw rgb565");
    if (!allocateBuffers())
        {
        Serial.println("ALLOC_FAILED=1");
        Serial.println("TGX_JPEG_RGB565_COPY_BENCH_END");
        return;
        }
    runBench();
    Serial.print("SINK=");
    Serial.println((uint32_t)checksum_sink, HEX);
    Serial.println("TGX_JPEG_RGB565_COPY_BENCH_END");
    }

void loop()
    {
    delay(1000);
    }
