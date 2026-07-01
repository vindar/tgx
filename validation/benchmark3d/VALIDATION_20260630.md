# TGX benchmark3d validation - 2026-06-30

Scope:

- JSON board/module configs.
- CPU, Arduino and ESP-IDF module selection.
- `core_raster`, `texture_raster`, `lighting_shading`, `primitives`, `mesh`
  and `wire_points` modules.
- Serial telemetry parsing and manifests.

Tooling:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board <board> --module <module>
```

Validated runs:

| Board config | Runtime board | Module | Status | Run directory |
|---|---|---|---|---|
| `cpu` | `cpu` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_200035_cpu_core_raster` |
| `cpu` | `cpu` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_200055_cpu_texture_raster` |
| `teensy41` | `teensy4` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_202014_teensy41_core_raster` |
| `teensy41` | `teensy4` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_200506_teensy41_texture_raster` |
| `core2` | `esp32` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_203409_core2_core_raster` |
| `core2` | `esp32` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_203506_core2_texture_raster` |
| `cores3` | `esp32s3` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_200857_cores3_core_raster` |
| `cores3` | `esp32s3` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_201001_cores3_texture_raster` |
| `picow` | `rp2040` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_201246_picow_core_raster` |
| `picow` | `rp2040` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_201329_picow_texture_raster` |
| `pico2` | `rp2350` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_201415_pico2_core_raster` |
| `pico2` | `rp2350` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_201506_pico2_texture_raster` |
| `esp32p4` | `esp32p4` | `core_raster` | OK, 15/15 | `tmp/benchmark3d/runs/20260630_211605_esp32p4_core_raster` |
| `esp32p4` | `esp32p4` | `texture_raster` | OK, 7/7 | `tmp/benchmark3d/runs/20260630_211723_esp32p4_texture_raster` |

Additional CPU validation after adding `lighting_shading`, `primitives` and
batch orchestration:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module lighting_shading,primitives
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module all
```

Results:

- `tmp/benchmark3d/runs/20260630_214421_batch`: CPU batch,
  `lighting_shading` 13/13 OK and `primitives` 15/15 OK.
- `tmp/benchmark3d/runs/20260630_214528_batch`: CPU batch, all four
  modules OK.

CPU visual validation:

```powershell
.\tmp\benchmark3d\bench3d_cpu_lighting_shading.exe --module lighting_shading --visual --visual-out tmp\benchmark3d\visual\lighting_shading_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_primitives.exe --module primitives --visual --visual-out tmp\benchmark3d\visual\primitives_mosaic.bmp
```

This generated one 4x5 mosaic per sub-test. Representative mosaics were checked
for moving point lights, soft spotlights, textured truncated cones and thick
generated primitive wireframes.

Connected-board batch after adding the new modules:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board all-connected --module lighting_shading,primitives
```

Batch directory:

- `tmp/benchmark3d/runs/20260630_214704_batch`

Summary from `batch_results.csv`:

| Board config | Module | Status | Tests | FPS min | FPS avg | FPS max |
|---|---|---:|---:|---:|---:|---:|
| `teensy41` | `lighting_shading` | OK | 13 | 497.83 | 764.76 | 1167.22 |
| `teensy41` | `primitives` | OK | 15 | 944.58 | 3227.42 | 7462.50 |
| `cores3` | `lighting_shading` | OK | 13 | 124.64 | 179.54 | 237.36 |
| `cores3` | `primitives` | OK | 15 | 117.79 | 538.56 | 1017.48 |
| `core2` | `lighting_shading` | OK | 13 | 101.15 | 148.36 | 196.90 |
| `core2` | `primitives` | OK | 15 | 79.27 | 415.65 | 943.82 |
| `esp32p4` | `lighting_shading` | OK | 13 | 139.82 | 210.33 | 335.66 |
| `esp32p4` | `primitives` | OK | 15 | 349.85 | 988.09 | 2264.63 |
| `picow` | `lighting_shading` | OK | 13 | 13.98 | 24.97 | 37.74 |
| `picow` | `primitives` | OK | 15 | 28.53 | 102.07 | 353.95 |
| `pico2` | `lighting_shading` | OK | 13 | 110.50 | 164.68 | 240.17 |
| `pico2` | `primitives` | OK | 15 | 61.43 | 501.62 | 1556.00 |

Additional CPU validation after adding `mesh` and `wire_points`:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module mesh
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module wire_points
```

