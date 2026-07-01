#include <Arduino.h>

#define TGX_BENCH_MODULE_PRIMITIVES 1
#include <TGXBenchmark3D.h>

void setup()
{
    tgxBenchRunSelectedModule();
}

void loop()
{
    delay(1000);
}
