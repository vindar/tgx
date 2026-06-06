#include <Arduino.h>
#include <tgx.h>
#include <math.h>
#include <string.h>

using tgx::RGB565;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
#define TGX_TEXCOORD_HAS_DWT 1
#else
#define TGX_TEXCOORD_HAS_DWT 0
#endif

static const uint32_t kMaxLen = 320;
static const uint32_t kMaxTexW = 64;
static const uint32_t kMaxTexH = 64;
static const uint32_t kMaxTexels = kMaxTexW * kMaxTexH;

static uint16_t gTex[kMaxTexels];
static uint16_t gRef[kMaxLen];
static uint16_t gCand[kMaxLen];
static volatile uint32_t gSink = 0;
static uint32_t gRng = 0x9E3779B9u;

enum SampleMode : uint8_t
    {
    MODE_NEAREST = 0,
    MODE_BILINEAR = 1
    };

enum TexturePattern : uint8_t
    {
    TEX_CHECKER = 0,
    TEX_GRADIENT = 1,
    TEX_RANDOM = 2,
    TEX_STRIPES = 3
    };

struct TextureCase
    {
    const char* name;
    int w;
    int h;
    TexturePattern pattern;
    bool wrap;
    };

struct CoordCase
    {
    const char* name;
    bool perspective;
    float x0Norm;
    float y0Norm;
    float dxNorm;
    float dyNorm;
    float cw0;
    float dcw;
    };

static const uint32_t kLengths[] = {1, 2, 3, 4, 5, 8, 16, 32, 64, 128, 320};

static const TextureCase kTextureCases[] =
    {
    {"8_checker_clamp", 8, 8, TEX_CHECKER, false},
    {"8_checker_wrap", 8, 8, TEX_CHECKER, true},
    {"64_gradient_clamp", 64, 64, TEX_GRADIENT, false},
    {"64_random_wrap", 64, 64, TEX_RANDOM, true},
    {"37_stripes_clamp", 37, 29, TEX_STRIPES, false}
    };

