# TGX benchmark3D

Modular 3D benchmark suite.

This directory replaces the legacy monolithic Arduino benchmark that used to
live under `examples/benchmark`. The benchmark now lives under `validation/`
because it has CPU, Arduino and ESP-IDF backends plus shared tooling. The goal
is to keep it in small, reusable layers:

- board-independent test modules;
- a common benchmark harness;
- CPU and MCU backends;
- thin launchers for CPU and Arduino;
- stable serial telemetry that can be parsed by automation scripts.

The design prompt for this work is currently kept in
`codex_notes/benchmark3d_suite_feature_prompt.md`.

## Directory Layout

- `arduino/`: local Arduino wrapper library used by the MCU sketches.
- `boards/`: platform backends for CPU, Arduino-style MCUs and ESP-IDF.
- `common/`: shared harness, timing, telemetry and checksum code.
- `cpu/`: native desktop entry point, including optional CImg visual playback.
- `idf/`: ESP-IDF project used by ESP32-P4.
- `modules/`: board-independent benchmark modules.
- `sketches/`: minimal Arduino launchers, one per module.
- `tools/`: Python runner and JSON board/module configuration.

Implemented modules:

- `modules/core_raster`: large triangles, small triangle grids, Gouraud,
  no-zbuffer, near/far clipping, rasterizer viewport margin, renderer screen
  clipping variants and face culling.
- `modules/texture_raster`: synthetic textures, perspective-correct texturing,
  affine texturing, nearest/bilinear sampling, wrap/clamp modes, triangle
  batches, triangle strips and quads.
- `modules/lighting_shading`: directional flat/Gouraud lighting, directional
  specular, compiled-but-inactive spotlight capacity, point lights, soft/hard
  spotlights, local specular, textured Gouraud lighting and per-frame light
  updates.
- `modules/primitives`: generated cube/sphere/cylinder/cone/truncated-cone
  drawing, textured caps/sides, open vs closed primitives, near-plane clipping
  and generated primitive wireframes.
- `modules/mesh`: synthetic legacy `Mesh3D` and compact `Mesh3Dv2` meshes,
  material override vs mesh materials, small-triangle grids, textured
  perspective/affine paths, multi-material meshlets, near-plane clipping,
  whole-mesh discard and face culling.
- `modules/wire_points`: raw wireframe line/triangle/strip/quad APIs,
  `Mesh3D`/`Mesh3Dv2` wireframes, antialiased and thick wire paths, projected
  pixels and projected dots with zbuffer, no-zbuffer, palette and opacity modes.
- `modules/skybox_noz`: skybox rendering without zbuffer, nearest/bilinear
  skybox sampling, null/partial face handling, reference-height path, zbuffered
  foreground objects and no-zbuffer overlays.
- `modules/integrated`: realistic mixed scenes combining textured meshes,
  point lights, soft spotlights, generated primitives, skybox backgrounds,
  zbuffered rendering and no-zbuffer overlays.
- `modules/mixed_stress`: deliberately large mixed Renderer3D scenes for
  codegen/layout stress, combining several shader modes in one renderer,
  textured and untextured paths, flat/Gouraud/unlit rendering, zbuffer/no-zbuffer
  switching, generated primitives, meshes, triangle lists, triangle strips,
  wireframes, skyboxes, clipping and local lights.

Board and module orchestration is data-driven:

- `tools/config/modules.json` defines the module compile macro and Arduino
  sketch launcher.
- `tools/config/boards/*.json` defines board kind, FQBN, ports, serial policy
  and supported modules.

The measured time is rendering only. Framebuffer clear, zbuffer clear, setup,
checksum and telemetry are deliberately outside the timed section.

Backends may yield between tests to keep serial/RTOS/watchdog services alive;
those yields are outside the timed render section.

## Current build commands and tools

