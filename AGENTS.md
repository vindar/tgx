# AGENTS.md — Instructions for Codex on the tgx graphics library

This repository contains **tgx**, a C++ graphics library for embedded systems, especially Arduino-compatible boards and Teensy 4.x.

The library provides 2D/3D graphics primitives, image manipulation, rasterization, drawing routines, color handling, and possibly display-oriented utilities for constrained environments.

Codex must treat this project as an **embedded C++ graphics library**, not as a desktop C++ application.

---

## 1. General working rules

- Start every task by understanding the relevant code before proposing changes.
- Do not modify files unless explicitly asked.
- Prefer read-only analysis first.
- When proposing a fix, show a minimal diff before applying it.
- Keep patches small, localized, and easy to review.
- Do not perform broad refactorings unless explicitly requested.
- Do not change the public API unless explicitly requested.
- Preserve backward compatibility with existing user code whenever possible.
- When unsure, ask for confirmation or produce an analysis rather than changing code.
- Always classify findings by severity:
  - Critical bug / undefined behavior
  - Probable bug
  - Portability issue
  - Performance issue
  - API/design issue
  - Style/maintainability issue

---

## 2. Target platforms and constraints

This library targets microcontrollers and embedded boards, especially:

- Teensy 4.x / ARM Cortex-M7
- Arduino-compatible platforms
- Possibly boards with limited RAM, flash, and stack

Assume that the code may run in memory-constrained and performance-sensitive environments.

Important constraints:

- Avoid unnecessary dynamic allocation.
- Avoid introducing exceptions.
- Avoid RTTI.
- Avoid `<iostream>`.
- Avoid heavy STL constructs unless the project already uses them in the relevant module.
- Avoid `std::function`, heap-allocating containers, and complex abstractions in hot paths.
- Be careful with stack usage, especially large local arrays.
- Prefer deterministic, allocation-free code in rendering paths.
- Prefer explicit, simple code over elegant but costly abstractions.
- Preserve compatibility with Arduino/Teensy toolchains.

If the existing code intentionally avoids parts of the C++ standard library, do not introduce them.

---

## 3. C++ standard and portability

Before changing code, infer the C++ standard currently expected by the project.

Default assumption unless the project says otherwise:

- C++11 or C++14 compatibility is preferred.
- Do not require C++17/C++20 features unless explicitly allowed.
- Avoid non-portable compiler extensions unless already used.
- Be cautious with GCC-specific attributes or intrinsics.
- When using Teensy-specific or ARM-specific optimizations, isolate them behind clear preprocessor guards.

Check portability across:

- Arduino IDE / Teensyduino
- PlatformIO if applicable
- GCC / ARM GCC
- Desktop builds if the project supports simulation or tests

---

## 4. Public API preservation

The public API is especially important.

Before modifying a header in `include/`, `src/`, or any public-facing file:

- Identify whether the change affects user code.
- Do not rename public types, functions, methods, fields, macros, or enum values without explicit approval.
- Do not change function signatures without explicit approval.
- Do not change template parameters without explicit approval.
- Do not change binary layout of public structs/classes unless clearly internal or explicitly approved.
- Do not remove overloads.
- Do not silently alter semantics of drawing functions.

If an API improvement is desirable, propose it separately under "Possible API improvement", but do not implement it automatically.

---

## 5. Graphics-specific correctness rules

When reviewing or modifying drawing code, pay special attention to:

- Pixel bounds and clipping.
- Off-by-one errors in rectangles, lines, triangles, and image regions.
- Inclusive vs exclusive coordinate conventions.
- Integer overflow in coordinate calculations.
- Signed/unsigned conversions.
- Negative coordinates.
- Degenerate primitives:
  - zero-width rectangles
  - zero-height rectangles
  - zero-length lines
  - degenerate triangles
  - empty images
  - null or invalid buffers
- Correct handling of image stride / pitch if applicable.
- Correct handling of color formats.
- Correct behavior at screen or image boundaries.
- Clipping before memory access.
- Avoiding writes outside framebuffer/image buffers.

Never assume coordinates are valid unless the API explicitly requires it.

---

## 6. 2D geometry and rasterization

For 2D rendering code, verify:

