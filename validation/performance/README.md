# TGX Performance Validation

This directory contains reusable performance-validation tooling and documentation.
It must not contain persistent benchmark results in the public repository.

## Public, Tracked Content

- `tools/`: reusable upload, capture, parsing, aggregation and comparison tools.
- `README.md`: how performance validation is organized.
- `ARTIFACTS.md`: what belongs in Git and what belongs only locally.

## Local, Ignored Content

All benchmark outputs, example telemetry, release baselines, development
baselines, comparison reports, investigation artifacts, serial logs and build
products must be stored under:

```text
validation/local_results/performance/
```

That directory is ignored by Git. It is the canonical local workspace for
performance data on the development machine.

Recommended layout:

```text
validation/local_results/performance/
  baselines/
    dev/
    releases/
    comparisons/
  investigations/
  cleanup/
  builds/
  notes/
```

Keep release baselines complete locally under `baselines/releases/`. Keep the
current development baseline complete locally under `baselines/dev/` while it is
the active comparison reference. Store comparisons separately under
`baselines/comparisons/`, referencing two exact baseline directories. Prune
temporary raw logs and build directories once compact summaries or reports
preserve the decision.

Do not create public placeholder directories for generated data. If a directory
only explains where local results should go, document that policy here instead.
