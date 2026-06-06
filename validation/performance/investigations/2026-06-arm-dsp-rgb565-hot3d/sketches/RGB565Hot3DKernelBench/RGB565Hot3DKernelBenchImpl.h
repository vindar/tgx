#include <Arduino.h>
#include <tgx.h>

using tgx::RGB565;
using tgx::RGBf;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(__ARM_ARCH_7EM__) || defined(__ARM_FEATURE_DSP) || defined(__ARM_FEATURE_SIMD32)
#define TGX_BENCH_HAS_DSP32 1
#else
#define TGX_BENCH_HAS_DSP32 0
#endif

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
#define TGX_BENCH_HAS_DWT 1
#else
#define TGX_BENCH_HAS_DWT 0
#endif

static const uint32_t kCount = 512;
static const uint32_t kTimingRepeats = 512;
static const uint32_t kFloatTimingRepeats = 128;
static const uint32_t kSpanTimingRepeats = 512;

static uint16_t gA[kCount + 4];
static uint16_t gB[kCount + 4];
static uint16_t gC[kCount + 4];
static uint16_t gD[kCount + 4];
static uint16_t gOut[kCount + 4];
static uint16_t gRef[kCount + 4];
static uint16_t gSpanOut[kCount + 8];
static uint16_t gSpanRef[kCount + 8];
static uint32_t gAlpha[kCount + 4];
static int32_t gMr[kCount + 4];
static int32_t gMg[kCount + 4];
static int32_t gMb[kCount + 4];
static int32_t gMa[kCount + 4];
static int32_t gC1[kCount + 4];
static int32_t gC2[kCount + 4];
static int32_t gTot[kCount + 4];
static float gAx[kCount + 4];
static float gAy[kCount + 4];
static RGBf gFloat[kCount + 4];
static volatile uint32_t gSink = 0;

static uint32_t rngState = 0x12345678u;

static uint32_t xorshift32()
{
    uint32_t x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState = x;
    return x;
}

static void waitForSerial()
{
    const uint32_t start = millis();
    while (!Serial && (millis() - start < 5000)) {
        delay(10);
    }

    const uint32_t kickStart = millis();
    while (millis() - kickStart < 12000) {
        if (Serial.available() > 0) {
            while (Serial.available() > 0) {
                (void)Serial.read();
            }
            break;
        }
        delay(10);
    }
    delay(100);
}

#if TGX_BENCH_HAS_DWT
static void initCycleCounter()
{
    volatile uint32_t* demcr = (volatile uint32_t*)0xE000EDFCu;
    volatile uint32_t* dwtCtrl = (volatile uint32_t*)0xE0001000u;
    volatile uint32_t* dwtCyccnt = (volatile uint32_t*)0xE0001004u;
    *demcr |= (1u << 24);
    *dwtCyccnt = 0;
    *dwtCtrl |= 1u;
}

static uint32_t readCycles()
{
    return *((volatile uint32_t*)0xE0001004u);
}
#else
static void initCycleCounter() {}
static uint32_t readCycles() { return 0; }
#endif