- Line drawing correctness for all octants.
- Horizontal and vertical line special cases.
- Rectangle fill boundaries.
- Circle/ellipse symmetry and boundary behavior.
- Triangle filling rules.
- Text rendering bounds.
- Blitting bounds.
- Alpha blending and color interpolation if present.
- Consistency between filled and outline primitives.
- Whether endpoints are meant to be included or excluded.

When modifying rasterization code:

- Preserve exact pixel conventions unless the current behavior is demonstrably wrong.
- Do not "simplify" algorithms if it changes rendered output subtly.
- Mention any possible change in pixel-level output.

---

## 7. 3D graphics rules

For 3D code, pay attention to:

- Matrix/vector conventions:
  - row-major vs column-major
  - left-handed vs right-handed coordinates
  - pre-multiplication vs post-multiplication
- Transform order.
- Projection conventions.
- Near/far clipping.
- Perspective divide safety.
- Division by zero or very small values.
- Backface culling orientation.
- Triangle winding order.
- Z-buffer precision and initialization.
- Fixed-point vs floating-point assumptions.
- Overflow in intermediate calculations.
- Degenerate triangles.
- Texture coordinate bounds if texturing exists.
- Normal interpolation and lighting if present.

Before changing 3D math, first document the existing convention inferred from the code.

---

## 8. Numeric correctness

This library may use integer arithmetic, fixed-point arithmetic, and floating-point arithmetic.

When reviewing numeric code, check:

- Overflow in multiplications such as width * height, x + y * stride, color interpolation, barycentric coordinates, matrix operations.
- Loss of precision from integer division.
- Signed/unsigned comparison bugs.
- Undefined behavior from signed overflow.
- Shift operations with invalid shift counts.
- Rounding behavior.
- Saturation/clamping logic.
- Conversion between float and integer pixel coordinates.
- NaN or infinity propagation if using floating point.
- Behavior on ARM Cortex-M7 with single/double precision differences.

If changing arithmetic, explain whether the rendered output may change.

---

## 9. Memory safety

Pay special attention to:

- Raw pointer ownership.
- Null pointer handling.
- Pointer arithmetic.
- Buffer length calculations.
- Image width/height/stride consistency.
- Out-of-bounds reads/writes.
- Lifetime of image buffers.
- Copy constructors / assignment operators if classes own memory.
- Move semantics if present.
- Aliasing between source and destination images.
- Self-copy / overlapping blits.
- Alignment assumptions.
- Large stack allocations.

If a class does not own the buffer it points to, preserve that ownership model and document it in the analysis.

---

## 10. Performance rules

Rendering code is often performance-critical.

When reviewing performance, look for:

- Unnecessary per-pixel branches.
- Expensive divisions in inner loops.
- Repeated computations inside inner loops.
- Avoidable floating-point operations in hot paths.
- Large object copies.
- Passing heavy objects by value.
- Cache-unfriendly memory traversal.
- Missed opportunities for clipping before loops.
- Avoidable virtual dispatch in hot paths.
- Excessive template instantiation or code bloat.

However:

- Do not replace clear code with obscure micro-optimizations without strong justification.
- Do not introduce architecture-specific optimizations unless guarded and optional.
- Preserve correctness first.

For Teensy 4.x, performance matters, but maintainability still matters.

---

## 11. Embedded / Arduino style constraints

When editing code intended for Arduino/Teensy:

- Avoid relying on OS features.
- Avoid file system assumptions unless already present.
- Avoid threads, mutexes, or desktop concurrency primitives.
- Avoid standard input/output.
- Avoid exceptions.
- Avoid dynamic initialization with complex global constructors.
- Be cautious with static initialization order.
- Avoid hidden heap usage.
- Prefer `constexpr`, `inline`, and simple templates only when compatible with the existing style.

Do not introduce dependencies that complicate Arduino library installation.

---

## 12. Header-only / template code

If parts of the library are header-only or template-heavy:

- Be careful with compile times and code bloat.
- Avoid adding large helper functions in headers unless necessary.
- Prefer small `inline` helpers when appropriate.
- Do not move implementation into headers unless needed for templates.
- Watch for ODR violations.
- Check that inline variables are not introduced unless the C++ standard allows them.

---

## 13. Documentation and comments

When modifying comments or Doxygen:

- Preserve technical meaning.
- Do not rewrite style unnecessarily.
- Fix typos only if asked or if touching nearby code.
- Keep comments concise and useful.
- For public API, prefer Doxygen-style comments if already used.
- Do not document uncertain behavior as fact.

