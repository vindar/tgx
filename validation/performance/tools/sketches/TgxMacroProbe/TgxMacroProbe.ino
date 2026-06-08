#include <Arduino.h>
#include <tgx.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

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

static void printMacro01(const char* name, int value)
{
    Serial.print(name);
    Serial.print('=');
    Serial.println(value);
}

void setup()
{
    Serial.begin(9600);
    waitForSerial();
    Serial.println("TGX_MACRO_PROBE_BEGIN");
    Serial.print("TGX_INLINE=");
    Serial.println(STR(TGX_INLINE));
    Serial.print("TGX_NOINLINE=");
    Serial.println(STR(TGX_NOINLINE));
    Serial.print("TGX_INLINE_ZDIVIDE=");
    Serial.println(STR(TGX_INLINE_ZDIVIDE));
    printMacro01("TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS", TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS);
    printMacro01("TGX_USE_FAST_INV_TRICK", TGX_USE_FAST_INV_TRICK);
    printMacro01("TGX_USE_FAST_SQRT_TRICK", TGX_USE_FAST_SQRT_TRICK);
    printMacro01("TGX_USE_FAST_INV_SQRT_TRICK", TGX_USE_FAST_INV_SQRT_TRICK);
    printMacro01("TGX_USE_FMA_MATH", TGX_USE_FMA_MATH);
#if defined(ARDUINO_TEENSY41)
    printMacro01("ARDUINO_TEENSY41", 1);
#else
    printMacro01("ARDUINO_TEENSY41", 0);
#endif
#if defined(ARDUINO_ARCH_RP2040)
    printMacro01("ARDUINO_ARCH_RP2040", 1);
#else
    printMacro01("ARDUINO_ARCH_RP2040", 0);
#endif
#if defined(ARDUINO_ARCH_RP2350)
    printMacro01("ARDUINO_ARCH_RP2350", 1);
#else
    printMacro01("ARDUINO_ARCH_RP2350", 0);
#endif
#if defined(PICO_RP2040)
    printMacro01("PICO_RP2040", 1);
#else
    printMacro01("PICO_RP2040", 0);
#endif
#if defined(PICO_RP2350)
    printMacro01("PICO_RP2350", 1);
#else
    printMacro01("PICO_RP2350", 0);
#endif
#if defined(ESP32)
    printMacro01("ESP32", 1);
#else
    printMacro01("ESP32", 0);
#endif
#if defined(ESP32S3)
    printMacro01("ESP32S3", 1);
#else
    printMacro01("ESP32S3", 0);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32)
    printMacro01("CONFIG_IDF_TARGET_ESP32", 1);
#else
    printMacro01("CONFIG_IDF_TARGET_ESP32", 0);
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    printMacro01("CONFIG_IDF_TARGET_ESP32S3", 1);
#else
    printMacro01("CONFIG_IDF_TARGET_ESP32S3", 0);
#endif
#if defined(__ARM_ARCH_8M_MAIN__)
    printMacro01("__ARM_ARCH_8M_MAIN__", 1);
#else
    printMacro01("__ARM_ARCH_8M_MAIN__", 0);
#endif
    Serial.println("TGX_MACRO_PROBE_OK");
    Serial.println("TGX_MACRO_PROBE_END");
}

void loop()
{
    delay(1000);
}
