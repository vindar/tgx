#include <Arduino.h>
#include <tgx.h>

using namespace tgx;

static const int ANGLE_COUNT = 361;
#if defined(TGX_RUN_ON_RP2350)
static const int REPEAT_COUNT = 500;
#else
static const int REPEAT_COUNT = 2000;
#endif
static const float TGX_BENCH_DEG_TO_RAD = 0.017453292519943295769f;

float degAngles[ANGLE_COUNT];
float radAngles[ANGLE_COUNT];
volatile float benchSink = 0.0f;
uint32_t resultFast0 = 0;
uint32_t resultFast1 = 0;
uint32_t resultSinDeg0 = 0;
uint32_t resultSinDeg1 = 0;
uint32_t resultSinRad0 = 0;
uint32_t resultSinRad1 = 0;


static void setupAngles()
    {
    for (int i = 0; i < ANGLE_COUNT; i++)
        {
        const float deg = -180.0f + (float)i;
        degAngles[i] = deg;
        radAngles[i] = deg * TGX_BENCH_DEG_TO_RAD;
        }
    }


static uint32_t benchFastSin()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += tgx_fast_sin_deg_clamped(degAngles[i]);
            }
        }
    const uint32_t dt = micros() - t0;
    benchSink = acc;
    return dt;
    }


static uint32_t benchSinfDegConvert()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += sinf(degAngles[i] * TGX_BENCH_DEG_TO_RAD);
            }
        }
    const uint32_t dt = micros() - t0;
    benchSink = acc;
    return dt;
    }


static uint32_t benchSinfRadPrecomputed()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += sinf(radAngles[i]);
            }
        }
    const uint32_t dt = micros() - t0;
    benchSink = acc;
    return dt;
    }


static void printResult(const char* name, uint32_t us)
    {
    const uint32_t calls = (uint32_t)ANGLE_COUNT * (uint32_t)REPEAT_COUNT;
    Serial.print(name);
    Serial.print(",us=");
    Serial.print(us);
    Serial.print(",calls=");
    Serial.print(calls);
    Serial.print(",ns_per_call=");
    Serial.println(((uint64_t)us * 1000ULL) / calls);
    }


static void printAllResults()
    {
    Serial.println("FastSinBench begin");
    Serial.print("ANGLE_COUNT=");
    Serial.println(ANGLE_COUNT);
    Serial.print("REPEAT_COUNT=");
    Serial.println(REPEAT_COUNT);
    printResult("tgx_fast_sin_deg_clamped_run0", resultFast0);
    printResult("tgx_fast_sin_deg_clamped_run1", resultFast1);
    printResult("sinf_deg_convert_run0", resultSinDeg0);
    printResult("sinf_deg_convert_run1", resultSinDeg1);
    printResult("sinf_rad_precomputed_run0", resultSinRad0);
    printResult("sinf_rad_precomputed_run1", resultSinRad1);
    Serial.print("sink=");
    Serial.println((float)benchSink, 6);
    Serial.println("FastSinBench done");
    }


void setup()
    {
    Serial.begin(115200);
    const uint32_t waitStart = millis();
    while (!Serial && millis() - waitStart < 5000)
        {
        delay(10);
        }
    delay(300);
    setupAngles();

    Serial.println("FastSinBench measuring");
    Serial.print("ANGLE_COUNT=");
    Serial.println(ANGLE_COUNT);
    Serial.print("REPEAT_COUNT=");
    Serial.println(REPEAT_COUNT);

    (void)benchFastSin();
    (void)benchSinfDegConvert();
    (void)benchSinfRadPrecomputed();

    resultFast0 = benchFastSin();
    resultFast1 = benchFastSin();
    resultSinDeg0 = benchSinfDegConvert();
    resultSinDeg1 = benchSinfDegConvert();
    resultSinRad0 = benchSinfRadPrecomputed();
    resultSinRad1 = benchSinfRadPrecomputed();

    printAllResults();
    }


void loop()
    {
    delay(3000);
    printAllResults();
    }
