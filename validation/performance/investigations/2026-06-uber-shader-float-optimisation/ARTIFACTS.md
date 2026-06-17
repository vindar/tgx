# Artifact notes

This investigation was compacted after the hardware validation.

Kept files:

- `REPORT.md`: human-readable final report, accepted/rejected candidates, key performance numbers.
- `command_manifest_floatopt_final_vs_base.csv`: exact base/final commands, boards, ports, FQBNs, statuses, telemetry row counts and build sizes.
- `delta_floatopt_final_vs_base.csv`: scene-level final-vs-base performance deltas.
- `rollup_floatopt_final_vs_base.csv`: board/example rollups.
- `build_size_floatopt_final_vs_base.csv`: compact build-size comparison.
- `delta_floatopt_*_vs_base.csv` and `example_summary_floatopt_*.csv`: compact traces for individual tested candidates.
- `delta_floatopt_zlikely_vs_final.csv`: explicit rejection data for the z-pass branch-likelihood hint.

Removed after compaction:

- `validation/performance/builds/`: Arduino build directories and object files.
- `validation/performance/logs/`: raw upload/build/capture logs.
- `validation/performance/parsed/`: raw per-run JSON and parser CSVs already summarized in the kept manifest/CSV files.
- `validation/performance/telemetry/`: raw serial telemetry already summarized in the kept CSV files.

The earlier affine fixed-point investigation remains separately preserved in:

- `validation/performance/investigations/2026-06-affine-texture-fixedpoint/`
