# Regression deep dive - 2026-07-01

Scope:
1. Pico2/RP2350 `scream` real-example regression.
2. Core2/ESP32 `core_raster` synthetic regression.
3. Pico2/RP2350 `skybox_noz` and `wire_points` synthetic regressions.

This investigation must explain performance changes with source/codegen evidence and targeted measurements, not just attribute them to generic layout effects.

Final report:

- `reports/regression_deep_dive_report.md`

Most useful compact summaries:

- `summaries/targeted_dev_vs_main_comparison.csv`: original dev vs main targeted comparison.
- `summaries/final_candidate_vs_main_dev_comparison.csv`: final candidate vs both references for comparable targeted tests.
- `summaries/final_candidate_validation_all_results.csv`: final candidate validation on the connected boards for the affected modules.
- `summaries/final_candidate_example_vs_main_dev.csv`: real-example validation after the fixes.

Retained compact codegen evidence:

- `codegen/*.nm.txt`: symbol extracts used in the report.
- `codegen/*symbol_size_delta.csv`: compact symbol-size deltas.
- `codegen/diff_*.patch`: source diffs used during analysis.

Large raw disassembly dumps and serial/build logs were pruned after the relevant conclusions were folded into the reports.
