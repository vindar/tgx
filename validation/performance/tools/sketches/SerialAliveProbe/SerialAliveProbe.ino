#include <Arduino.h>

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

void setup()
{
    Serial.begin(9600);
    waitForSerial();
    Serial.println("TGX_HW_PROBE_BEGIN");
#if defined(ARDUINO_TEENSY41)
    Serial.println("BOARD_MACRO=ARDUINO_TEENSY41");
#elif defined(PICO_RP2350) || defined(ARDUINO_ARCH_RP2350)
    Serial.println("BOARD_MACRO=RP2350");
#elif defined(PICO_RP2040) || defined(ARDUINO_ARCH_RP2040)
    Serial.println("BOARD_MACRO=RP2040");
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    Serial.println("BOARD_MACRO=ESP32S3");
#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP32)
    Serial.println("BOARD_MACRO=ESP32");
#else
    Serial.println("BOARD_MACRO=UNKNOWN");
#endif
    Serial.print("MILLIS=");
    Serial.println(millis());
    Serial.println("TGX_HW_PROBE_OK");
    Serial.println("TGX_HW_PROBE_END");
}

void loop()
{
    delay(1000);
}
