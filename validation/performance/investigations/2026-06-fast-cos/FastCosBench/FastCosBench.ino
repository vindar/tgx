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
uint32_t resultCosDeg0 = 0;
uint32_t resultCosDeg1 = 0;
uint32_t resultCosRad0 = 0;
uint32_t resultCosRad1 = 0;


static void setupAngles()
    {
    for (int i = 0; i < ANGLE_COUNT; i++)
        {
        const float deg = -180.0f + (float)i;
        degAngles[i] = deg;
        radAngles[i] = deg * TGX_BENCH_DEG_TO_RAD;
        }
    }


static uint32_t benchFastCos()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += tgx_fast_cos_deg_clamped(degAngles[i]);
            }
        }
    const uint32_t dt = micros() - t0;
    benchSink = acc;
    return dt;
    }


static uint32_t benchCosfDegConvert()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += cosf(degAngles[i] * TGX_BENCH_DEG_TO_RAD);
            }
        }
    const uint32_t dt = micros() - t0;
    benchSink = acc;
    return dt;
    }


static uint32_t benchCosfRadPrecomputed()
    {
    volatile float acc = 0.0f;
    const uint32_t t0 = micros();
    for (int r = 0; r < REPEAT_COUNT; r++)
        {
        for (int i = 0; i < ANGLE_COUNT; i++)
            {
            acc += cosf(radAngles[i]);
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
    Serial.println("FastCosBench begin");
    Serial.print("ANGLE_COUNT=");
    Serial.println(ANGLE_COUNT);
    Serial.print("REPEAT_COUNT=");
    Serial.println(REPEAT_COUNT);
    printResult("tgx_fast_cos_deg_clamped_run0", resultFast0);
    printResult("tgx_fast_cos_deg_clamped_run1", resultFast1);
    printResult("cosf_deg_convert_run0", resultCosDeg0);
    printResult("cosf_deg_convert_run1", resultCosDeg1);
    printResult("cosf_rad_precomputed_run0", resultCosRad0);
    printResult("cosf_rad_precomputed_run1", resultCosRad1);
    Serial.print("sink=");
    Serial.println((float)benchSink, 6);
    Serial.println("FastCosBench done");
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

    Serial.println("FastCosBench measuring");
    Serial.print("ANGLE_COUNT=");
    Serial.println(ANGLE_COUNT);
    Serial.print("REPEAT_COUNT=");
    Serial.println(REPEAT_COUNT);

    // Warm up caches and math code paths before the measured runs.
    (void)benchFastCos();
    (void)benchCosfDegConvert();
    (void)benchCosfRadPrecomputed();

    resultFast0 = benchFastCos();
    resultFast1 = benchFastCos();
    resultCosDeg0 = benchCosfDegConvert();
    resultCosDeg1 = benchCosfDegConvert();
    resultCosRad0 = benchCosfRadPrecomputed();
    resultCosRad1 = benchCosfRadPrecomputed();

    printAllResults();
    }


void loop()
    {
    delay(3000);
    printAllResults();
    }
