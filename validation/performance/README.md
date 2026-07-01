# TGX Performance Validation

This directory keeps reusable performance tools, compact baselines, and compact
records of past optimization investigations.

Start here:

- `tools/README.md` for upload/capture and parsing tools.
- `baselines/README.md` for the baseline directory policy.
- `baselines/current/README.md` for the current reusable benchmark baseline.
- `ARTIFACTS.md` for what should be kept or pruned.
- `investigations/README.md` for the compact history of tested optimization paths.
- `cleanup/` for manifests of raw artifacts that were pruned.

## Directory Roles

- `tools/`: reusable scripts only.
- `baselines/current/`: the active compact baseline used for quick comparisons.
- `baselines/previous/`: older compact baselines that are still useful as
  reference points.
- `baselines/archive/`: older long-term snapshots kept for historical context.
- `investigations/`: dated reports and compact per-investigation evidence.
- `cleanup/`: manifests of large raw artifacts that were intentionally pruned.

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
