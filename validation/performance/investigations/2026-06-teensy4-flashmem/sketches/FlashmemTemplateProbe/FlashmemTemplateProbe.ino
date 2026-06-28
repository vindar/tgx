#include <Arduino.h>
#include "probe_templates.h"

volatile int sinkValue = 0;

void setup()
{
    sinkValue += templ_prefix<int>(1);
    sinkValue += templ_decl_suffix<int>(2);
    sinkValue += templ_prefix_direct<int>(3);
    sinkValue += templ_used_direct<int>(4);
    sinkValue += plain_prefix(5);
    sinkValue += plain_decl_suffix(6);
}

void loop()
{
    delay(1000);
}
