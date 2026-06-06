# TGX Hardware Readiness

Date: 2026-06-05

Reusable capture tool:

- `tmp/hardware_validation/tools/upload_and_capture.ps1`
- Probe matrix helper: `tmp/hardware_validation/tools/run_probe_matrix.ps1`

The capture protocol now supports upload, port-return wait, serial-open retries,
optional kick text, start/end marker checks, minimum telemetry-line checks,
benchmark parsing, example telemetry parsing, metadata JSON, and raw telemetry logs.

## Board Status

| Board | Port | Upload | Serial Capture | Macro Probe | Telemetry Probe | Trusted For Campaign | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Core2 | COM5 | OK | OK | OK | OK | Yes | Initial no-output failures were caused by probes printing before capture opened. Fixed with kick-synchronized probes. |
| CoreS3 | COM10 | OK | OK | OK | OK | Yes | Benchmark parser sanity passed: 3 global scores, 162 subtests. |
| Pico W / RP2040 | COM28 | Failed | Not tested after failed upload | Failed | Failed | No | Default UF2 upload reports `No drive to deploy`. `uploadmethod=picotool` reports a RP2040 BOOTSEL device but cannot connect and suggests a Zadig driver issue. Treat as compile-only/unavailable until driver/bootloader access is fixed. |
| Pico2 / RP2350 | COM21 | OK | OK | OK | OK | Yes | Benchmark parser sanity passed: 3 global scores, 162 subtests. Real `bunny_fig` telemetry passed with 42 rows. |
| Teensy 4.1 | COM3 | OK after Teensy Loader launch | OK | OK | OK | Yes | First serial-alive upload failed because Teensy Loader was not running; subsequent serial, macro, and telemetry probes passed. Real `scream` telemetry passed with 39 rows. |

## Macro Probe Highlights

| Board | TGX_INLINE | TGX_INLINE_ZDIVIDE | Incremental Pixel Pointers | Fast inv/sqrt | FMA |
| --- | --- | --- | --- | --- | --- |
| Core2 | `__attribute__((always_inline))` | empty | 0 | 0 / 0 / 0 | 0 |
| CoreS3 | `__attribute__((always_inline))` | `__attribute__((always_inline))` | 0 | 0 / 0 / 0 | 0 |
| Pico2 | `__attribute__((always_inline))` | `__attribute__((always_inline))` | 1 | 1 / 1 / 1 | 0 |
| Teensy 4.1 | `__attribute__((always_inline))` | `__attribute__((always_inline))` | 0 | 1 / 1 / 1 | 1 |

## Parser Sanity

| Test | Board | Status | Parsed |
| --- | --- | --- | --- |
| `examples/benchmark` | CoreS3 | SUCCESS | 3 global scores, 162 subtests |
| `examples/benchmark` | Pico2 | SUCCESS | 3 global scores, 162 subtests |
| `examples/Teensy4/3D/scream` | Teensy 4.1 | SUCCESS | 39 telemetry rows |
| `examples/Pico_RP2040_RP2350/bunny_fig` | Pico2 | SUCCESS | 42 telemetry rows |

## Important Capture Notes

- Probe sketches wait for a serial kick byte before printing markers, with a timeout fallback. This prevents missing one-shot boot output on ESP32 and RP boards.
- Benchmark sketches are started with `-KickText x` because `examples/benchmark` waits for a serial keypress.
- Example telemetry is accepted only when enough lines are parsed; partial or empty telemetry is rejected.
- CSV output is forced to invariant culture so decimal values use `.` on this French Windows system.
