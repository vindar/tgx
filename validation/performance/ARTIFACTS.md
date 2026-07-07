# Performance Validation Artifacts

The public TGX repository should contain reproducible validation code, not the
results produced on one workstation.

## Commit To Git

Commit only reusable material:

- benchmark harness and modules;
- upload/capture/parser/aggregation scripts;
- small documentation files explaining how to run or interpret the tools;
- small source fixtures required by the tools.

## Keep Local Only

Keep all generated data under `validation/performance/local_results/`, which is
ignored by Git:

- full benchmark baselines;
- release and development telemetry;
- comparison CSV/Markdown reports;
- dated investigation folders;
- serial logs and raw captures;
- generated build directories;
- codegen dumps, objdump/nm output and large matrices;
- temporary notes from optimization sessions.

## Local Layout

Use this local structure:

```text
validation/performance/local_results/
  baselines/
    dev/<commit-or-topic>/
    releases/<version_commit>/
    comparisons/
  investigations/<YYYY-MM-DD-topic>/
  cleanup/
  builds/
  notes/
```

Release baselines are kept complete locally. The current development baseline is
also kept complete locally while it is the active reference. Older exploratory
data should be pruned once a compact local report preserves the decision.

## Public Placeholders

The tracked directories `baselines/`, `investigations/` and `cleanup/` contain
README files only. They document where local data should be stored, but they are
not the storage location for generated results.
