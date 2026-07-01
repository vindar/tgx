# Arduino Wrapper

This directory contains the local Arduino library used by the benchmark runner.

`TGXBenchmark3D/` is intentionally a thin wrapper: each sketch defines one
`TGX_BENCH_MODULE_*` macro, includes `<TGXBenchmark3D.h>`, and the wrapper pulls
in the common harness plus exactly one benchmark module.

When compiling manually, add this directory as an extra Arduino library path:

```powershell
--libraries D:\Programmation\tgx\validation\benchmark3d\arduino
```

Do not install this wrapper in the global Arduino sketchbook. It belongs to the
validation suite and should stay local to the repository.
