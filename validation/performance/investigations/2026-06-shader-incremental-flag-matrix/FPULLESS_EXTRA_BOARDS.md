# FPU-less extra board check

Date: 2026-06-15

Purpose:

- Compare the committed `HEAD` shader path with the current `src/Shaders.h` candidate on boards without FPU.
- Candidate flags forced to:
  - `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=1`
  - `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=1`

Boards requested:

| Board | Port | Example | Status |
| --- | --- | --- | --- |
| Pico W + ILI9341 | `COM19` | `examples/Pico_RP2040_RP2350/bunny_fig` | completed |
| Feather S2 + ILI9341 | `COM11` | `examples/ESP32/naruto` | blocked: Windows refuses to open `COM11` for upload |

## Pico W result

Build profile:

- `rp2040:rp2040:rpipicow`
- TFT_eSPI setup: `Setup_RP2040_RP2350_ILI9341.h`

Initial result, candidate `cand_11` vs `HEAD`:

| Scene | HEAD frame avg | cand_11 frame avg | Delta |
| --- | ---: | ---: | ---: |
| `flat` | 130111.743 us | 122172.800 us | -6.102% |
| `gouraud` | 156622.593 us | 190254.386 us | +21.473% |
| `gouraud_texture` | 215089.890 us | 227208.590 us | +5.634% |
| `wireframe` | 77099.892 us | 77590.332 us | +0.636% |

Interpretation:

- On Pico W / RP2040 without FPU, forcing both incremental-float flags is not good.
- The flat gain is incidental and not enough to compensate.
- The main Gouraud path regresses badly, and the textured Gouraud path also regresses.
- This suggests that Pico W should not inherit the same default as Pico2/RP2350 without separate flag selection.

## Pico W flag matrix

The missing `00`, `01`, and `10` variants were then measured on the same `bunny_fig` example.

Flags:

| Variant | `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL` | `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL` |
| --- | ---: | ---: |
| `cand_00` | 0 | 0 |
| `cand_01` | 0 | 1 |
| `cand_10` | 1 | 0 |
| `cand_11` | 1 | 1 |

Delta vs `HEAD`:

| Variant | Flat | Gouraud | Gouraud texture | Wireframe |
| --- | ---: | ---: | ---: | ---: |
| `cand_00` | -4.776% | -7.007% | -8.744% | -0.158% |
| `cand_01` | -2.998% | +21.949% | -7.248% | +0.093% |
| `cand_10` | -5.864% | -6.617% | +11.434% | +0.199% |
| `cand_11` | -6.102% | +21.473% | +5.634% | +0.636% |

Conclusion for Pico W:

- `TGX_SHADER_GOURAUD_RGB565_FLOAT_INCREMENTAL=1` is bad on RP2040: non-textured Gouraud regresses by about +21%.
- `TGX_SHADER_GOURAUD_TEXTURE_FLOAT_INCREMENTAL=1` is also bad for the textured Gouraud path on RP2040: +11.4% alone, +5.6% combined with the RGB565 flag.
- The best Pico W choice is therefore `cand_00`, i.e. keep the candidate source shape but disable both incremental-float paths on RP2040.
- `cand_00` is still faster than `HEAD` on all measured `bunny_fig` scenes, including -7.0% on Gouraud and -8.7% on Gouraud texture.

Binary size on Pico W `bunny_fig`:

| Variant | Sketch bytes | Globals bytes |
| --- | ---: | ---: |
| `HEAD` | 572052 | 205884 |
| `cand_00` | 571740 | 205884 |
| `cand_01` | 572028 | 205884 |
| `cand_10` | 571956 | 205884 |
| `cand_11` | 572244 | 205884 |

Retained CSVs:

- `results/fpuless_picow_feathers2_summary.csv`
- `results/fpuless_picow_feathers2_delta_vs_head.csv`
- `results/picow_flag_matrix_summary.csv`
- `results/picow_flag_matrix_delta_vs_head.csv`
- `results/picow_flag_matrix_best_variant.csv`
- `results/picow_flag_matrix_binary_size.csv`

## Feather S2 status

TFT_eSPI was switched to:

- `Setup_feather_TFT_S2_S3_ILI9341.h`

The Naruto sketch compiled, but upload failed twice:

```text
A fatal error occurred: Could not open COM11, the port is busy or doesn't exist.
Cannot configure port, something went wrong.
Original message: PermissionError(13, 'Un périphérique attaché au système ne fonctionne pas correctement.', None, 31)
```

A direct .NET serial open test also failed with:

```text
Un périphérique attaché au système ne fonctionne pas correctement.
```

Next action:

- Put Feather S2 in bootloader mode or unplug/replug it, then rerun the `HEAD` and `cand_11` Naruto comparison.

Note:

- After the Pico W candidate run, TFT_eSPI was switched back to `Setup_RP2040_RP2350_ILI9341.h`.
