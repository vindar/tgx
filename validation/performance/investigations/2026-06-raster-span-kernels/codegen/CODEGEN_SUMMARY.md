# Raster Span Kernel Codegen Summary

Date: 2026-06-06

This summary covers the final `RasterSpanKernelBench` build used for the
reported results.

## Build Size

| Board | Text | Data | BSS | Decimal |
| --- | ---: | ---: | ---: | ---: |
| Pico2 / RP2350 | 351580 | 12288 | 234720 | 598588 |
| Teensy 4.1 | 25920 | 7872 | 242496 | 276288 |

Raw size output:

- `pico2_size.txt`
- `teensy41_size.txt`

## Selected Symbol Sizes

### Pico2 / RP2350

| Symbol | Size |
| --- | ---: |
| `flatNoZIndex` | 0x2c |
| `flatNoZPtr` | 0x2c |
| `flatNoZStore32` | 0x50 |
| `flatNoZUnroll4` | 0x54 |
| `zOnlyIndex` | 0x48 |
| `zOnlyPtr` | 0x50 |
| `zOnlySelectWrite` | 0x48 |
| `flatZIndex` | 0x60 |
| `flatZPtr` | 0x68 |
| `gouraudRef` | 0x8c |
| `gouraudScalar` | 0x90 |
| `texNearestZIndex` | 0x84 |
| `texNearestZPtr` | 0x90 |
| `texNearestIncFloat` | 0xb8 |
| `texNearestIncFixed8` | 0x94 |
| `texBilinearRef` | 0x5c |
| `texBilinearScalar` | 0x180 |
| `texBilinearIncFloat` | 0xb8 |
| `texBilinearIncFixed8` | 0x1b8 |
| `texModZIndex` | 0x9c |
| `texModZPtr` | 0xa4 |

### Teensy 4.1

| Symbol | Size |
| --- | ---: |
| `flatNoZIndex` | 0x5c |
| `flatNoZPtr` | 0x54 |
| `flatNoZStore32` | 0x5c |
| `flatNoZUnroll4` | 0x80 |
| `zOnlyIndex` | 0x5c |
| `zOnlyPtr` | 0x54 |
| `zOnlySelectWrite` | 0x60 |
| `flatZIndex` | 0x74 |
| `flatZPtr` | 0x68 |
| `gouraudRef` | 0xb8 |
| `gouraudScalar` | 0xb0 |
| `texNearestZIndex` | 0xc8 |
| `texNearestZPtr` | 0xc8 |
| `texNearestIncFloat` | 0xcc |
| `texNearestIncFixed8` | 0x98 |
| `texBilinearRef` | 0x1a4 |
| `texBilinearScalar` | 0x1ac |
| `texBilinearIncFloat` | 0x1e0 |
| `texBilinearIncFixed8` | 0x1c4 |
| `texModZIndex` | 0xd0 |
| `texModZPtr` | 0xb4 |

## Observations

- The strongest measured wins are not ARM DSP wins. They come from replacing
  per-pixel float texture coordinate work with fixed-point incremental texture
  coordinates in the isolated benchmark.
- `texNearestIncFixed8` is smaller than the float reference on both boards:
  0x94 vs 0xb8 on Pico2, and 0x98 vs 0xcc on Teensy.
- `texBilinearIncFixed8` is larger on Pico2 than the float reference
  (0x1b8 vs 0xb8), but is still much faster in the microbenchmark. This points
  to avoided float coordinate/floor work rather than simple code-size shrink.
- `texBilinearIncFixed8` is slightly smaller than the float reference on Teensy
  (0x1c4 vs 0x1e0) and is also faster.
- Pointer increment loops are not uniformly better. `flatZPtr` is smaller than
  `flatZIndex` on Teensy and slightly faster, but on Pico2 it is larger and only
  weakly positive. Pure z-only pointer loops regress on both boards.
- The 32-bit flat store and unroll variants are useful on Pico2 long spans, but
  have poor portability and short-span hazards. Teensy did not benefit.
- The `allpass_fast` diagnostic proves that a known-all-pass depth span can be
  much faster, but it is deliberately incorrect for mixed and all-fail spans.
  It is not an integration candidate without a cheap and correct all-pass test.

## Files

- `pico2_nm.txt`
- `pico2_objdump.txt`
- `pico2_selected_symbols.txt`
- `pico2_size.txt`
- `teensy41_nm.txt`
- `teensy41_objdump.txt`
- `teensy41_selected_symbols.txt`
- `teensy41_size.txt`
