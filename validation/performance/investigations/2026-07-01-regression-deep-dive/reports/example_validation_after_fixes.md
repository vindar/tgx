# Real Example Validation After Candidate Fixes

Date: 2026-07-01

Branch state: current development tree with the candidate regression fixes applied.

Label used for serial logs: `final_fix_examples_20260701`

## Scope

The real examples comparable with the archived `main` and pre-fix development baselines were uploaded and captured one by one. Each capture was checked immediately before moving to the next example.

Boards/examples:

- Pico2: `borg_cube`, `bunny_fig`, `scream`
- Pico W: `borg_cube`, `bunny_fig`, `scream`
- Core2: `borg_cube`, `donkeykong`, `scream`
- CoreS3: `borg_cube`, `donkeykong`, `scream`
- Teensy 4.1: `buddha`, `mars`, `scream`, `test-shading`, `test-texture`

Some examples were deliberately re-run with a longer capture window when the first capture did not cover all comparable scenes or had fewer samples than desired:

- `core2/donkeykong`: first capture stopped at the wireframe scene transition; re-run with 24 s.
- `core2/borg_cube` and `cores3/borg_cube`: re-run with 16 s to match/beat the historical sample count.
- `teensy41/scream`: re-run with 18 s to cover a comparable set of explosion cycles.

## Artifacts

- Current per-scene summary:
  `validation/performance/investigations/2026-07-01-regression-deep-dive/summaries/final_candidate_example_telemetry_summary.csv`
- Current vs pre-fix dev vs main comparison:
  `validation/performance/investigations/2026-07-01-regression-deep-dive/summaries/final_candidate_example_vs_main_dev.csv`
- Raw serial logs were temporary and were pruned after the compact CSV summaries were generated.

## Result

All 29 scene rows that were comparable between `main`, pre-fix dev, and current candidate were captured. No comparable row is missing.

The original Pico2 `scream` regression is mostly recovered:

- main: 31.375 fps
- pre-fix dev: 30.375 fps
- current candidate: 31.1538 fps
- current vs pre-fix dev: +2.564%
- current vs main: -0.705%

That remaining difference is borderline and should be treated as noise/near-noise rather than a clear new regression.

Core2 real examples look healthy:

- `borg_cube`: 66.2778 fps, +0.611% vs main, +0.042% vs pre-fix dev.
- `donkeykong/gouraud_texture`: 26.2387 fps, +1.573% vs main, +1.187% vs pre-fix dev.
- `donkeykong/wireframe`: 27.1970 fps, +6.384% vs main, -0.541% vs pre-fix dev by mean.
- `scream`: 18.8571 fps, within noise vs both baselines.

CoreS3 real examples also look healthy after the longer `borg_cube` capture:

- `borg_cube`: 188.2222 fps, +0.318% vs main, +0.687% vs pre-fix dev.
- `donkeykong/gouraud_texture`: 30.5547 fps, +1.227% vs main, +1.426% vs pre-fix dev.
- `donkeykong/wireframe`: 29.7770 fps, +6.062% vs main, +0.598% vs pre-fix dev.
- `scream`: 40.3077 fps, +2.207% vs main, +1.884% vs pre-fix dev.

Teensy 4.1 remains sensitive to phase composition in the animated examples:

- `buddha` is stable and within noise.
- `test-texture` is faster than both baselines on the three historical scenes.
- `test-shading` is mostly stable or faster; `bunny_flat` has a lower mean, but its median is within noise against both baselines and the archived pre-fix baseline only had one sample for that scene.
- `mars/movie` has a lower mean because the current capture has many more `movie` samples and the scene has very uneven instantaneous FPS; the median is within noise vs both baselines.
- `teensy41/scream` after the longer capture has a mean within noise vs both baselines.

## Conclusion

The candidate fixes do not introduce a concerning regression on the real example suite. The suspicious benchmark regressions remain addressed, and the example-level validation does not reveal a new blocker.

For future comparisons, prefer the saved CSVs over raw visual reading of the serial logs, and be careful with `mars` and `scream`: their averages depend strongly on which animation phases are included in the capture.
