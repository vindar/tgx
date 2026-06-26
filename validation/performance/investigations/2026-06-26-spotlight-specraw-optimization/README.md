# Spotlight specRaw optimization investigation

Date: 2026-06-26

Base commit: `ff8cc0b3` (`optimization`)

Goal: find simple, explainable optimizations for the new point/spot light runtime
without regressing the historical path where spotlights are disabled.

## Accepted change

Accepted optimization: in the local-specular path, compute the raw specular
numerator first:

```cpp
specRaw = dot(N, L) + N.z
```

When `specRaw <= 0`, the existing `_powSpecular(...)` contribution would be zero
anyway, so the code can skip the half-vector `fast_invsqrt()` and table lookup.
This is mathematically equivalent for the tested path and saves useful work on
back-facing or poorly aligned specular samples.

CPU validation against the reference build produced identical hashes for three
frames:

| Frame | Reference hash | Current hash |
|---:|---:|---:|
| 0 | 14661566424370029289 | 14661566424370029289 |
| 1 | 5422464622175942429 | 5422464622175942429 |
| 2 | 9832561839460195462 | 9832561839460195462 |

## Hardware probe result

The spotlight room example with two moving point lights and local specular was
used as the main focused probe.

| Board | Reference fps | specRaw fps | Delta |
|---|---:|---:|---:|
| Pico2 | 28.88 | 29.19 | +1.06% |
| Core2 | 20.96 | 21.15 | +0.9% |
| CoreS3 | 38.15 | 38.27 | +0.31% |
| Teensy 4.1 | 102.58 | 101.96 | -0.6% |

The Teensy result is inside the observed noise for this example. The accepted
change is kept because the operation count reduction is real, the image is
unchanged, and the weaker board gains are repeatable enough to matter.

## Rejected trials

| Trial | Result | Reason |
|---|---|---|
| Hoist `localNormalScale * N.z` outside the light loop | Rejected | Pico2 dropped to about 21.87 fps on the focused example; likely register-pressure/codegen loss larger than the saved multiply. |
| Add global `allSpecular` flag | Rejected | Pico2 dropped to about 15.87 fps and code size grew; the extra global path hurt codegen. |
| Hoist `_spotLights.count` into a local | Rejected | Pico2 measured about 28.67 fps, no real gain over reference. |
| Cone precheck before diffuse/invsqrt | Rejected | The priority point-light path dropped to about 25.3 fps on Pico2 because it enlarged the common path and did not help point lights. |

## Follow-up notes

Do not interpret isolated large gains from mixed point-light vs cone-light
probes as optimization wins unless the rendered scene and active culling are
identical. The reliable comparison is same sketch, same light mode, same board,
same capture method, before and after one code change.

The next possible optimization to study is a normal-transform precompute, but it
touches the model-matrix path and is riskier than the local runtime guards tested
here.
