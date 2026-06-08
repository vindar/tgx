# ARM DSP / RGB565 Microbenchmark Report

## A. Executive Summary

Final recommendation: keep one small source optimization in `src/Color.h`.

The accepted patch replaces `meanColor(RGB565, RGB565)` with the exact packed RGB565 truncating average:

```cpp
(a & b) + (((a ^ b) & 0xF7DEu) >> 1)
```

This is not an approximate color change. It matches current TGX output exactly and avoids per-channel bitfield extraction/repacking.

Boards tested:

| Board | Port | Result |
| --- | --- | --- |
| Teensy 4.1 / Cortex-M7 | `COM3` | Probe OK, microbench OK, TGX sanity benchmark OK |
| Pico2 / RP2350 / Cortex-M33 | `COM21` | Probe OK, microbench OK, TGX sanity benchmark OK |

Best accepted gains:

| Kernel | Teensy 4.1 | Pico2 |
| --- | ---: | ---: |
| `meanColor2 current_tgx` after patch vs before | +66.64% | +83.12% |
| `span_mean2 current_tgx_loop`, length 16 aligned | +35.38% | +47.30% |
| `span_mean2 current_tgx_loop`, length 320 aligned | +38.30% | +53.21% |

No other production source optimization was kept. The investigated CMSIS/DSP intrinsic variants were either incorrect for exact RGB565 channel semantics, board-specific, or slower than the packed scalar formulation.

## B. Hardware And Tooling

Reusable capture tooling came from `validation/performance/tools/`, especially `upload_and_capture.ps1`.

The session artifacts are under:

```text
validation/performance/investigations/2026-06-arm-dsp-rgb565/
```

Important files:

| File | Purpose |
| --- | --- |
| `sketches/ArmDspProbe/ArmDspProbe.ino` | ARM DSP capability probe |
| `sketches/RGB565KernelBench/` | Display-free RGB565 kernel microbenchmark |
| `tools/parse_rgb565_kernel_bench.py` | Parser and speedup summarizer |
| `results/rgb565_kernel_results.csv` | Final full matrix rows |
| `results/summary.csv` | Final speedup summary |
| `results/correctness_summary.csv` | Exactness/mismatch table |
| `results/integration_mean2_before_after.csv` | Before/after accepted patch comparison |
| `results/benchmark_sanity_delta.csv` | TGX benchmark sanity deltas |
| `codegen/CODEGEN_SUMMARY.md` | Symbol/disassembly notes |
| `patches/integrate_rgb565_mean2_candidate.patch` | Accepted source patch |

The raw upload/capture logs and telemetry were archived in:

```text
results/raw_capture/
```

Large Arduino build directories were intentionally not preserved.

## C. Source / Kernel Inventory

Inspected source files:

- `src/Color.h`
- `src/Color.inl`
- `src/Misc.h`
- `src/tgx_config.h`

Main TGX functions inspected:

- `RGB565::blend256(const RGB565&, uint32_t)`
- `meanColor(RGB565, RGB565)`
- `meanColor(RGB565, RGB565, RGB565, RGB565)`
- `interpolateColorsTriangle(const RGB565&, int32_t, const RGB565&, int32_t, const RGB565&, int32_t)`
- `interpolateColorsBilinear(const RGB565&, const RGB565&, const RGB565&, const RGB565&, float, float)`
- `RGB565(const RGBf&)`
- `RGB565::operator=(const RGBf&)`

The microbenchmark also tested span kernels because packed or DSP code often pays off only across arrays:

- blend span, constant alpha
- blend span, variable alpha
- mean2 span
- bilinear span
- triangle interpolation span
- RGB565 fill/copy spans

Span lengths tested:

```text
1, 2, 4, 8, 16, 32, 64, 128, 320
```

Both aligned and offset-by-one output/input cases were covered where relevant.

## D. Correctness Results

Final correctness CSV: `results/correctness_summary.csv`.

Summary:

