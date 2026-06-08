# LVGL TGX Mesh Viewer

Interactive Teensy 4.1 example combining:

- LVGL 9.5 for widgets, touch input, and screen composition.
- TGX for software 3D rendering into a RGB565 memory framebuffer.
- ILI9341_T4 for partial-region upload to a 320 x 240 ILI9341 display.

The TGX viewport is intentionally reduced to 240 x 160 to leave RAM for LVGL, the ILI9341_T4 internal framebuffer, diff buffers, z-buffer, and mesh cache. The UI uses a bright turquoise/pink/yellow LVGL theme, while the TGX viewport uses a separate very light gray background. Non-textured meshes use a shiny gold material, and textured meshes use bilinear filtering with a stronger specular highlight.

## Hardware

- Teensy 4.1
- ILI9341 320 x 240 TFT
- XPT2046 touch controller on the same SPI bus

Default SPI0 wiring:

| Signal | Teensy 4.1 pin |
| --- | --- |
| SCK | 13 |
| MISO | 12 |
| MOSI | 11 |
| TFT DC | 10 |
| TFT CS | 9 |
| TFT RESET | 6 |
| Touch CS | 4 |
| Touch IRQ | 3 |
| Backlight | 255, not controlled |

## Required Libraries

- TGX from this repository
- ILI9341_T4 1.7.0
- LVGL 9.5.0
- Teensyduino / Teensy Arduino core 1.61.0

The local LVGL configuration must be enabled and set to `LV_COLOR_DEPTH 16`. The local setup used for this example has `LV_USE_CANVAS`, `LV_USE_DROPDOWN`, `LV_USE_SLIDER`, `LV_USE_SWITCH`, and Montserrat 12/14 enabled.

This example is close to the Teensy 4.1 RAM1/ITCM limit. If the default build does not fit, either compile it with `Smallest Code with LTO`, or reduce LVGL's memory pool in `lv_conf.h`:

```cpp
#define LV_MEM_SIZE (32 * 1024U)
```


## Controls

- Drag inside the TGX viewport to rotate the mesh. Drag rotations are accumulated around fixed screen/world axes so the controls do not invert when the object turns around.
- Use the Wire/Flat/Smooth dropdown to switch between antialiased wireframe, flat shading, and Gouraud shading.
- Use the zoom slider to scale the mesh.
- Use the dropdown to select Teapot, Bunny, Bob tex, Blub tex, Falcon, Donkey, Cyborg, or Spot. Teapot and Bunny render as shiny gold meshes; the other meshes render with bilinear textures.
- The lower mesh line shows the selected mesh triangle count and generated mesh memory size, excluding texture images.
- Use the Auto switch for slow yaw rotation.
- Press Reset to restore yaw, pitch, zoom, and auto-rotation.
- The top-right label shows the current FPS.

## Touch Calibration

The default build calls `tft.calibrateTouch()` at startup so the touch input can be matched to the connected screen.

To skip calibration and use the stored constants, set:

```cpp
#define TGX_LVGL_MESHVIEWER_RUN_TOUCH_CALIBRATION 0
```

The sketch prints four calibration constants to Serial. Paste them back into the stored calibration block if you want to reuse them later.

## Memory Notes

Main buffers:

- ILI9341_T4 internal framebuffer: 320 x 240 x 2 bytes.
- LVGL partial draw buffer: 320 x 40 x 2 bytes.
- TGX viewport framebuffer: 240 x 160 x 2 bytes.
- TGX z-buffer: 240 x 160 x 2 bytes.
- TGX mesh cache: 96 KB in `DMAMEM`, re-used for the selected mesh. Smaller payloads fit better in cache; larger textured meshes keep remaining payload and texture pixels mostly in PROGMEM.
- Two ILI9341_T4 diff buffers: 8 KB each.

## Troubleshooting

- No display: check SPI wiring, power, reset pin, and that the sketch uses landscape rotation.
- No touch: check `PIN_TOUCH_CS`, `PIN_TOUCH_IRQ`, and rerun calibration if coordinates are wrong.
- LVGL compile errors: verify LVGL 9.5, `lv_conf.h` in the Arduino libraries root, `LV_COLOR_DEPTH 16`, and the required widgets enabled.
