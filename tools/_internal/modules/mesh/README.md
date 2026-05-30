# TGX Mesh Converter

`tools/tgx_mesh.py` is the main graphical mesh conversion entry point, including
per-material texture selection. `tools/cli_tools/tgx_mesh_cli.py` provides the
same conversion pipeline for scripts and advanced users.

It can read:

- Wavefront `.obj` files, with `.mtl` materials and textures when present.
- Existing TGX `Mesh3D` headers.
- Existing TGX `Mesh3Dv2` headers.

It can write:

- `Mesh3Dv2`, the recommended format for new projects.
- `Mesh3D`, the legacy format, rebuilt with the current stripifier.

The reusable implementation is in `tools/_internal/modules/mesh_pipeline/`; most users
should use the public GUI or CLI entry points above.

In the graphical tool, the default `Mesh3Dv2` mode is **Compact** with
**Non-adjacent packing** set to `0`. This builds larger meshlets to reduce
static memory, keeps visibility cones, and avoids the optional second packing
pass. The alternate **Visibility culling** mode exposes `target vertices` and
`max normal angle`.

## Dependencies and Setup

Run the public setup script before using the mesh converter:

```bash
python tools/tgx_setup.py
```

The setup checks the Python modules, CMake, the C++ compiler, and then builds
the required C++ helpers:

- TGX visibility helper;
- GA-EAX stripifier helper.

The helper sources are kept outside the internal converter package:
`tools/_internal/cpp/tgx_visibility/` for the TGX renderer helper, `tools/external_lib/GA_EAX/`
for the bundled stripifier, and `tools/external_lib/LKH/` for optional local LKH
sources. Generated executables are written to `tools/_internal/bin/`.

LKH is optional and can be added later because it has its own license. If LKH is
missing, the converter still works but prints a warning because generated
triangle strips may be less optimal.

To install only the Python dependencies in the current environment:

```bash
python -m pip install -r tools/requirements.txt
```

To inspect the current setup status:

```bash
python tools/tgx_setup.py --check
```

## Basic Commands

OBJ to `Mesh3Dv2`, single Arduino-friendly header:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
```

OBJ to legacy `Mesh3D`:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_legacy.h --format Mesh3D --name model
```

Existing `Mesh3D` header to `Mesh3Dv2`:

```bash
python tools/cli_tools/tgx_mesh_cli.py old_model.h -o model_v2.h --root old_model --name model_v2
```

Existing mesh header to split `.h + .cpp` output:

```bash
python tools/cli_tools/tgx_mesh_cli.py model_v2.h -o model_v2_split.h --layout split
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
| `--force-normals` | Recompute smooth vertex normals before quantize/dedupe cleanup. Mutually exclusive with `--remove-normals`. |
| `--remove-normals` | Drop all normals before quantize/dedupe cleanup. Mutually exclusive with `--force-normals`. |
| `--no-cleanup` | Disable quantize/dedupe cleanup. |

## Mesh3Dv2 Options

`Mesh3Dv2` always uses the visibility-merge meshlet builder in the public tools.
The command-line default is the classic visibility-culling mode with the
MCU-tuned values used by the TGX examples:

- target vertices: `30`
- max normal angle: `90`
- visibility views: `1024`
- visibility render size: `1024`

The GUI default is the compact mode with non-adjacent packing set to `0`. To
match that from the CLI, use:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --compact --compact-compression 0
```

The main advanced knobs are:

| Option | Meaning |
| --- | --- |
| `--compact` | Use larger meshlets to reduce static memory while still exporting visibility cones. |
| `--compact-compression N` | Optional non-adjacent packing strength for `--compact`, from `0` to `100`. `0` disables this second pass and is the conservative GUI default. If omitted after `--compact`, the CLI uses `50`. |
| `--target-vertices N` | Preferred meshlet size. Larger values reduce meshlet overhead but may reduce culling. |
| `--max-normal-angle DEG` | Maximum normal spread accepted while merging meshlets. |
| `--visibility-samples N` | Number of visibility directions used by visibility-merge. |
| `--visibility-size N` | Offscreen render size used to estimate visibility. |
| `--visibility-margin DEG` | Extra visibility cone margin; negative means automatic. |

`--target-vertices` and `--max-normal-angle` tune the classic visibility mode.
They are not used together with `--compact`; use `--compact-compression` for
the compact mode.

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

When the input is already a TGX `Mesh3D` or `Mesh3Dv2`, existing TGX texture
headers are copied to the mesh output directory by default, and the generated
mesh includes the local copies. This makes the converted asset self-contained
instead of leaving references to the original example or source directory.

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
| `--texture MATERIAL=PATH` | Force one material to use a specific image file or TGX texture header. |
| `--skip-texture MATERIAL` | Leave one material untextured. |
| `--texture-symbol MATERIAL=SYMBOL` | Use an already existing TGX image symbol for one material. |
| `--list-textures` | Print resolved texture choices and stop before mesh generation. |
| `--confirm-textures` | Ask interactively before exporting ambiguous or repaired texture choices. |
| `--include FILE` | Add an include for an existing texture header. |

Examples:

```bash
python tools/cli_tools/tgx_mesh_cli.py stormtrooper.obj -o stormtrooper_v2.h --texture-size 256x256
```

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --list-textures
```

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --texture Body=body_albedo.png --texture-resize Body=256x256
```

In the GUI, choose an OBJ file and press **Reload textures**. Each material is
listed with the texture requested by the `.mtl`, the automatically selected
image, and an optional resize. Use **Browse...** on a selected material to
override the automatic choice.

Run `python tools/cli_tools/tgx_mesh_cli.py --help` for the exact command-line reference.
