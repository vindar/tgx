# Teensy 4.1 FLASHMEM / Inline Investigation

Date: 2026-06-28

Branch: `feature/point-spot-lights`

Board: Teensy 4.1 + ILI9341

FQBN: `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std`

## Goal

Evaluate whether TGX 3D code should be moved out of Teensy 4.1 ITCM with
`FLASHMEM`, or whether selected inlining policy changes can reduce RAM1 pressure
without slowing the real 3D examples.

## Hardware / Toolchain Facts Used

- Teensy 4.1 uses an i.MX RT1062 Cortex-M7 at 600 MHz, with 1024 KB RAM, 512 KB
  tightly-coupled memory, and 32 KB instruction + 32 KB data caches.
- NXP's i.MX RT notes describe ITCM/DTCM as fastest for CPU access, with QSPI
  flash performance depending heavily on cache/prefetch and access pattern.
- Local Teensy core defines `FLASHMEM` as
  `__attribute__((section(".flashmem")))`.
- Local Teensy linker script places `*(.flashmem*)` in `.text.code` in flash,
  and `*(.text*)` in `.text.itcm`.
- The linker computes the FlexRAM ITCM bank count from `.text.itcm`; reducing
  ITCM by crossing a 32 KB bank boundary can free a whole extra DTCM bank.

External references:

- PJRC Teensy 4.1 page: https://www.pjrc.com/store/teensy41.html
- NXP AN12437, i.MX RT Series Performance Optimization:
  https://www.nxp.com/docs/en/application-note/AN12437.pdf
- NXP AN12042, Using the i.MXRT L1 Cache:
  https://www.nxp.com/docs/en/application-note/AN12042.pdf

Local references:

- `C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\hardware\avr\1.61.0\cores\teensy4\avr\pgmspace.h`
- `C:\Users\Vindar\AppData\Local\Arduino15\packages\teensy\hardware\avr\1.61.0\cores\teensy4\imxrt1062_t41.ld`

## Variants Tested

1. `TGX_NOINLINE = FLASHMEM`
   - Intended to move non-inline helpers into flash.
   - No useful size effect on TGX 3D templates.

2. `TGX_NOINLINE = FLASHMEM __attribute__((noinline))`
   - Forces out-of-line calls, but template instantiations still land in ITCM.
   - Increased code/RAM1, rejected.

3. Global `-DTGX_INLINE=`
   - Removes TGX global forced-inline policy.
   - Saves a lot of RAM1 and makes `mars` compile.
   - Rejected because real `test-shading` wireframe AA paths regress badly.

4. Coarse granular 3D-only override:
   - `-DTGX_RENDERER3D_INLINE= -DTGX_SHADER_INLINE= -DTGX_RASTERIZER_INLINE=`
   - Keeps non-3D helpers untouched.
   - Saves RAM1 in 3D examples while preserving or improving measured FPS.
   - Rejected as a source-level shape because the macros are still too broad:
     they also cover small arithmetic and shader helper functions that should
     keep the normal `TGX_INLINE` policy.

5. `mars` wrapper functions marked `FLASHMEM`
   - Tried on two sketch-level `TGX_NOINLINE` wrappers.
   - Did not reduce the failing ITCM size; rejected.

## Key Codegen Finding

`FLASHMEM` works for ordinary functions but not for the TGX-style function
template instantiations we need to move.

Probe result:

```text
60001618 T plain_prefix(int)
00000070 W int templ_prefix<int>(int)
60001624 T plain_decl_suffix(int)
0000007c W int templ_decl_suffix<int>(int)
```

The object sections explain why:

```text
.flashmem                         ordinary FLASHMEM functions
.text._Z12templ_prefixIiEiT_      template instantiation
.text._Z17templ_decl_suffixIiEiT_ template instantiation
```

Therefore, a simple `FLASHMEM` annotation is not a practical way to move
Renderer3D/rasterizer/shader template code out of ITCM.

Follow-up probe:

- An explicit specialization, `template<> FLASHMEM int f<int>(...)`, is emitted
  in flash.
- A normal `FLASHMEM` wrapper can also contain code from an inlined template.
- A custom linker script can force selected template instantiation sections,
  such as `.text._Z19flash_decl_template*`, into `.text.code` before the default
  `*(.text*)` rule.

This proves that template code can be placed in flash, but not with plain
`FLASHMEM` on the generic template definition. Doing it for TGX would require
either explicit specializations/wrappers or a custom Teensy linker script that
matches selected mangled template sections.

