# Hot3D ARM DSP Hardware Probe

Probe sketch reused:

```text
validation/performance/investigations/2026-06-arm-dsp-rgb565/sketches/ArmDspProbe/
```

Capture tool:

```text
validation/performance/tools/upload_and_capture.ps1
```

## Results

| Board | Port | Upload | Serial capture | Lines | Start/end markers | DSP32 |
| --- | --- | --- | --- | ---: | --- | --- |
| Teensy 4.1 | `COM3` | OK | OK | 31 | OK | yes |
| Pico2 / RP2350 | `COM21` | OK | OK | 31 | OK | yes |

## Teensy 4.1

Key macros:

```text
BOARD=TEENSY41
TGX_INLINE=__attribute__((always_inline))
TGX_INLINE_ZDIVIDE=__attribute__((always_inline))
__ARM_ARCH_7EM__=1
__ARM_ARCH_8M_MAIN__=0
__ARM_FEATURE_DSP=1
__ARM_FEATURE_SIMD32=1
__ARM_FP=0xE
```

Runtime intrinsic checks compiled and matched scalar reference values:

```text
ASM_UHADD16, ASM_UADD16, ASM_USUB16, ASM_UQADD16,
ASM_SMLAD, ASM_SMUAD, ASM_PKHBT, ASM_PKHTB,
ASM_USAT, ASM_USAT16, ASM_REV16
```

## Pico2 / RP2350

Key macros:

```text
BOARD=PICO2_RP2350
TGX_INLINE=__attribute__((always_inline))
TGX_INLINE_ZDIVIDE=__attribute__((always_inline))
__ARM_ARCH_7EM__=0
__ARM_ARCH_8M_MAIN__=1
__ARM_FEATURE_DSP=1
__ARM_FEATURE_SIMD32=1
__ARM_FP=0x4
```

Runtime intrinsic checks compiled and matched scalar reference values:

```text
ASM_UHADD16, ASM_UADD16, ASM_USUB16, ASM_UQADD16,
ASM_SMLAD, ASM_SMUAD, ASM_PKHBT, ASM_PKHTB,
ASM_USAT, ASM_USAT16, ASM_REV16
```

## Raw Capture

The upload/capture script initially wrote raw files under `validation/performance/{logs,telemetry,parsed}`. They are archived into this investigation at cleanup time under:

```text
results/raw_capture/
```

