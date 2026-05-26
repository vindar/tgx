# TGX Tools

This directory contains optional Python tools used to generate assets and validate TGX.

## First-Time Setup

Run the setup script before using the asset converters:

```bash
python tools/tgx_setup.py
```

The setup checks the current Python environment, CMake, the C++ compiler, and
the required TGX mesh helper programs. It then builds the C++ helpers used by
the mesh converter:

- the TGX visibility helper;
- the GA-EAX stripifier helper.

The helper sources and outputs use stable locations:

- `_internal/cpp/tgx_visibility/` contains the TGX visibility helper source;
- `external_lib/GA_EAX/` contains the bundled GA-EAX source;
- `external_lib/LKH/` is where optional LKH 2.x sources can be copied;
- `_internal/bin/` receives generated helper executables;
- `_internal/build/` receives generated build files.

LKH is optional because it has its own license and is not bundled directly. If
LKH is missing, the setup prints instructions. Mesh conversion still works
without LKH, but generated triangle strips may be less optimal.

To only inspect the current status:

```bash
python tools/tgx_setup.py --check
```

If Python modules are missing, install them in the same Python environment:

```bash
python -m pip install -r tools/requirements.txt
```

On Linux, the graphical tools may also need the system Tk package, for example:

```bash
sudo apt install python3-tk
```

On Windows, install CMake and Visual Studio Build Tools with the
**Desktop development with C++** workload. On macOS, install Xcode command line
tools and CMake.

Each public tool checks the setup again at startup, so launching from the wrong
Python environment gives a clear setup error instead of a later import/build
failure.

## Image Converter

`tgx_image.py` converts PNG/JPEG/BMP images into C++ `tgx::Image` objects. A command-line version is also available
for advanced or scripted use.

- GUI: `python tools/tgx_image.py`
- CLI: `python tools/cli_tools/tgx_image_cli.py ...`

## Font Converter

`tgx_font.py` converts TrueType/OpenType fonts into TGX-compatible font
headers. It can generate the current antialiased `ILI9341_t3_font_t` format,
the older monochrome ILI9341_t3 format, or Adafruit `GFXfont`.

- GUI: `python tools/tgx_font.py`
- CLI: `python tools/cli_tools/tgx_font_cli.py ...`

The GUI lets you choose one or more font sizes, select the character set with
presets or by clicking characters in a grid, choose PROGMEM and output layout,
and optionally save a PNG preview of the exported glyphs.

## Legacy Tools

`legacy_tools/` contains the older conversion scripts kept for reference and compatibility:

- `image_converter.py`
- `obj_to_h.py`
- `texture_2_h.py`

New projects should prefer the newer entry points in this directory.

## Mesh Converter

`tgx_mesh.py` converts OBJ or existing TGX mesh headers to `Mesh3D` or `Mesh3Dv2`. A command-line version is also
available for advanced or scripted use.

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
```

```bash
python tools/tgx_mesh.py
```

The GUI is useful for OBJ files with incomplete or broken material files: it
shows every texture reference, the automatically selected diffuse texture, and
lets you override or resize each texture before conversion.

The reusable implementation lives under `_internal/`; users normally do not
need to open that directory. The command-line and graphical entry points above
are the supported interface.