#if TGX_BENCH_HAS_DSP32
static uint32_t asmUhadd16(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("uhadd16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

#endif

static inline int getR(uint16_t c)
{
#if TGX_RGB565_ORDER_BGR
    return (int)((c >> 11) & 31u);
#else
    return (int)(c & 31u);
#endif
}

static inline int getG(uint16_t c)
{
    return (int)((c >> 5) & 63u);
}

static inline int getB(uint16_t c)
{
#if TGX_RGB565_ORDER_BGR
    return (int)(c & 31u);
#else
    return (int)((c >> 11) & 31u);
#endif
}

static inline uint16_t packRgb565(int r, int g, int b)
{
    const uint16_t rr = (uint16_t)(r & 31);
    const uint16_t gg = (uint16_t)(g & 63);
    const uint16_t bb = (uint16_t)(b & 31);
#if TGX_RGB565_ORDER_BGR
    return (uint16_t)((rr << 11) | (gg << 5) | bb);
#else
    return (uint16_t)((bb << 11) | (gg << 5) | rr);
#endif
}

static inline uint16_t contractPackedRgb565(uint32_t result)
{
    return (uint16_t)((result >> 16) | result);
}

static inline uint16_t refBlend256(uint16_t bg, uint16_t fg, uint32_t alpha)
{
    RGB565 c(bg);
    c.blend256(RGB565(fg), alpha);
    return c.val;
}

static inline uint16_t refMult256(uint16_t c, int mr, int mg, int mb)
{
    RGB565 out(c);
    out.mult256(mr, mg, mb);
    return out.val;
}

static inline uint16_t refMult256A(uint16_t c, int mr, int mg, int mb, int ma)
{
    RGB565 out(c);
    out.mult256(mr, mg, mb, ma);
    return out.val;
}

static inline uint16_t refMean2(uint16_t a, uint16_t b)
{
    return tgx::meanColor(RGB565(a), RGB565(b)).val;
}

static inline uint16_t refMean4(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    return tgx::meanColor(RGB565(a), RGB565(b), RGB565(c), RGB565(d)).val;
}

static inline uint16_t refTriangle(uint16_t a, int32_t c1, uint16_t b, int32_t c2, uint16_t c, int32_t tot)
{
    return tgx::interpolateColorsTriangle(RGB565(a), c1, RGB565(b), c2, RGB565(c), tot).val;
}

static inline uint16_t refBilinear(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, float ax, float ay)
{
    return tgx::interpolateColorsBilinear(RGB565(c00), RGB565(c10), RGB565(c01), RGB565(c11), ax, ay).val;
}

static inline uint16_t refRgbfTo565(const RGBf& c)
{
    return RGB565(c).val;
}

static inline uint16_t blendPackedCopy(uint16_t bgVal, uint16_t fgVal, uint32_t alpha)
{
    const uint32_t a = (alpha >> 3);
    const uint32_t bg = (bgVal | (bgVal << 16)) & 0x07E0F81Fu;
    const uint32_t fg = (fgVal | (fgVal << 16)) & 0x07E0F81Fu;
    const uint32_t result = ((((fg - bg) * a) >> 5) + bg) & 0x07E0F81Fu;
    return contractPackedRgb565(result);
}

static inline uint16_t blendScalarWeighted(uint16_t bg, uint16_t fg, uint32_t alpha)
{
    const int a = (int)(alpha >> 3);
    const int ia = 32 - a;
    const int r = (getR(bg) * ia + getR(fg) * a) >> 5;
    const int g = (getG(bg) * ia + getG(fg) * a) >> 5;
    const int b = (getB(bg) * ia + getB(fg) * a) >> 5;
    return packRgb565(r, g, b);
}

static inline uint16_t blendChannelDiff(uint16_t bg, uint16_t fg, uint32_t alpha)
{
    const int a = (int)(alpha >> 3);
    const int r = getR(bg) + (((getR(fg) - getR(bg)) * a) >> 5);
    const int g = getG(bg) + (((getG(fg) - getG(bg)) * a) >> 5);
    const int b = getB(bg) + (((getB(fg) - getB(bg)) * a) >> 5);
    return packRgb565(r, g, b);
}

static inline uint16_t multShiftPack(uint16_t c, int mr, int mg, int mb)
{
    return packRgb565((getR(c) * mr) >> 8,
                      (getG(c) * mg) >> 8,
                      (getB(c) * mb) >> 8);
}

static inline uint16_t multShiftPackA(uint16_t c, int mr, int mg, int mb, int ma)
{
    (void)ma;
    return multShiftPack(c, mr, mg, mb);
}

static inline uint16_t mean2Packed(uint16_t a, uint16_t b)
{
    return (uint16_t)((a & b) + (((a ^ b) & 0xF7DEu) >> 1));
}

static inline uint16_t mean2Scalar(uint16_t a, uint16_t b)
{
    return packRgb565((getR(a) + getR(b)) >> 1,
                      (getG(a) + getG(b)) >> 1,
                      (getB(a) + getB(b)) >> 1);
}

static inline uint16_t mean2UhaddNaive(uint16_t a, uint16_t b)
{
#if TGX_BENCH_HAS_DSP32
    return (uint16_t)asmUhadd16(a, b);
#else
    return mean2Packed(a, b);
#endif
}

static inline uint16_t mean4Packed(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    const uint32_t pa = (a | (a << 16)) & 0x07E0F81Fu;
    const uint32_t pb = (b | (b << 16)) & 0x07E0F81Fu;
    const uint32_t pc = (c | (c << 16)) & 0x07E0F81Fu;
    const uint32_t pd = (d | (d << 16)) & 0x07E0F81Fu;
    const uint32_t result = ((pa + pb + pc + pd) >> 2) & 0x07E0F81Fu;
    return contractPackedRgb565(result);
}

static inline uint16_t mean4Scalar(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    return packRgb565((getR(a) + getR(b) + getR(c) + getR(d)) >> 2,
                      (getG(a) + getG(b) + getG(c) + getG(d)) >> 2,
                      (getB(a) + getB(b) + getB(c) + getB(d)) >> 2);
}

static inline uint16_t mean4PairwiseMean2(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    return mean2Packed(mean2Packed(a, b), mean2Packed(c, d));
}

static inline uint16_t mean4UhaddNaive(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
#if TGX_BENCH_HAS_DSP32
    return (uint16_t)asmUhadd16(asmUhadd16(a, b), asmUhadd16(c, d));
#else
    return mean4PairwiseMean2(a, b, c, d);
#endif
}

static inline uint16_t trianglePackedCopy(uint16_t col1, int32_t c1, uint16_t col2, int32_t c2, uint16_t col3, int32_t tot)
{
    c1 <<= 5;
    c1 /= tot;
    c2 <<= 5;
    c2 /= tot;
    const uint32_t bg1 = (col1 | (col1 << 16)) & 0x07E0F81Fu;
    const uint32_t bg2 = (col2 | (col2 << 16)) & 0x07E0F81Fu;
    const uint32_t bg3 = (col3 | (col3 << 16)) & 0x07E0F81Fu;
    const uint32_t result = ((bg1 * (uint32_t)c1 + bg2 * (uint32_t)c2 + bg3 * (uint32_t)(32 - c1 - c2)) >> 5) & 0x07E0F81Fu;
    return contractPackedRgb565(result);
}

static inline uint16_t triangleScalar(uint16_t col1, int32_t c1, uint16_t col2, int32_t c2, uint16_t col3, int32_t tot)
{
    c1 = (c1 << 5) / tot;
    c2 = (c2 << 5) / tot;
    const int c3 = 32 - (int)c1 - (int)c2;
    return packRgb565((getR(col1) * c1 + getR(col2) * c2 + getR(col3) * c3) >> 5,
                      (getG(col1) * c1 + getG(col2) * c2 + getG(col3) * c3) >> 5,
                      (getB(col1) * c1 + getB(col2) * c2 + getB(col3) * c3) >> 5);
}

static inline uint16_t bilinearScalar(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, float ax, float ay)
{
    const int iax = (int)(ax * 256);
    const int iay = (int)(ay * 256);
    const int rax = 256 - iax;
    const int ray = 256 - iay;
    const int r = rax * (ray * getR(c00) + iay * getR(c01)) + iax * (ray * getR(c10) + iay * getR(c11));
    const int g = rax * (ray * getG(c00) + iay * getG(c01)) + iax * (ray * getG(c10) + iay * getG(c11));
    const int b = rax * (ray * getB(c00) + iay * getB(c01)) + iax * (ray * getB(c10) + iay * getB(c11));
    return packRgb565(r >> 16, g >> 16, b >> 16);
}

static inline uint16_t bilinearPackedMasks(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, float ax, float ay)
{
    const uint32_t iax = (uint32_t)(int)(ax * 256);
    const uint32_t iay = (uint32_t)(int)(ay * 256);
    const uint32_t rax = 256u - iax;
    const uint32_t ray = 256u - iay;
    const uint32_t w00 = rax * ray;
    const uint32_t w10 = iax * ray;
    const uint32_t w01 = rax * iay;
    const uint32_t w11 = iax * iay;
    const uint32_t rb = ((((uint32_t)(c00 & 0xF81Fu) * w00)
                       + ((uint32_t)(c10 & 0xF81Fu) * w10)
                       + ((uint32_t)(c01 & 0xF81Fu) * w01)
                       + ((uint32_t)(c11 & 0xF81Fu) * w11)) >> 16) & 0xF81Fu;
    const uint32_t g = ((((uint32_t)(c00 & 0x07E0u) * w00)
                      + ((uint32_t)(c10 & 0x07E0u) * w10)
                      + ((uint32_t)(c01 & 0x07E0u) * w01)
                      + ((uint32_t)(c11 & 0x07E0u) * w11)) >> 16) & 0x07E0u;
    return (uint16_t)(rb | g);
}

static inline uint16_t bilinearTwoStageApprox(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, float ax, float ay)
{
    const int iax = (int)(ax * 256);
    const int iay = (int)(ay * 256);
    const int r0 = (getR(c00) * (256 - iax) + getR(c10) * iax) >> 8;
    const int g0 = (getG(c00) * (256 - iax) + getG(c10) * iax) >> 8;
    const int b0 = (getB(c00) * (256 - iax) + getB(c10) * iax) >> 8;
    const int r1 = (getR(c01) * (256 - iax) + getR(c11) * iax) >> 8;
    const int g1 = (getG(c01) * (256 - iax) + getG(c11) * iax) >> 8;
    const int b1 = (getB(c01) * (256 - iax) + getB(c11) * iax) >> 8;
    return packRgb565((r0 * (256 - iay) + r1 * iay) >> 8,
                      (g0 * (256 - iay) + g1 * iay) >> 8,
                      (b0 * (256 - iay) + b1 * iay) >> 8);
}

static inline uint16_t rgbfScalar(const RGBf& c)
{
    return packRgb565((int)(c.R * 31.0f), (int)(c.G * 63.0f), (int)(c.B * 31.0f));
}

static uint32_t checksum16(const uint16_t* data, uint32_t count)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < count; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t countMismatches(const uint16_t* a, const uint16_t* b, uint32_t count)
{
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (a[i] != b[i]) {
            ++mismatches;
        }
    }
    return mismatches;
}

static void printResult(const char* kernel, const char* variant, uint32_t correct, uint32_t mismatches,
                        uint32_t timeUs, uint32_t cycles, uint32_t iterations, uint32_t count,
                        uint32_t checksum, const char* notes)
{
    Serial.print("KERNEL=");
    Serial.print(kernel);
    Serial.print(" VARIANT=");
    Serial.print(variant);
    Serial.print(" CORRECT=");
    Serial.print(correct);
    Serial.print(" MISMATCHES=");
    Serial.print(mismatches);
    Serial.print(" TIME_US=");
    Serial.print(timeUs);
    Serial.print(" CYCLES=");
    Serial.print(cycles);
    Serial.print(" ITERATIONS=");
    Serial.print(iterations);
    Serial.print(" COUNT=");
    Serial.print(count);
    Serial.print(" CHECKSUM=0x");
    Serial.print(checksum, HEX);
    if (notes && notes[0]) {
        Serial.print(" NOTES=");
        Serial.print(notes);
    }
    Serial.println();
}

static void initInputs()
{
    rngState = 0x9e3779b9u;
    const uint16_t edge[] = {
        0x0000u, 0xffffu, packRgb565(31, 0, 0), packRgb565(0, 63, 0),
        packRgb565(0, 0, 31), packRgb565(31, 63, 31), packRgb565(1, 1, 1),
        packRgb565(30, 62, 30)
    };

    for (uint32_t i = 0; i < kCount + 4; ++i) {
        if (i < (sizeof(edge) / sizeof(edge[0]))) {
            gA[i] = edge[i];
            gB[i] = edge[(i + 3) & 7];
            gC[i] = edge[(i + 5) & 7];
            gD[i] = edge[(i + 7) & 7];
        } else {
            gA[i] = (uint16_t)xorshift32();
            gB[i] = (uint16_t)xorshift32();
            gC[i] = (uint16_t)xorshift32();
            gD[i] = (uint16_t)xorshift32();
        }
        gAlpha[i] = xorshift32() % 257u;
        gMr[i] = (int32_t)(xorshift32() % 257u);
        gMg[i] = (int32_t)(xorshift32() % 257u);
        gMb[i] = (int32_t)(xorshift32() % 257u);
        gMa[i] = (int32_t)(xorshift32() % 257u);
        const int32_t tot = (int32_t)((xorshift32() % 4095u) + 1u);
        const int32_t c1 = (int32_t)(xorshift32() % (uint32_t)(tot + 1));
        const int32_t c2 = (int32_t)(xorshift32() % (uint32_t)(tot - c1 + 1));
        gTot[i] = tot;
        gC1[i] = c1;
        gC2[i] = c2;
        gAx[i] = (float)(xorshift32() & 0xffffu) / 65535.0f;
        gAy[i] = (float)(xorshift32() & 0xffffu) / 65535.0f;
        gFloat[i] = RGBf((float)(xorshift32() & 0xffffu) / 65535.0f,
                         (float)(xorshift32() & 0xffffu) / 65535.0f,
                         (float)(xorshift32() & 0xffffu) / 65535.0f);
    }

    gAlpha[0] = 0;
    gAlpha[1] = 1;
    gAlpha[2] = 16;
    gAlpha[3] = 32;
    gAlpha[4] = 64;
    gAlpha[5] = 128;
    gAlpha[6] = 192;
    gAlpha[7] = 255;
    gAlpha[8] = 256;
    gMr[0] = 0; gMg[0] = 0; gMb[0] = 0; gMa[0] = 0;
    gMr[1] = 1; gMg[1] = 1; gMb[1] = 1; gMa[1] = 1;
    gMr[2] = 16; gMg[2] = 16; gMb[2] = 16; gMa[2] = 16;
    gMr[3] = 128; gMg[3] = 128; gMb[3] = 128; gMa[3] = 128;
    gMr[4] = 255; gMg[4] = 255; gMb[4] = 255; gMa[4] = 255;
    gMr[5] = 256; gMg[5] = 256; gMb[5] = 256; gMa[5] = 256;
    gAx[0] = 0.0f; gAy[0] = 0.0f;
    gAx[1] = 1.0f; gAy[1] = 1.0f;
    gAx[2] = 0.5f; gAy[2] = 0.5f;
}

struct BlendRef { static inline uint16_t apply(uint16_t a, uint16_t b, uint32_t alpha) { return refBlend256(a, b, alpha); } };
struct BlendPacked { static inline uint16_t apply(uint16_t a, uint16_t b, uint32_t alpha) { return blendPackedCopy(a, b, alpha); } };
struct BlendWeighted { static inline uint16_t apply(uint16_t a, uint16_t b, uint32_t alpha) { return blendScalarWeighted(a, b, alpha); } };
struct BlendDiff { static inline uint16_t apply(uint16_t a, uint16_t b, uint32_t alpha) { return blendChannelDiff(a, b, alpha); } };

struct MultRef { static inline uint16_t apply(uint16_t c, int mr, int mg, int mb) { return refMult256(c, mr, mg, mb); } };
struct MultShiftPack { static inline uint16_t apply(uint16_t c, int mr, int mg, int mb) { return multShiftPack(c, mr, mg, mb); } };
struct MultARef { static inline uint16_t apply(uint16_t c, int mr, int mg, int mb, int ma) { return refMult256A(c, mr, mg, mb, ma); } };
struct MultAShiftPack { static inline uint16_t apply(uint16_t c, int mr, int mg, int mb, int ma) { return multShiftPackA(c, mr, mg, mb, ma); } };

struct Mean2Ref { static inline uint16_t apply(uint16_t a, uint16_t b) { return refMean2(a, b); } };
struct Mean2Packed { static inline uint16_t apply(uint16_t a, uint16_t b) { return mean2Packed(a, b); } };
struct Mean2Scalar { static inline uint16_t apply(uint16_t a, uint16_t b) { return mean2Scalar(a, b); } };
struct Mean2Uhadd { static inline uint16_t apply(uint16_t a, uint16_t b) { return mean2UhaddNaive(a, b); } };

struct Mean4Ref { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return refMean4(a, b, c, d); } };
struct Mean4Packed { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return mean4Packed(a, b, c, d); } };
struct Mean4Scalar { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return mean4Scalar(a, b, c, d); } };
struct Mean4Pairwise { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return mean4PairwiseMean2(a, b, c, d); } };
struct Mean4Uhadd { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d) { return mean4UhaddNaive(a, b, c, d); } };

