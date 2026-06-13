# Baseline Comparison

Comparison between previous `validation/performance/baselines/current` and new baseline `baseline_20260613_bbdba104`.

## Benchmark 3D Global Scores

| Board | Score | Previous | New | Delta |
| ----- | ----- | -------: | --: | ----: |
| core2 | 1 | 23.31 | 26.8 | +14.972% |
| core2 | 2 | 17.49 | 19.51 | +11.549% |
| cores3 | 1 | 32.72 | 36.82 | +12.531% |
| cores3 | 2 | 24.59 | 26.71 | +8.621% |
| cores3 | 3 | 21.86 | 23.46 | +7.319% |
| pico2 | 1 | 19.78 | 18.88 | -4.550% |
| pico2 | 2 | 15.5 | 14.85 | -4.194% |
| pico2 | 3 | 13.69 | 13.16 | -3.871% |
| teensy41 | 1 | 105.37 | 106.34 | +0.921% |
| teensy41 | 2 | 80.24 | 80.81 | +0.710% |
| teensy41 | 3 | 70.66 | 70.95 | +0.410% |

## Real Example Aggregate

| Board | Example | Previous mean fps | New mean fps | Delta |
| ----- | ------- | ----------------: | -----------: | ----: |
| core2 | borg_cube | 46.4556 | 46.4246 | -0.067% |
| core2 | donkeykong | 27.0951 | 28.4694 | +5.072% |
| core2 | scream | 14.5909 | 15.1600 | +3.900% |
| cores3 | borg_cube | 49.5714 | 49.5810 | +0.019% |
| cores3 | donkeykong | 30.6921 | 32.0928 | +4.564% |
| cores3 | scream | 22.1778 | 23.2203 | +4.701% |
| pico2 | borg_cube | 31.0000 | 31.0000 | +0.000% |
| pico2 | bunny_fig | 27.0797 | 27.3563 | +1.022% |
| pico2 | scream | 25.1500 | 25.3107 | +0.639% |
| teensy41 | buddha | 29.7372 | 29.6431 | -0.316% |
| teensy41 | mars | 188.0389 | 191.7207 | +1.958% |
| teensy41 | test-shading | 81.0662 | 81.9755 | +1.122% |
| teensy41 | test-texture | 72.3896 | 71.3345 | -1.457% |

## Largest Per-Scene Improvements

| Board | Example | Scene | Previous | New | Delta |
| ----- | ------- | ----- | -------: | --: | ----: |
| pico2 | bunny_fig | gouraud_texture | 17.9986 | 19.9889 | +11.058% |
| core2 | donkeykong | gouraud_texture | 21.2714 | 23.0867 | +8.534% |
| core2 | donkeykong | gouraud | 27.9247 | 30.0111 | +7.472% |
| cores3 | donkeykong | gouraud | 32.2840 | 34.3178 | +6.300% |
| cores3 | donkeykong | gouraud_texture | 25.7610 | 27.3002 | +5.975% |
| cores3 | donkeykong | flat | 36.7413 | 38.6269 | +5.132% |
| teensy41 | mars | falcon | 37.9755 | 39.9143 | +5.105% |
| core2 | donkeykong | flat | 33.6200 | 35.2211 | +4.762% |
| cores3 | scream | unknown | 22.1778 | 23.2203 | +4.701% |
| teensy41 | test-shading | suzanne_gouraud | 58.7425 | 61.1560 | +4.109% |
| core2 | scream | unknown | 14.5909 | 15.1600 | +3.900% |
| teensy41 | test-shading | teapot_gouraud | 141.7233 | 146.3100 | +3.236% |

## Largest Per-Scene Regressions

| Board | Example | Scene | Previous | New | Delta |
| ----- | ------- | ----- | -------: | --: | ----: |
| teensy41 | test-texture | bob_tex_nearest | 60.2270 | 58.3220 | -3.163% |
| pico2 | bunny_fig | gouraud | 30.7200 | 29.8150 | -2.946% |
| teensy41 | test-texture | spot_tex_nearest | 93.0070 | 90.3910 | -2.813% |
| teensy41 | test-texture | blub_tex_nearest | 59.2800 | 57.7293 | -2.616% |
| teensy41 | test-shading | dragon_flat | 42.6867 | 41.6622 | -2.400% |
| teensy41 | test-shading | suzanne_flat | 71.3233 | 69.9278 | -1.957% |
| teensy41 | test-texture | spot_tex_bilinear | 80.0120 | 78.7065 | -1.632% |
| teensy41 | test-texture | bob_tex_bilinear | 53.4500 | 52.6527 | -1.492% |
| teensy41 | test-texture | blub_tex_bilinear | 54.9680 | 54.2947 | -1.225% |
| teensy41 | test-shading | skull_flat | 77.8350 | 76.8856 | -1.220% |
| teensy41 | test-texture | blub_gouraud | 64.9220 | 64.5647 | -0.550% |
| teensy41 | mars | beginning | 190.2067 | 189.2067 | -0.526% |

## Notes

- Core2/CoreS3 show strong gains on triangle-heavy `donkeykong` scenes and on `scream`, while `borg_cube` remains effectively unchanged.
- Pico2 benchmark global score is lower, but selected examples are neutral to positive overall; `bunny_fig/gouraud` is the main negative scene and `bunny_fig/gouraud_texture` the main positive one.
- Teensy4.1 remains close overall; `test-texture` is modestly lower, especially nearest texture rows, while `mars` and `test-shading` are mostly positive.
- Treat rows within about +/-0.5% as noise unless repeated and consistent.
