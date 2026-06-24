# Pico2 / RP2350 TGX Config And Build-Option Investigation

Date: 2026-06-17

Repo state at start:

- Branch: `affine-texture-fixedpoint`
- HEAD: `7d3a00ee (HEAD -> affine-texture-fixedpoint) optimization`
- Pico2 port: `COM21`
- Reference FQBN: `rp2040:rp2040:rpipico2:opt=Fast`
- TFT_eSPI setup checked: `User_Setup_Select.h` selects `Setup_RP2040_RP2350_ILI9341.h`

## Scope

Goal: re-test Pico2/RP2350 compile and config options carefully, using real TGX examples rather than `benchmark3D`, and only keep changes that are robust across examples.

Examples used:

- `examples/Pico_RP2040_RP2350/bunny_fig`
- `examples/Pico_RP2040_RP2350/borg_cube`
- `examples/Pico_RP2040_RP2350/scream`

Main reference metrics:

- `bunny_fig`: frame render time in microseconds. Lower is better.
- `borg_cube` and `scream`: FPS telemetry. Higher is better.

## Guard Rails Used

- Before each source-option candidate, `git diff -- src/tgx_config.h` was checked.
- Macro-probe was used for RP2350 source candidates, confirming:
  - `PICO_RP2350=1`
  - `__ARM_ARCH_8M_MAIN__=1`
  - `ARDUINO_ARCH_RP2040=1`
  - `ARDUINO_ARCH_RP2350=0`
- Two ambiguous patch attempts were caught by `git diff` before any measurement was accepted:
  - a too-broad macro edit briefly touched CPU/Teensy blocks instead of only RP2350;
  - a too-broad restore briefly swapped `TGX_INLINE_ZDIVIDE` in CPU/RP2350.
- Those states were corrected immediately; no retained measurement was taken from the wrong block.

## Baseline

Baseline is current source, FQBN `rp2040:rp2040:rpipico2:opt=Fast`.

| Example | Scene | Metric |
| --- | --- | ---: |
| bunny_fig | flat | 17318.273916 us |
| bunny_fig | gouraud | 22810.921134 us |
| bunny_fig | gouraud_texture | 37586.830661 us |
| bunny_fig | wireframe | 8778.652873 us |
| borg_cube | unknown | 106.772727 FPS |
| scream | unknown | 31.497175 FPS |

## Candidate Decisions

For `bunny_fig`, negative frame delta is faster. For `borg_cube` and `scream`, positive FPS delta is faster.

| Candidate | Type | Result |
| --- | --- | --- |
| `opt_o1` (`opt=Optimize`) | FQBN | Rejected. Smaller binary, but slower: bunny +7.4% to +24.6% frame time. |
| `opt_o2` (`opt=Optimize2`) | FQBN | Rejected. Flat was 0.88% faster, but Gouraud/textured/wireframe were 1.17% to 1.61% slower. |
| `opt_o3` (`opt=Optimize3`) | FQBN | Rejected. Slower everywhere tested: bunny +4.16% to +10.91% frame time, borg -3.51% FPS, scream -5.70% FPS. |
| `opt_os` (`opt=Small`) | FQBN | Rejected. Much slower: bunny +8.30% to +19.42% frame time. |
| `fma1` (`TGX_USE_FMA_MATH=1`) | RP2350 config | Rejected. One tiny textured gain (-0.28%) but flat/Gouraud/wireframe slower and borg/scream slightly slower. |
| `ptr0` (`TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS=0`) | RP2350 config | Rejected. Mostly neutral/slower; borg -0.83% FPS. |
| `fastinv0` (`TGX_USE_FAST_INV_TRICK=0`) | RP2350 config | Rejected. Bunny flat improved 2.09%, but textured path was +9.12% slower, borg -3.23% FPS, scream -6.74% FPS. |
| `fastsqrt0` (`TGX_USE_FAST_SQRT_TRICK=0`) | RP2350 config | Rejected as no-effect/noise. Binary unchanged on bunny; deltas below 0.01%. |
| `fastinvsqrt0` (`TGX_USE_FAST_INV_SQRT_TRICK=0`) | RP2350 config | Rejected. Tiny mixed deltas, not robust: borg +0.14% FPS, scream -0.09% FPS, bunny mixed within about 0.3%. |
| `zdiv_empty` (`TGX_INLINE_ZDIVIDE` empty) | RP2350 config | Rejected. Bunny slower on every scene: +0.51% to +3.64% frame time, binary +576 bytes. |
| `freq200` (`freq=200,opt=Fast`) | Board overclock | Not a TGX source optimization. Worked for bunny; frame time improved 4.45% to 21.23%. |
| `freq300` (`freq=300,opt=Fast`) | Board overclock | Not a TGX source optimization. Worked for bunny; frame time improved 25.69% to 45.18%. Needs explicit overclock validation before regular use. |

## Notes On Untested/Skipped Options

- `TGX_PROGMEM_DEFAULT_CACHE_SIZE` was not benchmarked for this 3D pass. Its current uses are in `Image::blitScaledRotated*` defaults, not the measured 3D shader paths.
- `arch=riscv` was not benchmarked here. This investigation targets the ARM Cortex-M33 RP2350 path with the active FPU assumptions used by current TGX baselines.
- RTTI, exceptions, debug, USB stack, IP stack and filesystem split options were not treated as TGX renderer optimizations.

## Outcome

No TGX source/config change is accepted from this investigation.

Keep current RP2350 defaults:

```cpp
#define TGX_USE_FAST_INV_SQRT_TRICK 1
#define TGX_USE_FAST_SQRT_TRICK 1
#define TGX_USE_FAST_INV_TRICK 1
#define TGX_USE_FMA_MATH 0
#define TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS 1
#define TGX_INLINE  __attribute__((always_inline))
#define TGX_INLINE_ZDIVIDE __attribute__((always_inline))
```

Keep `rp2040:rp2040:rpipico2:opt=Fast` as the TGX Pico2 performance baseline.

Overclocking is the only large lever found in this pass, but it is a board-level choice, not a TGX code optimization. It should be documented separately if used:

- `rp2040:rp2040:rpipico2:opt=Fast,freq=200`
- `rp2040:rp2040:rpipico2:opt=Fast,freq=300`

## Artifacts

Compact summaries:

- `summary_pico2_config.csv`
- `delta_pico2_config_vs_base_fast.csv`

Generated script:

- `aggregate_pico2_config.py`

Raw build/log/telemetry files were generated under `validation/performance/{builds,logs,parsed,telemetry}` during the investigation and can be pruned once this report is accepted.
