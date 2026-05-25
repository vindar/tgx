# TGX Tools

This directory contains optional Python tools used to generate assets and validate TGX.

## Image Converter

`tgx_image_cli.py` and `tgx_image_gui.py` convert PNG/JPEG/BMP images into C++ `tgx::Image` objects.

- CLI: `python tools/tgx_image_cli.py ...`
- GUI: `python tools/tgx_image_gui.py`

Install the only required Python dependency with:

```bash
python -m pip install pillow
```

The optional graphical interface also needs `tkinter`, which is usually bundled with Python on Windows/macOS. On Linux, install your distribution's Tk package if it is missing, for example `python3-tk`.

See `tools/modules/image/README.md` for details.

## Legacy Tools

`legacy_tools/` contains the older conversion scripts kept for reference and compatibility:

- `image_converter.py`
- `obj_to_h.py`
- `texture_2_h.py`

New projects should prefer the newer entry points in this directory.

## Mesh Converter

`tgx_mesh_cli.py` and `tgx_mesh_gui.py` convert OBJ or existing TGX mesh headers to `Mesh3D` or `Mesh3Dv2`.

```bash
python tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
```

```bash
python tools/tgx_mesh_gui.py
```

The GUI is useful for OBJ files with incomplete or broken material files: it
shows every texture reference, the automatically selected diffuse texture, and
lets you override or resize each texture before conversion.

See `tools/modules/mesh/README.md` for details.

`mesh3d2_converter/` contains the older lower-level scripts and the current internal mesh conversion pipeline. They are kept for advanced debugging and compatibility while the public tools are being cleaned up.

## Validation

`validation/` contains the CPU and MCU validation suites.
