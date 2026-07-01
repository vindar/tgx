# Arduino Sketch Launchers

This directory contains one minimal Arduino sketch per benchmark module.

The sketches are intentionally tiny: they select a module with a
`TGX_BENCH_MODULE_*` define, include `<TGXBenchmark3D.h>`, and call
`tgxBenchRunSelectedModule()` from `setup()`. Keep board-specific logic out of
these sketches; put it in `boards/` or the board JSON config instead.
