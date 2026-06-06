| Board | Mean | Median | >+1% | +/-0.5% | -0.5..-1% | <=-1% | Worst | Best |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| pico2 | -0.07% | 0.00% | 0 | 154 | 5 | 2 | -4.26% | 0.51% |
| core2 | 0.00% | 0.00% | 0 | 107 | 0 | 0 | -0.39% | 0.70% |
| cores3 | 1.44% | 0.71% | 71 | 67 | 0 | 0 | -0.48% | 12.54% |
| teensy41 | 0.96% | 0.44% | 65 | 74 | 6 | 4 | -2.41% | 5.72% |

### pico2 worst regressions
- -4.26% | score 2 | 200 x 200 (medium / balanced) | Point cloud pixels | base 2549.98 fps -> 2441.26 fps
- -3.42% | score 1 | 100 x 100 (small / vertex-heavy) | Wire cube fast | base 5912.12 fps -> 5710.21 fps
- -0.86% | score 2 | 200 x 200 (medium / balanced) | Wire cube AA | base 2036.12 fps -> 2018.67 fps
- -0.85% | score 2 | 200 x 200 (medium / balanced) | Wire cube fast | base 5511.08 fps -> 5464.48 fps
- -0.69% | score 1 | 100 x 100 (small / vertex-heavy) | Vertex color direct | base 598.04 fps -> 593.91 fps
- -0.61% | score 2 | 200 x 200 (medium / balanced) | Wire rib strip fast | base 587.60 fps -> 584.03 fps
- -0.51% | score 3 | 320 x 240 (large / ILI9341) | Point cloud pixels | base 2473.46 fps -> 2460.78 fps
- -0.41% | score 3 | 320 x 240 (large / ILI9341) | Wire cube fast | base 4862.36 fps -> 4842.62 fps
### pico2 largest gains
- +0.51% | score 2 | 200 x 200 (medium / balanced) | Wire sphere fast | base 1154.74 fps -> 1160.60 fps
- +0.46% | score 1 | 100 x 100 (small / vertex-heavy) | Point cloud pixels | base 2503.31 fps -> 2514.93 fps
- +0.30% | score 1 | 100 x 100 (small / vertex-heavy) | Ribbon strip | base 190.09 fps -> 190.65 fps
- +0.27% | score 1 | 100 x 100 (small / vertex-heavy) | Wire sphere AA | base 384.34 fps -> 385.39 fps
- +0.21% | score 2 | 200 x 200 (medium / balanced) | Bunny (farclip) | base 52.75 fps -> 52.86 fps
- +0.20% | score 2 | 200 x 200 (medium / balanced) | Large tris FLAT Z | base 310.83 fps -> 311.44 fps
- +0.20% | score 1 | 100 x 100 (small / vertex-heavy) | Cube direct FLAT | base 540.98 fps -> 542.04 fps
- +0.14% | score 3 | 320 x 240 (large / ILI9341) | Wire rib quad fast | base 537.83 fps -> 538.58 fps

### core2 worst regressions
- -0.39% | score 2 | 200 x 200 (large for this target) | Wire cube fast | base 7851.01 fps -> 7820.14 fps
- -0.29% | score 1 | 100 x 100 (small / vertex-heavy) | Wire sphere fast | base 1377.17 fps -> 1373.15 fps
- -0.10% | score 1 | 100 x 100 (small / vertex-heavy) | Cube direct FLAT | base 659.94 fps -> 659.30 fps
- -0.09% | score 1 | 100 x 100 (small / vertex-heavy) | Vertex color direct | base 703.02 fps -> 702.41 fps
- -0.08% | score 1 | 100 x 100 (small / vertex-heavy) | Point cloud pixels | base 2093.02 fps -> 2091.32 fps
- -0.07% | score 2 | 200 x 200 (large for this target) | Wire sphere fast | base 1153.85 fps -> 1153.07 fps
- -0.05% | score 1 | 100 x 100 (small / vertex-heavy) | Sphere direct TEX | base 141.86 fps -> 141.79 fps
- -0.04% | score 2 | 200 x 200 (large for this target) | R2D2 FLAT | base 26.67 fps -> 26.66 fps
### core2 largest gains
- +0.70% | score 1 | 100 x 100 (small / vertex-heavy) | Wire cube fast | base 9334.93 fps -> 9400.71 fps
- +0.10% | score 2 | 200 x 200 (large for this target) | Wire rib quad fast | base 582.28 fps -> 582.84 fps
- +0.06% | score 2 | 200 x 200 (large for this target) | Wire cube thick | base 375.68 fps -> 375.92 fps
- +0.06% | score 1 | 100 x 100 (small / vertex-heavy) | Ribbon quads | base 280.42 fps -> 280.60 fps
- +0.05% | score 2 | 200 x 200 (large for this target) | Wire sphere AA | base 297.62 fps -> 297.76 fps
- +0.05% | score 1 | 100 x 100 (small / vertex-heavy) | Wire cube AA | base 4050.64 fps -> 4052.68 fps
- +0.04% | score 1 | 100 x 100 (small / vertex-heavy) | Wire rib strip fast | base 763.48 fps -> 763.80 fps
- +0.04% | score 1 | 100 x 100 (small / vertex-heavy) | Wire rib quad fast | base 731.93 fps -> 732.20 fps

