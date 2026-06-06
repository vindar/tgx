# June 2026 ARM DSP RGB565 Investigation

This investigation tested isolated RGB565 color kernels on:

- Teensy 4.1 / Cortex-M7, `COM3`
- Pico2 / RP2350 / Cortex-M33, `COM21`

The session was microbenchmark-first. Production renderer/layout experiments were intentionally avoided.

## Result

One production source optimization was kept:

- `src/Color.h`: `meanColor(RGB565, RGB565)` now uses the exact packed RGB565 average:

```cpp
(a & b) + (((a ^ b) & 0xF7DEu) >> 1)
```

This matched current TGX output exactly and improved the isolated `meanColor2` kernel on both ARM boards.

## Directory Map

```text
SESSION_START.md
REPORT_ARM_DSP_RGB565.md
sketches/
  ArmDspProbe/
  RGB565KernelBench/
tools/
  parse_rgb565_kernel_bench.py
results/
  rgb565_kernel_results.csv
  rgb565_kernel_results_after_mean2_patch.csv
  integration_mean2_before_after.csv
  benchmark_sanity_delta.csv
  correctness_summary.csv
  hardware_dsp_probe.md
  raw_capture/
patches/
  integrate_rgb565_mean2_candidate.patch
codegen/
  CODEGEN_SUMMARY.md
```

Large Arduino build directories are intentionally not stored here; compact codegen summaries and raw telemetry are preserved instead.
