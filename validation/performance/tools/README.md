# TGX Performance Tools

This directory contains reusable upload, serial-capture, parsing, aggregation, and codegen helpers used during the June 2026 TGX performance investigations.

These tools are intentionally stored outside `tmp/` so future optimization sessions can find them without digging through dated scratch directories.

## Purpose

- Upload probe or example sketches to known boards.
- Wait for USB serial ports to disappear/reappear after upload.
- Capture complete serial telemetry with start/end marker checks.
- Reject missing, stale, partial, or unparseable telemetry.
- Parse legacy benchmark output, example telemetry, macro probes, and codegen
  summaries.
- Aggregate historical benchmark runs into compact CSVs.

## June 2026 Fixed Ports

| Board | Port | Status |
| --- | --- | --- |
| Pico2 / RP2350 + ILI9341 | `COM21` | Primary Pico runtime board |
| Teensy 4.1 + ILI9341 | `COM3` | Dedicated Teensy USB port |
| Core2 | `COM5` | ESP32 Core2 runtime board |
| CoreS3 | `COM10` | ESP32-S3 CoreS3 runtime board |
| Feather S3 + ILI9341 | `COM14` | Runtime serial/display board; bootloader may appear as `COM13` |
| Feather S2 + ILI9341 | `COM11` | Runtime serial/display board; bootloader may appear as `COM12` |
| Pico W / RP2040 + ILI9341 | `COM19` | Arduino runtime port after recovery from UF2/bootloader mode |
| Teensy 3.6 + ST7735 | `COM23` | Runtime serial; Teensy upload port may share the Teensy USB path |

## Quick Probe

Run a probe matrix with:

```powershell
powershell -ExecutionPolicy Bypass -File validation\performance\tools\run_probe_matrix.ps1
```

Or run a single probe through the generic capture tool:

```powershell
powershell -ExecutionPolicy Bypass -File validation\performance\tools\upload_and_capture.ps1 `
  -Board pico2 `
  -Port COM21 `
  -Sketch validation\performance\tools\sketches\TgxMacroProbe `
  -StartMarker TGX_MACRO_PROBE_BEGIN `
  -EndMarker TGX_MACRO_PROBE_END `
  -MinTelemetryLines 4
```

Probe sketches are stored under:

```text
validation/performance/tools/sketches/
```

## Benchmark Capture

The old monolithic Arduino benchmark under `examples/benchmark` has been
removed. New benchmark runs should use the modular suite:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board <board> --module <module>
```

The parsers and aggregators in this directory are kept for old investigation
data and compact baseline files. They are not the entry point for new 3D
benchmark campaigns.

## Teensy4 Renderer3D Flash Macro Matrix

To validate the optional Teensy4 `Renderer3D` flash-placement macros, use:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" `
  validation\performance\tools\run_teensy4_flash_macro_matrix.py `
  --out validation\performance\local_results\investigations\<YYYY-MM-DD>-teensy4-flash-macro-matrix
```

The script runs the modular benchmark on `teensy41` for these candidates:

```text
none
sphere
truncated_cone
mesh3d
mesh3dv2
sphere_cone
mesh_all
all
```

Each candidate is implemented by setting the corresponding
`TGX_BENCH_FLASH_*` defines and calling
`validation/benchmark3d/tools/run_bench3d.py`. For Teensyduino 1.62 and newer,
the benchmark runner passes these flags through `build.flags.cpp`, because
`compiler.cpp.extra_flags` does not reliably reach all Teensy compilation
steps.

The matrix output contains:

```text
matrix_manifest.json
<candidate>/...
summary/candidate_status.csv
summary/perf_delta_vs_none.csv
summary/size_delta_vs_none.csv
summary/README.md
```

Use `--dry-run` to check the commands without compiling/uploading. Use
`--modules primitives,mesh` or `--candidates none,sphere,all` for shorter
targeted checks.

Aggregate renderer-style candidate outputs with:

```powershell
python validation\performance\tools\aggregate_renderer_candidates.py renderer_
```

Generic aggregation scripts are also available:

```text
aggregate_capture_runs.py
aggregate_candidates.py
aggregate_examples.py
```

## Example Telemetry

Use:

```powershell
powershell -ExecutionPolicy Bypass -File validation\performance\tools\run_example_baselines.ps1
```

or the lower-level `run_example.ps1` when running one example/candidate manually.

Example parsing helpers:

```text
parse_example_telemetry.py
aggregate_examples.py
```

## Partial Telemetry Rejection

The capture workflow treats a run as valid only when:

- upload succeeds;
- the expected COM port returns and can be opened;
- serial capture sees the expected start marker when configured;
- serial capture sees the expected completion marker, or the parser determines completion;
- enough telemetry lines were captured;
- benchmark or example parsing succeeds;
- stale output from a previous run is not mixed with current output.

Failure states are reported explicitly, including:

```text
UPLOAD_FAILED
UPLOAD_OK_SERIAL_CAPTURE_FAILED
PORT_NOT_AVAILABLE
SERIAL_TIMEOUT
PARTIAL_TELEMETRY
PARSE_FAILED
SUCCESS
```

## Teensy Note

Teensyduino may report upload completion before the Teensy has fully rebooted and before the sketch is ready on `COM3`. The upload/capture script therefore releases serial before upload, waits for the Teensy port to return, and retries serial opening before declaring failure.

## Pico Note

Pico ports can disappear during UF2 upload and reappear after reboot. The capture script logs visible ports before/after upload and waits for the expected port (`COM21` for Pico2 in the June 2026 setup) to be readable before starting telemetry capture.

## Feather S2/S3 Note

Feather S2/S3 boards use native USB CDC. During upload they may switch away
from their runtime ports. In the June 2026 setup, Feather S3 sometimes used
bootloader `COM13` then runtime `COM14`; Feather S2 used bootloader `COM12`
then runtime `COM11`. For sketches using `TFT_eSPI`, set
`TFT_eSPI/User_Setup_Select.h` to
`Setup_feather_TFT_S2_S3_ILI9341.h` before compiling.

## Future Investigation Output

Store future performance work under:

```text
validation/performance/local_results/investigations/<date-and-topic>/
```

Recommended structure:

```text
reports/
summaries/
patches/
codegen/
hardware/
raw_or_large_artifacts/
```

Keep reusable scripts in this `tools/` directory rather than inside dated local
investigation folders. `validation/performance/local_results/` is ignored by
Git and is the right place for generated telemetry, baselines, reports, codegen
dumps and build outputs.
