# TGX 2D Validation And Benchmark Suites

This directory contains broad 2D API validation and timing suites.

## CPU Suite

Location: `benchmark/2d/cpu`

The CPU suite renders the same 800x600 scene with:

- `RGB24`
- `RGB32`
- `RGBf`

It writes:

- `tgx_2d_cpu_RGB24.bmp`
- `tgx_2d_cpu_RGB32.bmp`
- `tgx_2d_cpu_RGBf.bmp`
- `tgx_2d_cpu_ops_RGB24.bmp`
- `tgx_2d_cpu_ops_RGB32.bmp`
- `tgx_2d_cpu_ops_RGBf.bmp`
- `tgx_2d_cpu_results.csv`

Build and run from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1
```

The script configures, builds, runs the suite, and compares hashes/changed-pixel
counts against `benchmark/2d/baselines/cpu_hashes.csv`. The CSV schema is stable:

```text
schema_version,platform,color_type,group,kind,iterations,total_us,per_iter_us,hash,changed_pixels,ok,note
```

Use this only when a rendering change is intentional and the new output has been
reviewed:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -UpdateBaseline
```

For visual regression checks, the CPU suite can compare the generated BMP
snapshots pixel-by-pixel against archived golden images in
`benchmark/2d/golden/cpu`:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Golden
```

This adds `kind=golden` rows to the CSV. `changed_pixels` is the number of
pixels that differ from the archived image, and the row fails if any pixel is
different. When a comparison fails, a diagnostic BMP is written to
`tmp\tgx_2d_cpu_suite\golden_diff` by default; unchanged pixels are shown as a
dimmed copy of the current image, while changed pixels are amplified color
deltas.

Use this only after reviewing an intentional visual change:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -UpdateGolden
```

The golden pass covers the six regular 800x600 snapshots for `RGB24`, `RGB32`
and `RGBf`. The optional large-frame snapshots are intentionally kept out of the
archived golden set to avoid very large reference files.

For desktop stress/performance passes, add `-Large`. This keeps the regular
800x600 coverage and appends 2048x2048 large-frame groups for gradients,
primitives, blits/textured primitives, text and a full layered scene. It writes
to `tmp\tgx_2d_cpu_suite_large` by default and compares against
`benchmark/2d/baselines/cpu_large_hashes.csv`:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Large
```

The large frame size can be changed for heavier local tests:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Large -LargeSize 4096 -NoBaseline
```

Update the large baseline only after reviewing an intentional rendering change:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Large -UpdateBaseline
```

Recommended CPU validation before committing changes to the 2D suite:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Golden
powershell -ExecutionPolicy Bypass -File benchmark\2d\run_cpu.ps1 -Large
```

The old direct executable form is still supported:

```powershell
.\tmp\build_2d_cpu\Release\tgx_2d_cpu_suite.exe --out tmp\tgx_2d_cpu_suite --baseline benchmark\2d\baselines\cpu_hashes.csv
```

## Teensy 4.1 Suite

Location: `benchmark/2d/teensy4/TGX2DTeensySuite`

The Teensy suite uses a 320x240 `RGB565` framebuffer with the standard
`ILI9341_T4` wiring used by the examples. It displays one page per method group
and prints timing/hash results to USB Serial. Open the serial monitor and send
any character to start the run. At the end of a run, the sketch returns to the
start screen and waits for another character so the same upload can be repeated.

Compile from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\build_teensy.ps1
```

The sketch is intentionally a single upload so the complete 2D pass can run
without repeatedly reprogramming the board.

Upload and run:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\upload_teensy.ps1
powershell -ExecutionPolicy Bypass -File benchmark\2d\read_teensy_serial.ps1
```

Recommended Teensy build validation before committing changes to the 2D suite:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\build_teensy.ps1
powershell -ExecutionPolicy Bypass -File benchmark\2d\build_teensy.ps1 -BuildDir tmp\build_2d_teensy_optional -OptionalSet All
```

The serial output contains human-readable timing lines and machine-readable
`RESULT,...` lines. The reader script writes:

- `tmp\tgx_2d_teensy_results.txt`
- `tmp\tgx_2d_teensy_results.csv`

It also compares the machine-readable results with
`benchmark/2d/baselines/teensy4_hashes.csv`. Update that baseline only after
reviewing an intentional rendering change:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\read_teensy_serial.ps1 -UpdateBaseline
```

## Teensy 4.1 Optional 2D Integrations

Location: `benchmark/2d/teensy4/TGX2DOptionalSuite`

This separate sketch validates and benchmarks TGX wrappers for optional Arduino
libraries when they are installed:

- `PNGdec`
- `JPEGDEC`
- `AnimatedGIF`
- `OpenFontRender`

It is separate from the broad 2D sketch because combining the full suite,
FreeType/OpenFontRender and all image decoders overflows Teensy 4.1 ITCM in the
standard 600MHz / Optimize Fastest configuration. The optional sketch keeps the
same display wiring and CSV format, but only exercises the optional wrappers.

Build, upload and read results:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\build_teensy.ps1 -BuildDir tmp\build_2d_teensy_optional -OptionalSet All
powershell -ExecutionPolicy Bypass -File benchmark\2d\upload_teensy.ps1 -BuildDir tmp\build_2d_teensy_optional -Sketch benchmark\2d\teensy4\TGX2DOptionalSuite
powershell -ExecutionPolicy Bypass -File benchmark\2d\read_teensy_serial.ps1 -OutFile tmp\tgx_2d_teensy_optional_results.txt -CsvOut tmp\tgx_2d_teensy_optional_results.csv -Baseline benchmark\2d\baselines\teensy4_optional_hashes.csv
```

Update the optional baseline only after reviewing an intentional rendering
change:

```powershell
powershell -ExecutionPolicy Bypass -File benchmark\2d\read_teensy_serial.ps1 -OutFile tmp\tgx_2d_teensy_optional_results.txt -CsvOut tmp\tgx_2d_teensy_optional_results.csv -Baseline benchmark\2d\baselines\teensy4_optional_hashes.csv -UpdateBaseline
```
