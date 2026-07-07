# TGX Performance Validation

This directory contains reusable performance-validation tooling and documentation.
It must not contain persistent benchmark results in the public repository.

## Public, Tracked Content

- `tools/`: reusable upload, capture, parsing, aggregation and comparison tools.
- `baselines/README.md`: local baseline layout policy.
- `investigations/README.md`: local investigation layout policy.
- `cleanup/README.md`: local cleanup-note policy.
- `ARTIFACTS.md`: what belongs in Git and what belongs only locally.

## Local, Ignored Content

All benchmark outputs, example telemetry, release baselines, development
baselines, comparison reports, investigation artifacts, serial logs and build
products must be stored under:

```text
validation/performance/local_results/
```

That directory is ignored by Git. It is the canonical local workspace for
performance data on the development machine.

Recommended layout:

```text
validation/performance/local_results/
  baselines/
    dev/
    releases/
    comparisons/
    current/      # old-format local baselines, if still useful
    previous/     # old-format local baselines, if still useful
    archive/      # old-format local baselines, if still useful
  investigations/
  cleanup/
  builds/
  notes/
```

Keep release baselines complete locally. Keep the current development baseline
complete locally while it is the active comparison reference. Prune temporary
raw logs and build directories once compact summaries or reports preserve the
decision.
