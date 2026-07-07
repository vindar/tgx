# Local Performance Baselines

Baseline data is not stored in the public repository.

Store local baselines under:

```text
validation/performance/local_results/baselines/
```

Recommended layout:

```text
validation/performance/local_results/baselines/
  dev/<commit-or-topic>/
  releases/<version_commit>/
  comparisons/
  current/      # old-format local baselines, if still useful
  previous/     # old-format local baselines, if still useful
  archive/      # old-format local baselines, if still useful
```

A baseline directory should correspond to one exact TGX code state. Do not store
mixed raw data such as `current-vs-main` as a baseline; put comparisons in
`comparisons/` and make them reference two existing baseline directories.

Keep official release baselines complete locally. Keep the current development
baseline complete locally while it is the active reference for ongoing work.
