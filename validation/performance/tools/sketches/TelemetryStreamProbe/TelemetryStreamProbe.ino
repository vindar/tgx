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
    Serial.println("TGX_TELEMETRY_PROBE_BEGIN");
    const char* scenes[] = {"alpha", "beta", "gamma"};
    for (int s = 0; s < 3; ++s) {
        for (int i = 0; i < 3; ++i) {
            Serial.print("SCENE=");
            Serial.print(scenes[s]);
            Serial.print(" FPS=");
            Serial.print(30.0f + 5.0f * s + 0.25f * i, 2);
            Serial.print(" FRAME_US=");
            Serial.print(33333 - 2000 * s - 100 * i);
            Serial.print(" SAMPLE=");
            Serial.println(i + 1);
            delay(120);
        }
    }
    Serial.println("TGX_TELEMETRY_PROBE_OK");
    Serial.println("TGX_TELEMETRY_PROBE_END");
}

void loop()
{
    delay(1000);
}
