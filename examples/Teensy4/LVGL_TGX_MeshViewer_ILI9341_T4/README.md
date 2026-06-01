# LVGL TGX Mesh Viewer ILI9341_T4

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

## Compile

From `D:\Programmation\tgx`:

```powershell
arduino-cli compile --fqbn teensy:avr:teensy41 --libraries D:\Programmation\arduino\libraries examples\Teensy4\LVGL_TGX_MeshViewer_ILI9341_T4
```

If `arduino-cli` is not in `PATH`:

```powershell
& "C:\Users\Vindar\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn teensy:avr:teensy41 --libraries D:\Programmation\arduino\libraries examples\Teensy4\LVGL_TGX_MeshViewer_ILI9341_T4
```

## Upload

Detect the current Teensy port:

```powershell
arduino-cli board list
```

Then compile and upload, replacing the port if needed:

```powershell
arduino-cli compile --fqbn teensy:avr:teensy41 --libraries D:\Programmation\arduino\libraries --port usb:80000/3/0/1 --upload examples\Teensy4\LVGL_TGX_MeshViewer_ILI9341_T4
```

Close any serial monitor before uploading.

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

The default build does not call `tft.calibrateTouch()` because it blocks waiting for user input.

To run calibration manually, set:

```cpp
#define TGX_LVGL_MESHVIEWER_RUN_TOUCH_CALIBRATION 1
```

The sketch prints four calibration constants to Serial. Paste them back into the disabled-by-default calibration block, then set the macro back to `0`.

## Color Test

To diagnose RGB565 byte order or swapped red/blue colors, set:

```cpp
#define TGX_LVGL_MESHVIEWER_COLOR_TEST 1
```

The TGX viewport will draw red, green, and blue bars instead of rendering the mesh.

## Memory Notes

Main buffers:

- ILI9341_T4 internal framebuffer: 320 x 240 x 2 bytes.
- LVGL partial draw buffer: 320 x 40 x 2 bytes.
- TGX viewport framebuffer: 240 x 160 x 2 bytes.
- TGX z-buffer: 240 x 160 x 2 bytes.
- TGX mesh cache: 96 KB in `DMAMEM`, re-used for the selected mesh. Smaller payloads fit better in cache; larger textured meshes keep remaining payload and texture pixels mostly in PROGMEM.
- Two ILI9341_T4 diff buffers: 8 KB each.

The example caches only the selected mesh. If the cache is too small, `tgx::cacheMesh()` still returns a usable mesh pointer with the data that did not fit left in PROGMEM.

## Troubleshooting

- No display: check SPI wiring, power, reset pin, and that the sketch uses landscape rotation.
- No touch: check `PIN_TOUCH_CS`, `PIN_TOUCH_IRQ`, and run calibration manually if coordinates are wrong.
- Swapped colors: enable `TGX_LVGL_MESHVIEWER_COLOR_TEST` and verify the red, green, and blue bars.
- LVGL compile errors: verify LVGL 9.5, `lv_conf.h` in the Arduino libraries root, `LV_COLOR_DEPTH 16`, and the required widgets enabled.
- Upload problems: run `arduino-cli board list`, close serial monitors, reconnect the Teensy, or press the program button.
- Low FPS: use the Teapot, Bunny, Donkey, Cyborg, or Spot mesh, reduce `VIEW_W`/`VIEW_H`, reduce mesh cache pressure, or disable Serial render-time prints. Textured Bob, Blub, and Falcon are heavier.

## Known Limitations

- One ILI9341 display only.
- Reduced TGX viewport, not full-screen 3D.
- Single-touch pointer input.
- Includes local copies of all mesh and texture headers used by the sketch: Teapot, Bunny, Bob, Blub, Falcon, Donkey, Cyborg, and Spot. The sketch does not include mesh assets from another example directory.
- Visual validation still requires checking the connected hardware after upload.