### cores3 worst regressions
- -0.48% | score 1 | 100 x 100 (small / vertex-heavy) | Large tris FLAT Z | base 1959.61 fps -> 1950.27 fps
- -0.31% | score 1 | 100 x 100 (small / vertex-heavy) | Cube direct FLAT | base 941.99 fps -> 939.11 fps
- -0.14% | score 2 | 200 x 200 (medium / balanced) | Cube direct FLAT | base 251.03 fps -> 250.67 fps
- -0.10% | score 3 | 240 x 240 (large for this target) | Cube direct FLAT | base 175.92 fps -> 175.74 fps
- -0.09% | score 2 | 200 x 200 (medium / balanced) | Large tris FLAT Z | base 515.07 fps -> 514.62 fps
- -0.07% | score 3 | 240 x 240 (large for this target) | Large tris FLAT Z | base 360.35 fps -> 360.10 fps
- -0.05% | score 1 | 100 x 100 (small / vertex-heavy) | Buddha ORTHO | base 19.37 fps -> 19.36 fps
- -0.04% | score 2 | 200 x 200 (medium / balanced) | Vertex color direct | base 268.63 fps -> 268.52 fps
### cores3 largest gains
- +12.54% | score 1 | 100 x 100 (small / vertex-heavy) | Wire rib quad fast | base 896.56 fps -> 1008.95 fps
- +10.50% | score 2 | 200 x 200 (medium / balanced) | Point cloud pixels | base 2557.73 fps -> 2826.19 fps
- +10.45% | score 1 | 100 x 100 (small / vertex-heavy) | Point cloud pixels | base 2557.00 fps -> 2824.19 fps
- +10.43% | score 2 | 200 x 200 (medium / balanced) | Wire rib quad fast | base 757.41 fps -> 836.38 fps
- +10.31% | score 3 | 240 x 240 (large for this target) | Point cloud pixels | base 2555.91 fps -> 2819.55 fps
- +9.78% | score 3 | 240 x 240 (large for this target) | Wire rib quad fast | base 716.57 fps -> 786.63 fps
- +9.30% | score 1 | 100 x 100 (small / vertex-heavy) | Wire rib tri fast | base 673.44 fps -> 736.04 fps
- +7.01% | score 2 | 200 x 200 (medium / balanced) | Wire rib tri fast | base 525.37 fps -> 562.19 fps

### teensy41 worst regressions
- -2.41% | score 1 | 100 x 100 (small / vertex-heavy) | Wire rib quad fast | base 3524.23 fps -> 3439.38 fps
- -2.16% | score 2 | 200 x 200 (medium / balanced) | Wire rib quad fast | base 3105.59 fps -> 3038.36 fps
- -1.89% | score 3 | 320 x 240 (large / ILI9341) | Wire rib quad fast | base 2914.39 fps -> 2859.19 fps
- -1.36% | score 1 | 100 x 100 (small / vertex-heavy) | Wire bunny fast | base 787.63 fps -> 776.93 fps
- -0.76% | score 2 | 200 x 200 (medium / balanced) | Wire bunny fast | base 739.17 fps -> 733.54 fps
- -0.75% | score 1 | 100 x 100 (small / vertex-heavy) | Wire sphere fast | base 7554.30 fps -> 7497.66 fps
- -0.67% | score 2 | 200 x 200 (medium / balanced) | Wire sphere fast | base 6762.47 fps -> 6717.04 fps
- -0.65% | score 2 | 200 x 200 (medium / balanced) | Wire cube fast | base 65359.48 fps -> 64935.07 fps
### teensy41 largest gains
- +5.72% | score 1 | 100 x 100 (small / vertex-heavy) | Bunny (far) | base 498.45 fps -> 526.95 fps
- +4.52% | score 1 | 100 x 100 (small / vertex-heavy) | Bunny (farclip) | base 500.85 fps -> 523.48 fps
- +4.25% | score 2 | 200 x 200 (medium / balanced) | Bunny (far) | base 408.70 fps -> 426.08 fps
- +4.22% | score 2 | 200 x 200 (medium / balanced) | Bunny (farclip) | base 424.64 fps -> 442.56 fps
- +3.80% | score 1 | 100 x 100 (small / vertex-heavy) | Buddha ORTHO | base 55.00 fps -> 57.09 fps
- +3.62% | score 2 | 200 x 200 (medium / balanced) | R2D2 (far) | base 106.24 fps -> 110.09 fps
- +3.54% | score 3 | 320 x 240 (large / ILI9341) | Bunny (farclip) | base 400.02 fps -> 414.20 fps
- +3.51% | score 1 | 100 x 100 (small / vertex-heavy) | Buddha (far) | base 63.26 fps -> 65.48 fps

