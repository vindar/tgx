#include <Arduino.h>
#include <tgx.h>
#include <string.h>

using tgx::RGB565;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
#define TGX_SPAN_HAS_DWT 1
#else
#define TGX_SPAN_HAS_DWT 0
#endif

static const uint32_t kMaxLen = 320;
static const uint32_t kRows = 16;
static const uint32_t kPitch = kMaxLen + 8;
static const uint32_t kTotal = kRows * kPitch;
static const uint32_t kTexW = 64;
static const uint32_t kTexH = 64;
static const uint32_t kTexMask = 63;

static uint16_t gFbInit[kTotal + 4];
static uint16_t gFbRef[kTotal + 4];
static uint16_t gFbOut[kTotal + 4];
static float gZInit[kTotal + 4];
static float gZValue[kTotal + 4];
static float gZRef[kTotal + 4];
static float gZOut[kTotal + 4];
static float gTx[kTotal + 4];
static float gTy[kTotal + 4];
static int32_t gRowTx0[kRows];
static int32_t gRowTy0[kRows];
static int32_t gRowDtx[kRows];
static int32_t gRowDty[kRows];
static uint16_t gTex[kTexW * kTexH + 4];
static uint16_t gTexel[kTotal + 4];
static uint16_t gTexelBilinear[kTotal + 4];
static int32_t gC2[kTotal + 4];
static int32_t gC3[kTotal + 4];
static volatile uint32_t gSink = 0;
static uint32_t gRng = 0x31415926u;

enum ZPattern : uint8_t
    {
    Z_ALLPASS = 0,
    Z_MIXED = 1,
    Z_ALLFAIL = 2
    };

typedef void (*SpanFn)(uint32_t len, uint32_t offset, ZPattern pattern);

static uint32_t xorshift32()
{
    uint32_t x = gRng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    gRng = x;
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

#if TGX_SPAN_HAS_DWT
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

static inline uint16_t rgb565FromSeed(uint32_t s)
{
    return packRgb565((int)(s & 31u), (int)((s >> 5) & 63u), (int)((s >> 11) & 31u));
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

static inline uint16_t triangleScalar(uint16_t col1, int32_t c1, uint16_t col2, int32_t c2, uint16_t col3, int32_t tot)
{
    c1 = (c1 << 5) / tot;
    c2 = (c2 << 5) / tot;
    const int c3 = 32 - (int)c1 - (int)c2;
    return packRgb565((getR(col1) * c1 + getR(col2) * c2 + getR(col3) * c3) >> 5,
                      (getG(col1) * c1 + getG(col2) * c2 + getG(col3) * c3) >> 5,
                      (getB(col1) * c1 + getB(col2) * c2 + getB(col3) * c3) >> 5);
}

static inline uint16_t sampleNearest(float tx, float ty)
{
    const int x = ((int)tx) & (int)kTexMask;
    const int y = ((int)ty) & (int)kTexMask;
    return gTex[(uint32_t)x + ((uint32_t)y * kTexW)];
}

static inline uint16_t sampleBilinear(float tx, float ty)
{
    const int ix = (int)floorf(tx);
    const int iy = (int)floorf(ty);
    const float ax = tx - ix;
    const float ay = ty - iy;
    const uint32_t minx = (uint32_t)(ix & (int)kTexMask);
    const uint32_t maxx = (uint32_t)((ix + 1) & (int)kTexMask);
    const uint32_t miny = (uint32_t)(iy & (int)kTexMask) * kTexW;
    const uint32_t maxy = (uint32_t)((iy + 1) & (int)kTexMask) * kTexW;
    return tgx::interpolateColorsBilinear(RGB565(gTex[minx + miny]), RGB565(gTex[maxx + miny]), RGB565(gTex[minx + maxy]), RGB565(gTex[maxx + maxy]), ax, ay).val;
}

static inline uint16_t sampleNearestFixed8(int32_t tx, int32_t ty)
{
    const uint32_t x = (uint32_t)((tx >> 8) & (int32_t)kTexMask);
    const uint32_t y = (uint32_t)((ty >> 8) & (int32_t)kTexMask);
    return gTex[x + y * kTexW];
}

static inline uint16_t sampleBilinearFixed8(int32_t tx, int32_t ty)
{
    const int ix = tx >> 8;
    const int iy = ty >> 8;
    const float ax = (float)(tx & 255) * (1.0f / 256.0f);
    const float ay = (float)(ty & 255) * (1.0f / 256.0f);
    const uint32_t minx = (uint32_t)(ix & (int)kTexMask);
    const uint32_t maxx = (uint32_t)((ix + 1) & (int)kTexMask);
    const uint32_t miny = (uint32_t)(iy & (int)kTexMask) * kTexW;
    const uint32_t maxy = (uint32_t)((iy + 1) & (int)kTexMask) * kTexW;
    return tgx::interpolateColorsBilinear(RGB565(gTex[minx + miny]), RGB565(gTex[maxx + miny]), RGB565(gTex[minx + maxy]), RGB565(gTex[maxx + maxy]), ax, ay).val;
}

static inline bool zPass(float oldz, float newz)
{
    return oldz < newz;
}

static uint32_t checksum16Span(const uint16_t* p, uint32_t len, uint32_t offset)
{
    uint32_t h = 2166136261u;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            h ^= p[base + x];
            h *= 16777619u;
        }
    }
    return h;
}

