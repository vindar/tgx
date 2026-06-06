# Rejected Inline Granularity Candidates

Date: 2026-06-05

All candidates were measured against current HEAD baseline `5fbe8cf`.

## Rejected Overrides

| Patch / variant | Board focus | Reason rejected |
| --- | --- | --- |
| `inline_vec_policy.patch` with `TGX_INLINE_VEC2=` | Pico2 | No global gain and several local regressions. Mean subtest delta was slightly positive, but the result is too weak to justify an override. |
| `inline_vec_policy.patch` with `TGX_INLINE_VEC3=` | Pico2 | Global score regressed by -6.12%, -3.78%, -2.86%; 74 subtests regressed by at least 3%. |
| `inline_vec_policy.patch` with `TGX_INLINE_VEC4=` | Pico2 | Global score regressed by -22.31%, -18.46%, -16.20%; 100 subtests regressed by at least 3%. |
| `inline_vec_policy.patch` with all Vec categories empty | Pico2 | Very large global and local regressions. |
| `inline_color_policy.patch` with `TGX_INLINE_COLOR=` | Pico2 | Global score regressed by -11.07%, -8.73%, -7.35%; 52 subtests regressed by at least 3%. |
| `inline_color_policy.patch` with `TGX_INLINE_RGBF=` | Pico2 | Global score regressed by -18.01%, -14.78%, -12.78%; 49 subtests regressed by at least 3%. |
| `inline_color_policy.patch` with Color and RGBf empty | Pico2 | Combined override still regressed heavily. |
| `inline_math_policy.patch` with `TGX_INLINE_MATH=` | Pico2 | Global score regressed by -14.67%, -12.96%, -11.78%; 92 subtests regressed by at least 3%. |
| `inline_shader_policy.patch` with `TGX_INLINE_SHADER=` | Pico2 | Mostly neutral but not useful. One large local regression around `Wire cube fast`; no robust gain. |
| `inline_image_policy.patch` with `TGX_INLINE_IMAGE=` | Pico2 | Global score improved by about +4.5%, but 25 subtests regressed by at least 3%; worst was -19.62% on `Wire rib tri thick`. Rejected because useful wire/dots paths degrade badly. |
| Vec2 + Shader empty combined | Pico2 | Slight mean subtest improvement, no global gain; not strong enough to keep. |

## Useful Observations

- Removing forced inline from `Vec3`, `Vec4`, `RGBf`, color helpers, or math helpers on RP2350 is not viable in the current renderer.
- `TGX_INLINE_IMAGE=` is the only rejected candidate with a strong global positive signal, but it is probably a layout/codegen trade rather than a safe optimization.
- The final accepted source diff keeps category macros but leaves all of them mapped to `TGX_INLINE`.

Detailed data:

- `tmp/inline_granularity/candidate_subtest_summary.csv`
- `tmp/inline_granularity/subtest_matrix.csv`
- `tmp/inline_granularity/global_scores.csv`
- `tmp/inline_granularity/codegen/pico2_hot_symbol_deltas.csv`