static const CoordCase kCoordCases[] =
    {
    {"interior_inc", false, 0.15f, 0.20f, 0.0037f, 0.0021f, 1.0f, 0.0f},
    {"interior_dec", false, 0.82f, 0.77f, -0.0031f, -0.0019f, 1.0f, 0.0f},
    {"subtexel", false, 0.33f, 0.41f, 0.00021f, -0.00017f, 1.0f, 0.0f},
    {"edge_min", false, 0.0003f, 0.002f, -0.00041f, 0.00033f, 1.0f, 0.0f},
    {"edge_max", false, 0.985f, 0.972f, 0.0025f, 0.0017f, 1.0f, 0.0f},
    {"negative", false, -0.08f, -0.03f, 0.0029f, 0.0018f, 1.0f, 0.0f},
    {"above", false, 1.03f, 1.07f, -0.0022f, -0.0015f, 1.0f, 0.0f},
    {"persp_mild", true, 0.18f, 0.23f, 0.0042f, 0.0017f, 0.92f, 0.00042f},
    {"persp_strong", true, 0.10f, 0.82f, 0.0095f, -0.0067f, 0.65f, 0.0027f}
    };

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
    while (millis() - kickStart < 15000) {
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

#if TGX_TEXCOORD_HAS_DWT
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

static int shaderClipLocal(int v, int maxv)
{
    return (v < 0) ? 0 : ((v > maxv) ? maxv : v);
}

static int fixedFloor(int32_t q, int fracBits)
{
    const int32_t scale = 1L << fracBits;
    if (q >= 0) {
        return (int)(q >> fracBits);
    }
    return (int)(-(((-q) + scale - 1) >> fracBits));
}

static int fixedTrunc(int32_t q, int fracBits)
{
    if (q >= 0) {
        return (int)(q >> fracBits);
    }
    return (int)(-((-q) >> fracBits));
}

static int32_t floatToFixedFloor(float v, int fracBits)
{
    return (int32_t)floorf(v * (float)(1L << fracBits));
}

static int32_t floatToFixedRound(float v, int fracBits)
{
    const float s = (float)(1L << fracBits);
    return (int32_t)((v >= 0.0f) ? (v * s + 0.5f) : (v * s - 0.5f));
}

static int getR(uint16_t c)
{
#if TGX_RGB565_ORDER_BGR
    return (int)((c >> 11) & 31u);
#else
    return (int)(c & 31u);
#endif
}

static int getG(uint16_t c)
{
    return (int)((c >> 5) & 63u);
}

static int getB(uint16_t c)
{
#if TGX_RGB565_ORDER_BGR
    return (int)(c & 31u);
#else
    return (int)((c >> 11) & 31u);
#endif
}

static RGB565 makeColor(int r, int g, int b)
{
    return RGB565(r & 31, g & 63, b & 31);
}

static void fillTexture(const TextureCase& tc)
{
    gRng = 0x12345678u + (uint32_t)tc.w * 17u + (uint32_t)tc.h * 257u + (uint32_t)tc.pattern * 65537u;
    for (int y = 0; y < tc.h; ++y) {
        for (int x = 0; x < tc.w; ++x) {
            RGB565 c;
            switch (tc.pattern) {
            case TEX_CHECKER:
                c = (((x >> 1) ^ (y >> 1)) & 1) ? makeColor(31, 63, 31) : makeColor(0, 0, 0);
                break;
            case TEX_GRADIENT:
                c = makeColor((x * 31) / (tc.w - 1), (y * 63) / (tc.h - 1), ((x + y) * 31) / (tc.w + tc.h - 2));
                break;
            case TEX_RANDOM:
                c = makeColor((int)(xorshift32() & 31u), (int)(xorshift32() & 63u), (int)(xorshift32() & 31u));
                break;
            default:
                c = ((x / 3 + y / 5) & 1) ? makeColor(31, 4, 2) : makeColor(2, 44, 31);
                break;
            }
            gTex[(uint32_t)y * (uint32_t)tc.w + (uint32_t)x] = c.val;
        }
    }
}

static RGB565 sampleFloatNearest(const TextureCase& tc, float xx, float yy)
{
    const int maxx = tc.w - 1;
    const int maxy = tc.h - 1;
    const int ix0 = (int)xx;
    const int iy0 = (int)yy;
    const int ix = tc.wrap ? (ix0 & maxx) : shaderClipLocal(ix0, maxx);
    const int iy = tc.wrap ? (iy0 & maxy) : shaderClipLocal(iy0, maxy);
    return RGB565(gTex[(uint32_t)iy * (uint32_t)tc.w + (uint32_t)ix]);
}

static RGB565 sampleFloatBilinear(const TextureCase& tc, float xx, float yy)
{
    const int maxxv = tc.w - 1;
    const int maxyv = tc.h - 1;
    const int ttx = tgx::lfloorf(xx);
    const int tty = tgx::lfloorf(yy);
    const float ax = xx - (float)ttx;
    const float ay = yy - (float)tty;
    const int minx = tc.wrap ? (ttx & maxxv) : shaderClipLocal(ttx, maxxv);
    const int maxx = tc.wrap ? ((ttx + 1) & maxxv) : shaderClipLocal(ttx + 1, maxxv);
    const int miny = (tc.wrap ? (tty & maxyv) : shaderClipLocal(tty, maxyv)) * tc.w;
    const int maxy = (tc.wrap ? ((tty + 1) & maxyv) : shaderClipLocal(tty + 1, maxyv)) * tc.w;
    return tgx::interpolateColorsBilinear(
        RGB565(gTex[(uint32_t)(minx + miny)]),
        RGB565(gTex[(uint32_t)(maxx + miny)]),
        RGB565(gTex[(uint32_t)(minx + maxy)]),
        RGB565(gTex[(uint32_t)(maxx + maxy)]),
        ax, ay);
}

static RGB565 sampleFixedNearest(const TextureCase& tc, int32_t qx, int32_t qy, int fracBits)
{
    const int maxx = tc.w - 1;
    const int maxy = tc.h - 1;
    const int ix0 = fixedTrunc(qx, fracBits);
    const int iy0 = fixedTrunc(qy, fracBits);
    const int ix = tc.wrap ? (ix0 & maxx) : shaderClipLocal(ix0, maxx);
    const int iy = tc.wrap ? (iy0 & maxy) : shaderClipLocal(iy0, maxy);
    return RGB565(gTex[(uint32_t)iy * (uint32_t)tc.w + (uint32_t)ix]);
}

static RGB565 bilinearFixedWeights(uint16_t c00, uint16_t c10, uint16_t c01, uint16_t c11, int iax, int iay)
{
    const RGB565 C00(c00);
    const RGB565 C10(c10);
    const RGB565 C01(c01);
    const RGB565 C11(c11);
    const int rax = 256 - iax;
    const int ray = 256 - iay;
    const int R = rax * (ray * C00.R + iay * C01.R) + iax * (ray * C10.R + iay * C11.R);
    const int G = rax * (ray * C00.G + iay * C01.G) + iax * (ray * C10.G + iay * C11.G);
    const int B = rax * (ray * C00.B + iay * C01.B) + iax * (ray * C10.B + iay * C11.B);
    return RGB565(R >> 16, G >> 16, B >> 16);
}

static RGB565 sampleFixedBilinear(const TextureCase& tc, int32_t qx, int32_t qy, int fracBits)
{
    const int maxxv = tc.w - 1;
    const int maxyv = tc.h - 1;
    const int ttx = fixedFloor(qx, fracBits);
    const int tty = fixedFloor(qy, fracBits);
    const int32_t fracMask = (1L << fracBits) - 1;
    int32_t fx = qx - ((int32_t)ttx << fracBits);
    int32_t fy = qy - ((int32_t)tty << fracBits);
    if (fx < 0) fx = 0;
    if (fy < 0) fy = 0;
    const int shift = fracBits - 8;
    const int iax = (shift >= 0) ? (int)(fx >> shift) : (int)(fx << (-shift));
    const int iay = (shift >= 0) ? (int)(fy >> shift) : (int)(fy << (-shift));
    const int minx = tc.wrap ? (ttx & maxxv) : shaderClipLocal(ttx, maxxv);
    const int maxx = tc.wrap ? ((ttx + 1) & maxxv) : shaderClipLocal(ttx + 1, maxxv);
    const int miny = (tc.wrap ? (tty & maxyv) : shaderClipLocal(tty, maxyv)) * tc.w;
    const int maxy = (tc.wrap ? ((tty + 1) & maxyv) : shaderClipLocal(tty + 1, maxyv)) * tc.w;
    (void)fracMask;
    return bilinearFixedWeights(
        gTex[(uint32_t)(minx + miny)],
        gTex[(uint32_t)(maxx + miny)],
        gTex[(uint32_t)(minx + maxy)],
        gTex[(uint32_t)(maxx + maxy)],
        iax, iay);
}

static RGB565 sampleFixedNearestInterior(const TextureCase& tc, int32_t qx, int32_t qy, int fracBits)
{
    const int ix = (int)(qx >> fracBits);
    const int iy = (int)(qy >> fracBits);
    return RGB565(gTex[(uint32_t)iy * (uint32_t)tc.w + (uint32_t)ix]);
}

static RGB565 sampleFixedBilinearInterior(const TextureCase& tc, int32_t qx, int32_t qy, int fracBits)
{
    const int ix = (int)(qx >> fracBits);
    const int iy = (int)(qy >> fracBits);
    const int32_t fracMask = (1L << fracBits) - 1;
    const int shift = fracBits - 8;
    const int iax = (shift >= 0) ? (int)((qx & fracMask) >> shift) : (int)((qx & fracMask) << (-shift));
    const int iay = (shift >= 0) ? (int)((qy & fracMask) >> shift) : (int)((qy & fracMask) << (-shift));
    const int row0 = iy * tc.w;
    const int row1 = row0 + tc.w;
    return bilinearFixedWeights(
        gTex[(uint32_t)(ix + row0)],
        gTex[(uint32_t)(ix + 1 + row0)],
        gTex[(uint32_t)(ix + row1)],
        gTex[(uint32_t)(ix + 1 + row1)],
        iax, iay);
}

static void coordinateAt(const TextureCase& tc, const CoordCase& cc, uint32_t i, float& xx, float& yy)
{
    const float tx = (cc.x0Norm + cc.dxNorm * (float)i) * (float)tc.w;
    const float ty = (cc.y0Norm + cc.dyNorm * (float)i) * (float)tc.h;
    if (cc.perspective) {
        const float cw = cc.cw0 + cc.dcw * (float)i;
        xx = tx / cw;
        yy = ty / cw;
    } else {
        xx = tx;
        yy = ty;
    }
}

static void fixedStartAndStep(const TextureCase& tc, const CoordCase& cc, uint32_t len, int fracBits, int32_t& qx, int32_t& qy, int32_t& qdx, int32_t& qdy)
{
    float x0, y0;
    coordinateAt(tc, cc, 0, x0, y0);
    qx = floatToFixedFloor(x0, fracBits);
    qy = floatToFixedFloor(y0, fracBits);

    if (cc.perspective && len > 1) {
        float x1, y1;
        coordinateAt(tc, cc, len - 1, x1, y1);
        qdx = floatToFixedRound((x1 - x0) / (float)(len - 1), fracBits);
        qdy = floatToFixedRound((y1 - y0) / (float)(len - 1), fracBits);
    } else {
        qdx = floatToFixedRound(cc.dxNorm * (float)tc.w, fracBits);
        qdy = floatToFixedRound(cc.dyNorm * (float)tc.h, fracBits);
    }
}

static void renderReference(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, uint16_t* out)
{
    for (uint32_t i = 0; i < len; ++i) {
        float xx, yy;
        coordinateAt(tc, cc, i, xx, yy);
        const RGB565 c = (mode == MODE_NEAREST) ? sampleFloatNearest(tc, xx, yy) : sampleFloatBilinear(tc, xx, yy);
        out[i] = c.val;
    }
}

static void renderCandidate(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, int fracBits, uint16_t* out)
{
    int32_t qx, qy, qdx, qdy;
    fixedStartAndStep(tc, cc, len, fracBits, qx, qy, qdx, qdy);
    for (uint32_t i = 0; i < len; ++i) {
        const RGB565 c = (mode == MODE_NEAREST) ? sampleFixedNearest(tc, qx, qy, fracBits) : sampleFixedBilinear(tc, qx, qy, fracBits);
        out[i] = c.val;
        qx += qdx;
        qy += qdy;
    }
}

static void renderCandidateInterior(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, int fracBits, uint16_t* out)
{
    int32_t qx, qy, qdx, qdy;
    fixedStartAndStep(tc, cc, len, fracBits, qx, qy, qdx, qdy);
    for (uint32_t i = 0; i < len; ++i) {
        const RGB565 c = (mode == MODE_NEAREST) ? sampleFixedNearestInterior(tc, qx, qy, fracBits) : sampleFixedBilinearInterior(tc, qx, qy, fracBits);
        out[i] = c.val;
        qx += qdx;
        qy += qdy;
    }
}

static bool isInteriorSafe(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len)
{
    if (cc.perspective) {
        return false;
    }
    for (uint32_t i = 0; i < len; ++i) {
        float xx, yy;
        coordinateAt(tc, cc, i, xx, yy);
        if (mode == MODE_NEAREST) {
            if ((xx < 0.0f) || (yy < 0.0f) || (xx >= (float)tc.w) || (yy >= (float)tc.h)) {
                return false;
            }
            const int ix = (int)xx;
            const int iy = (int)yy;
            if ((ix < 0) || (iy < 0) || (ix > tc.w - 1) || (iy > tc.h - 1)) {
                return false;
            }
        } else {
            const int ix = tgx::lfloorf(xx);
            const int iy = tgx::lfloorf(yy);
            if ((ix < 0) || (iy < 0) || (ix + 1 > tc.w - 1) || (iy + 1 > tc.h - 1)) {
                return false;
            }
        }
    }
    return true;
}

static uint32_t checksum16(const uint16_t* p, uint32_t len)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t repeatsForLen(uint32_t len)
{
    if (len <= 5) return 512;
    if (len <= 16) return 256;
    if (len <= 64) return 96;
    if (len <= 128) return 48;
    return 20;
}

static uint32_t timeReference(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, uint32_t repeats, uint32_t& cycles)
{
    const uint32_t c0 = readCycles();
    const uint32_t t0 = micros();
    uint32_t h = 0;
    for (uint32_t r = 0; r < repeats; ++r) {
        renderReference(tc, cc, mode, len, gRef);
        h ^= checksum16(gRef, len) + r;
    }
    const uint32_t t1 = micros();
    const uint32_t c1 = readCycles();
    gSink ^= h;
    cycles = c1 - c0;
    return t1 - t0;
}

static uint32_t timeCandidate(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, int fracBits, uint32_t repeats, uint32_t& cycles)
{
    const uint32_t c0 = readCycles();
    const uint32_t t0 = micros();
    uint32_t h = 0;
    for (uint32_t r = 0; r < repeats; ++r) {
        renderCandidate(tc, cc, mode, len, fracBits, gCand);
        h ^= checksum16(gCand, len) + r;
    }
    const uint32_t t1 = micros();
    const uint32_t c1 = readCycles();
    gSink ^= h;
    cycles = c1 - c0;
    return t1 - t0;
}

static uint32_t timeCandidateInterior(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, int fracBits, uint32_t repeats, uint32_t& cycles)
{
    const uint32_t c0 = readCycles();
    const uint32_t t0 = micros();
    uint32_t h = 0;
    for (uint32_t r = 0; r < repeats; ++r) {
        renderCandidateInterior(tc, cc, mode, len, fracBits, gCand);
        h ^= checksum16(gCand, len) + r;
    }
    const uint32_t t1 = micros();
    const uint32_t c1 = readCycles();
    gSink ^= h;
    cycles = c1 - c0;
    return t1 - t0;
}

static void printHex8(uint32_t v)
{
    char buf[9];
    static const char* hex = "0123456789ABCDEF";
    for (int i = 0; i < 8; ++i) {
        buf[7 - i] = hex[v & 15u];
        v >>= 4;
    }
    buf[8] = 0;
    Serial.print(buf);
}

static void runOne(const TextureCase& tc, const CoordCase& cc, SampleMode mode, uint32_t len, int fracBits, bool interiorFast)
{
    if (interiorFast && !isInteriorSafe(tc, cc, mode, len)) {
        return;
    }

    renderReference(tc, cc, mode, len, gRef);
    if (interiorFast) {
        renderCandidateInterior(tc, cc, mode, len, fracBits, gCand);
    } else {
        renderCandidate(tc, cc, mode, len, fracBits, gCand);
    }

    uint32_t mismatches = 0;
    uint32_t gtOne = 0;
    uint32_t maxErr = 0;
    uint64_t sumAbs = 0;
    uint64_t sumSq = 0;

    for (uint32_t i = 0; i < len; ++i) {
        const uint16_t a = gRef[i];
        const uint16_t b = gCand[i];
        if (a != b) {
            ++mismatches;
        }
        const int dr = abs(getR(a) - getR(b));
        const int dg = abs(getG(a) - getG(b));
        const int db = abs(getB(a) - getB(b));
        const int localMax = max(dr, max(dg, db));
        if (localMax > 1) {
            ++gtOne;
        }
        maxErr = max(maxErr, (uint32_t)localMax);
        sumAbs += (uint32_t)(dr + dg + db);
        sumSq += (uint32_t)(dr * dr + dg * dg + db * db);
    }

    const uint32_t repeats = repeatsForLen(len);
    uint32_t cyclesRef = 0;
    uint32_t cyclesCand = 0;
    const uint32_t timeRef = timeReference(tc, cc, mode, len, repeats, cyclesRef);
    const uint32_t timeCand = interiorFast ? timeCandidateInterior(tc, cc, mode, len, fracBits, repeats, cyclesCand) : timeCandidate(tc, cc, mode, len, fracBits, repeats, cyclesCand);
    const float delta = (timeCand == 0) ? 0.0f : (100.0f * ((float)timeRef - (float)timeCand) / (float)timeCand);
    const float mismatchPct = (len == 0) ? 0.0f : (100.0f * (float)mismatches / (float)len);
    const float meanAbs = (len == 0) ? 0.0f : ((float)sumAbs / (float)(len * 3u));
    const float rmse = (len == 0) ? 0.0f : sqrtf((float)sumSq / (float)(len * 3u));

    Serial.print("CASE=");
    Serial.print(tc.name);
    Serial.print("/");
    Serial.print(cc.name);
    Serial.print(" MODE=");
    Serial.print((mode == MODE_NEAREST) ? "nearest" : "bilinear");
    Serial.print(" VARIANT=");
    Serial.print(interiorFast ? "fixed16_interior" : "fixed_round");
    Serial.print(" SAFE=");
    Serial.print(interiorFast ? 1 : 0);
    Serial.print(" PERSPECTIVE=");
    Serial.print(cc.perspective ? 1 : 0);
    Serial.print(" WRAP=");
    Serial.print(tc.wrap ? "wrap_pow2" : "clamp");
    Serial.print(" TEX=");
    Serial.print(tc.w);
    Serial.print("x");
    Serial.print(tc.h);
    Serial.print(" FRAC=");
    Serial.print(fracBits);
    Serial.print(" LEN=");
    Serial.print(len);
    Serial.print(" CORRECT=");
    Serial.print((mismatches == 0) ? 1 : 0);
    Serial.print(" PIXEL_MISMATCHES=");
    Serial.print(mismatches);
    Serial.print(" MISMATCH_PCT=");
    Serial.print(mismatchPct, 3);
    Serial.print(" MAX_ABS_CHANNEL_ERROR=");
    Serial.print(maxErr);
    Serial.print(" MEAN_ABS_CHANNEL_ERROR=");
    Serial.print(meanAbs, 5);
    Serial.print(" RMSE=");
    Serial.print(rmse, 5);
    Serial.print(" PIXELS_GT1=");
    Serial.print(gtOne);
    Serial.print(" FB_HASH_REF=");
    printHex8(checksum16(gRef, len));
    Serial.print(" FB_HASH_CAND=");
    printHex8(checksum16(gCand, len));
    Serial.print(" TIME_REF_US=");
    Serial.print(timeRef);
    Serial.print(" TIME_CAND_US=");
    Serial.print(timeCand);
    Serial.print(" CYCLES_REF=");
    Serial.print(cyclesRef);
    Serial.print(" CYCLES_CAND=");
    Serial.print(cyclesCand);
    Serial.print(" REPEATS=");
    Serial.print(repeats);
    Serial.print(" DELTA=");
    Serial.println(delta, 3);
    Serial.flush();
}

static void printBoardConfig()
{
#if defined(ARDUINO_TEENSY41)
    Serial.println("BOARD=TEENSY41");
#elif defined(PICO_RP2350)
    Serial.println("BOARD=PICO2_RP2350");
#else
    Serial.println("BOARD=UNKNOWN");
#endif
    Serial.print("TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS=");
    Serial.println(TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS);
    Serial.print("TGX_INLINE_ZDIVIDE=");
    Serial.println(STR(TGX_INLINE_ZDIVIDE));
}

static void runBench()
{
    Serial.println("TGX_TEXCOORD_EQ_BEGIN");
    printBoardConfig();
    Serial.println("CONFIG=round_step_fixed_floor_start");

    for (uint32_t ti = 0; ti < sizeof(kTextureCases) / sizeof(kTextureCases[0]); ++ti) {
        const TextureCase& tc = kTextureCases[ti];
        fillTexture(tc);
        for (uint32_t ci = 0; ci < sizeof(kCoordCases) / sizeof(kCoordCases[0]); ++ci) {
            const CoordCase& cc = kCoordCases[ci];
            for (uint32_t li = 0; li < sizeof(kLengths) / sizeof(kLengths[0]); ++li) {
                const uint32_t len = kLengths[li];
                for (int mode = 0; mode < 2; ++mode) {
                    runOne(tc, cc, (SampleMode)mode, len, 8, false);
                    runOne(tc, cc, (SampleMode)mode, len, 10, false);
                    runOne(tc, cc, (SampleMode)mode, len, 12, false);
                    runOne(tc, cc, (SampleMode)mode, len, 16, false);
                    runOne(tc, cc, (SampleMode)mode, len, 16, true);
                }
            }
        }
    }

    Serial.print("SINK=");
    printHex8(gSink);
    Serial.println();
    Serial.println("TGX_TEXCOORD_EQ_END");
}

void setup()
{
    Serial.begin(115200);
    waitForSerial();
    initCycleCounter();
    runBench();
}

void loop()
{
}
