# Codegen Summary

Artifacts were extracted from the full-length run5 TextureCoordEquivalenceBench builds.

## Binary Size

```text
Teensy 4.1
   text	   data	    bss	    dec	    hex	filename
  28992	   8896	  22944	  60832	   eda0	D:\Programmation\tgx\validation\performance\builds\teensy41_texcoord_eq_teensy41_run5_full_lengths\TextureCoordEquivalenceBench.ino.elf

Pico2 / RP2350
   text	   data	    bss	    dec	    hex	filename
 569492	  12288	  15152	 596932	  91bc4	D:\Programmation\tgx\validation\performance\builds\pico2_texcoord_eq_pico2_run5_full_lengths\TextureCoordEquivalenceBench.ino.elf
```

## Relevant Symbols

```text
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:142:00000068 00000096 t bilinearFixedWeights(unsigned short, unsigned short, unsigned short, unsigned short, int, int)
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:154:0000050c 000000e4 t renderCandidateInterior(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:173:00000380 0000018c t fixedStartAndStep(TextureCase const&, CoordCase const&, unsigned long, int, long&, long&, long&, long&)
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:177:000005f0 00000208 t renderCandidate(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:179:00000100 00000280 t renderReference(TextureCase const&, CoordCase const&, SampleMode, unsigned long, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\teensy41_nm.txt:188:000007f8 000008ac t runOne(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, bool) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:541:100032b8 00000092 t bilinearFixedWeights(unsigned short, unsigned short, unsigned short, unsigned short, int, int)
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:554:100036b4 000000b8 t renderCandidateInterior(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:569:100035d8 000000dc t fixedStartAndStep(TextureCase const&, CoordCase const&, unsigned long, int, long&, long&, long&, long&)
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:594:1000376c 000001b4 t renderCandidate(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:598:100033fc 000001dc t renderReference(TextureCase const&, CoordCase const&, SampleMode, unsigned long, unsigned short*) [clone .constprop.0]
D:\Programmation\tgx\validation\performance\investigations\2026-06-texture-coord-fixedpoint\codegen\pico2_nm.txt:615:10003924 0000065c t runOne(TextureCase const&, CoordCase const&, SampleMode, unsigned long, int, bool)
```

## Observations

- `renderReference(...)` remains a distinct symbol in both builds and models the production-like float path with per-pixel `floor`/truncation and float arithmetic.
- `renderCandidate(...)` is the general fixed path with edge/clamp/wrap handling. It was not accepted because the general path still pays edge/floor-equivalent handling costs and is not consistently faster.
- `renderCandidateInterior(...)` is much smaller than both reference and general fixed path: 0xe4 bytes on Teensy and 0xb8 bytes on Pico2. This matches the measured result: it is excellent for affine interior spans once setup is amortized.
- `bilinearFixedWeights(...)` is a small integer helper (0x96 bytes Teensy, 0x92 bytes Pico2). It uses integer multiply/shift/recombine work rather than per-pixel float bilinear weight conversion.
- Pico2 disassembly references show many `vcvt.*`, `vmul.f32`, and `vadd.*` instructions in the float/reference and setup paths. The fixed interior loop uses integer shifts/index arithmetic for coordinate progression. This supports the measured speedup mechanism: removed per-pixel float coordinate conversion/floor work, not a layout-only effect.
- The benchmark still contains setup functions such as `fixedStartAndStep(...)` using float conversion to initialize fixed coordinates. This setup is why very short spans regress; the full-length data shows `LEN=1` is bad, while `LEN>=4` is already positive and `LEN>=8` is very strong.

## Integration Implication

A production patch should target only `USE_TEXTURE && USE_ORTHO` scanlines whose whole span is safely interior, and it should avoid very short spans. Perspective paths should remain on the current float implementation.
