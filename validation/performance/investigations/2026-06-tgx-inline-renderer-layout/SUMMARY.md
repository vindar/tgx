# Summary

Final status: no additional source changes retained after the June 2026 inline/layout investigations.

Useful retained conclusion:

- `TGX_INLINE_ZDIVIDE` and the RP2350 shader incremental pixel pointer path were the stable committed optimizations from the broader work.
- Later category-inline, Image-inline, and Renderer3D/layout patches were rejected because their global gains concealed important local regressions.

Most important lesson:

Pico2 / RP2350 is highly sensitive to code size, layout, alignment, and template instantiation placement in TGX's Renderer3D and shader paths. Optimizations must be validated with detailed subtest matrices, not just global benchmark scores.

Recommended future direction:

- Build isolated microbenchmarks for `rasterizeTriangle()`, `shader_select()`, and selected `uber_shader` variants.
- Use map/symbol/codegen inspection for every promising patch.
- Keep wire/dots/thick-wire, bilinear texture, flat, and real mesh rows as hard guard tests.
- Avoid broad inline/layout perturbations unless a stable mechanism is found.
