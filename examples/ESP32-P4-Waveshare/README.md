# TGX examples for Waveshare ESP32-P4-WIFI6-Touch-LCD-4B

These examples target the Waveshare ESP32-P4 board with the 720x720
MIPI-DSI ST7703 display. They use ESP-IDF, not Arduino, because this
board is not currently supported by the Arduino ESP32 core.

The examples share the `components/tgx_p4_display` display helper and
the `components/tgx_idf` TGX component.

## Build and upload

From an ESP-IDF shell:

```powershell
. D:\Espressif\Initialize-Idf.ps1 -IdfId esp-idf-20ee62e792ea89630ac6a777ab3ebc57
cd D:\Programmation\tgx\examples\ESP32-P4-Waveshare\test-shading
idf.py build
idf.py -p COM27 flash
```

The local test board is an ESP32-P4 revision v1.3. The examples therefore
set the ESP-IDF compatibility options for pre-v3 ESP32-P4 revisions in
`sdkconfig.defaults`.

## Example families

The simple examples render into a 320x240 TGX framebuffer and copy it to
the center of the 720x720 LCD. This keeps RAM use modest and is the best
starting point when porting an Arduino/TGX sketch to ESP-IDF.

The `_large` examples render a larger virtual viewport with TGX tile
rendering, then copy each tile into a full 720x720 LCD framebuffer held
by the ESP LCD driver. This uses more CPU because scene geometry is
processed once per tile, but it demonstrates how to cover more of the
high-resolution display without allocating a full TGX framebuffer and
z-buffer.
