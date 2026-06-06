# texture_path_split_candidate

Patch: `tmp/renderer_layout_investigation/patches/texture_path_split_candidate.patch`

## Change

Rewrote the `TEXTURE_WRAP` choices in the textured part of `uber_shader(...)` as explicit `if constexpr` branches for bilinear and nearest texture sampling.

This tested whether the RP2350 compiler generated better texture-path code or reduced register pressure when wrap/clamp was expressed as an explicit compile-time split rather than ternaries.

## Pico2 benchmark result

Run: `pico2_renderer_texture_path_split_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores were essentially unchanged:

- score 1: `23.04`
- score 2: `16.55` vs baseline median `16.54`
- score 3: `14.01`

Summary:

- mean subtest delta: `-0.0422%`
- median: `0%`
- regressions below `-1%`: `2`
- below `-3%`: `1`
- below `-5%`: `1`
- worst: `Wire cube fast` 100x100, `-5.508%`
- best: `Wire cube fast` 200x200, `+1.390%`

Texture rows were not meaningfully improved:

- `R2D2 TEX_NEAREST`: `0%` to `+0.066%`.
- `R2D2 TEX_BILINEAR`: `0%` to `+0.085%`.
- `Bunny TEX_NEAREST`: `-0.062%` to `+0.087%`.
- `Bunny TEX_BILINEAR`: `-0.108%` to `+0.026%`.
- `Grid small TEX`: `+0.015%` to `+0.135%`.

Guard rows:

- `Wire rib tri thick`: neutral.
- `Point cloud dots/pixels`: mostly neutral.
- `Wire cube fast`: `-5.508%` at 100x100 and `-2.163%` at 320x240, with a contradictory `+1.390%` at 200x200.

## Decision

Rejected. The intended texture paths stayed near noise, while an unrelated wire guard row regressed badly. This looks like layout sensitivity rather than a direct improvement.
