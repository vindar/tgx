#include <Arduino.h>
#include <tgx.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#if defined(__ARM_ARCH_7EM__) || defined(__ARM_FEATURE_DSP) || defined(__ARM_FEATURE_SIMD32)
#define TGX_PROBE_HAS_DSP32 1
#else
#define TGX_PROBE_HAS_DSP32 0
#endif

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__)
#define TGX_PROBE_HAS_THUMB2 1
#else
#define TGX_PROBE_HAS_THUMB2 0
#endif

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

static void print01(const char* name, int value)
{
    Serial.print(name);
    Serial.print('=');
    Serial.println(value);
}

static void printHex32(const char* name, uint32_t value)
{
    Serial.print(name);
    Serial.print("=0x");
    Serial.println(value, HEX);
}

static void printIntrinsic(const char* name, int compiled, uint32_t result)
{
    Serial.print("INTRINSIC=");
    Serial.print(name);
    Serial.print(" COMPILED=");
    Serial.print(compiled);
    Serial.print(" RESULT=0x");
    Serial.println(result, HEX);
}

static uint32_t scalarUhadd16(uint32_t a, uint32_t b)
{
    return (((a & 0xffffu) + (b & 0xffffu)) >> 1)
         | (((((a >> 16) & 0xffffu) + ((b >> 16) & 0xffffu)) >> 1) << 16);
}

static uint32_t scalarUadd16(uint32_t a, uint32_t b)
{
    return ((a + b) & 0x0000ffffu) | (((a >> 16) + (b >> 16)) << 16);
}

static uint32_t scalarUsub16(uint32_t a, uint32_t b)
{
    return ((a - b) & 0x0000ffffu) | (((a >> 16) - (b >> 16)) << 16);
}

static uint32_t scalarUqadd16(uint32_t a, uint32_t b)
{
    const uint32_t lo = (a & 0xffffu) + (b & 0xffffu);
    const uint32_t hi = ((a >> 16) & 0xffffu) + ((b >> 16) & 0xffffu);
    return (lo > 0xffffu ? 0xffffu : lo) | ((hi > 0xffffu ? 0xffffu : hi) << 16);
}

static int32_t scalarSmlad(uint32_t a, uint32_t b, int32_t acc)
{
    const int16_t a0 = (int16_t)(a & 0xffffu);
    const int16_t a1 = (int16_t)((a >> 16) & 0xffffu);
    const int16_t b0 = (int16_t)(b & 0xffffu);
    const int16_t b1 = (int16_t)((b >> 16) & 0xffffu);
    return acc + (int32_t)a0 * b0 + (int32_t)a1 * b1;
}

static int32_t scalarSmuad(uint32_t a, uint32_t b)
{
    return scalarSmlad(a, b, 0);
}

static uint32_t scalarPkhbt(uint32_t a, uint32_t b)
{
    return (a & 0xffffu) | ((b & 0xffffu) << 16);
}

static uint32_t scalarPkhtb(uint32_t a, uint32_t b)
{
    return (a & 0xffff0000u) | ((b >> 16) & 0xffffu);
}

