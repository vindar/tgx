# drawSphere strip investigation - 2026-06-27

## Scope

Tested `Renderer3D::drawSphere()` with a dedicated 12-case benchmark covering:

- small/medium/large sphere sizes;
- low and high sector/stack counts;
- flat and Gouraud shading;
- textured and non-textured rendering;
- directional, point, and spot lights;
- specular enabled/disabled.

CPU visual references and final candidate images were generated during the
investigation, then pruned before commit because the compact summaries below
preserve the result. The source generator is kept as
`cpu/sphere_visual_suite.cpp`, and per-variant CPU summary CSVs are retained
under `cpu/*/summary.csv`.

## CPU Visual Result

Final helper-strip/fan rendering is identical to the previous strip candidate.
Compared with the original reference:

- flat cases: 0 changed pixels;
- Gouraud without texture: 0 changed pixels;
- Gouraud textured cases: small edge differences only, caused by strip triangle order on shared edges.

Worst CPU diff:

- `high_large_gouraud_tex_spot`: 225 / 76800 changed pixels, max channel diff 9.

## Implemented Variants

1. `opt1_direct`
   - `_drawSphere()` now prepares shader/specular/texture state once.
   - Top/bottom triangles call `_drawTriangle()` directly.
   - Middle quads call `_drawQuad()` directly.
   - CPU output is bit-identical to the baseline.

2. `opt2_helper`
   - Gouraud middle bands use a streaming strip helper.
   - The strip path is controlled by `TGX_DRAWSPHERE_USE_STRIP_BANDS`.
   - Flat and wireframe paths keep the previous quad loop.

3. `opt3_cap_fan_final`
   - Gouraud top/bottom caps use a streaming fan helper when strip bands are enabled.
   - The helper is a single runtime-bool function, not a `template<bool>`, to avoid duplicating top/bottom code.
   - A smaller no-clipping cap variant was rejected: it saved little code but produced a reproducible Core2 regression on several Gouraud cases.

## Final Hardware Summary

Final summary CSVs:

- `hardware/opt3_cap_fan_final_vs_opt2_helper.csv`
- `hardware/opt3_cap_fan_final_vs_baseline_current.csv`
- `hardware/opt3_cap_fan_final.csv`

Average FPS delta for final `opt3_cap_fan_final` vs `opt2_helper`:

| Board | All cases | Flat | Gouraud | Gouraud textured | Gouraud local lights | High Gouraud |
|---|---:|---:|---:|---:|---:|---:|
| Teensy 4.1 | +2.30% | -0.03% | +3.07% | +3.02% | +3.09% | +2.98% |
| Core2 | +1.70% | +1.09% | +1.90% | +0.88% | +1.18% | +1.56% |
| CoreS3 | +3.12% | +4.32% | +2.72% | +2.52% | +1.32% | +1.87% |
| Pico2 | +0.00% | +0.06% | -0.02% | -0.03% | -0.02% | -0.01% |

Average FPS delta for final `opt3_cap_fan_final` vs original `baseline_current`:

| Board | All cases | Flat | Gouraud | Gouraud textured | Gouraud local lights | High Gouraud |
|---|---:|---:|---:|---:|---:|---:|
| Teensy 4.1 | +17.49% | +1.89% | +22.69% | +23.74% | +26.77% | +31.70% |
| Core2 | +9.52% | +4.72% | +11.12% | +11.96% | +12.53% | +9.37% |
| CoreS3 | +11.40% | +7.45% | +12.72% | +13.87% | +13.21% | +13.43% |
| Pico2 | +1.78% | +0.59% | +2.18% | +1.75% | +1.96% | +2.64% |

Final compile sizes for `SpherePrimitiveBench`:

| Board | `opt2_helper` | Final | Delta |
|---|---:|---:|---:|
| Teensy 4.1 | 93256 code bytes | 104008 code bytes | +10752 |
| Core2 | 360405 flash bytes | 368241 flash bytes | +7836 |
| CoreS3 | 356935 flash bytes | 364707 flash bytes | +7772 |
| Pico2 | 113240 flash bytes | 113240 flash bytes | +0 |

Pico2 is intentionally neutral because the strip path is disabled for RP2350.

## Notes

- Remaining regressions vs `opt1_direct` are small and are still positive vs the original baseline.
- Textured Gouraud hashes differ from the original reference because the strip draws the same diagonal with a different triangle order. Visual diffs are very small.
- Non-textured Gouraud and flat hashes remain identical where the path is unchanged.
