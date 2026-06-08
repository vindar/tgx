# TGX tools

This directory contains small Python programs that create, convert or inspect
C++ asset files usable by TGX:

- `tgx_image.py`: converts PNG/JPEG/BMP/TGA images to TGX `Image` headers.
- `tgx_mesh.py`: converts OBJ files or existing TGX meshes to `Mesh3D` or `Mesh3Dv2`.
- `tgx_font.py`: converts TrueType/OpenType fonts to TGX-compatible font headers.
- `tgx_info.py`: inspects generated TGX image, mesh and font files.

The graphical tools are the recommended entry points. Command-line versions are
available in `cli_tools/` for scripts and batch conversion.


## First-time setup

Use a recent Python 3 installation, then run:

```bash
python tools/tgx_setup.py
```

The setup script checks the required Python packages, CMake, a C++ compiler and
the native helper programs used by the mesh converter. If something is missing,
it prints what to install and can be run again afterwards.

If Python modules are missing, install them in the same Python environment:

```bash
python -m pip install -r tools/requirements.txt
```

The graphical tools also need Tkinter. On Windows and macOS it is usually
included with Python. On Linux, install the system Tk package if needed, for
example:

```bash
sudo apt install python3-tk
```

The mesh converter also needs CMake and a C++ compiler:

- Windows: install CMake and Visual Studio Build Tools with the C++ workload.
- Linux: install `cmake` plus `g++` or `clang++`.
- macOS: install CMake and the Xcode command line tools.

LKH is optional. If it is not installed, mesh conversion still works, but some
triangle strips may be less optimal. `tgx_setup.py` explains where to place the
LKH 2.x sources if you want to enable it later; then just run the setup again.

To only check the current setup:

```bash
python tools/tgx_setup.py --check
```

Each public tool checks the setup at startup, so using the wrong Python
environment should give a clear error message.


## tgx_image

Run:

```bash
python tools/tgx_image.py
```

`tgx_image.py` converts regular image files to C++ `tgx::Image` objects. It can:

- choose the TGX color type, such as `RGB565`, `RGB24` or `RGB32`;
- resize the image;
- choose the generated C++ symbol name;
- write either a single `.h` file or a `.h + .cpp` pair.


## tgx_mesh

Run:

```bash
python tools/tgx_mesh.py
```

`tgx_mesh.py` converts Wavefront OBJ models, or existing TGX mesh headers, to:

- `Mesh3Dv2`, the recommended compact mesh format for new sketches;
- `Mesh3D`, the legacy format kept for compatibility.

For OBJ files, the tool reads materials and textures when available. The GUI
shows two asset lists:

- **Materials**: one row per material used by the mesh. A mesh always has at
  least one material, using silver defaults if the source provides none.
  Materials can be renamed and edited. Each material stores diffuse color,
  ambient/diffuse/specular strengths, specular exponent, and an optional diffuse
  texture link.
- **Textures**: one row per image asset. Textures can be added, removed,
  renamed and resized independently, then linked from one or more materials.

For `Mesh3Dv2`, a material can also enable an extended material entry with
emissive color, emissive strength and an optional emissive texture link. OBJ/MTL
`Ke` becomes emissive color and `map_Ke` becomes the emissive texture. TGX
stores this metadata in the optional Mesh3Dv2 `material_extras` table. It can
also preview the mesh with PyVista, using resized texture previews when
applicable.

For `Mesh3Dv2`, the GUI defaults to **Compact** meshlet generation with
**Non-adjacent packing** set to `0`. This is the usual starting point for
embedded projects: it reduces static mesh data while avoiding the extra
aggressive packing pass. The alternate **Visibility culling** mode exposes
`target vertices` and `max normal angle` for users who want to favor meshlet
culling over compactness.



## tgx_font

Run:

```bash
python tools/tgx_font.py
```

`tgx_font.py` converts TrueType/OpenType fonts to font formats supported by TGX:

- antialiased `ILI9341_t3_font_t` v2.3;
- older monochrome ILI9341_t3 fonts;
- Adafruit `GFXfont`.

The GUI lets you select one or several sizes, choose the character set, choose
the output format, enable `PROGMEM`, and write either a single `.h` file or a
`.h + .cpp` pair.



## tgx_info

Run:

```bash
python tools/tgx_info.py
```

`tgx_info.py` is a read-only inspector. Give it a generated `.h` or `.cpp` file
and it detects whether it contains an image, mesh or font.

It reports useful information such as dimensions, memory size, meshlet counts,
triangle counts, material tables, diffuse/emissive texture links, Mesh3Dv2
material extras, texture lists or glyph lists. It can also save image/font
preview PNGs, and can open the mesh viewer for mesh assets.



## Legacy tools

`legacy_tools/` contains the older TGX conversion scripts:

- `image_converter.py`
- `obj_to_h.py`
- `texture_2_h.py`

They are kept for old projects and reference. New projects should normally use
`tgx_image.py`, `tgx_mesh.py`, `tgx_font.py` and `tgx_info.py`.
