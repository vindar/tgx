# Codegen summary

This pass did not pursue architecture-specific codegen tricks. The retained source
change is a portable C/C++ row-copy path in `src/Image.inl`.

## Source-level change

In `_jpegdec_color_convert565`, the opaque fully in-bounds RGB565 path now chooses:

- row-wise `memcpy` when `iWidth >= 16` and `iWidth` is a multiple of 8;
- the original per-pixel copy loop for narrow or partial-width blocks;
- the original `drawPixel<true>` fallback for clipping/opacity cases.

The selected guard avoids measured short-span regressions while keeping common JPEG
block/full-row widths eligible for row copy.

## Instruction-level expectation

The inner per-pixel assignment loop is removed only for the selected path. Generated
code is expected to call or inline the platform C library `memcpy` according to each
toolchain's normal policy. No `dsps_memcpy`, DMA, SIMD, or Xtensa/ARM-specific code
is introduced.

## Build-size observations from validation sketches

The copy microbench was compiled before and after the source patch on all four boards.
Those sketch sizes include benchmark helper code and the extra `production` benchmark
variant, so they are not clean library-only code-size deltas. They were used only as
compile sanity. The production source diff itself is small:

```text
src/Image.inl | 18 +++++++++++++++---
```

## Risk notes

- The fallback loop remains present, so code size increases slightly.
- The threshold prevents pathological small-width `memcpy` calls measured in the
  microbench.
- The change is isolated to JPEGDEC RGB565 opaque/in-bounds decode, not PNG/GIF or
  renderer paths.
