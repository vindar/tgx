# TGX ARM DSP RGB565 Hot 3D Investigation

This investigation continued the ARM DSP / RGB565 microbenchmark work after the
accepted `meanColor(RGB565, RGB565)` packed-average optimization.

The focus was deliberately narrowed to color kernels that are actually used by
the 3D renderer and shaders:

- `interpolateColorsBilinear(RGB565, ...)` in bilinear textured shader paths.
- `interpolateColorsTriangle(RGB565, ...)` in untextured Gouraud paths.
- `RGB565::mult256(...)` in texture x lighting / gradient modulation.
- `RGBf -> RGB565` only as a lower-priority setup/vertex conversion path.

Runtime targets:

- Teensy 4.1 / Cortex-M7 on `COM3`.
- Pico2 / RP2350 / Cortex-M33 on `COM21`.

The final decision is that no additional production source patch is kept from
this investigation. The tested exact source candidates did not produce a
meaningful transferable speedup, and the fastest new packed bilinear candidate
was not bit-exact.

Useful files:

- `SESSION_START.md`: start state and scope.
- `REPORT_ARM_DSP_RGB565_HOT3D.md`: final report.
- `results/color_callsite_inventory.csv`: source call-site inventory.
- `notes/color_callsite_map.md`: priority map for color kernels in 3D paths.
- `results/hot3d_kernel_results.csv`: baseline Teensy/Pico2 microbench results.
- `results/hot3d_candidate_summary.csv`: selected candidate comparison rows.
- `results/correctness_summary_all_runs.csv`: all invalid variants observed.
- `patches/`: rejected experimental patches.
- `codegen/CODEGEN_SUMMARY.md`: symbol-size and codegen notes.
