@page tools_mesh TGX tools

TGX provides a small set of desktop tools to prepare assets before they are
included in a sketch or C++ project. These tools convert common image, mesh and
font formats into C++ files directly usable by TGX, and they can inspect the
generated files afterwards.

The graphical tools are meant to be the comfortable path for everyday use. For
example, `tgx_mesh.py` can load an OBJ or TGX mesh, preview materials and
textures, tune conversion settings and export ready-to-include C++ files:

![tgx_mesh graphical converter](../tgx_mesh_screenshot_small.png)


| Tool | Purpose |
|------|---------|
| `tools/tgx_image.py` | Convert PNG/JPEG/BMP/TGA images to TGX `Image` headers. |
| `tools/tgx_mesh.py` | Convert OBJ files or existing TGX meshes to `Mesh3D` or `Mesh3Dv2`. |
| `tools/tgx_font.py` | Convert TrueType/OpenType fonts to TGX-compatible font headers. |
| `tools/tgx_info.py` | Inspect generated TGX image, mesh and font files. |

The graphical tools are the recommended entry points for most users. Command
line versions also exist under `tools/cli_tools/` for scripts, batch conversion
and repeatable builds.

The tools are optional. They run on the development computer when preparing
assets; they are not needed by the MCU at runtime, and they are not needed to
compile a sketch that already contains its generated assets.


@section tools_install Installing the tools

Install TGX as usual first. The tools are only needed on the development
computer when you want to generate assets.

Use a recent Python 3 installation, then run this command from the TGX
repository root:

~~~{.sh}
python tools/tgx_setup.py
~~~

The setup script checks:

- Python packages used by the tools;
- Tkinter, used by the graphical interfaces;
- CMake and a C++ compiler, used by the mesh converter helpers;
- the native helper programs needed by mesh conversion.

If Python packages are missing, install them in the same Python environment:

~~~{.sh}
python -m pip install -r tools/requirements.txt
~~~

On Linux, Tkinter may require a system package, for example:

~~~{.sh}
sudo apt install python3-tk
~~~

The mesh converter needs CMake and a C++ compiler:

- Windows: install CMake and Visual Studio Build Tools with the C++ workload.
- Linux: install `cmake` and `g++` or `clang++`.
- macOS: install CMake and the Xcode command line tools.

LKH is optional. If it is not installed, mesh conversion still works, but
triangle strips may be less optimal. The setup script explains how to add LKH
later if desired.

To check the current setup without rebuilding anything:

~~~{.sh}
python tools/tgx_setup.py --check
~~~

Each public tool checks the setup at startup. This helps catch the common case
where the tool is launched from the wrong Python environment.


@section tools_image tgx_image

Run:

~~~{.sh}
python tools/tgx_image.py
~~~

`tgx_image.py` converts regular image files to C++ `tgx::Image` objects. It can:

- choose the TGX color type, such as `RGB565`, `RGB24` or `RGB32`;
- resize the image;
- choose the generated C++ object name;
- write either a single `.h` file or a `.h + .cpp` pair.

This is the tool to use for sprites, icons, texture images and other fixed
bitmap assets that should be compiled into a sketch.


@section tools_mesh_gui tgx_mesh

Run:

~~~{.sh}
python tools/tgx_mesh.py
~~~

`tgx_mesh.py` converts Wavefront OBJ models, or existing TGX mesh headers, to:

- \ref tgx::Mesh3Dv2 "Mesh3Dv2", the recommended compact mesh format for new
  static models;
- \ref tgx::Mesh3D "Mesh3D", the legacy format kept for compatibility.

For OBJ files, the tool reads materials and textures when available. The GUI
shows the detected texture references and lets the user override or resize each
texture before conversion. It can also preview the model with PyVista, including
textured rendering and Mesh3Dv2 meshlet visualization.

For a first conversion, keep the GUI defaults:

- `Mesh3Dv2` output;
- `RGB565` color, recommended on embedded displays;
- `Compact` meshlet generation;
- `Non-adjacent packing` set to `0`.

This usually gives a good embedded compromise: compact mesh data, no aggressive
second packing pass, and visibility cones still generated for runtime culling.
The alternate `Visibility culling` mode exposes `target vertices` and
`max normal angle` when culling quality is more important than static memory
size. Tune meshlet or texture options only after inspecting the generated mesh
and measuring the real sketch on the target board.


@section tools_font tgx_font

Run:

~~~{.sh}
python tools/tgx_font.py
~~~

`tgx_font.py` converts TrueType/OpenType fonts to font formats supported by TGX:

- antialiased `ILI9341_t3_font_t` v2.3;
- older monochrome ILI9341_t3 fonts;
- Adafruit `GFXfont`.

The GUI lets the user select one or several sizes, choose the character set,
choose the output format, enable `PROGMEM`, and write either a single `.h` file
or a `.h + .cpp` pair.

This is useful when only a subset of characters is needed. Keeping only the
characters used by a sketch can save a lot of flash memory.


@section tools_info tgx_info

Run:

~~~{.sh}
python tools/tgx_info.py
~~~

`tgx_info.py` is a read-only inspector. Give it a generated `.h` or `.cpp` file
and it detects whether it contains an image, mesh or font.

It reports information such as:

- image dimensions, color type and memory size;
- mesh triangle counts, meshlet counts, texture list and memory estimates;
- font objects, glyph counts, character ranges, antialiasing and memory size.

It can also save image/font preview PNGs, and can open the mesh viewer for mesh
assets. Use it to check generated files before including them in a sketch.


@section tools_cli Command-line tools

Each graphical tool has a command-line counterpart under `tools/cli_tools/`:

| CLI tool | Purpose |
|----------|---------|
| `tgx_image_cli.py` | Scripted image conversion. |
| `tgx_mesh_cli.py` | Scripted OBJ/TGX mesh conversion. |
| `tgx_font_cli.py` | Scripted font conversion. |
| `tgx_info_cli.py` | Scripted asset inspection. |

The command-line tools are useful for automation and reproducible asset builds.
They expose more options than the graphical tools. See
`tools/cli_tools/README.md` and the `--help` output of each command for the full
reference.


@section tools_generated_assets Generated files

The tools generate ordinary C++ files. They can be included in sketches like
hand-written assets:

- single-header output is usually the simplest option for Arduino examples;
- split `.h + .cpp` output is often cleaner for larger C++ projects;
- `PROGMEM` output keeps large constant data in flash on Arduino-style targets.

Generated files are meant to be readable and inspectable. They can be committed
with a project, so users of the final sketch do not need Python or the TGX tools.


@section tools_legacy Legacy tools

`tools/legacy_tools/` contains older conversion scripts kept for old projects
and reference:

- `image_converter.py`
- `obj_to_h.py`
- `texture_2_h.py`

New projects should normally use `tgx_image.py`, `tgx_mesh.py`, `tgx_font.py`
and `tgx_info.py`.