#if TGX_PROBE_HAS_DSP32
static uint32_t asmUhadd16(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("uhadd16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asmUadd16(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("uadd16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asmUsub16(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("usub16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asmUqadd16(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("uqadd16 %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static int32_t asmSmlad(uint32_t a, uint32_t b, int32_t acc)
{
    int32_t r;
    __asm volatile ("smlad %0, %1, %2, %3" : "=r"(r) : "r"(a), "r"(b), "r"(acc));
    return r;
}

static int32_t asmSmuad(uint32_t a, uint32_t b)
{
    int32_t r;
    __asm volatile ("smuad %0, %1, %2" : "=r"(r) : "r"(a), "r"(b));
    return r;
}
#endif

#if TGX_PROBE_HAS_THUMB2
static uint32_t asmPkhbt(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("pkhbt %0, %1, %2, lsl #16" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asmPkhtb(uint32_t a, uint32_t b)
{
    uint32_t r;
    __asm volatile ("pkhtb %0, %1, %2, asr #16" : "=r"(r) : "r"(a), "r"(b));
    return r;
}

static uint32_t asmUsat(int32_t a)
{
    uint32_t r;
    __asm volatile ("usat %0, #8, %1" : "=r"(r) : "r"(a));
    return r;
}

static uint32_t asmUsat16(uint32_t a)
{
    uint32_t r;
    __asm volatile ("usat16 %0, #8, %1" : "=r"(r) : "r"(a));
    return r;
}

static uint32_t asmRev16(uint32_t a)
{
    uint32_t r;
    __asm volatile ("rev16 %0, %1" : "=r"(r) : "r"(a));
    return r;
}
#endif

void setup()
{
    Serial.begin(9600);
    waitForSerial();

    Serial.println("TGX_ARM_DSP_PROBE_BEGIN");
#if defined(ARDUINO_TEENSY41)
    Serial.println("BOARD=TEENSY41");
#elif defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350) || defined(TARGET_RP2350)
    Serial.println("BOARD=PICO2_RP2350");
#else
    Serial.println("BOARD=UNKNOWN");
#endif

    Serial.print("TGX_INLINE=");
    Serial.println(STR(TGX_INLINE));
    Serial.print("TGX_INLINE_ZDIVIDE=");
    Serial.println(STR(TGX_INLINE_ZDIVIDE));

#if defined(__ARM_ARCH_7EM__)
    print01("__ARM_ARCH_7EM__", 1);
#else
    print01("__ARM_ARCH_7EM__", 0);
#endif
#if defined(__ARM_ARCH_8M_MAIN__)
    print01("__ARM_ARCH_8M_MAIN__", 1);
#else
    print01("__ARM_ARCH_8M_MAIN__", 0);
#endif
#if defined(__ARM_FEATURE_DSP)
    print01("__ARM_FEATURE_DSP", 1);
#else
    print01("__ARM_FEATURE_DSP", 0);
#endif
#if defined(__ARM_FEATURE_SIMD32)
    print01("__ARM_FEATURE_SIMD32", 1);
#else
    print01("__ARM_FEATURE_SIMD32", 0);
#endif
    print01("TGX_PROBE_HAS_DSP32", TGX_PROBE_HAS_DSP32);
    print01("TGX_PROBE_HAS_THUMB2", TGX_PROBE_HAS_THUMB2);
#if defined(__ARM_FP)
    printHex32("__ARM_FP", __ARM_FP);
#else
    printHex32("__ARM_FP", 0);
#endif

    const uint32_t a = 0x00030005u;
    const uint32_t b = 0x00070009u;
    const uint32_t satA = 0xfff00010u;
    const uint32_t satB = 0x0100fff0u;
    const uint32_t signedA = 0x0002fffeu; // 2, -2
    const uint32_t signedB = 0x00030004u; // 3, 4

    printIntrinsic("SCALAR_UHADD16_REF", 1, scalarUhadd16(a, b));
    printIntrinsic("SCALAR_UADD16_REF", 1, scalarUadd16(a, b));
    printIntrinsic("SCALAR_USUB16_REF", 1, scalarUsub16(b, a));
    printIntrinsic("SCALAR_UQADD16_REF", 1, scalarUqadd16(satA, satB));
    printIntrinsic("SCALAR_SMLAD_REF", 1, (uint32_t)scalarSmlad(signedA, signedB, 7));
    printIntrinsic("SCALAR_SMUAD_REF", 1, (uint32_t)scalarSmuad(signedA, signedB));
    printIntrinsic("SCALAR_PKHBT_REF", 1, scalarPkhbt(0xaaaa1234u, 0xbbbb5678u));
    printIntrinsic("SCALAR_PKHTB_REF", 1, scalarPkhtb(0xaaaa1234u, 0xbbbb5678u));

#if TGX_PROBE_HAS_DSP32
    printIntrinsic("ASM_UHADD16", 1, asmUhadd16(a, b));
    printIntrinsic("ASM_UADD16", 1, asmUadd16(a, b));
    printIntrinsic("ASM_USUB16", 1, asmUsub16(b, a));
    printIntrinsic("ASM_UQADD16", 1, asmUqadd16(satA, satB));
    printIntrinsic("ASM_SMLAD", 1, (uint32_t)asmSmlad(signedA, signedB, 7));
    printIntrinsic("ASM_SMUAD", 1, (uint32_t)asmSmuad(signedA, signedB));
#else
    printIntrinsic("ASM_UHADD16", 0, 0);
    printIntrinsic("ASM_UADD16", 0, 0);
    printIntrinsic("ASM_USUB16", 0, 0);
    printIntrinsic("ASM_UQADD16", 0, 0);
    printIntrinsic("ASM_SMLAD", 0, 0);
    printIntrinsic("ASM_SMUAD", 0, 0);
#endif

#if TGX_PROBE_HAS_THUMB2
    printIntrinsic("ASM_PKHBT", 1, asmPkhbt(0xaaaa1234u, 0xbbbb5678u));
    printIntrinsic("ASM_PKHTB", 1, asmPkhtb(0xaaaa1234u, 0xbbbb5678u));
    printIntrinsic("ASM_USAT", 1, (uint32_t)asmUsat(300));
    printIntrinsic("ASM_USAT16", 1, asmUsat16(0x012cfff0));
    printIntrinsic("ASM_REV16", 1, asmRev16(0x11223344u));
#else
    printIntrinsic("ASM_PKHBT", 0, 0);
    printIntrinsic("ASM_PKHTB", 0, 0);
    printIntrinsic("ASM_USAT", 0, 0);
    printIntrinsic("ASM_USAT16", 0, 0);
    printIntrinsic("ASM_REV16", 0, 0);
#endif

    Serial.println("TGX_ARM_DSP_PROBE_END");
}

void loop()
{
    delay(1000);
}
