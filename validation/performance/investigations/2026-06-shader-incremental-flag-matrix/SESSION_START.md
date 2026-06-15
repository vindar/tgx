# Shader Incremental Flag Matrix Session

Date: 2026-06-15

Current branch: main

Current HEAD:

```text
fa99d8894028ab824f64435375094c605d633e6b
```

Starting source state:

```text
clean HEAD after stashing src/Shaders.h candidate
```

Candidate stash:

```text
stash@{0}: On main: shader optimization candidate before flag matrix
```

Purpose:

- Rebuild the current HEAD baseline after recent example telemetry changes.
- Reapply the `src/Shaders.h` optimization candidate and measure the same benchmark/example matrix.
- Test the two independent candidate compile-time flags:
  - `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL`
  - `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL`
- Determine whether either flag should be enabled or disabled per target board.

Connected boards:

| Board | Port | Build profile |
| --- | --- | --- |
| Teensy 4.1 + ILI9341 | COM3 | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` |
| Pico2 / RP2350 + ILI9341 | COM21 | `rp2040:rp2040:rpipico2:opt=Fast` |
| Core2 / ESP32 | COM5 | `esp32:esp32:m5stack_core2` |
| CoreS3 / ESP32-S3 | COM10 | `esp32:esp32:m5stack_cores3` |

Test matrix:

- 3D benchmark on all four boards.
- Teensy 4.1 examples: `mars`, `test-shading`, `test-texture`, `buddha`.
- Pico2 examples: `borg_cube`, `bunny_fig`, `scream`.
- Core2/CoreS3 examples: `borg_cube`, `donkeykong`, `scream`.

Capture tooling:

- `validation/performance/tools/upload_and_capture.ps1`
- Robust upload/capture only; no ad-hoc serial capture.

Notes:

- Previous example telemetry is considered stale because examples were changed.
- The candidate default defines in the stash are both `1`; explicit flag matrix builds override them with compiler `-D` flags.
- Raw logs/builds are temporary; compact CSV summaries are retained in this investigation.
