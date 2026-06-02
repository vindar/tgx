# TGX Mesh Converter

This directory contains the Python tools used to build TGX 3D mesh headers.
The recommended output format for new models is `Mesh3Dv2<RGB565>`. The tools
can also regenerate legacy `Mesh3D` headers with better triangle chains when an
existing project still needs the old format.

The converter is designed for Arduino-style projects: it can produce either a
single `.h` file, convenient for sketches, or a split `.h` + `.cpp` pair,
convenient for larger C++ projects.

## What The Tool Does

The conversion pipeline is shared by OBJ input and existing TGX mesh headers:

1. Load the source model.
2. Triangulate OBJ polygons when needed.
3. Build a common internal mesh representation with vertices, normals, UVs,
   materials and texture references. OBJ/MTL `Ke` is parsed as emissive color,
   and `map_Ke` is parsed as an emissive texture reference.
4. Remove degenerate triangles.
5. Generate or normalize normals when needed.
6. Snap vertices, normals and UVs to conservative grids, then remove duplicate
   entries to improve strict triangle adjacency.
7. Build triangle chains with the stripifier module.
8. For `Mesh3Dv2`, build meshlets with the visibility-merge builder.
9. Export C++ data and print memory, strip and meshlet statistics.

`Mesh3Dv2` stores vertices, normals and UVs quantized in each meshlet payload.
The runtime dequantizes them with one multiply/add per component. Meshlet
metadata also stores a compact bounding sphere and visibility cone so TGX can
discard invisible meshlets cheaply.

`Mesh3Dv2` can also carry optional material metadata in a parallel
`material_extras` table. The table may be `nullptr`; when it is present, it has
exactly `nb_materials` entries and `material_extras[i]` extends
`materials[i]`. TGX uses this table to store emissive color, emissive strength
and optional `map_Ke` texture pointers. This metadata is preserved by the tools,
including Mesh3Dv2-to-Mesh3Dv2 conversion, but the renderer does not shade with
it yet. Emissive renderer/shader support is planned for a later release.

If one material needs extended data, the generated `material_extras` array still
contains one row for every material so that material index `i` can be used
directly in both arrays. Rows for non-emissive materials are initialized with
black color, zero strength, `nullptr` texture and zero flags. If no material
needs extended data, `Mesh3Dv2::material_extras` is `nullptr`.

## Python Environment

Use Python 3.11 or newer. Python 3.12 is the tested version on the development
machine.

Recommended packages:

```text
numpy
scipy
matplotlib
scikit-learn
numba
trimesh
meshio
pyvista
vtk
Pillow
```

### Windows

With conda:

```powershell
conda create -n tgxmesh3d2 python=3.12 numpy scipy matplotlib scikit-learn numba trimesh meshio pyvista vtk pillow
conda activate tgxmesh3d2
```

With venv:

```powershell
py -3.12 -m venv .venv-tgxmesh
.\.venv-tgxmesh\Scripts\Activate.ps1
python -m pip install numpy scipy matplotlib scikit-learn numba trimesh meshio pyvista vtk pillow
```

### Linux / macOS

```bash
python3 -m venv .venv-tgxmesh
. .venv-tgxmesh/bin/activate
python -m pip install numpy scipy matplotlib scikit-learn numba trimesh meshio pyvista vtk pillow
```

The TGX visibility helper is a small C++ program built with CMake. Install a C++
compiler and CMake if you want true visibility-cone generation:

- Windows: Visual Studio Build Tools or Visual Studio Community with C++.
- Linux: `g++` or `clang++`, `cmake`, and usual build tools.
- macOS: Xcode command-line tools and CMake.

## Stripifier Helpers

The converter uses `tools._internal.modules.mesh_pipeline.stripifier` for triangle chains. It
always has a greedy fallback. For best chains, build the optional helpers:

```powershell
python tools\tgx_setup.py
```

GA-EAX is bundled as source in `tools/external_lib/GA_EAX`.
Patched sparse LKH support is optional because of the LKH license. To enable it,
copy an LKH 2.x source tree into:

```text
tools/external_lib/LKH
```

Then run `tools/tgx_setup.py` again. The converter automatically uses all
available helpers and keeps the best chain result.

## Mesh3Dv2 Meshlet Path

The public converter exposes two user-facing `Mesh3Dv2` modes built on the same
visibility-merge builder.

The classic visibility-culling mode uses the MCU-tuned settings:

```text
target vertices  : 30
max triangles    : 127
max normal angle : 90 degrees
visibility views : 1024
visibility size  : 1024 x 1024
```

