# TGX 3D MCU Benchmark

This sketch is the portable 3D performance benchmark for TGX. It prints all
results on `Serial` and waits for one input character before starting, so the
same uploaded sketch can be restarted from a serial monitor.

Regular hardware targets:

- Teensy 4.1 + ILI9341_T4
- Teensy 3.6 + ST7735
- ESP32 Core2
- ESP32-S3 CoreS3
- ESP32-S2 Feather TFT + ILI9341
- RP2040 Pico W + ILI9341
- RP2350 Pico 2 + ILI9341

Build by defining exactly one `TGX_BENCHMARK_*` symbol for the target when the
board cannot be detected unambiguously. The benchmark intentionally avoids any
display driver dependency; it measures TGX rendering into memory buffers only.

Typical Arduino CLI examples:

```powershell
arduino-cli compile --fqbn "teensy:avr:teensy41:usb=serial,speed=600,opt=o3std" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_T4" examples/benchmark
arduino-cli compile --fqbn "teensy:avr:teensy36:usb=serial,speed=180,opt=o3std" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_T36" examples/benchmark
arduino-cli compile --fqbn "esp32:esp32:m5stack_core2" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_ESP32" examples/benchmark
arduino-cli compile --fqbn "esp32:esp32:m5stack_cores3" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_ESP32S3" examples/benchmark
arduino-cli compile --fqbn "esp32:esp32:adafruit_feather_esp32s2_tft" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_ESP32S2" examples/benchmark
arduino-cli compile --fqbn "rp2040:rp2040:rpipicow" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_RP2040" examples/benchmark
arduino-cli compile --fqbn "rp2040:rp2040:rpipico2" --build-property "compiler.cpp.extra_flags=-DTGX_BENCHMARK_RP2350" examples/benchmark
```
