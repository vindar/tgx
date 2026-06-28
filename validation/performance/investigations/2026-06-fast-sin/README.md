# Fast sine approximation check - 2026-06-27

Function under test:

```cpp
tgx::tgx_fast_sin_deg_clamped(float deg)
```

CPU accuracy test:

- Source: `cpu_fast_sin_error.cpp`
- Command:
  `g++ -std=c++17 -O2 -I src validation/performance/investigations/2026-06-fast-sin/cpu_fast_sin_error.cpp -o validation/performance/investigations/2026-06-fast-sin/cpu_fast_sin_error.exe`
- Reference: `sinf(deg * pi / 180)`
- Domain: `[-180, 180]`, step `0.001` degree, `360001` samples.

Accuracy result on `[-180, 180]`:

| Metric | Value |
|---|---:|
| Mean signed error | `-4.274717437e-13` |
| Mean absolute error | `0.0005051951001` |
| RMS error | `0.0005966867421` |
| Max absolute error | `0.001090615988` |
| Max abs location | `-168.904000 deg` |

Accuracy result on `[0, 180]`:

| Metric | Value |
|---|---:|
| Mean signed error | `4.687952868e-05` |
| Mean absolute error | `0.0005051936973` |
| RMS error | `0.000596685914` |
| Max absolute error | `0.001090615988` |
| Max abs location | `168.904000 deg` |

MCU speed test:

- Source: `FastSinBench/FastSinBench.ino`
- Angles: integer degrees from `-180` to `180`, `361` samples.
- For Teensy4/Core2/CoreS3: `2000` repeats, `722000` calls per run.
- For Pico2/RP2350: `500` repeats, `180500` calls per run.
- Compared against:
  - `sinf(deg * pi / 180)`: realistic replacement for a degree API.
  - `sinf(rad)`: radian input precomputed before the timed loop.

| Board | FQBN | fast sin ns/call | sinf deg-convert ns/call | speedup vs deg-convert | sinf rad-precomputed ns/call | speedup vs rad |
|---|---|---:|---:|---:|---:|---:|
| Teensy 4.1 | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` | `91` | `187` | `2.05x` | `187` | `2.05x` |
| Core2 | `esp32:esp32:m5stack_core2` | `284` | `968` | `3.41x` | `930` | `3.27x` |
| CoreS3 | `esp32:esp32:m5stack_cores3` | `249` | `791` | `3.18x` | `761` | `3.06x` |
| Pico2 | `rp2040:rp2040:rpipico2:opt=Fast` | `201` | `595` | `2.96x` | `575` | `2.86x` |

Raw serial highlights:

```text
Teensy4: fast 66213 us,  sinf deg 135624 us, sinf rad 135621 us, 722000 calls
Core2:   fast 205441 us, sinf deg 699006 us, sinf rad 671819 us, 722000 calls
CoreS3:  fast 179877 us, sinf deg 571277 us, sinf rad 550108 us, 722000 calls
Pico2:   fast 36315 us,  sinf deg 107447 us, sinf rad 103822 us, 180500 calls
```
