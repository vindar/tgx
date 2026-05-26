# TGX Image Converter

Convert a PNG/JPEG/BMP image into a C++ `tgx::Image` object that can be used directly in an Arduino sketch or in a TGX example.

The converter has two entry points in the `tools/` directory:

- `tgx_image.py`: graphical wrapper for interactive use.
- `cli_tools/tgx_image_cli.py`: command-line converter for scripts and batch work.

## Dependencies

Use Python 3.9 or newer.

The command-line converter only needs Pillow:

```bash
python -m pip install pillow
```

The graphical interface also uses `tkinter`.

- Windows/macOS: `tkinter` is usually included with the standard Python installer.
- Linux: install the Tk package from your distribution if needed, for example `sudo apt install python3-tk`.

You can check that both dependencies are available with:

```bash
python -c "from PIL import Image; import tkinter; print('ok')"
```

## CLI examples

Single-header RGB565 image:

```bash
python tools/cli_tools/tgx_image_cli.py logo.png --format RGB565 --name logo --output-dir generated
```

You can also give the generated file path directly:

```bash
python tools/cli_tools/tgx_image_cli.py diffuse.png --format RGB565 --name diffuse_texture --output generated/diffuse_texture.h
```

For `--layout split`, the path stem is used for both files. For example `--output generated/diffuse_texture.h` creates `generated/diffuse_texture.h` and `generated/diffuse_texture.cpp`.

Split `.h + .cpp` RGB32 image, resized to 128x64:

```bash
python tools/cli_tools/tgx_image_cli.py panel.png --format RGB32 --layout split --resize 128x64 --resample lanczos --name panel_img --output-dir generated
```

Texture-style export with vertically flipped rows:

```bash
python tools/cli_tools/tgx_image_cli.py diffuse.png --format RGB565 --flip-y --name diffuse_texture --output-dir generated
```

## Options

| Option | Meaning |
| --- | --- |
| `input` | Source image file (`.png`, `.jpg`, `.bmp`, ...). |
| `-o`, `--output-dir` | Directory where generated files are written. Default: current directory. |
| `--output` | Full output path. In split mode, the `.h` and `.cpp` files use the same path stem. |
| `--format` | TGX color type: `RGB565`, `RGB24`, `RGB32`, `RGB64`, or `RGBf`. Default: `RGB565`. |
| `--name` | C++ object name. Default: input filename stem. |
| `--layout header` | Generate a single `.h` file. This is the simplest option for Arduino sketches. |
| `--layout split` | Generate a `.h` declaration and a `.cpp` data file. This keeps large arrays out of the header. |
| `--resize WIDTHxHEIGHT` | Resize before exporting, for example `--resize 128x64`. |
| `--resample` | Resize filter: `nearest`, `bilinear`, `bicubic`, or `lanczos`. Default: `lanczos`. |
| `--flip-y` | Write rows bottom-to-top, useful when exporting textures for OBJ-style UV coordinates. |
| `--straight-alpha` | Keep source alpha for `RGB32`/`RGB64`. By default, RGB is pre-multiplied by alpha, as expected by TGX blending. |
| `--no-progmem` | Omit `PROGMEM` on the pixel data array. |

Run `python tools/cli_tools/tgx_image_cli.py --help` to see the same options from the command line.

## GUI

```bash
python tools/tgx_image.py
```

Choose an input image, output location, TGX color type, output layout, and optional resize settings, then click **Convert**.
