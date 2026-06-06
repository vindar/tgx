# rp2350_mesh_layout_candidate_2

Patch: `tmp/renderer_layout_investigation/patches/rp2350_mesh_layout_candidate_2.patch`

Intent: test whether a plain `inline` marker on `rasterizeTriangle()` would be enough to let the compiler improve RP2350 codegen without the large code growth and layout damage caused by `always_inline`.

Result on Pico2: rejected as neutral.

Summary:
- Global scores were essentially baseline: `23.04 / 16.55 / 14.01`.
- Mean subtest delta: +0.012%.
- Median subtest delta: 0%.
- Binary size returned to the baseline-sized range (`uf2_bytes=2762752`).
- No meaningful mesh-heavy gains survived.

Worst Pico2 rows:
- `Wire cube fast`, 320x240: -3.09%.
- `Wire cube fast`, 200x200: -1.02%.
- `Wire sphere fast`, 200x200: -0.79%.

Best Pico2 rows:
- `Wire cube fast`, 100x100: +2.96%.
- `Point cloud pixels`, 100x100: +1.21%.
- The remaining improvements were below +1%.

Conclusion:
Plain `inline` does not reproduce the useful mesh-heavy optimization. It also introduces one hard guard-row regression in `Wire cube fast` at 320x240. This variant is not worth keeping.
