# cold_uber_shader_candidate

Patch: `tmp/renderer_layout_investigation/patches/cold_uber_shader_candidate.patch`

## Change

Added an RP2350-only `noinline` macro for `uber_shader(...)` and applied it to the shader inner-loop template.

## Pico2 benchmark result

Run: `pico2_renderer_cold_uber_shader_candidate_bench_pico2`
Status: `SUCCESS`, 3 global scores and 162 subtests parsed.

Global scores regressed strongly:

- score 1: `21.67` vs baseline `23.04`
- score 2: `15.66` vs baseline `16.54`
- score 3: `13.34` vs baseline `14.01`

Summary:

- mean subtest delta: `-2.43773%`
- median: `-0.373632%`
- regressions below `-1%`: `67`
- below `-3%`: `52`
- below `-5%`: `39`
- worst: `R2D2 FLAT` 100x100, `-22.8241%`
- best: `Wire cube fast` 100x100, `+12.7132%`

Important rows:

- `R2D2 FLAT`: about `-22%` across sizes.
- `Bunny FLAT`: about `-17%` to `-19%`.
- `Buddha FLAT`: about `-16%` to `-17%`.
- `Bunny TEX_BILINEAR`: `-6.57%`, `-4.53%`, `-3.70%`.
- `Wire rib tri thick`: `-16.60%`, `-13.05%`, `-11.88%`.
- `Wire cube fast`: `+8%` to `+12.7%`, showing that this change reshuffles hot/cold costs rather than providing a generally useful improvement.

## Decision

Rejected. The shader inner loop must stay inline/in-template for mesh paths. This candidate is useful evidence: a direct call boundary around `uber_shader` reproduces large benchmark sensitivity, but it is the wrong tradeoff and fails the Pico2 guard gates badly.
