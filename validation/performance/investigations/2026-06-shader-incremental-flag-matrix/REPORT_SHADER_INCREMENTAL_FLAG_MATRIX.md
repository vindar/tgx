# Shader incremental flag matrix report

Date: 2026-06-15

## Executive summary

This pass rebuilt the current `HEAD` benchmark/example baseline after the example telemetry changes, then compared it with the current `src/Shaders.h` optimization candidate and all four combinations of:

- `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL`
- `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL`

All requested runs completed successfully:

- 4 boards.
- 5 variants: `head`, `cand_00`, `cand_01`, `cand_10`, `cand_11`.
- 85 total benchmark/example uploads and captures.
- 0 failed captures in the final manifest.

Recommendation: keep the candidate with both flags enabled by default, i.e. `cand_11`.

This is the best overall choice on Pico2, Core2, and CoreS3, and it is also a good compromise on Teensy 4.1. On Teensy, `mars/movie` regressed by +1.31% in this run, but the texture and Gouraud examples gained clearly, and the benchmark score2/score3 improved.

## Variant names

| Variant | Candidate source | Texture incremental | RGB565/Gouraud incremental |
| --- | --- | ---: | ---: |
| `head` | clean committed `HEAD` | n/a | n/a |
| `cand_00` | candidate | 0 | 0 |
| `cand_01` | candidate | 0 | 1 |
| `cand_10` | candidate | 1 | 0 |
| `cand_11` | candidate default | 1 | 1 |

The original candidate was saved before the run as:

```text
stash@{0}: On main: shader optimization candidate before flag matrix
```

The candidate is currently applied in the working tree.

## Boards and build profiles

