# ARM DSP Capability Probe

Probe sketch:

```text
validation/performance/investigations/2026-06-arm-dsp-rgb565/sketches/ArmDspProbe/
```

Capture tool:

```text
validation/performance/tools/upload_and_capture.ps1
```

The first Teensy attempt tried to include `cmsis_gcc.h` directly and failed because the Teensy core already defines IRQ helpers in `imxrt.h`. The probe now avoids direct `cmsis_gcc.h` inclusion and tests the relevant ARM DSP instructions through local inline-asm wrappers. This is suitable for codegen/capability testing, but it means named CMSIS wrappers such as `__UHADD16` are not directly available through `Arduino.h` alone on Teensy.

## Summary

| Board | Port | Upload/capture | Architecture macros | DSP/SIMD macro | FP macro | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Teensy 4.1 | `COM3` | SUCCESS | `__ARM_ARCH_7EM__=1` | `__ARM_FEATURE_DSP=1`, `__ARM_FEATURE_SIMD32=1` | `__ARM_FP=0xE` | Cortex-M7 path; all tested asm DSP instructions worked. |
| Pico2 / RP2350 | `COM21` | SUCCESS | `__ARM_ARCH_8M_MAIN__=1` | `__ARM_FEATURE_DSP=1`, `__ARM_FEATURE_SIMD32=1` | `__ARM_FP=0x4` | Cortex-M33 path; all tested asm DSP instructions worked. |

## Tested Instructions

Both boards produced matching scalar-reference and asm results for:

- `UHADD16`
- `UADD16`
- `USUB16`
- `UQADD16`
- `SMLAD`
- `SMUAD`
- `PKHBT`
- `PKHTB`
- `USAT`
- `USAT16`
- `REV16`

Reference result sample:

```text
SCALAR_UHADD16_REF = 0x50007
SCALAR_UADD16_REF  = 0xA000E
SCALAR_USUB16_REF  = 0x40004
SCALAR_UQADD16_REF = 0xFFFFFFFF
SCALAR_SMLAD_REF   = 0x5
SCALAR_SMUAD_REF   = 0xFFFFFFFE
SCALAR_PKHBT_REF   = 0x56781234
SCALAR_PKHTB_REF   = 0xAAAABBBB
ASM_USAT           = 0xFF
ASM_USAT16         = 0xFF0000
ASM_REV16          = 0x22114433
```

Raw telemetry:

- `results/raw_capture/telemetry/teensy41_arm_dsp_probe_teensy41.txt`
- `results/raw_capture/telemetry/pico2_arm_dsp_probe_pico2.txt`

Metadata:

- `results/raw_capture/parsed/teensy41_arm_dsp_probe_teensy41.json`
- `results/raw_capture/parsed/pico2_arm_dsp_probe_pico2.json`
