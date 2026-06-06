# Hardware Readiness

Date: 2026-06-06

Quick revalidation used `tmp/hardware_validation/tools/upload_and_capture.ps1` with `TgxMacroProbe`.

| Board | Port | Upload | Serial capture | Macro probe | Trusted for this session | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Core2 | COM5 | OK | OK | OK | Yes | Metadata in `tmp/hardware_validation/parsed/core2_image_session_macro_probe_*.json`. |
| CoreS3 | COM10 | OK | OK | OK | Yes | Metadata in `tmp/hardware_validation/parsed/cores3_image_session_macro_probe_*.json`. |
| Pico2 / RP2350 | COM21 | OK | OK | OK | Yes | Primary optimization target. Metadata in `tmp/hardware_validation/parsed/pico2_image_session_macro_probe_*.json`. |
| Teensy 4.1 | COM3 | OK | OK | OK | Yes | Upload used Teensy port `usb:80000/3/0/1`. |
| Pico W / RP2040 | COM28 | Not retried | Not trusted | Not trusted | Compile-only | Previous pass showed UF2/picotool upload failure. Left out of performance runs unless fixed separately. |

The quick probe confirms that the robust upload/capture pipeline is usable for the trusted boards before image-inline benchmarking starts.
