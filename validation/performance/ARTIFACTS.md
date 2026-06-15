# Performance Validation Artifacts

This directory is for reusable performance tooling, compact baselines, and dated investigation records.

It should not contain persistent Arduino build directories or large raw capture dumps at the top level.

## Directory Layout

- `tools/`
  - Reusable upload/capture, parser, and aggregation tools.
  - Keep these easy to find for future sessions.

- `baselines/current/`
  - Compact CSV baselines used for quick comparison before running new benchmarks.
  - These are the first files to check before deciding whether a full hardware baseline must be rerun.

- `investigations/`
  - Compact history plus one final report or summary per dated investigation.
  - Keep enough detail to understand what was accepted or rejected without storing the whole lab notebook.

- `cleanup/`
  - Small manifests describing pruned raw artifacts.

## What To Keep

Keep:

- final reports and READMEs;
- compact decision summaries;
- reusable baseline CSVs;
- small reusable scripts under `tools/`;
- compact baseline CSVs for current comparison.

## What To Prune

Usually prune:

- `validation/performance/builds/`;
- top-level `validation/performance/logs/`;
- top-level `validation/performance/telemetry/`;
- top-level `validation/performance/parsed/` once the useful CSVs are copied into a dated investigation;
- empty parser outputs such as two-byte CSV placeholders;
- raw serial captures that are already summarized in investigation results.
- full objdump dumps;
- per-candidate matrices once the final report records the decision;
- investigation-specific sketches/parsers unless they are expected to be reused.

Raw captures can be kept inside an investigation only when they contain unique evidence that is not represented in parsed CSVs or reports.

The 2026-06-07 cleanup pruned previously committed raw capture directories and full `objdump` dumps after keeping their reports, parsed CSVs, manifests, and codegen summaries. See `cleanup/`.

## Current Benchmark Baselines

See:

```text
validation/performance/baselines/current/
```

These files collect the currently reusable hardware benchmark/example baselines. The baseline README explains the exact commit, build options, boards, and reuse caveats.
