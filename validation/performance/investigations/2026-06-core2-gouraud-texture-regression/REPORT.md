# Core2 Gouraud Texture Inline Regression

Date: 2026-06-29 / 2026-06-30.

## Summary

A Core2 regression was reproduced on a dedicated `donkeykong_small`
Gouraud+texture probe after the global `TGX_INLINE` / `TGX_NOINLINE` cleanup.
The historical reference was about 25.96 fps. The slow state dropped to about
23.41 fps.

The minimal accepted fix is:

```cpp
TGX_INLINE bool _clip2(float clipboundXY, const fVec3 & P, const fMat4 & M)
```

No `RGB565::blend256` change is needed for this fix. `TGX_NOINLINE` can remain
empty on ESP32/Core2.

## Important Finding

The mesh-level clipping setup path is performance-sensitive on Core2.

Call shape:

```text
_drawMesh()
  -> _discardBox(bb, proj_modelview)
       -> _clip(...) for the 8 bounding-box corners
  -> _clipTestNeeded(CLIPBOUND_XY, bb, proj_modelview)
       -> _clip2(...) for the 8 bounding-box corners
```

`_discardBox()` answers whether the whole bounding box is outside the frustum and
the object can be skipped. `_clipTestNeeded()` answers whether the bounding box
intersects the frustum and triangle clipping must remain enabled for the mesh.
`_clip2()` is the small per-corner helper used by `_clipTestNeeded()`.

If this regression returns, inspect `_clip2()`, `_clipTestNeeded()`,
`_discardBox()`, and the generated `_drawMesh()` code first.

## Core2 Probe Results

The dedicated probe was:

```text
validation/performance/investigations/2026-06-core2-gouraud-texture-regression/sketches/core2_donkey_infos_probe
```

Main run command:

```powershell
.\validation\performance\tools\upload_and_capture.ps1 -Board core2 -Sketch D:\Programmation\tgx\validation\performance\investigations\2026-06-core2-gouraud-texture-regression\sketches\core2_donkey_infos_probe -Label core2_donkey_infos_probe_final_clip2_20260630 -ParseMode example -Baud 115200 -TimeoutSeconds 45 -PortWaitSeconds 90 -RetryCount 1
```

Key results are in `results/core2_candidate_summary.csv`.

## Codegen Notes

Slow state:

- `_clipTestNeeded` and `_clip2` remained emitted as separate symbols.
- `_drawMesh` was smaller, but the probe was slower.
- Rasterizer and texture shader symbol sizes were essentially unchanged.

Accepted `_clip2` fix:

- `_clip2` and `_clipTestNeeded` disappeared as separate symbols in the relevant
  build and were integrated into `_drawMesh`.
- `rasterizeTriangle` and the hot texture shader stayed unchanged.
- Program size for the Core2 probe was `646003` bytes, same after adding plain
  logical `inline` keywords.

Rejected alternatives:

- Re-inlining unrelated lighting/material helpers could recover fps but was not
  source-robust because those helpers are not in the per-frame hot path of the
  probe.
- `_discardBox` alone also recovered fps but produced a larger code-size change
  than `_clip2`.
- `_clipTestNeeded` alone was not sufficient.

## Cross-Board Check

Final `_clip2` state was uploaded and captured on CoreS3, Pico2, and Teensy 4.1
real examples. See `results/final_cross_board_window_check.csv`.

When compared against equivalent capture windows from the immediate pre-fix
state, no convincing regression was observed on the other boards.
