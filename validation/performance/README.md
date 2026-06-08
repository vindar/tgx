# TGX Performance Validation

This directory keeps reusable performance tools, compact baselines, and compact records of past optimization investigations.

Start here:

- `tools/README.md` for upload/capture and parsing tools.
- `baselines/current/README.md` for current reusable benchmark baselines.
- `ARTIFACTS.md` for what should be kept or pruned.
- `investigations/README.md` for the compact history of tested optimization paths.
- `cleanup/` for manifests of raw artifacts that were pruned.

## Current Policy

Keep compact evidence:

- final reports or concise summaries;
- current reusable baseline CSVs;
- accepted/rejected decision notes;
- reusable tooling.

Do not keep persistent Arduino build products, raw serial dumps, full objdump dumps, or large per-candidate matrices in the repository. Temporary raw data is fine while working, but it should be summarized before cleanup.

## Current Baselines

Use:

```text
validation/performance/baselines/current/
```

These are the quickest benchmark CSVs to compare against before deciding whether a new full hardware baseline is required.
