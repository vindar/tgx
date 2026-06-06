# Hardware Probe

Probe sketch:

```text
validation/performance/tools/sketches/TgxMacroProbe
```

Capture tool:

```text
validation/performance/tools/upload_and_capture.ps1
```

## Results

| Board | Port | Upload | Serial capture | Lines | Notes |
| --- | --- | --- | --- | ---: | --- |
| Teensy 4.1 | `COM3` | OK | OK | 21 | Teensy upload port `usb:80000/3/0/1`; extra post-upload wait used. |
| Pico2 / RP2350 | `COM21` | OK | OK | 21 | UF2 upload completed and serial returned on `COM21`. |

## Macro Highlights

| Board | `TGX_INLINE` | `TGX_INLINE_ZDIVIDE` | `TGX_SHADER_USE_INCREMENTAL_PIXEL_POINTERS` | Fast math |
| --- | --- | --- | ---: | --- |
| Teensy 4.1 | `__attribute__((always_inline))` | `__attribute__((always_inline))` | 0 | fast inv/sqrt/invsqrt and FMA enabled |
| Pico2 / RP2350 | `__attribute__((always_inline))` | `__attribute__((always_inline))` | 1 | fast inv/sqrt/invsqrt enabled, FMA disabled |

Raw captures were moved under `results/raw_capture/` at the end of the session.
