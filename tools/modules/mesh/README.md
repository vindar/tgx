# TGX Mesh Converter

`tools/tgx_mesh_cli.py` is the simplified mesh conversion entry point.
`tools/tgx_mesh_gui.py` provides the same workflow with a small graphical
interface, including per-material texture selection.

It can read:

- Wavefront `.obj` files, with `.mtl` materials and textures when present.
- Existing TGX `Mesh3D` headers.
- Existing TGX `Mesh3Dv2` headers.

It can write:

- `Mesh3Dv2`, the recommended format for new projects.
- `Mesh3D`, the legacy format, rebuilt with the current stripifier.

The older research/debug scripts are still available in `tools/mesh3d2_converter/`.

## Dependencies

The mesh converter uses the same Python environment as the existing mesh tools:

```bash
python -m pip install numpy scipy matplotlib scikit-learn numba trimesh meshio pyvista vtk pillow
```

For best triangle chains, build the optional stripifier helpers:

```bash
python tools/mesh3d2_converter/setup_stripifiers.py
```

`Mesh3Dv2` visibility-merge also uses the TGX C++ visibility helper. It is built automatically when needed if CMake and a C++ compiler are available.

## Basic Commands

OBJ to `Mesh3Dv2`, single Arduino-friendly header:

```bash
python tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
```

OBJ to legacy `Mesh3D`:

```bash
python tools/tgx_mesh_cli.py model.obj -o model_legacy.h --format Mesh3D --name model
```

Existing `Mesh3D` header to `Mesh3Dv2`:

```bash
python tools/tgx_mesh_cli.py old_model.h -o model_v2.h --root old_model --name model_v2
```

Existing mesh header to split `.h + .cpp` output:

```bash
python tools/tgx_mesh_cli.py model_v2.h -o model_v2_split.h --layout split
```

## Main Options

| Option | Meaning |
| --- | --- |
| `input` | Source `.obj`, `Mesh3D` header, or `Mesh3Dv2` header. |
| `-o`, `--output` | Output mesh header. |
| `--source` | Force input type: `obj`, `mesh3d`, `mesh3dv2`, or `auto`. |
| `--format` | Output format: `Mesh3Dv2` or `Mesh3D`. |
| `--name` | C++ mesh symbol. Defaults to the output filename stem. |
| `--root` | Root symbol when converting a legacy chained `Mesh3D` header. |
| `--layout` | `header` for one `.h`, or `split` for `.h + .cpp`. |
| `--color-type` | Mesh color type: `RGB565`, `RGB24`, `RGB32`, or `RGBf`. |
| `--normalize` | For OBJ input, center and scale the model. |
| `--normalize-size` | Size used by `--normalize`. |
| `--force-normals` | Recompute normals. |
| `--no-cleanup` | Disable quantize/dedupe cleanup. |

## Mesh3Dv2 Options

`Mesh3Dv2` always uses the visibility-merge meshlet builder in this public CLI.
The default settings are the MCU-tuned values used by the TGX examples:

- target vertices: `30`
- max normal angle: `90`
- visibility views: `1024`
- visibility render size: `1024`

The main advanced knobs are:

| Option | Meaning |
| --- | --- |
| `--target-vertices N` | Preferred meshlet size. Larger values reduce meshlet overhead but may reduce culling. |
| `--max-normal-angle DEG` | Maximum normal spread accepted while merging meshlets. |
| `--visibility-samples N` | Number of visibility directions used by the builder. |
| `--visibility-size N` | Offscreen render size used to estimate visibility. |
| `--visibility-margin DEG` | Extra visibility cone margin; negative means automatic. |

## Texture Export

For OBJ inputs, TGX can attach one texture to each material. The converter
therefore uses the diffuse/albedo map (`map_Kd`) as the TGX texture. Other OBJ
maps such as specular, normal/bump, emissive or reflection maps are kept as
references for inspection but are not exported to TGX materials.

The converter first tries the exact `map_Kd` path from the `.mtl` file. If that
file is missing, it scans the OBJ directory and any `--texture-search` directory
for image files with similar names. The chosen texture and the best candidates
are printed before conversion, so broken `.mtl` references are visible instead
of silently producing an untextured mesh.

Texture conversion reuses the TGX image converter, with vertical flipping
enabled to match OBJ UV conventions.

Useful options:

| Option | Meaning |
| --- | --- |
| `--texture-format` | `RGB565`, `RGB24`, `RGB32`, `RGB64`, or `RGBf`. Default: `RGB565`. |
| `--texture-size WxH` | Resize every exported texture. |
| `--texture-resize MATERIAL=WxH` | Resize one material texture; overrides `--texture-size` for that material. |
| `--texture-resample` | `nearest`, `bilinear`, `bicubic`, or `lanczos`. |
| `--texture-filter MATERIAL=FILTER` | Resize filter for one material texture; overrides `--texture-resample`. |
| `--texture-layout` | `header` or `split`. |
| `--texture-output-dir DIR` | Directory for generated texture files. Defaults to the mesh output directory. |
| `--texture-search DIR` | Extra directory searched when `.mtl` texture paths are missing. |
| `--texture MATERIAL=PATH` | Force one material to use a specific image file. |
| `--skip-texture MATERIAL` | Leave one material untextured. |
| `--texture-symbol MATERIAL=SYMBOL` | Use an already existing TGX image symbol for one material. |
| `--list-textures` | Print resolved texture choices and stop before mesh generation. |
| `--confirm-textures` | Ask interactively before exporting ambiguous or repaired texture choices. |
| `--include FILE` | Add an include for an existing texture header. |

Examples:

```bash
python tools/tgx_mesh_cli.py stormtrooper.obj -o stormtrooper_v2.h --texture-size 256x256
```

```bash
python tools/tgx_mesh_cli.py model.obj -o model_v2.h --list-textures
```

```bash
python tools/tgx_mesh_cli.py model.obj -o model_v2.h --texture Body=body_albedo.png --texture-resize Body=256x256
```

In the GUI, choose an OBJ file and press **Reload textures**. Each material is
listed with the texture requested by the `.mtl`, the automatically selected
image, and an optional resize. Use **Browse...** on a selected material to
override the automatic choice.

Run `python tools/tgx_mesh_cli.py --help` for the exact command-line reference.
