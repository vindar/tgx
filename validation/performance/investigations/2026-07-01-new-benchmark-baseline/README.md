# New benchmark3D baseline investigation

Date: 2026-07-01

Purpose: establish the first modular benchmark3D baseline on the development branch and compare it with the current main/release code where compatible.

Boards:
- Teensy 4.1 / Cortex-M7: COM3, upload usb:80000/3/0/1
- Core2 / ESP32: COM5
- CoreS3 / ESP32-S3: COM10
- Pico W / RP2040: COM19
- Pico2 / RP2350: COM21
- ESP32P4 / Waveshare: COM27, ESP-IDF backend

Policy:
- benchmark modules are run and validated one board/module at a time;
- real examples are uploaded and checked one at a time;
- generated raw logs remain under tmp unless needed to document a failure;
- compact CSV/JSON summaries and final notes are kept here.
