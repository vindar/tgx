# Shader Fixed-Point Integration Report

Date: 2026-06-06

Branch: `feature/multi-directional-lights`

Start commit: `2453846a3028c4a1adbf0c037edbd20f9db73452`

## A. Executive Summary

No production source patch is retained.

The previous fixed-point texture-coordinate equivalence result was real, and the
restricted nearest subset is correct. A forced CPU 3D strict validation build
with `TGX_SHADER_USE_FIXEDPOINT_TEXCOORDS=1` passed `82 PASS, 0 FAIL`.

However, every production `src/Shaders.h` candidate that default-enabled the
fast path caused unacceptable Pico2 benchmark regressions outside the intended
nearest path. The best narrowed Pico2 candidate still regressed global scores by
`-3.21%`, `-2.66%`, and `-2.14%`, with bilinear texture rows down as much as
`-21.34%`.

Nearest-only integration is therefore rejected for now. Bilinear integration was
not attempted because nearest did not pass the staged benchmark safety gate.

## B. Previous Fixed-Point Evidence

The preceding texture-coordinate investigation found a viable restricted subset:

- `USE_TEXTURE && USE_ORTHO`;
- affine / non-perspective coordinates;
- whole span inside the texture;
- fixed 16 fractional bits;
- length threshold `len >= 8`;
- nearest exact in tested clamp/interior cases;
- bilinear approximate but bounded in tested clamp/interior cases.

That evidence was used to build the production candidates here, but production
shader layout/codegen changed the outcome.

## C. Integration Design

Audited integration point:

```text
src/Shaders.h
uber_shader<color_t, ZBUFFER_t, USE_ZBUFFER, USE_GOURAUD, USE_TEXTURE,
            USE_ORTHO, TEXTURE_BILINEAR, TEXTURE_WRAP, USE_UNLIT>
```

The candidate replaced only the nearest texture sampling path when all guards
held:

- `USE_TEXTURE`;
- `USE_ORTHO`;
- nearest only, not bilinear;
- clamp only, not wrap;
- span length at least 8;
- float endpoints inside texture;
- fixed endpoints inside texture.

Fallback remained the original float path for perspective, wrap, bilinear,
short spans, and edge-risk spans.

## D. Render-Difference Validation

Sketch:

```text
sketches/ShaderFixedPointRenderDiff/
```

Parser:

```text
tools/parse_shader_fixedpoint_diff.py
```

Summary from `results/render_diff.csv`:

- Nearest clamp/interior cases matched exactly on Teensy and Pico2.
- Perspective fallback cases used no fast pixels and matched exactly.
- Nearest wrap was not exact: 2 differing pixels, max channel error 34.
- Bilinear clamp/interior was bounded: max channel error 1.
- Bilinear wrap was bounded in this sketch but not considered safe for the first
  production patch.

The production candidate was therefore restricted to nearest + clamp.

## E. Benchmark Results

All benchmark comparisons below use same-source controls built with
`-DTGX_SHADER_USE_FIXEDPOINT_TEXCOORDS=0`.

### Pico2 Candidate A: Inline Setup

Patch:

```text
patches/shader_fixedpoint_nearest_candidate.patch
```

Global scores:

- score 1: `23.04 -> 20.94` (`-9.11%`)
- score 2: `16.55 -> 15.51` (`-6.28%`)
- score 3: `14.01 -> 13.31` (`-5.00%`)

Important gains:

- `Bunny ORTHO TEX`: up to `+74.53%`
- `R2D2 TEX_NEAREST`: about `+4.3%` to `+4.8%`
- `Bunny TEX_NEAREST`: about `+4.6%` to `+5.0%`

Rejected because hidden regressions were severe:

- `R2D2 TEX_BILINEAR`: down to `-43.59%`
- `Bunny TEX_BILINEAR`: down to `-38.51%`
- thick wire rows: down to about `-23%`

### Pico2 Candidate B/C: Noinline Setup Helper

Patches:

```text
patches/shader_fixedpoint_nearest_helper_candidate.patch
patches/shader_fixedpoint_nearest_helper_late_candidate.patch
```

Moving the helper definition later did not change codegen, so only one runtime
run was needed.

Global scores:

- score 1: `23.04 -> 22.12` (`-3.99%`)
- score 2: `16.55 -> 15.99` (`-3.38%`)
- score 3: `14.01 -> 13.63` (`-2.71%`)

Still rejected:

- `Bunny TEX_BILINEAR`: down to `-25.21%`
- `R2D2 TEX_BILINEAR`: down to `-24.36%`
- thick wire rows: down to about `-7%`

### Pico2 Candidate D: Flat/Unlit Only

Patch:

```text
patches/shader_fixedpoint_nearest_flatonly_candidate.patch
```

Global scores:

- score 1: `23.04 -> 22.30` (`-3.21%`)
- score 2: `16.55 -> 16.11` (`-2.66%`)
- score 3: `14.01 -> 13.71` (`-2.14%`)