struct TriangleRef { static inline uint16_t apply(uint16_t a, int32_t c1, uint16_t b, int32_t c2, uint16_t c, int32_t tot) { return refTriangle(a, c1, b, c2, c, tot); } };
struct TrianglePacked { static inline uint16_t apply(uint16_t a, int32_t c1, uint16_t b, int32_t c2, uint16_t c, int32_t tot) { return trianglePackedCopy(a, c1, b, c2, c, tot); } };
struct TriangleScalar { static inline uint16_t apply(uint16_t a, int32_t c1, uint16_t b, int32_t c2, uint16_t c, int32_t tot) { return triangleScalar(a, c1, b, c2, c, tot); } };

struct BilinearRef { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d, float ax, float ay) { return refBilinear(a, b, c, d, ax, ay); } };
struct BilinearScalar { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d, float ax, float ay) { return bilinearScalar(a, b, c, d, ax, ay); } };
struct BilinearPackedMasks { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d, float ax, float ay) { return bilinearPackedMasks(a, b, c, d, ax, ay); } };
struct BilinearTwoStage { static inline uint16_t apply(uint16_t a, uint16_t b, uint16_t c, uint16_t d, float ax, float ay) { return bilinearTwoStageApprox(a, b, c, d, ax, ay); } };

struct RgbfRef { static inline uint16_t apply(const RGBf& c) { return refRgbfTo565(c); } };
struct RgbfScalar { static inline uint16_t apply(const RGBf& c) { return rgbfScalar(c); } };

