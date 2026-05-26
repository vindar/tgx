@page tools_mesh Mesh, image and font tools

TGX can be used as a pure header-style Arduino library, but the repository also contains tools that make larger
projects easier to maintain:

- mesh conversion tools for static 3D models;
- texture export helpers;
- font conversion tools;
- generated asset inspection and visualization helpers.

The tools are optional. A sketch that only uses hand-written geometry or already generated headers does not need
Python, CMake or any external program at build time.


@section tools_setup Installing the tool environment

The mesh tools are Python programs located in `tools/`. They are intended to run on the development computer, not on
the MCU. Use `tools/tgx_mesh.py` for the graphical tool, or `tools/cli_tools/tgx_mesh_cli.py` for advanced command-line
conversion and scripts.

Recommended setup:

~~~{.sh}
python --version
python -m pip install --upgrade pip
python tools/tgx_setup.py
~~~

The setup script checks the Python packages, CMake, the C++ compiler and the native helper programs. It can also guide
the installation of missing Python packages from `tools/requirements.txt`.

`numpy` is used for mesh processing, visibility masks and statistics. `Pillow` is used for texture/image/font
conversion, `fonttools` is used to inspect font character maps, and `pyvista`/`vtk` are used for interactive
visualization.

The optional native helper programs require a C++ compiler and CMake:

- Windows: install Visual Studio Community with the C++ workload, or another CMake-compatible compiler.
- Linux: install `cmake`, `make` or `ninja`, and `g++` or `clang++`.
- macOS: install Xcode command line tools and CMake.

On Windows, a recent Python from python.org, Miniconda, Anaconda or Visual Studio's Python environment can be used.
On Linux and macOS, the system Python often works, but a virtual environment is cleaner:

~~~{.sh}
python -m venv .venv
source .venv/bin/activate        # Linux/macOS
python -m pip install -r tools/requirements.txt
~~~

On Windows PowerShell:

~~~{.powershell}
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r tools/requirements.txt
~~~

If an interactive PyVista window does not open, first verify that the model can be converted without `--view`; viewer
problems are usually related to the local OpenGL/graphics driver setup, not to the mesh format itself.


@section tools_stripifiers Optional stripifier helpers

TGX stores triangles as strips/chains where possible. Fewer chains usually means fewer vertex loads and faster mesh
rendering. The converter includes a stripifier module and can use native helper programs for better chains.

The setup script is:

~~~{.sh}
python tools/tgx_setup.py
~~~

It builds helper executables under `tools/_internal/bin/` from sources in `tools/_internal/cpp/` and `tools/external_lib/`.

TGX can use two solvers:

- **GA-EAX**: bundled source helper for general stripification work;
- **LKH**: optional solver. Its license is separate, so users who want it should place the LKH source in the expected
  external directory and let the setup script build the local helper.

The converter still works without optional helpers, but high-quality conversion of large models is slower or less
optimal. When helpers are available, the tool uses them automatically.


@section tools_images_fonts Image and font converters

The graphical image converter is:

~~~{.sh}
python tools/tgx_image.py
~~~

The command-line image converter is useful for scripted texture export:

~~~{.sh}
python tools/cli_tools/tgx_image_cli.py diffuse.png --format RGB565 --name diffuse_texture --output-dir generated
~~~

The graphical font converter is:

~~~{.sh}
python tools/tgx_font.py
~~~

It converts TrueType/OpenType fonts to TGX-compatible font headers. The GUI lets you choose:

- the input font file;
- the output format: antialiased `ILI9341_t3_font_t` v2.3, monochrome ILI9341_t3 v1, or Adafruit `GFXfont`;
- one or several pixel sizes;
- `PROGMEM` and single-header or `.h + .cpp` output;
- character presets or a custom per-character selection from a grid.

The equivalent CLI is:

~~~{.sh}
python tools/cli_tools/tgx_font_cli.py MyFont.ttf -o my_font.h --name my_font --sizes 12,16,24 --chars printable --aa 4
~~~

Useful font options:

- `--format ili9341-v23|ili9341-v1|adafruit-gfx`: choose the runtime font format.
- `--sizes 10,12,16`: export several sizes from the same source font into the same generated font file.
- `--chars printable`: choose a preset. Explicit ranges such as `32-126` or `0xA0-0xFF` are also accepted.
- `--aa none|2|4|8`: choose antialiasing depth for `ili9341-v23`. Monochrome formats force 1 bit per pixel.
- `--layout header|split`: write either one `.h` file or a `.h + .cpp` pair.
- `--preview-png preview.png`: write a desktop preview grid of the exported glyphs, with one section per size.

