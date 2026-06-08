# Cleanup Manifests

This folder records performance-artifact cleanup operations.

For the 2026-06-07 cleanup:

- `2026-06-07-pruned_directories.csv` records top-level raw artifact directories that were removed or selected for removal.
- `2026-06-07-pruned_capture_files.csv` records the raw log/parsed/telemetry file names that existed before pruning.

The cleanup kept compact summaries, reports, patches, and baseline CSVs under:

```text
validation/performance/baselines/current/
validation/performance/investigations/
```

The removed build directories are reproducible with the archived sketches/tools and are not needed for normal review.