template<typename Variant>
static void runBlendFixed(const char* variant, uint32_t alpha)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = BlendRef::apply(gA[i], gB[i], alpha);
        gOut[i] = Variant::apply(gA[i], gB[i], alpha);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gB[i], alpha);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[32];
    snprintf(kernel, sizeof(kernel), "blend256_a%lu", (unsigned long)alpha);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runBlendRandom(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = BlendRef::apply(gA[i], gB[i], gAlpha[i]);
        gOut[i] = Variant::apply(gA[i], gB[i], gAlpha[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gB[i], gAlpha[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("blend256_random_alpha", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runMult256(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = MultRef::apply(gA[i], gMr[i], gMg[i], gMb[i]);
        gOut[i] = Variant::apply(gA[i], gMr[i], gMg[i], gMb[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gMr[i], gMg[i], gMb[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("mult256_rgb", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runMult256A(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = MultARef::apply(gA[i], gMr[i], gMg[i], gMb[i], gMa[i]);
        gOut[i] = Variant::apply(gA[i], gMr[i], gMg[i], gMb[i], gMa[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gMr[i], gMg[i], gMb[i], gMa[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("mult256_rgba", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runMean2(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = Mean2Ref::apply(gA[i], gB[i]);
        gOut[i] = Variant::apply(gA[i], gB[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gB[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("meanColor2", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runMean4(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = Mean4Ref::apply(gA[i], gB[i], gC[i], gD[i]);
        gOut[i] = Variant::apply(gA[i], gB[i], gC[i], gD[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gB[i], gC[i], gD[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("meanColor4", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runTriangle(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = TriangleRef::apply(gA[i], gC1[i], gB[i], gC2[i], gC[i], gTot[i]);
        gOut[i] = Variant::apply(gA[i], gC1[i], gB[i], gC2[i], gC[i], gTot[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gC1[i], gB[i], gC2[i], gC[i], gTot[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("interpolateTriangle", variant, mismatches == 0, mismatches, timeUs, cycles, kTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runBilinear(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = BilinearRef::apply(gA[i], gB[i], gC[i], gD[i], gAx[i], gAy[i]);
        gOut[i] = Variant::apply(gA[i], gB[i], gC[i], gD[i], gAx[i], gAy[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kFloatTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gA[i], gB[i], gC[i], gD[i], gAx[i], gAy[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("interpolateBilinear", variant, mismatches == 0, mismatches, timeUs, cycles, kFloatTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

template<typename Variant>
static void runRgbfTo565(const char* variant)
{
    for (uint32_t i = 0; i < kCount; ++i) {
        gRef[i] = RgbfRef::apply(gFloat[i]);
        gOut[i] = Variant::apply(gFloat[i]);
    }
    const uint32_t mismatches = countMismatches(gRef, gOut, kCount);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kFloatTimingRepeats; ++rep) {
        for (uint32_t i = 0; i < kCount; ++i) {
            sum = (sum * 33u) ^ Variant::apply(gFloat[i]);
        }
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    printResult("rgbf_to_rgb565", variant, mismatches == 0, mismatches, timeUs, cycles, kFloatTimingRepeats, kCount, checksum16(gOut, kCount), "");
}

static void spanMean2Ref(uint16_t* out, const uint16_t* a, const uint16_t* b, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refMean2(a[i], b[i]);
}

static void spanMean2Packed16(uint16_t* out, const uint16_t* a, const uint16_t* b, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = mean2Packed(a[i], b[i]);
}

static void spanMean2Packed32(uint16_t* out, const uint16_t* a, const uint16_t* b, uint32_t n)
{
    uint32_t i = 0;
    for (; i + 1 < n; i += 2) {
        uint32_t aa;
        uint32_t bb;
        memcpy(&aa, a + i, sizeof(uint32_t));
        memcpy(&bb, b + i, sizeof(uint32_t));
        const uint32_t r = (aa & bb) + (((aa ^ bb) & 0xF7DEF7DEu) >> 1);
        memcpy(out + i, &r, sizeof(uint32_t));
    }
    for (; i < n; ++i) out[i] = mean2Packed(a[i], b[i]);
}

static void spanBlendRef(uint16_t* out, const uint16_t* bg, const uint16_t* fg, uint32_t n, uint32_t alpha)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refBlend256(bg[i], fg[i], alpha);
}

static void spanBlendPacked(uint16_t* out, const uint16_t* bg, const uint16_t* fg, uint32_t n, uint32_t alpha)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = blendPackedCopy(bg[i], fg[i], alpha);
}

static void spanBlendVarRef(uint16_t* out, const uint16_t* bg, const uint16_t* fg, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refBlend256(bg[i], fg[i], gAlpha[i]);
}

static void spanBlendVarPacked(uint16_t* out, const uint16_t* bg, const uint16_t* fg, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = blendPackedCopy(bg[i], fg[i], gAlpha[i]);
}

static void spanMultRef(uint16_t* out, const uint16_t* in, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refMult256(in[i], gMr[i], gMg[i], gMb[i]);
}

static void spanMultShiftPack(uint16_t* out, const uint16_t* in, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = multShiftPack(in[i], gMr[i], gMg[i], gMb[i]);
}

static void spanMultARef(uint16_t* out, const uint16_t* in, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refMult256A(in[i], gMr[i], gMg[i], gMb[i], gMa[i]);
}

static void spanMultAShiftPack(uint16_t* out, const uint16_t* in, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = multShiftPackA(in[i], gMr[i], gMg[i], gMb[i], gMa[i]);
}

static void spanTriangleRef(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refTriangle(a[i], gC1[i], b[i], gC2[i], c[i], gTot[i]);
}

static void spanTrianglePacked(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = trianglePackedCopy(a[i], gC1[i], b[i], gC2[i], c[i], gTot[i]);
}

static void spanTriangleScalar(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = triangleScalar(a[i], gC1[i], b[i], gC2[i], c[i], gTot[i]);
}

static void spanBilinearRef(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, const uint16_t* d, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = refBilinear(a[i], b[i], c[i], d[i], gAx[i], gAy[i]);
}

static void spanBilinearScalar(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, const uint16_t* d, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = bilinearScalar(a[i], b[i], c[i], d[i], gAx[i], gAy[i]);
}

static void spanBilinearPackedMasks(uint16_t* out, const uint16_t* a, const uint16_t* b, const uint16_t* c, const uint16_t* d, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = bilinearPackedMasks(a[i], b[i], c[i], d[i], gAx[i], gAy[i]);
}

static void spanCopy16(uint16_t* out, const uint16_t* in, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = in[i];
}

static void spanCopy32(uint16_t* out, const uint16_t* in, uint32_t n)
{
    uint32_t i = 0;
    for (; i + 1 < n; i += 2) {
        uint32_t v;
        memcpy(&v, in + i, sizeof(uint32_t));
        memcpy(out + i, &v, sizeof(uint32_t));
    }
    for (; i < n; ++i) out[i] = in[i];
}

static void spanFill16(uint16_t* out, uint16_t value, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) out[i] = value;
}

static void spanFill32(uint16_t* out, uint16_t value, uint32_t n)
{
    const uint32_t v = (uint32_t)value | ((uint32_t)value << 16);
    uint32_t i = 0;
    for (; i + 1 < n; i += 2) {
        memcpy(out + i, &v, sizeof(uint32_t));
    }
    for (; i < n; ++i) out[i] = value;
}

typedef void (*SpanMean2Fn)(uint16_t*, const uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanBlendFn)(uint16_t*, const uint16_t*, const uint16_t*, uint32_t, uint32_t);
typedef void (*SpanBlendVarFn)(uint16_t*, const uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanMultFn)(uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanTriangleFn)(uint16_t*, const uint16_t*, const uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanBilinearFn)(uint16_t*, const uint16_t*, const uint16_t*, const uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanCopyFn)(uint16_t*, const uint16_t*, uint32_t);
typedef void (*SpanFillFn)(uint16_t*, uint16_t, uint32_t);

static void runSpanMean2(const char* variant, SpanMean2Fn fn, uint32_t n, uint32_t offset)
{
    spanMean2Ref(gSpanRef + offset, gA + offset, gB + offset, n);
    fn(gSpanOut + offset, gA + offset, gB + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, gB + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_mean2_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanBlend(const char* variant, SpanBlendFn fn, uint32_t n, uint32_t offset)
{
    spanBlendRef(gSpanRef + offset, gA + offset, gB + offset, n, 128);
    fn(gSpanOut + offset, gA + offset, gB + offset, n, 128);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, gB + offset, n, 128);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_blend_a128_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanBlendVar(const char* variant, SpanBlendVarFn fn, uint32_t n, uint32_t offset)
{
    spanBlendVarRef(gSpanRef + offset, gA + offset, gB + offset, n);
    fn(gSpanOut + offset, gA + offset, gB + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, gB + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_blend_var_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanMult(const char* variant, SpanMultFn fn, uint32_t n, uint32_t offset)
{
    spanMultRef(gSpanRef + offset, gA + offset, n);
    fn(gSpanOut + offset, gA + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_mult256_rgb_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanMultA(const char* variant, SpanMultFn fn, uint32_t n, uint32_t offset)
{
    spanMultARef(gSpanRef + offset, gA + offset, n);
    fn(gSpanOut + offset, gA + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_mult256_rgba_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanTriangle(const char* variant, SpanTriangleFn fn, uint32_t n, uint32_t offset)
{
    spanTriangleRef(gSpanRef + offset, gA + offset, gB + offset, gC + offset, n);
    fn(gSpanOut + offset, gA + offset, gB + offset, gC + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < 128; ++rep) {
        fn(gSpanOut + offset, gA + offset, gB + offset, gC + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_triangle_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, 128, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanBilinear(const char* variant, SpanBilinearFn fn, uint32_t n, uint32_t offset)
{
    spanBilinearRef(gSpanRef + offset, gA + offset, gB + offset, gC + offset, gD + offset, n);
    fn(gSpanOut + offset, gA + offset, gB + offset, gC + offset, gD + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < 64; ++rep) {
        fn(gSpanOut + offset, gA + offset, gB + offset, gC + offset, gD + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_bilinear_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, 64, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanCopy(const char* variant, SpanCopyFn fn, uint32_t n, uint32_t offset)
{
    spanCopy16(gSpanRef + offset, gA + offset, n);
    fn(gSpanOut + offset, gA + offset, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, gA + offset, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_copy_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runSpanFill(const char* variant, SpanFillFn fn, uint32_t n, uint32_t offset)
{
    spanFill16(gSpanRef + offset, 0x39e7u, n);
    fn(gSpanOut + offset, 0x39e7u, n);
    const uint32_t mismatches = countMismatches(gSpanRef + offset, gSpanOut + offset, n);
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    uint32_t sum = 0;
    for (uint32_t rep = 0; rep < kSpanTimingRepeats; ++rep) {
        fn(gSpanOut + offset, 0x39e7u, n);
        sum ^= checksum16(gSpanOut + offset, n);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sum;
    char kernel[48];
    snprintf(kernel, sizeof(kernel), "span_fill_n%lu_o%lu", (unsigned long)n, (unsigned long)offset);
    printResult(kernel, variant, mismatches == 0, mismatches, timeUs, cycles, kSpanTimingRepeats, n, checksum16(gSpanOut + offset, n), "");
}

static void runCoreKernels()
{
    const uint32_t alphas[] = {0, 1, 16, 32, 64, 128, 192, 255, 256};
    for (uint32_t i = 0; i < sizeof(alphas) / sizeof(alphas[0]); ++i) {
        runBlendFixed<BlendRef>("current_tgx", alphas[i]);
        runBlendFixed<BlendPacked>("packed_copy", alphas[i]);
        runBlendFixed<BlendWeighted>("scalar_weighted", alphas[i]);
        runBlendFixed<BlendDiff>("scalar_diff", alphas[i]);
    }
    runBlendRandom<BlendRef>("current_tgx");
    runBlendRandom<BlendPacked>("packed_copy");
    runBlendRandom<BlendWeighted>("scalar_weighted");
    runBlendRandom<BlendDiff>("scalar_diff");

    runMult256<MultRef>("current_tgx");
    runMult256<MultShiftPack>("shift_pack");
    runMult256A<MultARef>("current_tgx");
    runMult256A<MultAShiftPack>("shift_pack");

    runMean2<Mean2Ref>("current_tgx");
    runMean2<Mean2Packed>("packed_bit_hack");
    runMean2<Mean2Scalar>("scalar_channels");
    runMean2<Mean2Uhadd>("arm_uhadd16_naive");

    runMean4<Mean4Ref>("current_tgx");
    runMean4<Mean4Packed>("packed_lane_sum");
    runMean4<Mean4Scalar>("scalar_channels");
    runMean4<Mean4Pairwise>("pairwise_mean2");
    runMean4<Mean4Uhadd>("arm_uhadd16_pairwise_naive");

    runTriangle<TriangleRef>("current_tgx");
    runTriangle<TrianglePacked>("packed_copy");
    runTriangle<TriangleScalar>("scalar_channels");

    runBilinear<BilinearRef>("current_tgx");
    runBilinear<BilinearScalar>("scalar_channels");
    runBilinear<BilinearPackedMasks>("packed_masks");
    runBilinear<BilinearTwoStage>("two_stage_approx");

    runRgbfTo565<RgbfRef>("current_tgx");
    runRgbfTo565<RgbfScalar>("scalar_pack");
}

static void runSpanKernels()
{
    const uint32_t lengths[] = {1, 2, 4, 8, 16, 32, 64, 128, 320};
    for (uint32_t li = 0; li < sizeof(lengths) / sizeof(lengths[0]); ++li) {
        const uint32_t n = lengths[li];
        for (uint32_t offset = 0; offset <= 1; ++offset) {
            runSpanMean2("current_tgx_loop", spanMean2Ref, n, offset);
            runSpanMean2("packed16_loop", spanMean2Packed16, n, offset);
            runSpanMean2("packed32_two_pixels", spanMean2Packed32, n, offset);
            runSpanBlend("current_tgx_loop", spanBlendRef, n, offset);
            runSpanBlend("packed_copy_loop", spanBlendPacked, n, offset);
            runSpanBlendVar("current_tgx_loop", spanBlendVarRef, n, offset);
            runSpanBlendVar("packed_copy_loop", spanBlendVarPacked, n, offset);
            runSpanMult("current_tgx_loop", spanMultRef, n, offset);
            runSpanMult("shift_pack_loop", spanMultShiftPack, n, offset);
            runSpanMultA("current_tgx_loop", spanMultARef, n, offset);
            runSpanMultA("shift_pack_loop", spanMultAShiftPack, n, offset);
            runSpanTriangle("current_tgx_loop", spanTriangleRef, n, offset);
            runSpanTriangle("packed_copy_loop", spanTrianglePacked, n, offset);
            runSpanTriangle("scalar_channels_loop", spanTriangleScalar, n, offset);
            runSpanBilinear("current_tgx_loop", spanBilinearRef, n, offset);
            runSpanBilinear("scalar_channels_loop", spanBilinearScalar, n, offset);
            runSpanBilinear("packed_masks_loop", spanBilinearPackedMasks, n, offset);
            runSpanCopy("copy16_loop", spanCopy16, n, offset);
            runSpanCopy("copy32_two_pixels", spanCopy32, n, offset);
            runSpanFill("fill16_loop", spanFill16, n, offset);
            runSpanFill("fill32_two_pixels", spanFill32, n, offset);
        }
    }
}

void setup()
{
    Serial.begin(9600);
    waitForSerial();
    initCycleCounter();
    initInputs();

    Serial.println("TGX_RGB565_HOT3D_BENCH_BEGIN");
#if defined(ARDUINO_TEENSY41)
    Serial.println("BOARD=TEENSY41");
#elif defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350) || defined(TARGET_RP2350)
    Serial.println("BOARD=PICO2_RP2350");
#else
    Serial.println("BOARD=UNKNOWN");
#endif
    Serial.print("CONFIG=TGX_RGB565_ORDER_BGR:");
    Serial.print(TGX_RGB565_ORDER_BGR);
    Serial.print(",DSP32:");
    Serial.print(TGX_BENCH_HAS_DSP32);
    Serial.print(",DWT:");
    Serial.print(TGX_BENCH_HAS_DWT);
    Serial.print(",TGX_INLINE:");
    Serial.println(STR(TGX_INLINE));

    runCoreKernels();
    runSpanKernels();

    Serial.print("SINK=0x");
    Serial.println(gSink, HEX);
    Serial.println("TGX_RGB565_HOT3D_BENCH_END");
}

void loop()
{
    delay(1000);
}