The generated C++ symbol always includes the pixel size. For the ILI9341 v2.3 format, it also includes the AA depth
for compatibility with TGX font naming (`AA0` for monochrome v2.3 output). For example,
`--name ui_font --sizes 16 --aa 4` creates `ui_font_AA4_16`. Monochrome formats use names such as `ui_font_16`.


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

Use the public mesh CLI for Wavefront OBJ input:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
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

- `--layout header`: write all generated data into one header. This is the default and is convenient for Arduino
  examples.
- `--layout split`: write a `.h + .cpp` pair for larger C++ projects.
- `--name SYMBOL`: name of the generated C++ mesh variable.
- `--target-vertices N`: tune typical meshlet size. Lower values increase culling granularity; higher values reduce
  meshlet overhead.
- `--max-normal-angle DEG`: controls how aggressively meshlets with different normal directions can be merged.
- `--visibility-samples N`: number of view directions used to build visibility masks.
- `--visibility-size N`: render size used by the visibility helper.
- `--texture-size WxH`: resize exported textures.
- `--texture-layout header|split`: choose one header per texture or split `.h + .cpp` output.

For most models, start with no tuning options. Change `--target-vertices` only after checking the generated statistics
and the real framerate on the target board.


@section tools_legacy_to_mesh3dv2 Migrating an existing Mesh3D header

Existing TGX legacy mesh headers can be converted directly:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py old_mesh.h -o new_mesh_v2.h --root old_mesh --name new_mesh_v2
~~~

`--root` is the C++ symbol of the first legacy `Mesh3D` object. If the file contains a single obvious mesh chain, the
tool can often detect it automatically; when several mesh declarations exist, pass `--root` explicitly.

The converter keeps material and texture references when it can parse them. If a sketch modifies a texture pointer at
runtime, keep that texture/material data writable in the sketch just as with legacy `Mesh3D`.


@section tools_legacy_mesh3d Optimizing legacy Mesh3D output

Sometimes a project must keep the legacy \ref tgx::Mesh3D format. The newer stripifier can still rebuild better
triangle chains:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_legacy_opt.h --format Mesh3D --name model
~~~

For an existing legacy header:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py old_mesh.h -o model_legacy_opt.h --format Mesh3D --root old_mesh --name model
~~~

This does not add meshlet culling, but it can reduce the number of chains and improve legacy rendering speed.


@section tools_inspect_view Inspecting and visualizing meshes

The CLI prints memory and structure statistics after each conversion:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
~~~

For `Mesh3Dv2`, it reports meshlet counts, chain counts, material counts, payload size, visibility information and
other useful diagnostics.

Interactive visualization is available from the graphical tool:

~~~{.sh}
python tools/tgx_mesh.py
~~~

Viewer modes:

- `texture`: show textured geometry when texture information is available;
- `meshlets`: color meshlets differently to inspect the partition;
- `cones`: visualize visibility directions/cones for meshlets;
- `tabs`: start with keyboard-switchable views.

Use the viewer after conversion. It is the fastest way to detect a bad material split, a broken texture mapping or
meshlets that are too fragmented.


@section tools_info Inspecting generated TGX assets

The general-purpose inspector is:

~~~{.sh}
python tools/tgx_info.py
~~~

The command-line version is:

~~~{.sh}
python tools/cli_tools/tgx_info_cli.py generated_asset.h
~~~

It reads a generated `.h` or `.cpp`, detects the TGX asset type, and searches for the associated companion files next
to the selected file. It is intended as a read-only inspection tool.

For meshes, it reports:

- legacy `Mesh3D` or current `Mesh3Dv2` format;
- triangle, chain, meshlet and material counts;
- payload and static memory estimates, excluding texture image data;
- texture symbols and texture dimensions when texture headers can be found;
- bounding box and meshlet culling information for `Mesh3Dv2`.

For images, it reports the C++ symbol, TGX color type, dimensions and memory size, and can save a PNG preview.

For fonts, it reports all generated font objects in the file, their format, character range, antialiasing level,
memory size and glyph count. It can also save a PNG glyph sheet.

If the file cannot be parsed reliably, the inspector reports the problem instead of guessing from partial data. This is
useful when checking hand-edited generated files before including them in a sketch.


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


@section tools_workflow Suggested workflow

For a new textured model:

1. Start from a clean triangulated OBJ with material/texture files.
2. Convert with defaults:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2 --texture-size 256x256
~~~

3. Inspect statistics:

~~~{.sh}
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
~~~

4. Open the viewer:

~~~{.sh}
python tools/tgx_mesh.py
~~~

5. Include the generated header in a TGX sketch and benchmark on the real target board.
6. Only then tune `--target-vertices`, texture size or wrapping mode if needed.

Good desktop statistics are useful, but final performance depends on the MCU, flash speed, cache behavior, display
driver and memory layout. Always measure the real sketch when performance matters.
