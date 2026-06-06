# Renderer Layout Investigation - Session Start

Date: 2026-06-06
Branch: `feature/multi-directional-lights`
HEAD: `5fbe8cf57342a0bb3dd343fa63866c7ba510ae42`

## Current Source State

Tracked source diff at session start:

- `git diff --stat`: empty
- `git diff --name-only`: empty
- `git diff --check`: clean

Untracked reports from previous passes:

- `REPORT_INLINE_GRANULARITY.md`
- `REPORT_IMAGE_INLINE_INVESTIGATION.md`

No source changes from the previous broad inline macro layer are currently present. The source tree is therefore at the committed baseline `5fbe8cf`.

## Previous Results Accepted

The following results are accepted and should not be re-tested unless a direct correctness problem appears:

- `TGX_INLINE_ZDIVIDE` is already committed and part of baseline.
  - Enabled for Teensy4.x, ESP32-S3, and RP2350.
  - Core2/classic ESP32 keeps it empty.
- RP2350 shader incremental pixel pointer path is already committed and part of baseline.
- Broad category inline macro layer was tested and reverted.
  - No category override survived across boards.
  - Do not repeat the generic `TGX_INLINE_VEC*`, `TGX_INLINE_COLOR`, `TGX_INLINE_MATH`, `TGX_INLINE_SHADER`, or `TGX_INLINE_IMAGE` search.
- Coarse `TGX_INLINE_IMAGE=` on Pico2 was reproduced at about +4.4% global but rejected due severe wire/dots regressions.
  - Worst reproduced regression: `Wire rib tri thick` about -19.6%.
  - Point-cloud/dots paths regressed around -15%.
  - Narrow Image categories showed that protecting wire/dots removes the gain.

## Baselines Available for Reuse

The current source is `5fbe8cf`, matching the previous baseline files below. Same boards, same benchmark sketch, same board macros, same upload/capture tooling, and same display setup are used.

Reusable benchmark baselines:

- `tmp/inline_granularity/baseline_global_scores.csv`
- `tmp/inline_granularity/baseline_subtests.csv`
- `tmp/inline_granularity/baseline_binary_size.csv`

Reusable example baselines:

- `tmp/inline_granularity/baseline_example_telemetry.csv`
- `tmp/inline_granularity/baseline_example_telemetry_summary.csv`

Reusable codegen baselines:

- `tmp/image_inline_investigation/codegen/pico2_image_binary_sizes.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbols.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbol_deltas.csv`
- `tmp/image_inline_investigation/codegen/*.objdump.txt`
- `tmp/inline_granularity/codegen/`

Do not rerun full baselines unless a candidate result is near noise and requires confirmation, or the existing data is incomplete for a specific comparison.

## Boards Available

Runtime boards for this session:

| Board | Port | Status |
| --- | --- | --- |
| Pico2 / RP2350 + ILI9341 | `COM21` | Primary target |
| Teensy 4.1 + ILI9341 | `COM3` | Cross-board validation |
| Core2 | `COM5` | Cross-board validation |
| CoreS3 | `COM10` | Cross-board validation |

Pico W / RP2040 is unavailable for runtime testing in this session and should be ignored.

## Objective

Investigate direct Renderer3D / shader / rasterizer / code-layout changes that may capture some of the Pico2 mesh-heavy gains observed indirectly through Image inline layout changes, while preserving wire/dots paths and cross-board performance.

Experiment strategy:

1. Reuse existing baselines.
2. Screen candidates on Pico2 first.
3. Reject candidates that regress wire/dots or point-cloud guard rows.
4. Cross-board test only promising Pico2 candidates.
5. Run real examples only for finalists.
6. Keep the source tree clean if no robust source optimization survives.
