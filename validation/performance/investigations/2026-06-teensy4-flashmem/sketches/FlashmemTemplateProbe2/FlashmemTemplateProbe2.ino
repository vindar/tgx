#include <Arduino.h>
#include "probe_templates2.h"

volatile int sinkValue = 0;

void setup()
{
    sinkValue += regular_template<int>(1);
    sinkValue += flash_decl_template<int>(2);
    sinkValue += flash_def_template<int>(3);
    sinkValue += flashmem_named_template<int>(4);
    sinkValue += specialized_template<int>(5);
    sinkValue += flash_wrapper_calls_forceinline(6);
    sinkValue += plain_flash(7);
    sinkValue += cold_template<int>(8);
    sinkValue += cold_plain(9);
}

void loop()
{
    delay(1000);
}
