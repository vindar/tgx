# RGB565 Kernel Codegen Summary

## Toolchains

- Teensy 4.1: `teensy-compile 11.3.1` ARM GCC tools.
- Pico2 / RP2350: `rp2040/tools/pqt-gcc/4.1.0-1aec55e` ARM GCC tools.

## Binary Size

RGB565 microbenchmark sketch before and after the accepted `meanColor2` patch:

| Board | Text before | Text after | Delta |
| --- | ---: | ---: | ---: |
| Teensy 4.1 | 59712 | 58688 | -1024 |
| Pico2 / RP2350 | 567372 | 567340 | -32 |

Full size files:

- `teensy41_rgb565_bench_size.txt`
- `teensy41_rgb565_bench_after_mean2_size.txt`
- `pico2_rgb565_bench_size.txt`
- `pico2_rgb565_bench_after_mean2_size.txt`

## Pico2 MeanColor2

Before the patch:

```text
10003284 0000002e t refMean2(unsigned short, unsigned short)
100034b8 00000014 t mean2Packed(unsigned short, unsigned short)
```

After the patch:

```text
10003284 00000014 t refMean2(unsigned short, unsigned short)
1000349c 00000014 t mean2Packed(unsigned short, unsigned short)
```

The patched TGX `meanColor2` compiles to the same compact 20-byte sequence as the packed helper.

Relevant pre-patch disassembly:

```text
mean2Packed:
  movw   r3, #31727
  eor.w  r2, r0, r1
  and.w  r3, r3, r2, lsr #1
  ands   r0, r1
  add    r0, r3
  uxth   r0, r0
  bx     lr

refMean2 before:
  ubfx   r3, r0, #5, #6
  ubfx   r2, r1, #5, #6
  ...
  orr.w  r0, r3, r1, lsl #11
  uxth   r0, r0
```

The old implementation extracted all three bitfield channels, summed them separately, then repacked. The new implementation uses:

```cpp
(a & b) + (((a ^ b) & 0xF7DEu) >> 1)
```

This preserves RGB565 truncating average semantics while avoiding cross-channel carries.

## Rejected Codegen Paths

- Naive `UHADD16` on full RGB565 halfwords is fast but incorrect because RGB565 channel boundaries do not match the halfword average semantics.
- Pairwise `UHADD16` for four-color mean is also incorrect.
- Packed `meanColor4` is exact and very fast on Pico2, but it regressed Teensy in the microbenchmark; it was not accepted as a generic patch.
- Scalar-channel bilinear was exact and faster on Pico2 in the direct kernel, but slower on Teensy and less clean in span tests; it was not accepted.
