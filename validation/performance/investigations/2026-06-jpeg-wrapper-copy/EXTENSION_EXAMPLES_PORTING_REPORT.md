# Extension Examples Porting Report

Date: 2026-06-07

## Summary

The optional decoder examples were moved into architecture-specific `extensions/` folders and ported to the currently used boards:

- `examples/Teensy4/2D/extensions/{JPEG_test,PNG_test,GIF_test}`
- `examples/M5Stack/extensions/{JPEG_test,PNG_test,GIF_test}`
- `examples/ESP32/extensions/{JPEG_test,PNG_test,GIF_test}`
- `examples/Pico_RP2040_RP2350/extensions/{JPEG_test,PNG_test,GIF_test}`

Each example is standalone and carries its own image asset copy.

## Compile Status

| Board target | Example set | Status | Notes |
| --- | --- | --- | --- |
| Teensy 4.1 | JPEG / PNG / GIF | OK | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` |
| Core2 | JPEG / PNG / GIF | OK | `esp32:esp32:m5stack_core2` |
| CoreS3 | JPEG / PNG / GIF | OK | `esp32:esp32:m5stack_cores3` |
| Pico2 | JPEG / PNG / GIF | OK | `rp2040:rp2040:rpipico2:os=freertos`; GIF also needs `compiler.cpp.extra_flags=-include Arduino.h` because the external AnimatedGIF library omits `Arduino.h` under this core configuration. |
| FeatherS3 + external ILI9341 | JPEG / PNG / GIF | OK | `esp32:esp32:adafruit_feather_esp32s3_tft`; TFT_eSPI must use `Setup_feather_TFT_S2_S3_ILI9341.h`. |

## Upload / Runtime Capture Status

| Board | Port | Examples uploaded | Serial capture | Key telemetry |
| --- | --- | --- | --- | --- |
| Core2 | `COM5` | JPEG / PNG / GIF | OK | JPEG hash `116D1AF`; PNG hash `F70A5EE1`; GIF first-frame hash `EE9529CC`. |
| CoreS3 | `COM10` | JPEG / PNG / GIF | OK | JPEG hash `116D1AF`; PNG hash `F70A5EE1`; GIF first-frame hash `EE9529CC`. |
| Pico2 | `COM21` | JPEG / PNG / GIF | OK after repeated telemetry | JPEG hash `116D1AF`; PNG hash `F70A5EE1`; GIF repeat-frame hash `22CF23CE`. |
| Teensy 4.1 | `COM3` / Teensy upload port `usb:80000/3/0/6/4` | JPEG / PNG / GIF | OK | JPEG hash `116D1AF`; PNG hash `F70A5EE1`; GIF first-frame hash `EE9529CC`. |
| FeatherS3 | `COM14` runtime, upload reported through `COM13` | JPEG / PNG / GIF | Upload OK, serial capture unavailable | The board previously displayed `borg_cube` correctly with the same external ILI9341 TFT_eSPI setup. Extension upload succeeded, but no serial lines were captured from this board. |

## Tooling Notes

- `validation/performance/tools/upload_and_capture.ps1` now opens serial capture before running the slower post-upload `arduino-cli board list`, so one-shot ESP telemetry is not missed.
- For Pico/RP2040/RP2350 capture, DTR remains enabled but RTS is kept low. This avoids an unnecessary RTS assertion while still allowing USB CDC capture.
- Pico extension examples repeat a compact telemetry block from `loop()` because the RP2040/RP2350 USB serial timing can miss the first one-shot `setup()` print.
- `Serial.flush()` was added after telemetry marker blocks in the extension examples to make capture more deterministic.

## TFT_eSPI Setup Notes

The Arduino library file `D:\Programmation\arduino\libraries\TFT_eSPI\User_Setup_Select.h` must be switched depending on the target:

- Pico2 / RP2350 + ILI9341: `Setup_RP2040_RP2350_ILI9341.h`
- FeatherS3 + external ILI9341: `Setup_feather_TFT_S2_S3_ILI9341.h`

After the FeatherS3 tests, the local TFT_eSPI setup was restored to the Pico/RP2350 ILI9341 setup.

## Warnings / Limitations

- Pico builds warn that `TOUCH_CS` is not defined; these extension examples do not use touch.
- Pico GIF emits a `memcpy_P` redefinition warning from the external AnimatedGIF library; the build succeeds with the explicit `-include Arduino.h` workaround.
- FeatherS3 serial capture remains unreliable/unavailable in this setup, even though upload works. Visual validation should be used for FeatherS3 until its USB CDC behavior is diagnosed separately.
