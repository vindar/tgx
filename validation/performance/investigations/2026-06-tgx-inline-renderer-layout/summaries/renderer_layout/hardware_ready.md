# Hardware Readiness

Date: 2026-06-06

Quick probe used `tmp/hardware_validation/tools/upload_and_capture.ps1` with `tmp/hardware_validation/sketches/TgxMacroProbe`.

| Board | Port | Upload | Serial capture | Trusted | Notes |
| --- | --- | --- | --- | --- | --- |
| Pico2 / RP2350 | `COM21` | OK | OK | Yes | Probe `pico2_renderer_session_macro_probe`, baud `115200`. USB serial made baud irrelevant. |
| Teensy 4.1 | `COM3` | OK | OK | Yes | Probe `teensy41_renderer_session_macro_probe`, baud `115200`; Teensy upload port `usb:80000/3/0/1`. |
| Core2 | `COM5` | OK | OK | Yes | First probe at `115200` timed out because probe sketch uses `Serial.begin(9600)`. Retry `core2_renderer_session_macro_probe_9600` succeeded. |
| CoreS3 | `COM10` | OK | OK | Yes | Probe `cores3_renderer_session_macro_probe_9600` succeeded at `9600`. |

Pico W / RP2040 is intentionally ignored for this session.

The robust upload/capture pipeline is usable for candidate benchmarks. For ESP32 probe sketches, use `9600` baud unless the sketch explicitly starts serial at another baud. Benchmark sketches still use the benchmark runner's `115200` configuration.