Use the conda environment Python:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --list
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module core_raster
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module all
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board teensy41 --module texture_raster
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board esp32p4 --module core_raster
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board all-connected --module lighting_shading,primitives,mesh,wire_points
```

The helper compiles, uploads when needed, captures serial telemetry, validates
the marker sequence, writes parsed JSON/CSV, and writes a manifest containing
git state, board/module config, exact commands and parsed build sizes when
available.

`--board` accepts a single board id, a comma-separated list, `all`, or
`all-connected`. `--module` accepts a single module id, a comma-separated list,
or `all` for the modules supported by each selected board. Batch runs continue
after individual failures unless `--stop-on-error` is passed. A batch writes
`batch_manifest.json` and `batch_results.csv` next to the per-run directories.

CPU visual validation can be run directly from a CPU binary compiled by the
helper:

```powershell
.\tmp\benchmark3d\bench3d_cpu_core_raster.exe --module core_raster --visual --visual-out tmp\benchmark3d\core_raster_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_texture_raster.exe --module texture_raster --visual --show --visual-out tmp\benchmark3d\texture_raster_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_mesh.exe --module mesh --visual --visual-out tmp\benchmark3d\mesh_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_wire_points.exe --module wire_points --visual --visual-out tmp\benchmark3d\wire_points_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_mixed_stress.exe --module mixed_stress --visual --visual-out tmp\benchmark3d\mixed_stress_mosaic.bmp
```

`--visual` saves one 4x5 frame mosaic per sub-test. The 20 frames are sampled
across the full visual cycle, not only from the beginning of the test. For
example, the output prefix above creates files such as
`core_raster_mosaic_core_raster_big_tri_flat_z.bmp`.

`--show` also opens a CImg window and plays the sub-tests in real time, capped
around 60 FPS. Close the CImg window to stop the visual run.

The additional `--libraries ...\validation\benchmark3d\arduino` path exposes
the thin local Arduino wrapper library `TGXBenchmark3D`. The wrapper is
intentionally header-only: each sketch defines one `TGX_BENCH_MODULE_*` macro
before including the wrapper, so only the selected module is compiled into that
firmware.

Telemetry format is documented in `TELEMETRY.md`.

## Current configured boards

- `cpu`
- `teensy41`
- `core2`
- `cores3`
- `picow`
- `pico2`
- `esp32p4`

## Current connected MCU targets

These are the six primary hardware targets currently connected and usable for
cross-architecture validation:

| Architecture | Board config | Board | Port | Upload backend |
|---|---|---|---|---|
| Cortex-M7 | `teensy41` | Teensy 4.1 + ILI9341 | serial `COM3`, Teensy upload port detected by `arduino-cli board list` | Arduino / Teensyduino |
| ESP32-S3 | `cores3` | M5Stack CoreS3 | `COM10` | Arduino |
| ESP32 | `core2` | M5Stack Core2 | `COM5` | Arduino |
| ESP32-P4 | `esp32p4` | Waveshare ESP32-P4-WIFI6 Touch LCD 4B | `COM27` | ESP-IDF |
| RP2040 | `picow` | Pico W + ILI9341 | `COM19` | Arduino-Pico |
| RP2350 | `pico2` | Pico 2 + ILI9341 | `COM21` | Arduino-Pico |

Typical validation commands:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board teensy41 --module core_raster
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cores3 --module texture_raster
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board esp32p4 --module core_raster
```

`esp32p4` uses the ESP-IDF backend instead of Arduino. The runner calls the
local ESP-IDF Python environment directly and keeps the generated `sdkconfig`
inside `tmp/benchmark3d/idf_builds/...`. The project default config selects
ESP32-P4 revisions `<3.0`, because the connected Waveshare board reports chip
revision `v1.3`.
The project also declares the 32 MB flash size reported by this board, avoiding
the default ESP-IDF 2 MB image-header warning.

`feathers2` / ESP32-S2 was compile-tested, but runtime validation is deferred
for now because the board is not part of the currently connected benchmark set.

Each run writes raw telemetry, parsed JSON, parsed CSV and a small manifest under
`tmp/benchmark3d/runs/<timestamp>_<board>_<module>/`.

Serial capture uses `pyserial`, installed in the `tgxmesh3d2` environment.