This preserved some orthographic texture gains:

- `Bunny ORTHO TEX`: `+15.79%`, `+16.96%`, `+20.86%`

But it still failed safety gates:

- `Bunny TEX_BILINEAR`: down to `-21.34%`
- `R2D2 TEX_BILINEAR`: down to `-18.29%`
- `Grid small FLAT`: about `-3%`

### Teensy Candidate B/C

Same helper-late candidate.

Global scores:

- score 1: `126.01 -> 126.01` (`+0.00%`)
- score 2: `92.74 -> 92.73` (`-0.01%`)
- score 3: `80.54 -> 80.56` (`+0.02%`)

Detailed rows:

- mean delta: `-0.07%`
- median delta: `+0.01%`
- rows `< -3%`: 0
- rows `< -1%`: 4

Teensy was effectively neutral, with no compelling texture-nearest gain.

## F. Threshold / Precision Matrix

No production threshold/precision matrix was expanded beyond fixed16 and
`len >= 8`, because the first nearest candidates failed Pico2 benchmark safety.

The previous equivalence run showed fixed20 did not solve nearest exactness
cleanly and broke the bilinear sketch path because the bilinear fixed extractor
was hardcoded for 16 fractional bits.

## G. Codegen Analysis

Codegen artifacts are under `codegen/`.

Pico2 code-size observations:

- disabled control: `1358560` program bytes;
- inline setup: `1359688` (`+1128`);
- helper setup: `1359264` (`+704`);
- flat/unlit-only: `1359168` (`+608`).

Symbol deltas:

- Inline setup added `+566` and `+554` bytes to the two orthographic nearest
  texture shader instantiations.
- Helper setup reduced that to `+116` and `+92` bytes.
- Flat/unlit-only left only one nearest shader instantiation at `+116` bytes.
- Even when bilinear shader symbol sizes were unchanged, later hot symbols moved
  by `+608` to `+1124` bytes.

Conclusion: the fast-path logic works locally, but adding it to central shader
templates perturbs Pico2 flash/cache/layout enough to damage unrelated hot paths.
The bilinear regressions are not a semantic bilinear change; they are code
layout/code placement fallout from the extra nearest-path code.

## H. CPU Validation

Forced production-path validation:

```text
cmake -S validation\3d\cpu -B tmp\build_validation_3d_cpu_shader_fp -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_FLAGS="/DTGX_SHADER_USE_FIXEDPOINT_TEXCOORDS=1"
cmake --build tmp\build_validation_3d_cpu_shader_fp --config Release
tmp\build_validation_3d_cpu_shader_fp\Release\tgx_3d_cpu_suite.exe --out validation\performance\investigations\2026-06-shader-fixedpoint-integration\results\cpu_3d_shader_fp --compare strict --baseline validation\3d\baselines\cpu_validation.csv
```

Result:

```text
82 PASS, 0 FAIL
```

CPU 2D validation was not run after final cleanup because no production source
patch is retained.

## I. Real Examples

Real examples were not run because no production candidate passed benchmark
safety gates. Running example telemetry for a rejected source patch would not
change the integration decision.

## J. Final Source Diff

No production source changes are retained. The only intended working-tree
changes are investigation artifacts under:

```text
validation/performance/investigations/2026-06-shader-fixedpoint-integration/
```

Final checks:

```text
git status --short
?? validation/performance/investigations/2026-06-shader-fixedpoint-integration/

git diff --stat
<empty>

git diff --name-only
<empty>

git diff --check
<clean>
```

Large CPU validation BMP outputs were removed from the artifact folder after the
strict validation run. The validation CSV remains in
`results/cpu_3d_shader_fp/tgx_3d_cpu_results.csv`.

## K. Rejected Candidates

- `shader_fixedpoint_nearest_candidate.patch`: correct but too much template
  growth and severe Pico2 layout regressions.
- `shader_fixedpoint_nearest_helper_candidate.patch`: smaller, but still caused
  unacceptable Pico2 bilinear and wire regressions.
- `shader_fixedpoint_nearest_helper_late_candidate.patch`: same codegen as the
  helper candidate; no separate runtime benchmark needed.
- `shader_fixedpoint_nearest_flatonly_candidate.patch`: narrower, but still
  caused unacceptable Pico2 bilinear regressions and global losses.

## L. Next Steps

The fixed-point coordinate idea remains useful, but it should not be integrated
by adding branches to the central `uber_shader` template.

Better next directions:

- build a dedicated orthographic nearest shader specialization outside the
  existing hot template cluster;
- investigate linker/function ordering or code-section placement only as a
  deliberate, measured tool, not as incidental layout drift;
- create a production render-hash benchmark that compares two full renderer
  builds for selected examples;
- revisit bilinear only after a nearest path can be integrated without unrelated
  layout regressions.
