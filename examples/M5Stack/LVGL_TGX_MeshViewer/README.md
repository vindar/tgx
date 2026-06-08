# LVGL TGX Mesh Viewer

Interactive M5Stack Core2/CoreS3 example combining:

- LVGL 9.5 for widgets, touch input, and screen composition.
- TGX for software 3D rendering into a RGB565 memory framebuffer.
- LovyanGFX for display upload and touch input on the built-in M5Stack screen.

The TGX viewport is reduced to 220 x 150 to leave RAM for LVGL, the z-buffer, and the selected mesh cache. Large buffers are allocated at startup and prefer PSRAM when available.

## Hardware

- M5Stack Core2 or M5Stack CoreS3.
- Built-in 320 x 240 display and touch controller.

LovyanGFX auto-detection is enabled with `LGFX_AUTODETECT`, matching the style used by the other M5Stack TGX examples.

## Required Libraries

- TGX from this repository.
- LovyanGFX.
- LVGL 9.5.0.
- ESP32 Arduino core with the M5Stack board definitions.

The local LVGL configuration must use `LV_COLOR_DEPTH 16` and have canvas, dropdown, slider, switch, button, and Montserrat 12/14 fonts enabled.


## Controls

- Drag inside the TGX viewport to rotate the mesh.
- Use the Wire/Flat/Smooth dropdown to switch between antialiased wireframe, flat shading, and Gouraud shading.
- Use the zoom slider to scale the mesh.
- Use the mesh dropdown to select Teapot, Bunny, Bob tex, Blub tex, Falcon, Donkey, Cyborg, or Spot.
- Use the Auto switch for slow yaw rotation.
- Press Reset to restore yaw, pitch, zoom, and auto-rotation.

## Memory Notes

Main buffers:

- LVGL partial draw buffer: 320 x 24 x 2 bytes.
- TGX viewport framebuffer: 220 x 150 x 2 bytes.
- TGX z-buffer: 220 x 150 x 2 bytes.
- TGX mesh cache: 96 KB, re-used for the selected mesh.


## Troubleshooting

- No display: verify the selected board is Core2/CoreS3 and that LovyanGFX auto-detection is enabled.
- No touch: check that `lcd.getTouch()` works for the board and that the display rotation is landscape.
- LVGL compile errors: verify LVGL 9.5, `lv_conf.h` in the Arduino libraries root, `LV_COLOR_DEPTH 16`, and the required widgets enabled.
- With ESP32 Arduino core 4.0.0-alpha1 and LVGL 9.5, this local setup also needed LVGL's ARM Helium assembly file to be skipped on non-ARM targets and `LV_ATTRIBUTE_LARGE_CONST` to be left empty instead of forcing a `.progmem` section. If you see `lv_blend_helium.S` assembler errors or `.progmem` orphan-section linker errors, check the local LVGL configuration/source.