Results:

- `tmp/benchmark3d/runs_mesh_cpu_probe/20260630_222357_cpu_mesh`:
  `mesh` 13/13 OK.
- `tmp/benchmark3d/runs_wire_cpu_probe/20260630_222447_cpu_wire_points`:
  `wire_points` 17/17 OK.
- `tmp/benchmark3d/runs_cpu_all_after_mesh_wire/20260630_223802_batch`:
  full CPU batch, all six configured modules OK.

CPU visual validation for the new modules:

```powershell
.\tmp\benchmark3d\bench3d_cpu_mesh.exe --module mesh --visual-out tmp\benchmark3d\visual\mesh_mosaic.bmp
.\tmp\benchmark3d\bench3d_cpu_wire_points.exe --module wire_points --visual-out tmp\benchmark3d\visual\wire_points_mosaic.bmp
```

This generated one 4x5 mosaic per sub-test. Representative mosaics were checked
for Mesh3Dv2 textured wrapping, near-plane clipping, face culling, thick
wireframe batches, thick Mesh3Dv2 wireframe and variable projected dots.

Connected-board batch after adding `mesh` and `wire_points`:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board all-connected --module mesh,wire_points --out tmp\benchmark3d\runs_mesh_wire_all_connected --stop-on-error
```

Batch directory:

- `tmp/benchmark3d/runs_mesh_wire_all_connected/20260630_222540_batch`

Summary from `batch_results.csv`:

| Board config | Module | Status | Tests | Slowest test | FPS min | FPS avg | FPS max |
|---|---|---:|---:|---|---:|---:|---:|
| `teensy41` | `mesh` | OK | 13 | `mesh.v2.texture.bilinear.clamp` | 638.11 | 21273.07 | 250312.89 |
| `teensy41` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 156.89 | 11523.95 | 62344.14 |
| `cores3` | `mesh` | OK | 13 | `mesh.v2.near_clip.gouraud` | 143.92 | 3235.14 | 37485.94 |
| `cores3` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 29.98 | 957.24 | 2000.20 |
| `core2` | `mesh` | OK | 13 | `mesh.v2.near_clip.gouraud` | 94.90 | 2222.94 | 25905.62 |
| `core2` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 33.83 | 830.09 | 1757.14 |
| `esp32p4` | `mesh` | OK | 13 | `mesh.v2.texture.bilinear.clamp` | 177.80 | 3561.07 | 37483.60 |
| `esp32p4` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 48.73 | 3398.52 | 18264.84 |
| `picow` | `mesh` | OK | 13 | `mesh.v2.texture.bilinear.clamp` | 17.20 | 249.84 | 2586.55 |
| `picow` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 15.12 | 360.20 | 1417.04 |
| `pico2` | `mesh` | OK | 13 | `mesh.v2.texture.bilinear.clamp` | 138.25 | 4541.01 | 54461.29 |
| `pico2` | `wire_points` | OK | 17 | `wire.quads.batch.thick` | 6.32 | 2459.96 | 11321.82 |

ESP32P4 status:

- Board: Waveshare ESP32-P4-WIFI6 Touch LCD 4B on `COM27`.
- Build backend: ESP-IDF v5.5.4, target `esp32p4`.
- The board reports chip revision `v1.3`; the benchmark project therefore uses
  `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y` and `CONFIG_ESP32P4_REV_MIN_100=y`.
- The first flash attempt failed when the default IDF image required chip
  revisions `v3.1` to `v3.99`; rebuilding with the explicit rev `<3.0` config
  fixed this cleanly without `--force`.
- `CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384` is required for the clipping-heavy
  `core_raster` tests; the default 3584-byte IDF main stack caused a stack
  protection fault in `near_clip`.
- `CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y` is set for this board, matching the
  physical flash detected at boot and removing the default 2 MB image-header
  warning.

FeatherS2 status, deferred:

- `feathers2` / `esp32:esp32:adafruit_feather_esp32s2_tft` on `COM11`
  compiles for both modules.
- Upload/runtime validation is blocked because `COM11` cannot be opened by
  either esptool or pyserial:
  `PermissionError(13, 'Un peripherique attache au systeme ne fonctionne pas correctement.', None, 31)`.
- `COM12` was not present during this retry.
- The board has been unplugged and ESP32-S2 runtime support is deferred for now.

FeatherS2 compile-only checks:

```powershell
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2_tft --libraries D:\Programmation\arduino\libraries --libraries D:\Programmation\tgx\validation\benchmark3d\arduino --build-property compiler.cpp.extra_flags=-DTGX_BENCH_MODULE_CORE_RASTER=1 validation\benchmark3d\sketches\core_raster
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2_tft --libraries D:\Programmation\arduino\libraries --libraries D:\Programmation\tgx\validation\benchmark3d\arduino --build-property compiler.cpp.extra_flags=-DTGX_BENCH_MODULE_TEXTURE_RASTER=1 validation\benchmark3d\sketches\texture_raster
```

Both compile-only checks succeeded.

Important fixes made during validation:

- Arduino wrapper was made header-only so each module sketch controls which
  benchmark module is compiled.
- ESP32 backend uses `heap_caps_malloc` for benchmark allocations instead of
  large static `.bss` arenas.
- RP2040/RP2350 serial startup delay was increased to keep telemetry capture
  from missing `TGXNB3D_BEGIN`.
- Build-size parsing handles Arduino logs in English and French.
- ESP-IDF backend added for ESP32P4, using direct IDF Python/tool paths,
  build-local `sdkconfig`, `heap_caps_malloc` allocation and IDF build-size
  parsing.

Additional validation after adding `skybox_noz`, `integrated` and gap-fill
tests:

New/expanded coverage:

- `core_raster`: 20 tests. Added direct per-vertex color paths, orthographic
  mode, explicit frustum/custom projection paths, far/near/viewport clipping
  variants and degenerate triangle coverage.
- `texture_raster`: 8 tests. Added textured cube/per-face texture coverage.
- `lighting_shading`: 15 tests. Added ambient-only and material setter paths.
- `primitives`: 18 tests. Added adaptive sphere, one-cap cylinder and textured
  cone cap coverage.
- `mesh`: 15 tests. Added partial viewport and far-plane mesh cases.
- `skybox_noz`: 6 tests. New module for skybox/no-zbuffer background paths.
- `integrated`: 5 tests. New module for realistic mixed scenes.
- `wire_points`: already covered the expected line/triangle/strip/quad,
  wireframe mesh, antialiasing, thickness, point and dot paths; no new tests
  were needed in this pass.

CPU validation:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module core_raster,texture_raster,lighting_shading,primitives,mesh,skybox_noz,integrated --out tmp\benchmark3d\runs_gapfill_probe
```

