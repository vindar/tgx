# Real Example Validation

No narrowed Image inline candidate survived Pico2 benchmark screening with a meaningful gain.

Because every performance-positive Image candidate violated hard Pico2 gates, especially wire/dots regressions, no new cross-board real-example validation was run for those rejected candidates.

The only safe Image candidate was `image_mesh_safe_keep_draw_wire_span_pico2`, which was effectively neutral:

- Mean subtest delta: +0.012%
- Median subtest delta: 0%
- Regressions <= -3%: 0
- Worst regression: -0.84%
- Global score: effectively baseline

This neutral candidate does not justify changing source code or running the full real-example matrix again.

Previous real-example telemetry from the inline-granularity pass remains relevant for the neutral macro-layer default:

- Pico2 `bunny_fig`, `borg_cube`, `scream`: effectively unchanged.
- Teensy 4.1 `characters`, `test-texture`, `test-shading`, `borg_cube`, `scream`: within noise.
- Core2/CoreS3 examples: mostly stable; DonkeyKong scene row counts differed between captures, so the apparent Gouraud deltas were treated as sampling variation.
