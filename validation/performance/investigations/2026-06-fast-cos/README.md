# Fast cosine approximation check - 2026-06-27

Function under test:

```cpp
tgx::tgx_fast_cos_deg_clamped(float deg)
```

CPU accuracy test:

- Source: `cpu_fast_cos_error.cpp`
- Command:
  `g++ -std=c++17 -O2 -I src validation/performance/investigations/2026-06-fast-cos/cpu_fast_cos_error.cpp -o validation/performance/investigations/2026-06-fast-cos/cpu_fast_cos_error.exe`
- Reference: `cosf(deg * pi / 180)`
- Domain: `[-180, 180]`, step `0.001` degree, `360001` samples.

Accuracy result on `[-180, 180]`:

| Metric | Value |
|---|---:|
| Mean signed error | `2.709369702e-11` |
| Mean absolute error | `0.0005051949632` |
| RMS error | `0.0005966826115` |
| Max absolute error | `0.001090407372` |
| Max abs location | `-101.090000 deg` |

The `[0, 180]` sweep gives the same practical result:

| Metric | Value |
|---|---:|
| Mean signed error | `2.850094583e-11` |
| Mean absolute error | `0.0005051935594` |
| RMS error | `0.0005966817821` |
| Max absolute error | `0.001090407372` |
| Max abs location | `78.924000 deg` |

MCU speed test:

- Source: `FastCosBench/FastCosBench.ino`
- Angles: integer degrees from `-180` to `180`, `361` samples.
- For Teensy4/Core2/CoreS3: `2000` repeats, `722000` calls per run.
- For Pico2/RP2350: `500` repeats, `180500` calls per run.
- Compared against:
  - `cosf(deg * pi / 180)`: realistic replacement for a degree API.
  - `cosf(rad)`: radian input precomputed before the timed loop.

| Board | FQBN | fast cos ns/call | cosf deg-convert ns/call | speedup vs deg-convert | cosf rad-precomputed ns/call | speedup vs rad |
|---|---|---:|---:|---:|---:|---:|
| Teensy 4.1 | `teensy:avr:teensy41:usb=serial,speed=600,opt=o3std` | `99` | `186` | `1.88x` | `186` | `1.88x` |
| Core2 | `esp32:esp32:m5stack_core2` | `280` | `962` | `3.44x` | `925` | `3.30x` |
| CoreS3 | `esp32:esp32:m5stack_cores3` | `251` | `791` | `3.15x` | `761` | `3.03x` |
| Pico2 | `rp2040:rp2040:rpipico2:opt=Fast` | `194` | `582` | `3.00x` | `561` | `2.89x` |

Raw serial highlights:

```text
Teensy4: fast 71697 us, cosf deg 134767 us, cosf rad 134764 us, 722000 calls
Core2:   fast 202464 us, cosf deg 695015 us, cosf rad 667862 us, 722000 calls
CoreS3:  fast 181409 us, cosf deg 571123 us, cosf rad 549981 us, 722000 calls
Pico2:   fast 35030 us,  cosf deg 105053 us, cosf rad 101433 us, 180500 calls
```
