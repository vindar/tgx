# Current Performance Baselines

This folder contains compact CSV files intended for quick comparison in future TGX performance sessions.

Do not treat these as magic constants. Check the commit/source-state notes below before reusing them.

## Files

Full benchmark baseline copied from the archived inline-granularity campaign:

- `benchmark_global_scores.csv`
- `benchmark_subtests.csv`
- `binary_size.csv`

Example telemetry baseline copied from the same campaign:

- `example_telemetry.csv`
- `example_telemetry_summary.csv`

Latest Core2/CoreS3 benchmark sanity for the Image blit candidate:

- `core2_image_blit_candidate_benchmark_global.csv`
- `core2_image_blit_candidate_benchmark_subtests.csv`
- `cores3_image_blit_candidate_benchmark_global.csv`
- `cores3_image_blit_candidate_benchmark_subtests.csv`

## Source State Notes

The full benchmark baseline CSVs come from:

```text
validation/performance/investigations/2026-06-tgx-inline-renderer-layout/summaries/inline_granularity/
```

Those rows are tagged with commit `5fbe8cf` in the CSVs. They were reused because later sanity checks showed unchanged global 3D benchmark scores for Core2/CoreS3 after the Image blit candidate.

The Image blit candidate sanity rows come from:

```text
validation/performance/investigations/2026-06-esp32s3-image-span-copyfill/results/
```

They are useful for checking that the `Image::blit()` optimization did not disturb Core2/CoreS3 3D benchmark scores.

## Reuse Policy

Reuse these baselines when:

- the same board and display setup are used;
- the same benchmark sketch and board define are used;
- the source/macro state is known to be equivalent for the measured path;
- the new candidate is not near the noise threshold.

Rerun hardware baselines when:

- TGX renderer/shader/Image behavior has changed in a way that may affect the benchmark;
- toolchain, board package, or display setup changed;
- the previous baseline lacks detailed subtests;
- a candidate result is close to noise or suspicious.

## Board Coverage

The archived full benchmark baselines include rows for:

- Core2 / ESP32
- CoreS3 / ESP32-S3
- Pico2 / RP2350
- Teensy 4.1
- Pico W / RP2040 where available, though Pico W was later marked unreliable

Use the CSV `board` column to filter.