Result: all seven modules OK. CPU visual mosaics were generated under
`tmp/benchmark3d/visual_gapfill/` and representative mosaics were inspected
for the new orthographic/projection paths, textured cube faces, primitive cap
textures, mesh clipping, skybox backgrounds and integrated spotlight/pointlight
scenes.

Connected-board validation:

| Board config | Batch/run directory | Status |
|---|---|---|
| `teensy41` | `tmp/benchmark3d/runs_gapfill_mcu/20260630_234747_batch`, plus `20260630_235353_batch` and `20260630_235545_teensy41_integrated` after memory reductions | all seven modules OK |
| `core2` | `tmp/benchmark3d/runs_gapfill_mcu/20260630_235628_batch` | all seven modules OK |
| `cores3` | `tmp/benchmark3d/runs_gapfill_mcu/20260701_000242_batch` | all seven modules OK |
| `picow` | `tmp/benchmark3d/runs_gapfill_mcu/20260701_001018_batch` | all seven modules OK |
| `pico2` | `tmp/benchmark3d/runs_gapfill_mcu/20260701_001458_batch` | all seven modules OK |
| `esp32p4` | `tmp/benchmark3d/runs_gapfill_mcu/20260701_002011_batch` | all seven modules OK |

Memory fixes made during this pass:

