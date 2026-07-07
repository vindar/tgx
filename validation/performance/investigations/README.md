# Local Performance Investigations

Dated investigation results are not stored in the public repository.

Store local investigation artifacts under:

```text
validation/performance/local_results/investigations/<YYYY-MM-DD-topic>/
```

Suggested local structure:

```text
reports/
summaries/
patches/
codegen/
hardware/
raw_or_large_artifacts/
```

Keep reusable scripts in `validation/performance/tools/` when they are useful
outside one investigation. Keep raw captures, build outputs, codegen dumps and
large matrices local only.