static uint32_t checksumFloatBitsSpan(const float* p, uint32_t len, uint32_t offset)
{
    uint32_t h = 2166136261u;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            uint32_t v;
            memcpy(&v, p + base + x, sizeof(v));
            h ^= v;
            h *= 16777619u;
        }
    }
    return h;
}

static uint32_t countMismatches(uint32_t len, uint32_t offset)
{
    uint32_t mismatches = 0;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            if (gFbRef[i] != gFbOut[i]) mismatches++;
            uint32_t a, b;
            memcpy(&a, gZRef + i, sizeof(a));
            memcpy(&b, gZOut + i, sizeof(b));
            if (a != b) mismatches++;
        }
    }
    return mismatches;
}

static uint32_t repeatsForLen(uint32_t len)
{
    if (len <= 5) return 512;
    if (len <= 16) return 256;
    if (len <= 64) return 128;
    return 64;
}

static const char* patternName(ZPattern pattern)
{
    switch (pattern) {
    case Z_ALLPASS: return "allpass";
    case Z_MIXED: return "mixed";
    case Z_ALLFAIL: return "allfail";
    default: return "unknown";
    }
}

static void initTexture()
{
    gRng = 0x10203040u;
    for (uint32_t y = 0; y < kTexH; ++y) {
        for (uint32_t x = 0; x < kTexW; ++x) {
            const uint32_t s = (x * 17u) ^ (y * 131u) ^ xorshift32();
            gTex[x + y * kTexW] = rgb565FromSeed(s);
        }
    }
}

static void initInputs()
{
    initTexture();
    gRng = 0x55667788u;
    for (uint32_t row = 0; row < kRows; ++row) {
        gRowTx0[row] = (int32_t)((row * 997u + 101u) & ((kTexW << 8) - 1u));
        gRowTy0[row] = (int32_t)((row * 619u + 217u) & ((kTexH << 8) - 1u));
        gRowDtx[row] = (int32_t)(17u + ((row * 13u) & 63u));
        gRowDty[row] = (int32_t)(11u + ((row * 19u) & 63u));
    }
    for (uint32_t row = 0; row < kRows; ++row) {
        for (uint32_t x = 0; x < kPitch; ++x) {
            const uint32_t i = row * kPitch + x;
            gFbInit[i] = rgb565FromSeed(0x1234u + i * 97u);
            const float base = 0.10f + 0.00011f * (float)((i * 13u) & 1023u);
            gZInit[i] = base;
            gZValue[i] = base + 0.20f + 0.00007f * (float)((i * 19u) & 255u);
            gTx[i] = (float)((i * 7u + row * 3u) & 4095u) * (1.0f / 23.0f);
            gTy[i] = (float)((i * 5u + row * 11u) & 4095u) * (1.0f / 29.0f);
            gC2[i] = (int32_t)((i * 37u) & 2047u);
            gC3[i] = (int32_t)((i * 53u) & 2047u);
            gTexel[i] = sampleNearest(gTx[i], gTy[i]);
            gTexelBilinear[i] = sampleBilinear(gTx[i], gTy[i]);
        }
    }
}

static void resetBuffers(uint32_t len, uint32_t offset, ZPattern pattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const bool pass = (pattern == Z_ALLPASS) || ((pattern == Z_MIXED) && (((x + row) & 1u) == 0));
            const float candidate = gZValue[i];
            gFbRef[i] = gFbInit[i];
            gFbOut[i] = gFbInit[i];
            if (pattern == Z_ALLFAIL) {
                gZRef[i] = candidate + 1.0f;
                gZOut[i] = candidate + 1.0f;
            } else if (pass) {
                gZRef[i] = gZInit[i];
                gZOut[i] = gZInit[i];
            } else {
                gZRef[i] = candidate + 1.0f;
                gZOut[i] = candidate + 1.0f;
            }
        }
    }
}