## Benchmark Result

Current official Teensy 4.1 baseline:

```text
118.55 / 90.37 / 80.65 fps
```

Granular 3D-inline override:

```text
119.24 / 90.76 / 80.96 fps
```

The default benchmark sketch no longer fits after recent code growth:

```text
default:  RAM1 code 262496, padding 32416, free -6720  -> upload failed
granular: RAM1 code 246048, padding 16096, free 26048 -> success
```

## Synthetic Sphere Bench

The sphere synthetic bench produced identical hashes across tested cases.

Granular 3D-only override:

- Average runtime delta: about `+1.37%`
- Worst observed case: about `-1.86%`
- Large RAM1 code reduction on the synthetic build:
  `101488 -> 58288` bytes

This suggests the visual output is unchanged and the speed impact is at worst
within normal noise on this synthetic workload.

## Real Examples

Real examples are the main decision criterion.

Granular override versus default, measured on Teensy 4.1:

- Worst measured runtime delta: `pointlight_textured_meshes`, `-0.77%`
- Wireframe AA paths are effectively unchanged:
  `bunny -0.03%`, `teapot -0.02%`, others slightly positive
- `borg_cube`: `+0.02%` perspective, `+0.18%` orthographic
- `characters`: about `+0.96%` to `+1.91%` depending on mesh
- `spotlights_room`: `+2.43%`
- `spotlight_checkerboard`: `+3.97%`
- Best measured gains are on flat/gouraud mesh/texture paths, up to about `+5.93%`

Global `TGX_INLINE=` was rejected because it regressed real AA wireframe scenes:

- `dragon_wireframe_aa`: `-12.14%`
- `suzanne_wireframe_aa`: `-10.81%`
- `skull_wireframe_aa`: `-10.25%`

## Size Highlights

Granular override RAM1 code deltas:

```text
pointlight_textured_meshes: 156056 -> 103000  (-53056)
spotlights_room:           142232 ->  98136  (-44096)
spotlight_checkerboard:    136984 ->  94680  (-42304)
multi-lighting:            135064 -> 122904  (-12160)
test-shading:              135992 -> 133432  ( -2560)
test-texture:              120952 -> 117112  ( -3840)
buddha:                     86040 ->  82008  ( -4032)
characters:                121656 -> 117048  ( -4608)
borg_cube:                  90520 ->  86424  ( -4096)
```

`mars` still does not fit:

```text
default:  RAM1 free -2144
granular: RAM1 free -3168
```

The granular override reduces code, but the altered padding/bank boundary still
leaves `mars` over the limit. The broad global `TGX_INLINE=` makes it fit, but
the AA regression makes that policy too risky as a default.

## Current Source Decision

Keep:

- `TGX_RASTERIZE_TRIANGLE_INLINE`
- `TGX_UBER_SHADER_INLINE`
- `TGX_SHADER_SELECT_INLINE`
- `TGX_RENDERER3D_SHADING_INLINE`
- `TGX_DRAWSPHERE_PUBLIC_INLINE`
- `TGX_DRAWSPHERE_INTERNAL_INLINE`

The first four default to `TGX_INLINE`, but they only apply to the large control
points we may want to experiment with independently:

- `rasterizeTriangle()`
- `uber_shader_inline()`
- `shader_select()`
- the main Renderer3D shading functions (`_shadeVertex()`, `_shadeFace()`,
  `_setFlatOrUnlitFaceColor()`)

Small helpers remain on plain `TGX_INLINE`, including rasterizer arithmetic
helpers, RGB565 shader helpers, and small Renderer3D state/update helpers.
This keeps the override granularity useful without weakening hot scalar helper
inlining by accident.

The sphere macros default to the historical behavior: public sphere methods
have no extra attribute and internal sphere helpers use `TGX_NOINLINE`.

Additional Mars-specific result:

- Marking only the `mars.ino` `drawSphere()` wrapper as `FLASHMEM` moved the
  wrapper to flash but did not reduce RAM1, because the large
  `Renderer3D::_drawSphere...` instantiations stayed in ITCM.
- Forcing the whole sphere call chain inline
  (`drawAdaptativeSphere()`, `drawSphere()`, `_drawSphere()`,
  `_drawSphereGouraudCap()`, `_drawSphereGouraudStripBand()`) and keeping the
  wrapper in `FLASHMEM` made `mars` compile:

```text
mars default before: RAM1 code 176616, padding 19992, free -2144
mars local inline:   RAM1 code 160424, padding  3416, free 30624
```