- `skybox_noz` uses 64x64 skybox textures on CPU/full profiles and 48x48
  textures on MCU-sized profiles.
- `integrated` uses 48x48 primary textures and 32x32 skybox textures so the
  module fits the 128 KB MCU bulk arena.

ESP32P4 watchdog note:

- The first full ESP32P4 batch completed successfully but printed an IDF task
  watchdog warning during the long `core_raster` sequence because the benchmark
  did not yield between tests.
- `benchYield()` was added to each backend and is called only outside the timed
  render section.
- Rerun after the fix:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board esp32p4 --module core_raster --out tmp\benchmark3d\runs_gapfill_after_yield --stop-on-error
```

Result: `tmp/benchmark3d/runs_gapfill_after_yield/20260701_003215_esp32p4_core_raster`
completed 20/20 OK with no watchdog warning.

Additional post-yield checks:

- CPU `core_raster`, `skybox_noz`, `integrated`:
  `tmp/benchmark3d/runs_gapfill_after_yield/20260701_003022_batch`, 3/3 OK.
- Core2 `core_raster`:
  `tmp/benchmark3d/runs_gapfill_after_yield/20260701_003116_core2_core_raster`,
  20/20 OK.

Additional validation after adding `mixed_stress`:

New coverage:

- `mixed_stress`: 5 tests. Added large mixed Renderer3D scenes intended to
  stress codegen/layout effects: repeated shader switching in one renderer,
  zbuffer/no-zbuffer switching, flat/Gouraud/unlit paths, textured nearest,
  bilinear, clamp/wrap and affine texture paths, skybox, generated cube/sphere/
  cylinder/truncated-cone primitives, legacy mesh drawing, raw triangle lists,
  triangle strips, wireframe mesh, vertex-color triangle, clipping and point
  lights.

CPU validation:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board cpu --module mixed_stress --out tmp\benchmark3d\runs_mixed_stress --stop-on-error
```

Result:

- `tmp/benchmark3d/runs_mixed_stress/20260701_020032_cpu_mixed_stress`:
  `mixed_stress` 5/5 OK.

CPU visual validation:

```powershell
tmp\benchmark3d\bench3d_cpu_mixed_stress.exe --module mixed_stress --visual-out tmp\benchmark3d\visual_mixed_stress\mixed_stress_mosaic.bmp
```

This generated one 4x5 mosaic per sub-test under
`tmp/benchmark3d/visual_mixed_stress/`. The mosaics were inspected for the
layered full scene, shader ping-pong state changes, geometry kitchen-sink mix,
local-light churn and near-plane clipping/no-z overlay case.

Connected-board validation:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --board all-connected --module mixed_stress --out tmp\benchmark3d\runs_mixed_stress_mcu --stop-on-error
```

Batch directory:

- `tmp/benchmark3d/runs_mixed_stress_mcu/20260701_020116_batch`

Summary from `batch_results.csv`:

| Board config | Status | Tests | Slowest test | FPS min | FPS avg | FPS max |
|---|---|---:|---|---:|---:|---:|
| `teensy41` | OK | 5 | `mixed_stress.geometry_kitchen_sink` | 137.29 | 772.07 | 2540.16 |
| `cores3` | OK | 5 | `mixed_stress.geometry_kitchen_sink` | 25.60 | 98.92 | 247.03 |
| `core2` | OK | 5 | `mixed_stress.geometry_kitchen_sink` | 24.53 | 81.02 | 209.12 |
| `esp32p4` | OK | 5 | `mixed_stress.geometry_kitchen_sink` | 34.98 | 217.05 | 701.77 |
| `picow` | OK | 5 | `mixed_stress.layered_full_scene` | 6.98 | 18.63 | 57.15 |
| `pico2` | OK | 5 | `mixed_stress.geometry_kitchen_sink` | 5.45 | 72.53 | 208.01 |
