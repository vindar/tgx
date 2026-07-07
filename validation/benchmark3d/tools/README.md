# Benchmark Tools

`run_bench3d.py` is the main orchestration script for the modular 3D benchmark
suite.

It reads board and module definitions from `config/`, compiles the selected
target, uploads when needed, captures telemetry, validates markers and writes
CSV/JSON results under repository-level `tmp/benchmark3d/`.

`tmp/benchmark3d/` is transient output. When a run becomes an official local
baseline or comparison reference, copy or aggregate the compact results under
`validation/local_results/performance/` using the layout documented in
`validation/performance/README.md`.

Use the TGX conda Python environment:

```powershell
& "C:\Users\Vindar\anaconda3\envs\tgxmesh3d2\python.exe" validation\benchmark3d\tools\run_bench3d.py --list
```