| Rows | Invalid rows |
| ---: | ---: |
| 690 | 8 |

Invalid rows were expected rejected variants:

| Variant | Invalid rows | Total mismatches | Reason |
| --- | ---: | ---: | --- |
| `arm_uhadd16_naive` | 2 | 774 | Halfword averaging crosses RGB565 channel boundaries |
| `arm_uhadd16_pairwise_naive` | 2 | 974 | Same issue for four-color mean |
| `pairwise_mean2` | 2 | 356 | Changes four-color mean rounding semantics |
| `two_stage_approx` | 2 | 828 | Approximate bilinear interpolation, not TGX exact |

All integration-relevant variants had zero mismatches. The accepted `meanColor2` patch preserved checksums:

| Board | Kernel | Before checksum | After checksum |
| --- | --- | --- | --- |
| Teensy 4.1 | `meanColor2 current_tgx` | `0x6947B40C` | `0x6947B40C` |
| Pico2 | `meanColor2 current_tgx` | `0x6947B40C` | `0x6947B40C` |

CPU validation after the source patch:

| Validation | Result |
| --- | --- |
| CPU 2D | OK, log in `results/validation_2d_cpu.log` |
| CPU 3D strict | `82 PASS, 0 FAIL`, log in `results/validation_3d_cpu_strict.log` |

## E. Performance Results

Final microbenchmark matrix:

- `results/rgb565_kernel_results.csv`
- `results/summary.csv`
- board-specific splits in `results/teensy41_results.csv` and `results/pico2_results.csv`

Accepted before/after patch measurements from `results/integration_mean2_before_after.csv`:

| Board | Kernel | Before time us | After time us | Delta |
| --- | --- | ---: | ---: | ---: |
| Teensy 4.1 | `meanColor2 current_tgx` | 6564 | 3939 | +66.64% |
| Pico2 | `meanColor2 current_tgx` | 57923 | 31631 | +83.12% |
| Teensy 4.1 | `span_mean2_n16_o0 current_tgx_loop` | 264 | 195 | +35.38% |
| Pico2 | `span_mean2_n16_o0 current_tgx_loop` | 2538 | 1723 | +47.30% |
| Teensy 4.1 | `span_mean2_n64_o0 current_tgx_loop` | 1001 | 727 | +37.69% |
| Pico2 | `span_mean2_n64_o0 current_tgx_loop` | 9606 | 6317 | +52.07% |
| Teensy 4.1 | `span_mean2_n320_o0 current_tgx_loop` | 4933 | 3567 | +38.30% |
| Pico2 | `span_mean2_n320_o0 current_tgx_loop` | 47312 | 30880 | +53.21% |

Other exact variants observed:

| Variant | Teensy 4.1 | Pico2 | Decision |
| --- | ---: | ---: | --- |
| `meanColor4 packed_lane_sum` | regression in earlier run, exact | about +89% in final matrix | Rejected as generic due Teensy behavior |
| `interpolateBilinear scalar_channels` | slower / mixed | small direct-kernel gain | Rejected, not robust enough |
| `RGBf->RGB565 scalar_pack` | significant Teensy regression | small Pico2 gain | Rejected |
| `copy32_two_pixels` / `fill32_two_pixels` spans | useful in isolated spans | useful in isolated spans | Not integrated; outside requested TGX color-kernel source path and needs real Image/span audit |

`blend256` conclusion:

- The current packed implementation is already strong.
- Scalar channel formulations were exact but much slower.
- No DSP variant improved it safely.

## F. Codegen Analysis

Detailed notes: `codegen/CODEGEN_SUMMARY.md`.

Pico2 symbols before accepted patch:

```text
10003284 0000002e t refMean2(unsigned short, unsigned short)
100034b8 00000014 t mean2Packed(unsigned short, unsigned short)
```

Pico2 symbols after accepted patch:

```text
10003284 00000014 t refMean2(unsigned short, unsigned short)
1000349c 00000014 t mean2Packed(unsigned short, unsigned short)
```