static void resetOutputBuffers(uint32_t len, uint32_t offset, ZPattern pattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const bool pass = (pattern == Z_ALLPASS) || ((pattern == Z_MIXED) && (((x + row) & 1u) == 0));
            const float candidate = gZValue[i];
            gFbOut[i] = gFbInit[i];
            if (pattern == Z_ALLFAIL) {
                gZOut[i] = candidate + 1.0f;
            } else if (pass) {
                gZOut[i] = gZInit[i];
            } else {
                gZOut[i] = candidate + 1.0f;
            }
        }
    }
}

static void saveReference(uint32_t len, uint32_t offset)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbRef[i] = gFbOut[i];
            gZRef[i] = gZOut[i];
        }
    }
}

static uint32_t activePixels(uint32_t len)
{
    return len * kRows;
}

static void zOnlyIndex(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float z = gZValue[i];
            if (gZOut[i] < z) gZOut[i] = z;
        }
    }
}

static void zOnlyPtr(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        float* z = gZOut + base;
        const float* zv = gZValue + base;
        for (uint32_t x = 0; x < len; ++x) {
            if (*z < *zv) *z = *zv;
            z++;
            zv++;
        }
    }
}

static void zOnlySelectWrite(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float oldz = gZOut[i];
            const float z = gZValue[i];
            gZOut[i] = (oldz < z) ? z : oldz;
        }
    }
}

static const uint16_t kFlatColor = 0x7BEFu;

static void flatNoZIndex(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) gFbOut[base + x] = kFlatColor;
    }
}

static void flatNoZPtr(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        uint16_t* p = gFbOut + row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) *p++ = kFlatColor;
    }
}

static void flatNoZStore32(uint32_t len, uint32_t offset, ZPattern)
{
    const uint32_t packed = (uint32_t)kFlatColor | ((uint32_t)kFlatColor << 16);
    for (uint32_t row = 0; row < kRows; ++row) {
        uint32_t x = 0;
        uint16_t* p = gFbOut + row * kPitch + offset;
        if (((uintptr_t)p & 3u) && (x < len)) {
            *p++ = kFlatColor;
            x++;
        }
        for (; x + 1 < len; x += 2) {
            memcpy(p, &packed, sizeof(packed));
            p += 2;
        }
        if (x < len) *p = kFlatColor;
    }
}

static void flatNoZUnroll4(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        uint32_t x = 0;
        uint16_t* p = gFbOut + row * kPitch + offset;
        for (; x + 3 < len; x += 4) {
            p[0] = kFlatColor; p[1] = kFlatColor; p[2] = kFlatColor; p[3] = kFlatColor;
            p += 4;
        }
        for (; x < len; ++x) *p++ = kFlatColor;
    }
}

static void flatZIndex(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float z = gZValue[i];
            if (gZOut[i] < z) {
                gZOut[i] = z;
                gFbOut[i] = kFlatColor;
            }
        }
    }
}

static void flatZPtr(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        uint16_t* fb = gFbOut + base;
        float* zbuf = gZOut + base;
        const float* zv = gZValue + base;
        for (uint32_t x = 0; x < len; ++x) {
            if (*zbuf < *zv) {
                *zbuf = *zv;
                *fb = kFlatColor;
            }
            fb++;
            zbuf++;
            zv++;
        }
    }
}

static void flatZAllPassFast(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gZOut[i] = gZValue[i];
            gFbOut[i] = kFlatColor;
        }
    }
}

static const uint16_t kG0 = 0x001Fu;
static const uint16_t kG1 = 0xF800u;
static const uint16_t kG2 = 0x07E0u;
static const int32_t kArea = 2048;

static void gouraudRef(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = tgx::interpolateColorsTriangle(RGB565(kG1), gC2[i], RGB565(kG2), gC3[i], RGB565(kG0), kArea).val;
        }
    }
}

static void gouraudScalar(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = triangleScalar(kG1, gC2[i], kG2, gC3[i], kG0, kArea);
        }
    }
}

