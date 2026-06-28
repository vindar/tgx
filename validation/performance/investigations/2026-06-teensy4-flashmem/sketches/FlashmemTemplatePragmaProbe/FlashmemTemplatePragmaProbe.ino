#include <Arduino.h>
#include "pragma_probe.h"

volatile int sinkValue = 0;

void setup()
{
    sinkValue += pragma_template<int>(1);
    sinkValue += pragma_plain(2);
}

void loop()
{
    delay(1000);
}
