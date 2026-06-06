# shaderclip_inline_relax_candidate

Patch: `tmp/renderer_layout_investigation/patches/shaderclip_inline_relax_candidate.patch`

## Change

Changed `shaderclip()` from `inline TGX_INLINE` to plain `inline`, relaxing forced always-inline on RP2350 while still allowing normal compiler inlining.

This tested whether a tiny texture-clamp helper was hurting code size/register pressure when forced inline inside many `uber_shader` texture variants.

## Pico2 benchmark result

Run: `pico2_renderer_shaderclip_inline_relax_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were almost unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.02` vs baseline median `14.01`

Summary:

- mean subtest delta: `+0.0169%`
- median: `0%`
- regressions below `-1%`: `1`
- worst: `Wire cube AA` 100x100, `-1.0159%`
- best: `Wire sphere fast` 100x100, `+4.2237%`

Texture rows did not show a meaningful direct gain:

- `R2D2 TEX_NEAREST/BILINEAR`: `0%` to `+0.139%`.
- `Bunny TEX_NEAREST/BILINEAR`: `-0.031%` to `+0.087%`.
- `Grid small TEX`: `0%` to `+0.251%`.
- `Sphere direct TEX`: `0%` to `+0.013%`.

Guard rows were mostly safe but not improved:

- `Wire rib tri thick`: neutral.
- `Point cloud pixels`: mixed `+1.335%`, `-0.635%`, `-0.046%`.
- `Wire cube AA`: one weak `-1.016%` regression.

## Decision

Rejected as an optimization. The change is small and mostly safe, but the measured effect is below useful threshold and does not target the intended texture/mesh paths. It looks like normal layout noise rather than a robust improvement.