static void gouraudZRef(uint32_t len, uint32_t offset, ZPattern pattern)
{
    (void)pattern;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float z = gZValue[i];
            if (gZOut[i] < z) {
                gZOut[i] = z;
                gFbOut[i] = tgx::interpolateColorsTriangle(RGB565(kG1), gC2[i], RGB565(kG2), gC3[i], RGB565(kG0), kArea).val;
            }
        }
    }
}

static void texNearestRef(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = sampleNearest(gTx[i], gTy[i]);
        }
    }
}

static void texNearestPrecomputed(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = gTexel[i];
        }
    }
}

static void texNearestZIndex(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float z = gZValue[i];
            if (gZOut[i] < z) {
                gZOut[i] = z;
                gFbOut[i] = sampleNearest(gTx[i], gTy[i]);
            }
        }
    }
}

static void texNearestZPtr(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        uint16_t* fb = gFbOut + base;
        float* zbuf = gZOut + base;
        const float* zv = gZValue + base;
        const float* tx = gTx + base;
        const float* ty = gTy + base;
        for (uint32_t x = 0; x < len; ++x) {
            if (*zbuf < *zv) {
                *zbuf = *zv;
                *fb = sampleNearest(*tx, *ty);
            }
            fb++;
            zbuf++;
            zv++;
            tx++;
            ty++;
        }
    }
}

static void texBilinearRef(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = sampleBilinear(gTx[i], gTy[i]);
        }
    }
}

static void texBilinearScalar(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const int ix = (int)floorf(gTx[i]);
            const int iy = (int)floorf(gTy[i]);
            const float ax = gTx[i] - ix;
            const float ay = gTy[i] - iy;
            const uint32_t minx = (uint32_t)(ix & (int)kTexMask);
            const uint32_t maxx = (uint32_t)((ix + 1) & (int)kTexMask);
            const uint32_t miny = (uint32_t)(iy & (int)kTexMask) * kTexW;
            const uint32_t maxy = (uint32_t)((iy + 1) & (int)kTexMask) * kTexW;
            gFbOut[i] = bilinearScalar(gTex[minx + miny], gTex[maxx + miny], gTex[minx + maxy], gTex[maxx + maxy], ax, ay);
        }
    }
}

static void texBilinearPrecomputed(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            gFbOut[i] = gTexelBilinear[i];
        }
    }
}

static void texNearestIncFloat(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        int32_t txi = gRowTx0[row] + (int32_t)(offset * 7u);
        int32_t tyi = gRowTy0[row] + (int32_t)(offset * 5u);
        for (uint32_t x = 0; x < len; ++x) {
            const float tx = (float)txi * (1.0f / 256.0f);
            const float ty = (float)tyi * (1.0f / 256.0f);
            gFbOut[base + x] = sampleNearest(tx, ty);
            txi += gRowDtx[row];
            tyi += gRowDty[row];
        }
    }
}

static void texNearestIncFixed8(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        int32_t tx = gRowTx0[row] + (int32_t)(offset * 7u);
        int32_t ty = gRowTy0[row] + (int32_t)(offset * 5u);
        const int32_t dtx = gRowDtx[row];
        const int32_t dty = gRowDty[row];
        for (uint32_t x = 0; x < len; ++x) {
            gFbOut[base + x] = sampleNearestFixed8(tx, ty);
            tx += dtx;
            ty += dty;
        }
    }
}

static void texBilinearIncFloat(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        int32_t txi = gRowTx0[row] + (int32_t)(offset * 7u);
        int32_t tyi = gRowTy0[row] + (int32_t)(offset * 5u);
        for (uint32_t x = 0; x < len; ++x) {
            const float tx = (float)txi * (1.0f / 256.0f);
            const float ty = (float)tyi * (1.0f / 256.0f);
            gFbOut[base + x] = sampleBilinear(tx, ty);
            txi += gRowDtx[row];
            tyi += gRowDty[row];
        }
    }
}

static void texBilinearIncFixed8(uint32_t len, uint32_t offset, ZPattern)
{
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        int32_t tx = gRowTx0[row] + (int32_t)(offset * 7u);
        int32_t ty = gRowTy0[row] + (int32_t)(offset * 5u);
        const int32_t dtx = gRowDtx[row];
        const int32_t dty = gRowDty[row];
        for (uint32_t x = 0; x < len; ++x) {
            gFbOut[base + x] = sampleBilinearFixed8(tx, ty);
            tx += dtx;
            ty += dty;
        }
    }
}

static void texModRef(uint32_t len, uint32_t offset, ZPattern)
{
    const int mr = 221;
    const int mg = 177;
    const int mb = 239;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            RGB565 c(gTexel[i]);
            c.mult256(mr, mg, mb);
            gFbOut[i] = c.val;
        }
    }
}

