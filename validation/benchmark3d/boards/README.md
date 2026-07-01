# Board Backends

Board backends provide the small platform-specific layer used by otherwise
board-independent benchmark modules.

- `cpu/`: desktop backend used by the native CPU runner and optional CImg visual
  playback.
- `mcu_simple/`: Arduino-compatible MCU backend used by Teensy, ESP32 and
  RP2040/RP2350 boards.
- `espidf/`: ESP-IDF backend used by ESP32-P4.

Backends define framebuffer allocation, timing, serial output and optional
platform yields. They should not contain benchmark scenes or TGX feature tests;
those belong under `modules/`.