These defaults came from measuring multiple textured and untextured models on
Teensy 4.1, ESP32/ESP32-S3 and RP2350. They are a good starting point. Larger
`--target-vertices` values can reduce meshlet overhead on some large textured
models, but can also reduce culling efficiency.

The compact mode keeps visibility cones but tunes the merge pass for fewer,
larger meshlets and lower static memory use. This is the default mode in the
graphical `tgx_mesh.py` tool, with non-adjacent packing set to `0`.
`--compact-compression` controls the optional non-adjacent packing pass:

- `0`: disable the second pass; conservative and usually the best embedded
  starting point;
- `50`: moderate non-adjacent packing;
- `100`: ultra-compact packing.

## Export From OBJ

Single-header Arduino output:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d2 export model.obj -o model_v2.h --name model_v2 --single-header --normalize
```

Split `.h` + `.cpp` output:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d2 export model.obj -o model_v2.h --name model_v2 --normalize
```

Useful options in this lower-level entry point:

- `--name SYMBOL`: C++ symbol name for the exported mesh.
- `--normalize`: center and scale the model to a unit box.
- `--single-header`: write payload arrays directly in the header.
- `--target-vertices N`: preferred meshlet size before hard limits.
- `--max-normal-angle DEG`: maximum normal spread accepted while merging.
- `--export-textures`: generate texture headers from OBJ/MTL `map_Kd` and
  `map_Ke`.
- `--texture-size W H`: resample exported textures.
- `--texture-symbol Material=Symbol`: link a material to an existing texture symbol.
- `--emissive-texture-symbol Material=Symbol`: link a material to an existing
  emissive texture symbol.
- `--include file.h`: add an include for an existing texture header.

For day-to-day use, prefer the public `tools/cli_tools/tgx_mesh_cli.py` command
or `tools/tgx_mesh.py` GUI. They expose newer material editing options, texture
renaming, TGX texture-header overrides and Mesh3Dv2-to-Mesh3Dv2 preservation
paths.

Example with existing textures:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d2 export falcon.obj -o falcon_v2.h --name falcon_v2 --single-header --texture-symbol Body=Body_texture --include Body_texture.h
```

## Convert Existing Mesh3D Headers

Existing TGX `Mesh3D` headers can be converted directly. This preserves texture
symbols and standard material values from the original header. A legacy
`Mesh3D` chain is interpreted as a sequence of standard materials/submeshes.

Existing TGX `Mesh3Dv2` headers can also be converted through the public
`tgx_mesh_cli.py` command. When a source Mesh3Dv2 header contains
`material_extras`, the conversion preserves emissive color, strength and
emissive texture symbols when the output is Mesh3Dv2. If the output is legacy
Mesh3D, emissive data is omitted because that format has no storage for it.
Only texture headers actually referenced by exported material links are copied.

```powershell
python -m tools._internal.modules.mesh_pipeline.mesh3d_to_mesh3d2 examples\Teensy4\3D\mars\falcon\falcon_vs.h -o falcon_vs_v2.h --root falcon_vs_1 --name falcon_vs_v2 --single-header
```

When the file contains only one mesh chain, `--root` is often optional. When it
contains several independent declarations, pass the first legacy `Mesh3D` symbol
explicitly.

## Legacy Mesh3D Strip Optimization

To keep the legacy format but rebuild better triangle chains:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d model.obj -o model_legacy_opt.h --name model
```

This is useful for comparing old and new formats, or for projects that cannot
move to `Mesh3Dv2` yet.

## Stats And Visualization

Print mesh statistics:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d2 stats model.obj --meshlets
```

Open an interactive PyVista viewer:

```powershell
python -m tools._internal.modules.mesh_pipeline.tgx_mesh3d2 view model.obj --meshlets --viewer pyvista --texture
```

Inspect a generated header:

```powershell
python -m tools._internal.modules.mesh_pipeline.mesh_stats generated_model_v2.h --view --meshlets
```

The viewer can display the textured mesh, meshlets with distinct colors and
visibility cones when they are present.

## Notes For Arduino Users

- Prefer `--single-header` for examples/sketches unless you already have a C++
  build system that compiles companion `.cpp` files.
- Put generated texture headers next to the generated mesh header, or use
  relative includes that match the sketch layout.
- Use `RGB565` textures for MCU display rendering.
- Use power-of-two texture dimensions when selecting
  `SHADER_TEXTURE_WRAP_POW2`.
- On Teensy 4.x, `cacheMesh()` and `copyMeshEXTMEM()` can improve performance
  when model payloads or textures are read from slower memory.