static void texModZIndex(uint32_t len, uint32_t offset, ZPattern)
{
    const int mr = 221;
    const int mg = 177;
    const int mb = 239;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        for (uint32_t x = 0; x < len; ++x) {
            const uint32_t i = base + x;
            const float z = gZValue[i];
            if (gZOut[i] < z) {
                gZOut[i] = z;
                RGB565 c(gTexel[i]);
                c.mult256(mr, mg, mb);
                gFbOut[i] = c.val;
            }
        }
    }
}

static void texModZPtr(uint32_t len, uint32_t offset, ZPattern)
{
    const int mr = 221;
    const int mg = 177;
    const int mb = 239;
    for (uint32_t row = 0; row < kRows; ++row) {
        const uint32_t base = row * kPitch + offset;
        uint16_t* fb = gFbOut + base;
        float* zbuf = gZOut + base;
        const float* zv = gZValue + base;
        const uint16_t* texel = gTexel + base;
        for (uint32_t x = 0; x < len; ++x) {
            if (*zbuf < *zv) {
                *zbuf = *zv;
                RGB565 c(*texel);
                c.mult256(mr, mg, mb);
                *fb = c.val;
            }
            fb++;
            zbuf++;
            zv++;
            texel++;
        }
    }
}

static void runOne(const char* kernel, const char* variant, SpanFn ref, SpanFn fn, uint32_t len, uint32_t offset, ZPattern pattern)
{
    resetBuffers(len, offset, pattern);
    ref(len, offset, pattern);
    saveReference(len, offset);
    resetOutputBuffers(len, offset, pattern);
    fn(len, offset, pattern);
    const uint32_t pixels = activePixels(len);
    const uint32_t mismatches = countMismatches(len, offset);
    const bool correct = (mismatches == 0);
    const uint32_t repeats = repeatsForLen(len);

    uint32_t sumFb = 0;
    uint32_t sumZ = 0;
    const uint32_t startCycles = readCycles();
    const uint32_t startUs = micros();
    for (uint32_t rep = 0; rep < repeats; ++rep) {
        resetOutputBuffers(len, offset, pattern);
        fn(len, offset, pattern);
        sumFb ^= checksum16Span(gFbOut, len, offset);
        sumZ ^= checksumFloatBitsSpan(gZOut, len, offset);
    }
    const uint32_t timeUs = micros() - startUs;
    const uint32_t cycles = readCycles() - startCycles;
    gSink ^= sumFb ^ sumZ;

    Serial.print("KERNEL=");
    Serial.print(kernel);
    Serial.print(" VARIANT=");
    Serial.print(variant);
    Serial.print(" LEN=");
    Serial.print(len);
    Serial.print(" ALIGN=");
    Serial.print(offset);
    Serial.print(" PATTERN=");
    Serial.print(patternName(pattern));
    Serial.print(" CORRECT=");
    Serial.print(correct ? 1 : 0);
    Serial.print(" MISMATCHES=");
    Serial.print(mismatches);
    Serial.print(" TIME_US=");
    Serial.print(timeUs);
    Serial.print(" CYCLES=");
    Serial.print(cycles);
    Serial.print(" REPEATS=");
    Serial.print(repeats);
    Serial.print(" OPS=");
    Serial.print((uint32_t)(repeats * pixels));
    Serial.print(" FB_CHECKSUM=0x");
    Serial.print(checksum16Span(gFbOut, len, offset), HEX);
    Serial.print(" Z_CHECKSUM=0x");
    Serial.print(checksumFloatBitsSpan(gZOut, len, offset), HEX);
    Serial.println();
}

static void runZKernels(uint32_t len, uint32_t offset, ZPattern pattern)
{
    runOne("z_only", "index_ref", zOnlyIndex, zOnlyIndex, len, offset, pattern);
    runOne("z_only", "ptr", zOnlyIndex, zOnlyPtr, len, offset, pattern);
    runOne("z_only", "select_write", zOnlyIndex, zOnlySelectWrite, len, offset, pattern);
}

