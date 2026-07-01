# Common Harness

This directory contains the benchmark harness shared by CPU, Arduino and
ESP-IDF builds.

`tgx_bench.h/cpp` defines test descriptors, timing helpers, telemetry emission,
checksum support and the module runner. Benchmark modules should use this layer
instead of printing ad hoc serial output so the Python runner can parse every
board consistently.
