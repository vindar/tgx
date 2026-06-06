# Hardware Probe

Date: 2026-06-06

Both target boards were probed with `validation/performance/tools/upload_and_capture.ps1` and marker-validated serial capture. The same tool was then used for the TextureCoordEquivalenceBench captures.

| Board | Port | Upload | Serial capture | Notes |
| --- | --- | --- | --- | --- |
| Teensy 4.1 | COM3 | OK | OK | Upload port detected as `usb:80000/3/0/1`; Teensyduino post-upload delay/wait handled by capture tool. Macro probe and equivalence bench both completed with start/end markers. |
| Pico2 / RP2350 | COM21 | OK | OK | UF2 upload via COM21 reset succeeded; port returned and capture completed with start/end markers. |

Validated equivalence runs:

| Board | Label | Telemetry lines | Status |
| --- | --- | ---: | --- |
| Teensy 4.1 | `texcoord_eq_teensy41_run4` | 1957 | SUCCESS |
| Pico2 / RP2350 | `texcoord_eq_pico2_run4` | 1957 | SUCCESS |

The measurements in this investigation use these valid run4 telemetry files, not the earlier partial Pico capture or pre-fix nearest-interior run.