static void runFlatKernels(uint32_t len, uint32_t offset, ZPattern pattern)
{
    runOne("flat_noz", "index_ref", flatNoZIndex, flatNoZIndex, len, offset, pattern);
    runOne("flat_noz", "ptr", flatNoZIndex, flatNoZPtr, len, offset, pattern);
    runOne("flat_noz", "store32", flatNoZIndex, flatNoZStore32, len, offset, pattern);
    runOne("flat_noz", "unroll4", flatNoZIndex, flatNoZUnroll4, len, offset, pattern);

    runOne("flat_z", "index_ref", flatZIndex, flatZIndex, len, offset, pattern);
    runOne("flat_z", "ptr", flatZIndex, flatZPtr, len, offset, pattern);
    runOne("flat_z", "allpass_fast", flatZIndex, flatZAllPassFast, len, offset, pattern);
}

static void runShaderKernels(uint32_t len, uint32_t offset, ZPattern pattern)
{
    runOne("gouraud_noz", "current_tgx", gouraudRef, gouraudRef, len, offset, pattern);
    runOne("gouraud_noz", "scalar_channels", gouraudRef, gouraudScalar, len, offset, pattern);
    runOne("gouraud_z", "current_tgx", gouraudZRef, gouraudZRef, len, offset, pattern);

    runOne("tex_nearest_noz", "current_tgx", texNearestRef, texNearestRef, len, offset, pattern);
    runOne("tex_nearest_noz", "precomputed_texel", texNearestRef, texNearestPrecomputed, len, offset, pattern);
    runOne("tex_nearest_z", "index_ref", texNearestZIndex, texNearestZIndex, len, offset, pattern);
    runOne("tex_nearest_z", "ptr", texNearestZIndex, texNearestZPtr, len, offset, pattern);

    runOne("tex_bilinear_noz", "current_tgx", texBilinearRef, texBilinearRef, len, offset, pattern);
    runOne("tex_bilinear_noz", "scalar_channels", texBilinearRef, texBilinearScalar, len, offset, pattern);
    runOne("tex_bilinear_noz", "precomputed_texel", texBilinearRef, texBilinearPrecomputed, len, offset, pattern);

    runOne("tex_nearest_inc", "float_ref", texNearestIncFloat, texNearestIncFloat, len, offset, pattern);
    runOne("tex_nearest_inc", "fixed8", texNearestIncFloat, texNearestIncFixed8, len, offset, pattern);
    runOne("tex_bilinear_inc", "float_ref", texBilinearIncFloat, texBilinearIncFloat, len, offset, pattern);
    runOne("tex_bilinear_inc", "fixed8", texBilinearIncFloat, texBilinearIncFixed8, len, offset, pattern);

    runOne("tex_mod_noz", "current_tgx", texModRef, texModRef, len, offset, pattern);
    runOne("tex_mod_z", "index_ref", texModZIndex, texModZIndex, len, offset, pattern);
    runOne("tex_mod_z", "ptr", texModZIndex, texModZPtr, len, offset, pattern);
}

static void runAll()
{
    const uint32_t lengths[] = {1, 2, 3, 4, 5, 8, 16, 32, 64, 128, 320};
    const ZPattern patterns[] = {Z_ALLPASS, Z_MIXED, Z_ALLFAIL};

    for (uint32_t li = 0; li < sizeof(lengths) / sizeof(lengths[0]); ++li) {
        const uint32_t len = lengths[li];
        for (uint32_t offset = 0; offset <= 1; ++offset) {
            for (uint32_t pi = 0; pi < sizeof(patterns) / sizeof(patterns[0]); ++pi) {
                const ZPattern pattern = patterns[pi];
                runFlatKernels(len, offset, pattern);
                runZKernels(len, offset, pattern);
                runShaderKernels(len, offset, pattern);
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    waitForSerial();
    initCycleCounter();
    initInputs();

    Serial.println("TGX_RASTER_SPAN_BENCH_BEGIN");
#if defined(ARDUINO_TEENSY41)
    Serial.println("BOARD=TEENSY41");
#elif defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350) || defined(TARGET_RP2350)
    Serial.println("BOARD=PICO2_RP2350");
#else
    Serial.println("BOARD=UNKNOWN");
#endif
    Serial.print("CONFIG=TGX_RGB565_ORDER_BGR:");
    Serial.print(TGX_RGB565_ORDER_BGR);
    Serial.print(",DWT:");
    Serial.print(TGX_SPAN_HAS_DWT);
    Serial.print(",TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS:");
    Serial.print(TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS);
    Serial.print(",TGX_INLINE:");
    Serial.println(STR(TGX_INLINE));

    runAll();

    Serial.print("SINK=0x");
    Serial.println(gSink, HEX);
    Serial.println("TGX_RASTER_SPAN_BENCH_END");
}

void loop()
{
}
