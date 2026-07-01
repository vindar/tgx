# Benchmark Modules

Each subdirectory is one board-independent benchmark module.

Modules should focus on one coherent TGX area, for example rasterization,
textures, lighting, generated primitives, meshes or integrated stress scenes.
They may adapt frame counts and resolutions through the board profile, but the
test logic should remain shared across CPU and MCU targets.

Keep modules independent enough that the runner can compile and upload them one
at a time. This avoids the old monolithic benchmark problem where every shader
and every path affected every measurement.

## Current Modules

| Module | Coverage |
|---|---|
| `core_raster` | Large triangles, small triangle grids, flat/Gouraud paths, no-zbuffer, near/far clipping, screen clipping variants and face culling. |
| `texture_raster` | Perspective and affine texturing, nearest/bilinear sampling, wrap/clamp modes, triangle batches, strips and quads. |
| `lighting_shading` | Directional lighting, specular, point lights, spotlights, inactive spotlight capacity and textured Gouraud lighting. |
| `primitives` | Generated cube, sphere, cylinder, cone and truncated-cone primitives, textured caps/sides, open/closed variants and wireframes. |
| `mesh` | Legacy `Mesh3D`, compact `Mesh3Dv2`, material modes, textured paths, multi-material meshlets, clipping, discard and culling. |
| `wire_points` | Raw line/triangle/strip/quad wireframe APIs, mesh wireframes, AA/thick lines, projected pixels and dots. |
| `skybox_noz` | No-zbuffer skybox rendering, sampling modes, partial/null faces, reference-height path and foreground/overlay interaction. |
| `integrated` | Realistic mixed scenes that combine meshes, generated primitives, skyboxes, zbuffered content, no-zbuffer overlays and local lights. |
| `mixed_stress` | Large mixed Renderer3D scenes intended to expose codegen/layout sensitivity when many shader paths coexist. |

When adding a module, add its `bench_<name>.h/.cpp` directory here, then update
`../tools/config/modules.json`, the Arduino wrapper in `../arduino/`, and the
minimal sketch launcher in `../sketches/`.
