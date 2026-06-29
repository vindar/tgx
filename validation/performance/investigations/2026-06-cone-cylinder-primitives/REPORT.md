# Cone/Cylinder Primitive Investigation

Date: 2026-06-29

## Scope

Implemented `Renderer3D` public primitives for cylinders, cones, and truncated cones:

- `drawCylinder()`
- `drawCone()`
- `drawTruncatedCone()`

The primitives are generated in model space around the Y axis, use the current renderer shader/material/light state, and support optional side/bottom/top textures. Caps are enabled by default. If a non-degenerate cap is omitted, culling is disabled locally for the primitive so the inside remains visible, then the previous renderer culling state is restored.

## Geometry And Mapping

- Lateral surface: quads for cylinder/truncated cone, triangles for true cones.
- Bottom/top caps: triangle fans.
- Side texture coordinates: `u` wraps around the primitive, `v = 0` at bottom and `v = 1` at top.
- Cap texture coordinates: radial projection into the full texture image, center at `(0.5, 0.5)`.

## CPU Visual Validation

CPU visual suite:

```powershell
g++ -std=c++17 -O2 -I . -I src -I examples\CPU\buddhaOnCPU validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_visual_suite.cpp src\Color.cpp src\Renderer3D.cpp -o validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_visual_suite.exe
.\validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_visual_suite.exe validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\final
```

The suite covers closed/open caps, flat and Gouraud shading, textured sides/caps, cone/truncated/inverted shapes, low-sector faceting, directional light, and point light.

Final hashes are bit-identical to the original primitive implementation:

```text
cylinder_gouraud_closed      0xed8a6946
cylinder_open_top_culling    0x92d9dc0d
cylinder_textured_all        0x903ac1d4
cylinder_side_texture_open   0x801799bb
cone_flat_closed             0xddee4797
cone_gouraud_textured        0x4a9f70fd
cone_open_bottom_culling     0xff7d109f
truncated_textured_all       0xcc087463
inverted_truncated_textured  0x985ef433
low_sector_faceted_caps      0x4415ed0a
```

Reference images are kept in:

```text
validation/performance/investigations/2026-06-cone-cylinder-primitives/cpu/current/
validation/performance/investigations/2026-06-cone-cylinder-primitives/cpu/final/
```

## CPU Clipping Validation

Clipping visual suite:

```powershell
g++ -std=c++17 -O2 -I . -I src -I examples\CPU\buddhaOnCPU validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_clipping_suite.cpp src\Color.cpp src\Renderer3D.cpp -o validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_clipping_suite.exe
.\validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\cone_cylinder_clipping_suite.exe validation\performance\investigations\2026-06-cone-cylinder-primitives\cpu\clipping
```

The suite covers cylinder/cone/truncated-cone clipping against the near plane and the viewport sides, using textured solid primitives and the fast/AA/thick wireframe overloads.

Near-plane cases disable backface culling deliberately: when the near plane cuts into a closed object, the remaining visible part can be the inside of the volume, so regular backface culling can hide it even if clipping itself is correct. Side-clipping cases keep culling enabled.

The committed artifacts keep only the compact overview image and the CSV hash summary:

```text
validation/performance/investigations/2026-06-cone-cylinder-primitives/cpu/clipping/overview.png
validation/performance/investigations/2026-06-cone-cylinder-primitives/cpu/clipping/summary.csv
```

## Optimization Pass

Tested candidates:

- `opt_skip_flat_normals`: avoid normal work for flat/unlit paths. Rejected: CPU was identical, but hardware performance was inconsistent and code size increased.
- `opt_gouraud_cached`: cache transformed/projected/shaded vertices for Gouraud side strips and caps. Good gains on cylinders/truncated cones, but true cones still had some regressions on ESP32-S3.
- `opt_gouraud_cached_cone`: add a dedicated true-cone Gouraud fan. It improved true cones on most boards, but produced larger code and clear Pico2 regressions.
- `opt_gouraud_cached_shared`: final accepted version. It keeps the Gouraud strip/cap/cone fan paths, but factors the shared per-triangle culling/projection/clipping/shading/rasterize code into one `TGX_NOINLINE` helper to reduce code duplication.
- `opt_gouraud_cached_shared_slots`: replace `Prev = Cur` vertex copies with two alternating slots. Rejected: despite better Gouraud numbers, it caused large Pico2 regressions in fallback/open/flat cases, which points to codegen/layout sensitivity rather than a robust algorithmic win.

Final accepted optimization:

- Active only for Gouraud generated cones/cylinders/truncated cones when local culling is enabled.
- Reuses already transformed/projected/shaded vertices across adjacent triangles.
- Uses the existing generic `_drawTriangle()` fallback for flat paths and open shapes where culling is locally disabled.
- Keeps CPU output bit-identical.

## Hardware Results

Validation sketch:

```text
validation/performance/investigations/2026-06-cone-cylinder-primitives/sketches/ConeCylinderPrimitiveBench
```

Final accepted comparison is saved in:

```text
validation/performance/investigations/2026-06-cone-cylinder-primitives/hardware/opt_gouraud_cached_shared_vs_initial.csv
```

FPS delta versus the simple initial implementation:

| Board | cone gouraud tex+point | cylinder gouraud | cylinder textured | truncated gouraud tex+point | open side textured | truncated flat textured |
|---|---:|---:|---:|---:|---:|---:|
| Teensy 4.1 | +5.95% | +9.02% | +6.66% | +15.24% | +0.08% | -0.70% |
| Core2 | +1.13% | +5.47% | +4.64% | +8.90% | +0.48% | +0.21% |
| CoreS3 | +1.50% | +9.54% | +8.92% | +21.12% | +1.63% | -3.13% |
| Pico2 | -2.79% | +25.32% | +31.40% | +41.39% | -1.61% | +3.84% |

Notes:

- The largest robust wins are the Gouraud cylinder/truncated-cone paths, especially on Pico2.
- Open-side and flat cases mostly remain near baseline because they intentionally use the fallback path.
- The remaining Pico2 true-cone delta is small and much better than the rejected duplicated cone-fan variant.
- The CoreS3 flat regression is a code-layout side effect; the flat path is not algorithmically changed.

## Code Size

Final accepted size versus the initial simple implementation:

| Board | Initial code/sketch | Final code/sketch | Delta |
|---|---:|---:|---:|
| Teensy 4.1 | FLASH code 71944 | FLASH code 83592 | +11648 |
| Core2 | sketch 343349 | sketch 350757 | +7408 |
| CoreS3 | sketch 345323 | sketch 352783 | +7460 |
| Pico2 | sketch 116080 | sketch 129768 | +13688 |

Final RAM/global figures:

| Board | Globals/RAM code |
|---|---:|
| Teensy 4.1 | RAM1 code 81072, variables 95808 |
| Core2 | globals 113884 |
| CoreS3 | globals 113600 |
| Pico2 | globals 100308 |

## Final Compile And Upload Checks

Final compile commands:

```powershell
& "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn teensy:avr:teensy41:usb=serial,speed=600,opt=o3std --libraries D:\Programmation\arduino\libraries --library D:\Programmation\tgx --build-path D:\Programmation\tgx\validation\performance\builds\cone_final_shared_teensy41 validation\performance\investigations\2026-06-cone-cylinder-primitives\sketches\ConeCylinderPrimitiveBench
& "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn esp32:esp32:m5stack_core2 --libraries D:\Programmation\arduino\libraries --library D:\Programmation\tgx --build-path D:\Programmation\tgx\validation\performance\builds\cone_final_shared_core2 validation\performance\investigations\2026-06-cone-cylinder-primitives\sketches\ConeCylinderPrimitiveBench
& "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn esp32:esp32:m5stack_cores3 --libraries D:\Programmation\arduino\libraries --library D:\Programmation\tgx --build-path D:\Programmation\tgx\validation\performance\builds\cone_final_shared_cores3 validation\performance\investigations\2026-06-cone-cylinder-primitives\sketches\ConeCylinderPrimitiveBench
& "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn rp2040:rp2040:rpipico2:opt=Fast --libraries D:\Programmation\arduino\libraries --library D:\Programmation\tgx --build-path D:\Programmation\tgx\validation\performance\builds\cone_final_shared_pico2 validation\performance\investigations\2026-06-cone-cylinder-primitives\sketches\ConeCylinderPrimitiveBench
```

Final Teensy upload command:

```powershell
& validation\performance\tools\upload_and_capture.ps1 -Board teensy41 -Sketch validation\performance\investigations\2026-06-cone-cylinder-primitives\sketches\ConeCylinderPrimitiveBench -Label cone_cylinder_final_shared_teensy41 -ParseMode example -Baud 115200 -TimeoutSeconds 30 -LineRegex "\[TGX telemetry\]" -MinTelemetryLines 12 -RetryCount 0
```

Result: compile and upload succeeded. The Teensy serial capture produced stable telemetry lines for the final accepted source. Pico2 emits known lwIP warnings from the board core; the sketch still compiles successfully.
