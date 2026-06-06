# Image Inline Investigation Session Start

Date: 2026-06-06  
Branch: `feature/multi-directional-lights`  
HEAD: `5fbe8cf`

## Current Working Tree

`git status --short` at session start:

```text
 M src/Color.h
 M src/Image.h
 M src/Misc.h
 M src/Shaders.h
 M src/Vec2.h
 M src/Vec3.h
 M src/Vec4.h
 M src/tgx_config.h
?? REPORT_INLINE_GRANULARITY.md
```

`git diff --stat` at session start:

```text
 src/Color.h      | 54 +++++++++++++++++++++----------------------
 src/Image.h      | 70 ++++++++++++++++++++++++++++----------------------------
 src/Misc.h       | 42 +++++++++++++++++-----------------
 src/Shaders.h    |  2 +-
 src/Vec2.h       | 58 +++++++++++++++++++++++-----------------------
 src/Vec3.h       | 54 +++++++++++++++++++++----------------------
 src/Vec4.h       | 54 +++++++++++++++++++++----------------------
 src/tgx_config.h | 32 ++++++++++++++++++++++++++
 8 files changed, 199 insertions(+), 167 deletions(-)
```

`git diff --check` was clean.

## Current Uncommitted Macro Layer

The current WIP introduces category inline macros:

- `TGX_INLINE_VEC2`
- `TGX_INLINE_VEC3`
- `TGX_INLINE_VEC4`
- `TGX_INLINE_COLOR`
- `TGX_INLINE_RGBF`
- `TGX_INLINE_MATH`
- `TGX_INLINE_SHADER`
- `TGX_INLINE_IMAGE`

All category macros currently fall back to `TGX_INLINE`, so this layer is expected to be behavior-neutral unless overridden by build flags or board-specific policy. `Vec4::zdivide()` remains controlled by `TGX_INLINE_ZDIVIDE`.

This session must decide whether that broad neutral macro layer is worth keeping, reducing, or reverting.

## Trustworthy Results From Previous Pass

Trustworthy:

- Hardware/capture tooling is reliable on Core2, CoreS3, Pico2, and Teensy 4.1.
- Pico W / RP2040 upload is unreliable and should be treated as compile-only unless fixed.
- Final default category macro layer was neutral on Pico2, CoreS3, and Teensy 4.1.
- CPU validation passed with 3D strict `82 PASS, 0 FAIL` and 2D validation all `OK`.
- Pico2 category overrides showed strong signals:
  - `TGX_INLINE_VEC3=`, `TGX_INLINE_VEC4=`, `TGX_INLINE_RGBF=`, `TGX_INLINE_COLOR=`, and `TGX_INLINE_MATH=` regress heavily.
  - `TGX_INLINE_SHADER=` is near-neutral and not useful.
  - `TGX_INLINE_IMAGE=` improves global Pico2 score by about +4.5%, but causes severe local regressions, especially wire/dots paths.

Previous `TGX_INLINE_IMAGE=` Pico2 result:

- Global score delta: about +4.38%, +4.62%, +4.43%.
- Mean subtest delta: +2.55%.
- Regressions <= -3%: 25 subtests.
- Worst regression: about -19.62% on `Wire rib tri thick`.

## Usable Boards

| Board | Port | Status |
| --- | --- | --- |
| Core2 | COM5 | Trusted |
| CoreS3 | COM10 | Trusted |
| Pico2 / RP2350 | COM21 | Trusted and primary target |
| Teensy 4.1 | COM3 | Trusted |
| Pico W / RP2040 | COM28 | Compile-only unless upload issue is fixed |

## Main Target

The main target is to investigate the unsafe but promising `TGX_INLINE_IMAGE=` signal.

Goals:

- Reproduce coarse `TGX_INLINE_IMAGE=` on Pico2 with detailed subtests.
- Identify which Image helpers are responsible for the global gain and which protect wire/dots paths.
- Split image inline policy into narrower experimental categories.
- Search for a candidate that keeps useful Pico2 gains without hidden local regressions.
- Validate any finalist across Core2, CoreS3, Teensy 4.1, and real examples.
- Decide whether to keep, reduce, or revert the current broad category macro layer.
