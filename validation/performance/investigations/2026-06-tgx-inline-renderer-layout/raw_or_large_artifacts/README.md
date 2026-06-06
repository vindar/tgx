# Omitted Raw / Large Artifacts

This archive intentionally keeps compact reports, CSVs, patches, codegen symbol summaries, hardware metadata, and telemetry.

The following bulky artifacts were not copied into `validation/`:

- Arduino build directories under `tmp/hardware_validation/builds/`.
- Arduino build directories under `tmp/config_optimization/builds/`.
- Example build directories under `tmp/config_optimization/example_builds/`.
- Macro probe build directories under `tmp/config_optimization/macro_probe_builds/`.
- Multi-MB objdump dumps such as `*.objdump.txt`.
- Older unrelated `tmp/` performance experiments outside the June 2026 inline/layout investigation chain.

Reason:

These files are large, mostly reproducible from the scripts and source commit, and would make the validation archive noisy for normal repository users.

If deeper forensic analysis is needed, check the original `tmp/` directories on this development machine before cleaning them.
