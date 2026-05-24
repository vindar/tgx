@page tools_mesh Mesh and validation tools

TGX can be used as a pure header-style Arduino library, but the repository also contains tools that make larger
projects easier to maintain:

- mesh conversion tools for static 3D models;
- texture export helpers;
- mesh inspection and visualization helpers;
- CPU validation programs used to check rendering behavior after changes.

The tools are optional. A sketch that only uses hand-written geometry or already generated headers does not need
Python, CMake or any external program at build time.


@section tools_setup Installing the tool environment

The mesh tools are Python programs located in `tools/mesh3d2_converter/`. They are intended to run on the development
computer, not on the MCU.

Recommended setup:

~~~{.sh}
python --version
python -m pip install --upgrade pip
python -m pip install numpy pillow pyvista
~~~

`numpy` is used for mesh processing, visibility masks and statistics. `Pillow` is used for texture/image conversion.
`pyvista` is only needed for interactive visualization; conversion itself can run without opening a viewer.

The optional native helper programs require a C++ compiler and CMake:

- Windows: install Visual Studio Community with the C++ workload, or another CMake-compatible compiler.
- Linux: install `cmake`, `make` or `ninja`, and `g++` or `clang++`.
- macOS: install Xcode command line tools and CMake.

On Windows, a recent Python from python.org, Miniconda, Anaconda or Visual Studio's Python environment can be used.
On Linux and macOS, the system Python often works, but a virtual environment is cleaner:

~~~{.sh}
python -m venv .venv
source .venv/bin/activate        # Linux/macOS
python -m pip install numpy pillow pyvista
~~~

On Windows PowerShell:

~~~{.powershell}
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install numpy pillow pyvista
~~~

If an interactive PyVista window does not open, first verify that the model can be converted without `--view`; viewer
problems are usually related to the local OpenGL/graphics driver setup, not to the mesh format itself.


@section tools_stripifiers Optional stripifier helpers

TGX stores triangles as strips/chains where possible. Fewer chains usually means fewer vertex loads and faster mesh
rendering. The converter includes a stripifier module and can use native helper programs for better chains.

The setup script is:

~~~{.sh}
python -m tools.mesh3d2_converter.setup_stripifiers
~~~

It builds helper executables under `tools/mesh3d2_converter/external_libs/` when the required source trees are
available.

TGX can use two solvers:

- **GA-EAX**: bundled source helper for general stripification work;
- **LKH**: optional solver. Its license is separate, so users who want it should place the LKH source in the expected
  external directory and let the setup script build the local helper.

The converter still works without optional helpers, but high-quality conversion of large models is slower or less
optimal. When helpers are available, the tool uses them automatically.


@section tools_formats Mesh formats in practice

TGX currently supports two public static mesh formats:

- \ref tgx::Mesh3D "Mesh3D": legacy format, still supported for existing sketches and hand-written/generated models.
- \ref tgx::Mesh3Dv2 "Mesh3Dv2": current compact format, recommended for new static meshes.

`Mesh3D` stores global arrays of vertices, normals, UV coordinates and triangle chains. It can chain several meshes
together, historically often to represent several materials.

`Mesh3Dv2` stores a material table, a meshlet table and a compact quantized payload. Each meshlet is a small group of
triangles associated with one material. The renderer can skip invisible meshlets and decode only the local payload of
visible meshlets. This usually improves memory locality and reduces work on embedded targets.

For Arduino examples, generated meshes can be exported either as:

- a single `.h` file, easiest to include in a sketch;
- a `.h` plus `.cpp` pair, cleaner for larger C++ projects.


@section tools_obj_to_mesh3dv2 Converting an OBJ model to Mesh3Dv2

Use `tgx_mesh3d2 export` for Wavefront OBJ input:

~~~{.sh}
python -m tools.mesh3d2_converter.tgx_mesh3d2 export model.obj -o model_v2.h --name model_v2 --single-header
~~~

The default profile is intended to be a good MCU compromise:

- builder: `visibility_merge`;
- target vertices per meshlet: `30`;
- maximum triangles per meshlet: `127`;
- maximum normal angle during visibility merging: `90` degrees;
- visibility sampling: `1024` views at `1024 x 1024` by default.

The default is deliberately conservative. It usually gives good culling without creating meshlets so tiny that their
management overhead dominates.

Useful options:

- `--single-header`: write all generated data into the header. This is convenient for Arduino examples.
- `--name SYMBOL`: name of the generated C++ mesh variable.
- `--target-vertices N`: tune typical meshlet size. Lower values increase culling granularity; higher values reduce
  meshlet overhead.
- `--max-normal-angle DEG`: controls how aggressively meshlets with different normal directions can be merged.
- `--visibility-merge-samples N`: number of view directions used to build visibility masks.
- `--visibility-merge-size N`: render size used by the visibility helper.
- `--export-textures`: convert OBJ material textures into TGX texture headers.
- `--texture-size W H`: resize exported textures.
- `--texture-require-pow2`: enforce power-of-two textures, useful with `SHADER_TEXTURE_WRAP_POW2`.