If behavior is unclear, write in the review: "The intended convention is unclear" rather than inventing one.

---

## 14. Tests and examples

If the project has tests:

- Run or propose the smallest relevant tests after each change.
- Add focused tests for bugs when practical.
- Prefer deterministic tests.
- For graphics routines, suggest small pixel-level tests for boundary cases.
- Avoid huge golden-image test files unless the project already uses them.

If no test framework exists:

- Suggest minimal test sketches or small standalone examples.
- For Arduino/Teensy, distinguish between host-side tests and hardware tests.

Useful test categories:

- Drawing at image boundaries.
- Negative coordinates.
- Empty primitives.
- Degenerate triangles.
- Very small images: 0x0, 1x1, 2x2.
- Large dimensions close to integer overflow.
- Source/destination overlap in blits.
- Color conversion round trips.
- 3D projection edge cases.

---

## 15. Build commands

Before running build commands, inspect the repository to identify the correct build system.

Possible build systems may include:

- Arduino library structure
- PlatformIO
- CMake
- Makefile
- custom examples
- Teensyduino sketches

Do not invent a build command without checking the repository.

If unsure, report the likely build options and ask which one should be used.

When commands are safe, prefer:

- configuration/listing commands first;
- build commands second;
- no flashing/uploading to hardware unless explicitly requested.

Never upload firmware to a board unless explicitly asked.

---

## 16. Git and patch discipline

Before making changes:

- Check `git status`.
- Avoid modifying unrelated files.
- Keep one logical change per patch.
- Show the diff before applying when requested.
- After applying, summarize exactly which files changed.
- Recommend running the relevant build/test command.

Never perform broad formatting changes in files unrelated to the task.

---

## 17. Review output format

For code review tasks, use this structure:

```text
## Summary

Short high-level summary.

## Critical issues

Each issue:
- Severity:
- File:
- Function/class:
- Problem:
- Why it matters:
- Suggested fix:
- Confidence:

## Probable bugs

Same format.

## Performance concerns

Same format.

## API/design concerns

Same format.

## Minor maintainability notes

Same format.

## Recommended next steps

Prioritized list.
```

For each issue, be specific. Avoid vague comments such as "could be improved" unless you explain how and why.

---

## 18. Patch output format

When proposing a patch, use this structure:

```text
## Goal

What the patch fixes.

## Scope

Files/functions changed.

## Diff

Show the proposed diff.

## Why this is safe

Explain API compatibility and behavioral impact.

## Risks

Mention possible side effects.

## Tests to run

List specific tests/builds.
```

Do not apply the patch until explicitly asked, unless the user has already granted permission.

---

## 19. Special attention for tgx

When analyzing tgx specifically, pay special attention to the following likely areas:

- `Image` class invariants.
- Pixel addressing.
- Color representation and conversions.
- RGB565 / RGB / RGBA formats if present.
- 2D drawing primitives.
- Blitting and clipping.
- Text rendering.
- Sprite/image transformations.
- 3D vector and matrix math.
- Mesh or triangle rasterization.
- Z-buffer handling if present.
- Texture mapping if present.
- Camera/projection code if present.
- Display driver interaction if present.
- Compatibility with Teensy 4.x memory model.
- Avoiding unnecessary RAM usage.
- Avoiding hidden heap allocations.
- Arithmetic overflow for image dimensions and pixel indices.

---

## 20. Things not to do

Do not:

- Rewrite the architecture without explicit request.
- Modernize the code just for style.
- Replace simple embedded-friendly code with heavy abstractions.
- Introduce exceptions.
- Introduce RTTI.
- Introduce iostream.
- Introduce new dependencies.
- Change public API silently.
- Change rendering conventions silently.
- Apply auto-formatting to the whole repository.
- Upload firmware to hardware.
- Assume desktop C++ constraints.
- Assume coordinates are always valid.
- Ignore degenerate geometry cases.

---

## 21. Preferred first task

When starting a fresh review of this repository, first perform a read-only analysis:

1. Identify the project structure.
2. Identify public headers.
3. Identify core image/framebuffer classes.
4. Identify 2D drawing modules.
5. Identify 3D math/rasterization modules.
6. Identify platform-specific code.
7. Identify build/test options.
8. List the top 10 files to review first.
9. List the highest-risk categories of bugs.

Do not modify files during this initial analysis.
