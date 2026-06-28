# Spotlight example validation preview - 2026-06-27

CPU preview:

- Built with MinGW g++ from `tmp/spotlight_examples_preview/preview.cpp`.
- Command:
  `g++ -std=c++17 -O2 -I src -I examples/CPU/buddhaOnCPU -I tmp/spotlight_examples_preview tmp/spotlight_examples_preview/preview.cpp src/Color.cpp -o tmp/spotlight_examples_preview/preview.exe`
- The generated BMP files show:
  - one moving point light on `spot`, `stormtrooper_v2`, and `donkeykong_small`;
  - one fixed spotlight whose direction sweeps across a blue/white checkerboard floor;
  - a small unlit marker for point lights and a compact unlit cone marker for the spotlight.

Compile/upload/runtime summary:

| Board | Sketch | Compile | Upload/runtime |
|---|---|---|---|
| Teensy 4.1 + ILI9341 | `examples/Teensy4/3D/pointlight_textured_meshes` | OK after `setRotation(3)` orientation fix | Re-uploaded on `usb:80000/3/0/1`, serial `COM3`, about 37-38 fps on `spot` |
| Teensy 4.1 + ILI9341 | `examples/Teensy4/3D/spotlight_checkerboard` | OK after `setRotation(3)` orientation fix | Initial upload before orientation fix was about 186-197 fps; re-upload pending |
| Core2 | `examples/M5Stack/pointlight_textured_meshes` | OK | Uploaded on `COM5`, about 8-9 fps on `spot`, DMA enabled |
| Core2 | `examples/M5Stack/spotlight_checkerboard` | OK | Not uploaded because `COM5` was not accessible |
| CoreS3 | `examples/M5Stack/pointlight_textured_meshes` | OK | Uploaded on `COM10`, about 18-19 fps, DMA enabled |
| CoreS3 | `examples/M5Stack/spotlight_checkerboard` | OK | Uploaded on `COM10`, about 42-43 fps, DMA enabled |
| Pico2 + ILI9341 | `examples/Pico_RP2040_RP2350/pointlight_textured_meshes` | OK | Uploaded on `COM21`, about 9-10 fps, DMA enabled |
| Pico2 + ILI9341 | `examples/Pico_RP2040_RP2350/spotlight_checkerboard` | OK | Uploaded on `COM21`, about 33 fps, DMA enabled |

Notes:

- Pico2 builds warn that global memory is high because the examples keep framebuffer, DMA framebuffer, and z-buffer statically allocated.
- Core2 upload initially failed once with a Windows port-open error on `COM5`, then succeeded on retry.
