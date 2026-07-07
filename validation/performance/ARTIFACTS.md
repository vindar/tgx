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

Keep all generated data under `validation/local_results/performance/`, which is
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
validation/local_results/performance/
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

Do not create public placeholder directories for generated data. Keep the
tracked tree simple: reusable tools and documentation live here; generated
baselines, reports and logs live under the ignored `local_results/` tree.
