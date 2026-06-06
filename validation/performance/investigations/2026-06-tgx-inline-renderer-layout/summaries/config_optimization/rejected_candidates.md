# Rejected Configuration Candidates

All patches are saved under `tmp/config_optimization/patches/`.

| Patch | Boards tested | Verdict |
|---|---|---|
| `rp2350_pointer_off.patch` | Pico2 | Rejected. Global scores dropped from `23.04 / 16.55 / 14.01` to `20.53 / 15.11 / 13.01`. It helped some orthographic/textured synthetic paths but severely regressed Gouraud and real-mesh paths. |
| `rp2350_zdivide_no_forced_inline.patch` | Pico2 | Rejected. Global scores dropped to `19.74 / 14.71 / 12.73`; 79 subtests regressed by at least 1%. Codegen showed `Vec4<float>::zdivide<float>()` reappearing as a standalone 100-byte symbol. |
| `rp2350_global_inline_empty.patch` | Pico2 | Rejected. Global scores dropped to `19.68 / 14.90 / 12.87`; mean subtest delta was about `-8.5%`. |
| `rp2350_fast_math_off.patch` | Pico2 | Rejected. Correct isolated run `rp2350_fast_math_off_v2` dropped to `19.82 / 14.81 / 12.78`, with many severe real mesh/flat/texture regressions. |
| `invalid_contaminated_fast_math_off_diff.patch` | Pico2 | Invalid experiment. The first fast-math-off attempt touched more than the intended RP2350 branch and must not be used as evidence. |
| `zdivide_inline_teensy_esp.patch` | Core2, CoreS3, Teensy4.1 | Partially accepted only after narrowing. Good on CoreS3 and Teensy4.1, but rejected globally because Core2 dropped from `32.65 / 22.81` to `26.42 / 19.51`. |
| `shader_incremental_teensy_esp.patch` | Core2, CoreS3, Teensy4.1 | Rejected. Core2 global score improved slightly, but important subtests regressed badly; CoreS3 mean subtest delta was negative. |
| `global_inline_empty_teensy_esp.patch` | Core2, CoreS3, Teensy4.1 | Rejected. Large cross-board regressions on Core2/CoreS3; Teensy global score was misleading and hid major wireframe regressions. |
| `teensy_fast_math_off.patch` | Teensy4.1 | Rejected. Global and subtest results regressed; fast approximate math remains beneficial on Teensy4.x. |
| `teensy_fma_off.patch` | Teensy4.1 | Rejected. Weak global gain, but local wireframe losses approached 3%; not robust enough. |
| `esp_fast_math_on.patch` | Core2, CoreS3 | Rejected/no-op. Results were essentially neutral; the existing Xtensa-specific assembly paths already dominate these helpers. |
| `forced_rp2350_cpu_validation_harness.patch` | CPU validation only | Temporary validation harness only, not a product patch. It proves the RP2350 macro profile can be forced in desktop validation, but strict CPU baselines are not compatible with the approximate RP2350 math profile. |

