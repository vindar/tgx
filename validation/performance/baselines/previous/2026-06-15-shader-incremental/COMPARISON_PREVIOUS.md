# Comparison With Previous Reference

This file compares the promoted 2026-06-15 shader-incremental baseline against the clean `HEAD` measurements captured in the same investigation before applying the `src/Shaders.h` candidate.

The older baseline that used to live in `validation/performance/baselines/current/` was archived here:

```text
validation/performance/baselines/previous/2026-06-14-before-shader-incremental/
```

## Benchmark Global Scores

| Board | Score 1 | Score 2 | Score 3 |
| ----- | ------: | ------: | ------: |
| Teensy 4.1 | -0.44% | +2.17% | +3.83% |
| Pico2 | +2.53% | +4.85% | +6.03% |
| Core2 | +2.33% | +5.73% | |
| CoreS3 | +2.04% | +4.74% | +5.83% |

Full data:

- `comparison_previous_benchmark_global.csv`

## Selected Example Deltas

| Board | Example / scene | Delta |
| ----- | --------------- | ----: |
| Teensy 4.1 | `buddha / buddha_rotation` | -5.18% frame time |
| Teensy 4.1 | `test-texture / spot_tex_nearest` | -7.35% frame time |
| Teensy 4.1 | `mars / movie` | +1.31% frame time |
| Pico2 | `bunny_fig / gouraud` | -10.30% frame time |
| Pico2 | `bunny_fig / gouraud_texture` | -18.84% frame time |
| Pico2 | `scream` | +20.47% FPS |
| Core2 | `donkeykong / gouraud` | -8.24% frame time |
| Core2 | `donkeykong / gouraud_texture` | -9.29% frame time |
| Core2 | `scream` | +11.37% FPS |
| CoreS3 | `donkeykong / gouraud` | -7.55% frame time |
| CoreS3 | `donkeykong / gouraud_texture` | -9.21% frame time |
| CoreS3 | `scream` | +14.35% FPS |

Full data:

- `comparison_previous_example_summary.csv`
- `comparison_previous_example_aggregate.csv`

## RP2040 / Pico W Note

Pico W was checked separately because it has no FPU. It should not use the same incremental-float defaults as Pico2/RP2350.

Best measured Pico W choice:

```cpp
TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=0
TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=0
```

Full data:

- `picow_rp2040_flag_matrix_delta_vs_head.csv`
- `picow_rp2040_flag_matrix_best_variant.csv`