The old implementation extracted and repacked all three channels. The new one compiles to the compact mask/xor/shift/add sequence:

```text
movw   r3, #31727
eor.w  r2, r0, r1
and.w  r3, r3, r2, lsr #1
ands   r0, r1
add    r0, r3
uxth   r0, r0
bx     lr
```

Microbenchmark sketch size before/after accepted patch:

| Board | Text before | Text after | Delta |
| --- | ---: | ---: | ---: |
| Teensy 4.1 | 59712 | 58688 | -1024 |
| Pico2 | 567372 | 567340 | -32 |

The DSP probe confirmed both target boards expose ARM DSP/SIMD32 features, but direct halfword averaging intrinsics do not match RGB565 channel boundaries. The exact scalar packed formula was faster, smaller, and simpler than trying to repair naive `UHADD16` results.

## G. Integration Decision

Accepted source patch:

```text
src/Color.h
```

Changed function:

```cpp
inline RGB565 meanColor(RGB565 colA, RGB565 colB)
```

Patch:

```text
patches/integrate_rgb565_mean2_candidate.patch
```

Why accepted:

- Exact output match against TGX-compatible reference.
- Large isolated speedup on both ARM boards.
- Smaller generated code.
- No public API change.
- Very small and readable source diff.
- CPU 2D and strict CPU 3D validation passed.
- TGX benchmark sanity showed no hidden renderer regressions.

## H. TGX Benchmark Sanity

Sanity benchmark was run only after the accepted source patch, as requested.

Global scores:

| Board | Score 1 | Score 2 | Score 3 |
| --- | ---: | ---: | ---: |
| Pico2 | 23.04 | 16.55 | 14.01 |
| Teensy 4.1 | 126.01 | 92.74 | 80.55 |

Detailed subtest delta summary from `results/benchmark_sanity_delta.csv`:

| Board | Subtests | Mean delta | Worst delta | Best delta | Regressions below -3% |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pico2 | 162 | +0.032% | -1.364% | +3.855% | 0 |
| Teensy 4.1 | 162 | +0.022% | -0.816% | +1.189% | 0 |

Conclusion: the accepted `meanColor2` patch does not introduce hidden benchmark regressions on the two ARM boards tested.

## I. Final Source Diff

Source diff:

```text
src/Color.h | 4 +---
1 file changed, 1 insertion(+), 3 deletions(-)
```

Changed tracked file:

```text
src/Color.h
```

`git diff --check` result:

- no whitespace errors;
- Windows warning only: `src/Color.h` LF will be replaced by CRLF the next time Git touches it.

## J. Rejected Candidates

| Candidate | Reason rejected |
| --- | --- |
| Naive `UHADD16` mean2 | Incorrect RGB565 output |
| Naive `UHADD16` mean4 | Incorrect RGB565 output |
| Pairwise `meanColor2` for mean4 | Exactness failure due rounding semantics |
| `meanColor4 packed_lane_sum` | Excellent Pico2 result but not robust enough for generic integration after Teensy regression in the pre-patch run |
| Scalar-channel `blend256` | Exact but much slower than current packed implementation |
| Alternative `blend256` diff formulation | Exact but much slower |
| `interpolateBilinear scalar_channels` | Exact but mixed board/span behavior |
| Two-stage bilinear approximation | Incorrect output |
| `RGBf->RGB565 scalar_pack` | Small Pico2 gain but significant Teensy regression |
| 32-bit copy/fill spans | Promising isolated span result, but not integrated because it requires a separate Image/span audit |

## K. Next Steps

Recommended follow-up sessions:

1. RGB565 Image/span microbench focused specifically on copy/fill/blit loops and real Image call sites.
2. ESP32-S3 SIMD RGB565 microbench, separate from this ARM DSP session.
3. Rasterizer span microbench with isolated TGX inner loops rather than whole-renderer layout perturbations.
4. A separate exact `meanColor4` investigation with board-specific codegen and real call-site profiling before any integration.

