# CoreS3 JPEG Framebuffer Hash Difference

The CoreS3 framebuffer hash differs from Core2, Teensy 4.1, and Pico2 for the
same JPEG visual test:

| Board/build | Hash | Nonzero pixels | Decode time |
| --- | --- | ---: | ---: |
| Core2 default | `93FA7269` | 51524 | 37051 us |
| Teensy 4.1 default | `93FA7269` | 51524 | 5081 us |
| Pico2 default | `93FA7269` | 51524 | 37794 us |
| CoreS3 default | `E984A9E2` | 52092 | 21276 us |
| CoreS3 with `-DNO_SIMD` | `93FA7269` | 51524 | 29186 us |

Conclusion: the difference is caused by JPEGDEC's ESP32-S3 SIMD color
conversion path, not by TGX's JPEG row-copy optimization.

Evidence:

- JPEGDEC's `README.md` states that the library includes SIMD optimized color
  conversion for ESP32-S3.
- `JPEGDEC/src/jpeg.inl` enables `ESP32S3_SIMD` when `ARDUINO_ARCH_ESP32` is
  defined, `NO_SIMD` is not defined, and the ESP-DSP AES3 SIMD platform support
  macro is enabled.
- `JPEGDEC/src/s3_simd_420.S` and `JPEGDEC/src/s3_simd_444.S` implement
  YCbCr-to-RGB565 conversion using ESP32-S3 vector instructions.
- Rebuilding the same CoreS3 sketch with `-DNO_SIMD` disables that path and
  produces the same framebuffer hash as Core2/Teensy/Pico2.

This is not an FPU approximation in TGX. The differing path is integer/SIMD
YCbCr-to-RGB565 conversion inside JPEGDEC. It appears to be a small rounding or
fixed-point conversion difference between JPEGDEC's portable scalar conversion
and its ESP32-S3 SIMD conversion. Visually the image looked correct on CoreS3.

If bit-identical framebuffer hashes across boards are required, compile the
CoreS3 JPEGDEC path with `-DNO_SIMD`, at the cost of a slower decode. For normal
display use, the default SIMD path is faster and visually acceptable based on
this validation pass.