- Applying that forced-inline policy globally is not acceptable: the synthetic
  sphere bench RAM1 code grows from `101488` to `137840`.
- Raw `FLASHMEM` explicit instantiation of the template helpers does not work:
  GCC tries to emit the template instantiation into `.flashmem`, but the
  template COMDAT section conflicts with normal `FLASHMEM` functions already
  using that same section name.
- Explicit instantiation into a dedicated linker-matched subsection works:

```cpp
template __attribute__((section(".flashmem.tgx.drawsphere")))
void Renderer3D<RGB565, LOADED_SHADER, uint16_t, 1, 0>::_drawSphere<false, Renderer3D_detail::WIREFRAME_FAST>(...);
```

  Teensy's linker script keeps `*(.flashmem*)` in flash, so
  `.flashmem.tgx.drawsphere` is included without linker changes.
- A hybrid Mars experiment with public `drawSphere()/drawAdaptativeSphere()`
  explicit specializations in `FLASHMEM`, plus explicit instantiations of the
  heavy private helpers in `.flashmem.tgx.drawsphere`, compiles and places the
  heavy sphere path in flash:

```text
mars wrapper only:     RAM1 code 176616, padding 19992, free -2144
mars local inline:     RAM1 code 160424, padding  3416, free 30624, FLASH code 276688
mars explicit section: RAM1 code 160744, padding  3096, free 30624, FLASH code 251344
```

  Relevant symbols are at `0x600...`:
  `_drawSphere<false,0>`, `_drawSphereGouraudStripBand()`,
  `_drawSphereGouraudCap()`, and the public sphere wrappers.
- Once the Renderer3D sphere instantiations are in flash, the local
  `mars.ino` wrapper no longer needs `FLASHMEM`; the RAM1 result is unchanged.
- Public-method true specializations in `FLASHMEM` were also tested and compile,
  but they copy small function bodies and increase code slightly. Pure explicit
  instantiation in `.flashmem.tgx.drawsphere` is cleaner and smaller.
- Runtime capture of the explicit-section variant succeeded. A 70 s Mars
  capture gave `movie` average `47.62 fps` over 61 samples, close to the
  previous local-inline run (`movie` about `48.77 fps`).
- The final no-local-wrapper-`FLASHMEM` variant was also uploaded and captured
  successfully; a shorter 45 s capture reached the `movie` scene with 37 samples
  and no runtime issue.
- Therefore the inline policy is not the preferred Mars fix. The better shape
  is a future TGX macro that emits explicit flash-section instantiations for a
  concrete renderer type and primitive path.

By default:

```cpp
TGX_DRAWSPHERE_PUBLIC_INLINE   // empty
TGX_DRAWSPHERE_INTERNAL_INLINE // TGX_NOINLINE
```

No example should force these to `TGX_INLINE inline` as the preferred solution;
that was only a diagnostic experiment.

Do not keep:

- `TGX_TEENSY4_EXPERIMENT_FLASHMEM_NOINLINE`
- the broad source macros `TGX_RENDERER3D_INLINE`, `TGX_RASTERIZER_INLINE`,
  and `TGX_SHADER_INLINE`
- global `TGX_INLINE=` as a recommended policy
- sketch-level `mars` `FLASHMEM` wrapper changes

## Recommendation

For now, do not change the Teensy 4.1 default inlining policy globally.

The safe result of this investigation is the new opt-in 3D granularity. It lets
us build and test with:

```powershell
-DTGX_RASTERIZE_TRIANGLE_INLINE= -DTGX_UBER_SHADER_INLINE= -DTGX_SHADER_SELECT_INLINE= -DTGX_RENDERER3D_SHADING_INLINE=
```

This is explainable, reproducible, and validated on benchmark, synthetic sphere
bench, and real Teensy4 examples. It should be kept as an available tuning
mechanism, then possibly promoted later only after one more full baseline
refresh and after deciding how to handle `mars` separately.

## Artifacts

- `hardware/binary_size_summary.csv`
- `hardware/size_granular_vs_default.csv`
- `hardware/example_granular_vs_default.csv`
- `hardware/example_inline_vs_default.csv`
- `hardware/sphere_granular_vs_default.csv`
- `hardware/benchmark_granular_global.csv`
- `codegen/template_probe_nm.txt`
- `codegen/template_probe_object_sections.txt`
- `codegen/hot_symbol_summary.csv`
- `sketches/FlashmemTemplateProbe/`
