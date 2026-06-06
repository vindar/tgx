# Baseline Reuse Audit

Date: 2026-06-06
Current HEAD: `5fbe8cf57342a0bb3dd343fa63866c7ba510ae42`

## Decision

Reuse existing baseline data from `tmp/inline_granularity/`.

Reason: the source tree has no tracked source diff and is at the same commit (`5fbe8cf`) as the inline-granularity baseline. The benchmark sketch, board macros, board ports, display setup, Arduino cores, and robust upload/capture tooling are unchanged.

## Reused Benchmark Baselines

Copied into this session:

- `tmp/renderer_layout_investigation/baseline_global_scores.csv`
- `tmp/renderer_layout_investigation/baseline_subtests.csv`
- `tmp/renderer_layout_investigation/baseline_binary_size.csv`

Original sources:

- `tmp/inline_granularity/baseline_global_scores.csv`
- `tmp/inline_granularity/baseline_subtests.csv`
- `tmp/inline_granularity/baseline_binary_size.csv`

Baseline rows report commit `5fbe8cf` and `SUCCESS` parse status.

| Board | Baseline run(s) | Reuse status |
| --- | --- | --- |
| Pico2 | `pico2_parse_sanity_bench_pico2`, `pico2_baseline_bench_pico2_run2` | Reused |
| Teensy 4.1 | `teensy41_baseline_bench_teensy41_run1`, `teensy41_baseline_bench_teensy41_run2` | Reused |
| Core2 | `core2_baseline_bench_core2_run1` | Reused |
| CoreS3 | `cores3_parse_sanity_bench_cores3`, `cores3_baseline_bench_cores3_run2` | Reused |

## Reused Example Baselines

Copied into this session:

- `tmp/renderer_layout_investigation/baseline_example_telemetry.csv`
- `tmp/renderer_layout_investigation/baseline_example_summary.csv`

Original sources:

- `tmp/inline_granularity/baseline_example_telemetry.csv`
- `tmp/inline_granularity/baseline_example_telemetry_summary.csv`

These will be used only for finalists. No full example baseline rerun is needed unless a finalist's effect is close to noise and same-session confirmation is necessary.

## Reused Codegen Baselines

The previous Image-inline pass already extracted Pico2 codegen for:

- baseline
- coarse `TGX_INLINE_IMAGE=`
- `_drawPixel=`
- `PIXEL_API_DRAW=`
- mesh-safe keep draw/wire/span

Sources:

- `tmp/image_inline_investigation/codegen/pico2_image_binary_sizes.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbols.csv`
- `tmp/image_inline_investigation/codegen/pico2_image_hot_symbol_deltas.csv`
- `tmp/image_inline_investigation/codegen/*.objdump.txt`

New candidates with nontrivial Pico2 effects will get fresh codegen extraction under `tmp/renderer_layout_investigation/codegen/`.

## Noise Policy

Use previous repeated-run estimates:

- Pico2: global score variation about one displayed hundredth at most.
- CoreS3: repeated scores identical at displayed precision in inline-granularity pass.
- Teensy 4.1: repeated scores varied below about 0.04%.
- Core2: previous global scores identical across repeated runs in config pass.

No repeated baseline runs are needed before candidate screening.