For most models, start with no tuning options. Change `--target-vertices` only after checking the generated statistics
and the real framerate on the target board.


@section tools_legacy_to_mesh3dv2 Migrating an existing Mesh3D header

Existing TGX legacy mesh headers can be converted directly:

~~~{.sh}
python -m tools.mesh3d2_converter.mesh3d_to_mesh3d2 old_mesh.h -o new_mesh_v2.h --root old_mesh --name new_mesh_v2 --single-header
~~~

`--root` is the C++ symbol of the first legacy `Mesh3D` object. If the file contains a single obvious mesh chain, the
tool can often detect it automatically; when several mesh declarations exist, pass `--root` explicitly.

The converter keeps material and texture references when it can parse them. If a sketch modifies a texture pointer at
runtime, keep that texture/material data writable in the sketch just as with legacy `Mesh3D`.


@section tools_legacy_mesh3d Optimizing legacy Mesh3D output

Sometimes a project must keep the legacy \ref tgx::Mesh3D format. The newer stripifier can still rebuild better
triangle chains:

~~~{.sh}
python -m tools.mesh3d2_converter.tgx_mesh3d model.obj -o model_legacy_opt.h --name model
~~~

For an existing legacy header:

~~~{.sh}
python -m tools.mesh3d2_converter.tgx_mesh3d old_mesh.h -o model_legacy_opt.h --source legacy --root old_mesh --name model
~~~

This does not add meshlet culling, but it can reduce the number of chains and improve legacy rendering speed.


@section tools_inspect_view Inspecting and visualizing meshes

The inspection tool prints memory and structure statistics:

~~~{.sh}
python -m tools.mesh3d2_converter.mesh_inspect model_v2.h
~~~

For `Mesh3Dv2`, it reports meshlet counts, chain counts, material counts, payload size, visibility information and
other useful diagnostics.

Interactive visualization:

~~~{.sh}
python -m tools.mesh3d2_converter.mesh_inspect model_v2.h --view --view-mode meshlets
~~~

Viewer modes:

- `texture`: show textured geometry when texture information is available;
- `meshlets`: color meshlets differently to inspect the partition;
- `cones`: visualize visibility directions/cones for meshlets;
- `tabs`: start with keyboard-switchable views.

Use the viewer after conversion. It is the fastest way to detect a bad material split, a broken texture mapping or
meshlets that are too fragmented.


@section tools_cleanup Preprocessing and cleanup

The conversion pipeline performs cleanup before building strips or meshlets:

- optional normalization of OBJ geometry;
- normal verification or recomputation when requested;
- removal of degenerate triangles;
- quantization/merge of nearly identical vertices;
- quantization/merge of UV coordinates on a fixed grid;
- quantization/merge of normals followed by renormalization;
- material grouping and texture association.

This preprocessing improves both memory use and stripification quality. For example, tiny UV differences along what
should be a continuous edge can prevent two triangles from being chained. Snapping UVs to a controlled grid often
recovers the intended connectivity without visibly changing the texture.

Important options:

- `--vertex-quant-bits N`: snap vertices relative to the model bounding box.
- `--texcoord-quant-bits N`: snap UVs to multiples of `1 / 2^N`.
- `--normal-quant-bits N`: snap normals to a signed fixed-point grid and renormalize.
- `--force-normals`: recompute flat normals for OBJ input.
- `--normalize --normalize-size S`: center and scale OBJ input.

Do not use `--texcoord-wrap` blindly. It identifies UV coordinates modulo 1 and is only correct when the model and
texture mapping were authored for that interpretation.


@section tools_validation Validation tools

The `tools/validation/` directory contains CPU validation programs. They are used by TGX development to check that
2D and 3D rendering still produce expected images after code changes.

Typical commands from the repository root:

~~~{.powershell}
powershell -ExecutionPolicy Bypass -File tools\validation\2d\run_cpu.ps1
powershell -ExecutionPolicy Bypass -File tools\validation\3d\run_cpu.ps1
~~~

The 3D validation renders both legacy `Mesh3D` and `Mesh3Dv2` scenes. It compares hashes, coarse image summaries and
tolerant image metrics. Some small pixel differences are acceptable after math or rasterizer changes, but large
visual changes should be inspected before accepting a patch.


@section tools_workflow Suggested workflow

For a new textured model:

1. Start from a clean triangulated OBJ with material/texture files.
2. Convert with defaults:

~~~{.sh}
python -m tools.mesh3d2_converter.tgx_mesh3d2 export model.obj -o model_v2.h --name model_v2 --single-header --export-textures --texture-require-pow2
~~~

3. Inspect statistics:

~~~{.sh}
python -m tools.mesh3d2_converter.mesh_inspect model_v2.h
~~~

4. Open the viewer:

~~~{.sh}
python -m tools.mesh3d2_converter.mesh_inspect model_v2.h --view --view-mode tabs
~~~

5. Include the generated header in a TGX sketch and benchmark on the real target board.
6. Only then tune `--target-vertices`, texture size or wrapping mode if needed.

Good desktop statistics are useful, but final performance depends on the MCU, flash speed, cache behavior, display
driver and memory layout. Always measure the real sketch when performance matters.
