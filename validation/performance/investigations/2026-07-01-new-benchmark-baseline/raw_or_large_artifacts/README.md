# Raw Artifacts

Large raw logs and per-run telemetry copies were intentionally pruned after the
run was validated. The compact CSV files under `summaries/`, plus the report and
manifest, preserve the data needed for follow-up investigations.

The first benchmark attempts were invalidated because the Arduino build could
pick the global TGX mirror instead of the repo under test. Those raw logs were
also removed; only the valid `*_local_*` benchmark CSV files should be used.
