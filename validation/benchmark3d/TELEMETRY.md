# TGX benchmark3d telemetry format

Telemetry version: `1`.

The format is line-oriented and CSV/JSON-friendly: each line starts with a stable
record type, followed by comma-separated `key=value` fields. Values should not
contain commas.

The automation parser is intentionally strict: malformed fields, missing result
records, duplicate result ids, missing required fields in an OK result, or a
final count mismatch make the run fail instead of producing a partial CSV.

The host manifest written next to each run also records:

- git commit and dirty status;
- board and module JSON configuration;
- exact compile/upload/run/capture commands;
- parsed build sizes when the Arduino log exposes them;
- begin/info/end telemetry records and result count.

Batch runs do not change the serial telemetry format. The host tool aggregates
single-run CSV files into:

- `batch_manifest.json`, listing selected board/module pairs and per-run status;
- `batch_results.csv`, containing all parsed test rows plus the source run
  directory.

## Required record types

### Run start

```text
TGXNB3D_BEGIN,version=1,board=<board>,profile=<profile>,module=<module_id>,module_name=<name>,width=<w>,height=<h>,tests=<count>
```

### Run information

```text
TGXNB3D_INFO,board=<board>,power_class=<n>,fast_arena=<bytes>,bulk_arena=<bytes>,memory_backend=<name>,loaded_shaders=<mask>
```

### Test start

```text
TGXNB3D_TEST_BEGIN,id=<test_id>,name=<name>,shaders=<mask>,min_frames=<n>,max_frames=<n>,target_us=<n>
```

### Progress

Progress lines are optional for short tests and should appear during longer
tests.

```text
TGXNB3D_RUNNING,module=<module_id>,test=<test_id>,frames=<n>,total_us=<n>
```

### Test result

```text
TGXNB3D_RESULT,id=<test_id>,status=OK,frames=<n>,total_us=<n>,mean_us_x100=<n>,min_us=<n>,max_us=<n>,stddev_us_x100=<n>,fps_x100=<n>,checksum=<hex64>
```

Failed or skipped tests use the same record type with explicit status and reason:

```text
TGXNB3D_RESULT,id=<test_id>,status=FAIL,reason=<reason>
TGXNB3D_RESULT,id=<test_id>,status=SKIP,reason=<reason>
```

### Run end

```text
TGXNB3D_END,module=<module_id>,status=OK,tests=<n>,ok=<n>,skip=<n>,fail=<n>
```

The firmware may repeat a short final summary after `TGXNB3D_END`:

```text
TGXNB3D_REPEAT,module=<module_id>,status=OK,ok=<n>,skip=<n>,fail=<n>
```

## Measurement policy

The timed section includes rendering only.

The following are outside the timed section:

- framebuffer clear;
- zbuffer clear;
- allocation;
- scene setup;
- matrix/material/shader setup;
- serial output;
- framebuffer checksum.

`mean_us_x100`, `stddev_us_x100` and `fps_x100` are fixed-point values scaled by
100 to avoid relying on floating-point serial formatting on MCU targets.