| Board | Port | Build profile |
| --- | --- | --- |
| Teensy 4.1 + ILI9341 | `COM3` | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` |
| Pico2 / RP2350 + ILI9341 | `COM21` | `rp2040:rp2040:rpipico2:opt=Fast` |
| Core2 / ESP32 | `COM5` | `esp32:esp32:m5stack_core2` |
| CoreS3 / ESP32-S3 | `COM10` | `esp32:esp32:m5stack_cores3` |

## Test matrix

3D benchmark:

- `examples/benchmark` on all four boards.

Examples:

- Teensy 4.1: `mars`, `test-shading`, `test-texture`, `buddha`.
- Pico2: `borg_cube`, `bunny_fig`, `scream`.
- Core2/CoreS3: `borg_cube`, `donkeykong`, `scream`.

Example telemetry parsing used `frame_avg_us` when available. Older examples that only emit FPS were compared with `fps_avg`.

## Benchmark global scores

`cand_11` vs `head`:

| Board | Score 1 | Score 2 | Score 3 |
| --- | ---: | ---: | ---: |
| Teensy 4.1 | -0.44% | +2.17% | +3.83% |
| Pico2 | +2.53% | +4.85% | +6.03% |
| Core2 | +2.33% | +5.73% | n/a |
| CoreS3 | +2.04% | +4.74% | +5.83% |

The benchmark activates broad shader combinations, so the example telemetry is the more useful decision signal for this candidate.

## Example results for `cand_11`

Key rows vs `head`:

| Board | Example / scene | Result |
| --- | --- | ---: |
| Teensy 4.1 | `buddha / buddha_rotation` | -5.18% frame time |
| Teensy 4.1 | `test-shading / bunny_gouraud` | -6.85% |
| Teensy 4.1 | `test-shading / teapot_gouraud` | -7.75% |
| Teensy 4.1 | `test-texture / spot_tex_nearest` | -7.35% |
| Teensy 4.1 | `test-texture / spot_tex_bilinear` | -5.11% |
| Teensy 4.1 | `mars / movie` | +1.31% frame time |
| Pico2 | `borg_cube` | +1.02% FPS |
| Pico2 | `bunny_fig / gouraud` | -10.30% frame time |
| Pico2 | `bunny_fig / gouraud_texture` | -18.84% |
| Pico2 | `scream` | +20.47% FPS |
| Core2 | `borg_cube` | +1.31% FPS |
| Core2 | `donkeykong / gouraud` | -8.24% frame time |
| Core2 | `donkeykong / gouraud_texture` | -9.29% |
| Core2 | `scream` | +11.37% FPS |
| CoreS3 | `borg_cube` | +1.03% FPS |
| CoreS3 | `donkeykong / flat` | -4.24% frame time |
| CoreS3 | `donkeykong / gouraud` | -7.55% |
| CoreS3 | `donkeykong / gouraud_texture` | -9.21% |
| CoreS3 | `scream` | +14.35% FPS |

`mars/movie` on Teensy is the only notable negative row in the selected examples for the full `cand_11` default. It is small compared with the repeated useful gains elsewhere, but it should be watched if Mars is used as a strict regression guard.

## Flag separation

The flags behave as intended:

- `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=1` drives the non-textured Gouraud gains.
- `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=1` drives the textured Gouraud gains.
- Enabling both combines the useful effects with little or no interference in most rows.

Representative rows:

| Board | Scene | `cand_00` | `cand_01` | `cand_10` | `cand_11` |
| --- | --- | ---: | ---: | ---: | ---: |
| Pico2 `bunny_fig` | Gouraud | -2.26% | -10.35% | -2.27% | -10.30% |
| Pico2 `bunny_fig` | Gouraud texture | -2.40% | -3.43% | -18.46% | -18.84% |
| CoreS3 `donkeykong` | Gouraud | -2.21% | -7.51% | -2.33% | -7.55% |
| CoreS3 `donkeykong` | Gouraud texture | -2.99% | -2.91% | -9.43% | -9.21% |
| Core2 `donkeykong` | Gouraud | -1.39% | -8.25% | -1.41% | -8.24% |
| Core2 `donkeykong` | Gouraud texture | -2.22% | -2.28% | -9.30% | -9.29% |
| Teensy `buddha` | Rotation | -1.93% | -5.18% | -1.93% | -5.18% |

`cand_10` is very slightly better than `cand_11` on some isolated textured rows, but the difference is small while `cand_11` also keeps the non-textured Gouraud improvement.

## Per-board recommendation

| Board | Recommended | Reason |
| --- | --- | --- |
| Teensy 4.1 | `cand_11` | Gouraud and texture examples improve; watch `mars/movie` +1.31%. |
| Pico2 | `cand_11` | Strongest overall result; especially `bunny_fig` and `scream`. |
| Core2 | `cand_11` | Best balanced choice across `donkeykong` and `scream`. |
| CoreS3 | `cand_11` | Best balanced choice; large useful gains and no meaningful guard regression in selected examples. |

No board needs a per-board disable based on this matrix.

## Retained result files

Compact retained results:

- `results/run_manifest.csv`
- `results/benchmark_global_scores.csv`
- `results/benchmark_subtests.csv`
- `results/benchmark_global_delta_vs_head.csv`
- `results/candidate_11_benchmark_global_delta.csv`
- `results/example_telemetry.csv`
- `results/example_summary.csv`
- `results/example_delta_vs_head.csv`
- `results/candidate_11_example_delta.csv`
- `results/example_best_variant.csv`
- `results/flag_recommendation.csv`
- `results/binary_size.csv`

Reusable tools for this matrix:

- `tools/run_matrix_variant.ps1`
- `tools/aggregate_matrix_results.py`

The raw build/log/parsed/telemetry directories created during the run were temporary and can be regenerated by the tools. The compact CSVs above are the retained evidence.

## Source state

Current production source change:

- `src/Shaders.h`

No benchmark/example source files were modified for this run.

`git diff --check` currently reports trailing whitespace in `src/Shaders.h`. This report does not clean it automatically because the user is actively editing this candidate.

## Conclusion

The current candidate is worth keeping for review with both flags enabled by default. The results are not just benchmark-only: the selected real examples show meaningful gains on the main optimization targets, especially Pico2 and CoreS3.

If accepted, the `cand_11` values in this investigation should become the new performance baseline for the updated example telemetry.
