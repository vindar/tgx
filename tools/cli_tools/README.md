# TGX command-line tools

This directory contains the command-line entry points for advanced use and
automation:

- `tgx_image_cli.py`: convert images to TGX `Image` headers or `.h + .cpp`
  pairs.
- `tgx_font_cli.py`: convert TrueType/OpenType fonts to TGX-compatible font
  headers.
- `tgx_mesh_cli.py`: convert OBJ or TGX mesh headers to `Mesh3D` or
  `Mesh3Dv2`.

Most users should start with the graphical tools in the parent directory:

```bash
python tools/tgx_image.py
python tools/tgx_font.py
python tools/tgx_mesh.py
```

Run the setup once before using the tools:

```bash
python tools/tgx_setup.py
```
