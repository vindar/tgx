# TGX examples for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B

These examples target the Waveshare ESP32-P4 board with the 720x720
MIPI-DSI ST7703 display. They use ESP-IDF, not Arduino, because this
board is not currently supported by the Arduino ESP32 core.

The examples share the `components/tgx_p4_display` display helper and
the `components/tgx_idf` TGX component.

## Build and upload

From an ESP-IDF shell, configure the ESP-IDF environment, then build and flash
one of the examples:

```powershell
cd <tgx-root>\examples\ESP32-P4-Waveshare\cone
idf.py build
idf.py -p <port> flash
```

The local test board is an ESP32-P4 revision v1.3. The examples therefore
set the ESP-IDF compatibility options for pre-v3 ESP32-P4 revisions in
`sdkconfig.defaults`.

## Examples

`cone` renders a non-textured cone into a tiled 640x480 TGX viewport and
lights it with two colored spotlights. It is the simplest ESP32-P4 display
example and a good starting point for ESP-IDF users.

`test-texture` uses the same tiled path. It alternates two Mesh3Dv2 models
between Gouraud-only and Gouraud textured rendering, and prints per-frame
timing for scene update, TGX tile drawing, tile copy and LCD present.
