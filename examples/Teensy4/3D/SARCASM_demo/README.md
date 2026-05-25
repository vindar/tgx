# SARCASM TGX demo

This Teensy 4.1 example recreates the display style of the S.A.R.C.A.S.M.
Rubik's cube solving robot as a self-contained TGX demo.

It keeps only the visual/audio layer:

- ILI9341_T4 display output with TGX drawing.
- Original SARCASM intro and face images.
- A TGX 3D Rubik cube animation.
- A fake solving sequence and console messages.
- Optional espeak-ng_T4 voice output over Teensy Audio I2S.

It intentionally does not include the real robot control code: no Kociemba
solver, no motors, no camera, no SD music, no LED strip and no threads.

The default wiring is the usual TGX Teensy 4 ILI9341 wiring used by the other
examples in this folder. Voice output requires an I2S amplifier/speaker. If no
external PSRAM is detected at runtime, the sketch keeps running and simply
disables espeak voice playback.

