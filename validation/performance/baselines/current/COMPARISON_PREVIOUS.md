# Baseline Comparison Against Previous Current

New baseline: `baseline_20260614_current` at commit `0f42085f76cf9cdf5a2a87289bce31c1ceb7e9fd` with the working-tree diff recorded in `source_diff.patch`.

## Benchmark Global Scores

| Board | Score | Previous | New | Delta |
| ----- | ----- | -------: | --: | ----: |
| core2 | 1 | 26.8 | 30.97 | +15.560% |
| core2 | 2 | 19.51 | 21.29 | +9.124% |
| cores3 | 1 | 36.82 | 41.59 | +12.955% |
| cores3 | 2 | 26.71 | 28.71 | +7.488% |
| cores3 | 3 | 23.46 | 24.89 | +6.095% |
| pico2 | 1 | 18.88 | 20.97 | +11.070% |
| pico2 | 2 | 14.85 | 15.89 | +7.003% |
| pico2 | 3 | 13.16 | 13.92 | +5.775% |
| teensy41 | 1 | 106.34 | 116.42 | +9.479% |
| teensy41 | 2 | 80.81 | 85.47 | +5.767% |
| teensy41 | 3 | 70.95 | 74.33 | +4.764% |

## Example Aggregate Mean FPS

| Board | Example | Previous | New | Delta |
| ----- | ------- | -------: | --: | ----: |
| core2 | borg_cube | 46.4246 | 46.4144 | -0.022% |
| core2 | donkeykong | 28.451 | 27.832 | -2.176% |
| core2 | scream | 15.16 | 15.3636 | +1.343% |
| cores3 | borg_cube | 49.581 | 49.6278 | +0.094% |
| cores3 | donkeykong | 32.0311 | 32.4788 | +1.398% |
| cores3 | scream | 23.2203 | 23.5337 | +1.350% |
| pico2 | borg_cube | 31 | 30.9944 | -0.018% |
| pico2 | bunny_fig | 27.6614 | 27.773 | +0.403% |
| pico2 | scream | 25.3107 | 25.7416 | +1.702% |
| teensy41 | buddha | 29.6431 | 29.681 | +0.128% |
| teensy41 | mars | 62.3685 | 61.9863 | -0.613% |
| teensy41 | test-shading | 80.9711 | 83.4098 | +3.012% |
| teensy41 | test-texture | 73.5801 | 77.4054 | +5.199% |

Detailed per-scene deltas are in `comparison_previous_example_summary.csv`.
