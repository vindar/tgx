# TGX command-line tools

This directory contains the command-line entry points for TGX asset generation
and inspection. They are intended for advanced users, automation scripts,
repeatable builds and batch conversion.

For interactive use, prefer the graphical tools in the parent directory:

- `tools/tgx_image.py`
- `tools/tgx_mesh.py`
- `tools/tgx_font.py`
- `tools/tgx_info.py`

Run the setup once before using the command-line tools:

```bash
python tools/tgx_setup.py
```

All commands below are meant to be run from the TGX repository root.


## Common conventions

The CLI tools generate C++ files that can be included directly in Arduino/TGX
sketches.

Output layout:

- `--layout header`: generate one self-contained `.h` file.
- `--layout split`: generate a `.h + .cpp` pair.

The single-header layout is convenient for Arduino examples. The split layout is
cleaner for larger C++ projects.

Most tools add `PROGMEM` by default where it makes sense. Use `--no-progmem`
when the generated object must remain writable or when the target platform does
not use `PROGMEM`.

Every tool supports `--help`:

```bash
python tools/cli_tools/tgx_mesh_cli.py --help
```


## tgx_image_cli.py

Converts a PNG/JPEG/BMP/TGA image to a TGX `tgx::Image`.

Basic usage:

```bash
python tools/cli_tools/tgx_image_cli.py input.png --output-dir generated
```

Write to an explicit file:

```bash
python tools/cli_tools/tgx_image_cli.py input.png --output generated/logo.h --name logo
```

Useful examples:

```bash
python tools/cli_tools/tgx_image_cli.py logo.png --format RGB565 --resize 128x64 --name logo
python tools/cli_tools/tgx_image_cli.py texture.png --format RGB565 --flip-y --layout split --output generated/texture.h
python tools/cli_tools/tgx_image_cli.py icon.png --format RGB32 --straight-alpha --output-dir generated
```

Options:

- `input`: input image file.
- `-o, --output-dir DIR`: directory for generated files. Ignored if `--output`
  supplies a full path.
- `--output PATH`: full output path. For split output, the `.h/.cpp` pair uses
  this path stem.
- `--format TYPE`: TGX color type. Current choices are `RGB565`, `RGB24`,
  `RGB32`, `RGB64`, `RGBf` when supported by the image converter.
- `--name SYMBOL`: generated C++ object name. Defaults to the input file stem.
- `--layout header|split`: generate one header or a `.h + .cpp` pair.
- `--resize WIDTHxHEIGHT`: resize before export.
- `--resample FILTER`: resize filter. Common choices include `nearest`,
  `bilinear`, `bicubic` and `lanczos`.
- `--flip-y`: write rows bottom-to-top. This is useful for OBJ texture
  workflows when UV coordinates expect flipped image rows.
- `--no-progmem`: omit `PROGMEM`.
- `--straight-alpha`: for alpha-capable formats, keep straight alpha instead of
  TGX premultiplied alpha.


## tgx_mesh_cli.py

Converts Wavefront OBJ files or existing TGX mesh headers to `Mesh3D` or
`Mesh3Dv2`.

Basic OBJ to Mesh3Dv2 conversion:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_v2.h --name model_v2
```

Convert an existing legacy Mesh3D header:

```bash
python tools/cli_tools/tgx_mesh_cli.py old_mesh.h -o model_v2.h --root old_mesh --name model_v2
```

Generate optimized legacy Mesh3D output:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_legacy.h --format Mesh3D --name model
```

The default output format is `Mesh3Dv2`. Mesh3Dv2 generation always uses the
visibility-merge meshlet builder. In the command-line tool, the default mode is
the classic visibility-culling configuration; pass `--compact` when static mesh
memory size is the main goal. The graphical `tgx_mesh.py` front-end selects the
compact mode by default because it is usually the best first choice on embedded
boards.

Generate a compact Mesh3Dv2 mesh:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_compact.h --compact
```

Use the same conservative compact mode as the GUI:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_compact.h --compact --compact-compression 0
```

Tune the optional non-adjacent packing strength:

```bash
python tools/cli_tools/tgx_mesh_cli.py model.obj -o model_compact.h --compact --compact-compression 75
```

Input and output options:

- `input`: `.obj`, legacy `Mesh3D` header or `Mesh3Dv2` header.
- `-o, --output PATH`: output mesh header path. Required.
- `--source auto|obj|mesh3d|mesh3dv2`: force input type. The default `auto`
  detects it from the file.
- `--format Mesh3D|Mesh3Dv2`: choose output mesh format.
- `--name SYMBOL`: generated C++ mesh symbol. Defaults to output file stem.
- `--root SYMBOL`: root `Mesh3D` symbol when a legacy header contains several
  chained meshes or several declarations.
- `--layout header|split`: generate one header or a `.h + .cpp` pair.
- `--color-type TYPE`: output TGX color type. Accepted values include
  `RGB565`, `RGB24`, `RGB32`, `RGBf` and the corresponding `tgx::...` names.
- `--include FILE`: add an extra quoted include before the mesh declaration.
  Can be repeated.

Mesh cleanup options:

- `--normalize`: for OBJ input, center the model and fit it to
  `--normalize-size`.
- `--normalize-size FLOAT`: normalized bounding-box size. Default is `2.0`.
- `--force-normals`: recompute smooth vertex normals before cleanup.
- `--remove-normals`: drop all normals before cleanup.
  This is mutually exclusive with `--force-normals`.
- `--no-cleanup`: disable the quantize/dedupe cleanup pass.
- `--texcoord-wrap`: identify UVs modulo 1 during cleanup. Use only when this
  is correct for the model and texture mapping.

Mesh3Dv2 meshlet options:

- `--compact`: use the compact Mesh3Dv2 mode. The first merge pass is tuned to
  build large meshlets with low payload duplication, while visibility cones are
  still computed and exported.
- `--compact-compression N`: non-adjacent packing strength used with
  `--compact`, from `0` to `100`. `0` disables the second pass and is the
  recommended conservative value. If `--compact` is used without this option,
  the CLI uses `50`. `100` is ultra-compact.
- `--target-vertices N`: preferred meshlet vertex count for classic visibility
  mode. Default is tuned for a good MCU speed/memory compromise. This option is
  not available with `--compact`.
- `--max-normal-angle DEG`: maximum normal spread accepted during
  classic visibility-merge. This option is not available with `--compact`.
- `--visibility-samples N`: number of TGX visibility directions used by
  visibility-merge.
- `--visibility-size N`: render size used by the visibility helper.
- `--visibility-margin DEG`: visibility cone margin in degrees. Negative values
  select the automatic margin.

Texture export options:

- `--no-textures`: do not export or copy material textures.
- `--texture-format TYPE`: TGX color type for generated textures.
- `--texture-size WIDTHxHEIGHT`: resize all generated textures.
- `--texture-resize MATERIAL=WIDTHxHEIGHT`: resize one material texture. Can be
  repeated and overrides `--texture-size`.
- `--texture-resample FILTER`: default resize filter for textures.
- `--texture-filter MATERIAL=FILTER`: resize filter for one material texture.
  Can be repeated.
- `--texture-layout header|split`: generated texture file layout.
- `--texture-output-dir DIR`: directory for generated texture headers. Defaults
  to the mesh output directory.
- `--texture-search DIR`: extra directory searched when OBJ/MTL texture paths
  are missing. Can be repeated.
- `--texture MATERIAL=PATH`: manually choose an image file or TGX texture
  header for one material. Can be repeated.
- `--skip-texture MATERIAL`: leave one material untextured.
- `--emissive-texture MATERIAL=PATH`: manually choose an image file or TGX
  texture header as one material's emissive texture. Can be repeated.
- `--skip-emissive-texture MATERIAL`: leave one material without an emissive
  texture.
- `--emissive-texture-resize MATERIAL=WIDTHxHEIGHT`: resize one material
  emissive texture. Can be repeated and overrides `--texture-size`.
- `--emissive-texture-filter MATERIAL=FILTER`: resize filter for one material
  emissive texture. Can be repeated.
- `--texture-straight-alpha`: keep straight alpha for generated texture images.
- `--texture-symbol MATERIAL=SYMBOL`: use an existing texture symbol instead of
  exporting an image for that material.
- `--emissive-texture-symbol MATERIAL=SYMBOL`: use an existing texture symbol
  as one material's emissive texture.
- `--texture-name MATERIAL=NAME`: choose the generated texture symbol name for
  one diffuse texture export.
- `--emissive-texture-name MATERIAL=NAME`: choose the generated texture symbol
  name for one emissive texture export.
- `--list-textures`: print resolved OBJ texture choices and exit.
- `--confirm-textures`: interactively confirm or override OBJ texture choices.

Material override options:

- `--material-color MATERIAL=R,G,B`: override one material's diffuse color.
- `--material-ambiant MATERIAL=VALUE`: override one material's ambient strength.
  `--material-ambient` is accepted as an alias.
- `--material-diffuse MATERIAL=VALUE`: override one material's diffuse strength.
- `--material-specular MATERIAL=VALUE`: override one material's specular strength.
- `--material-exponent MATERIAL=VALUE`: override one material's specular exponent.
- `--emissive-color MATERIAL=R,G,B`: override one material's emissive color.
- `--emissive-strength MATERIAL=VALUE`: override one material's emissive strength.
- `--material-simple MATERIAL`: force one material to omit
  `MeshMaterialExtra3Dv2` data.
- `--material-rename OLD=NEW`: rename a material after loading and texture
  processing.

Stripifier options:

- `--lkh PATH`: optional patched LKH helper path.
- `--strip-time-limit SECONDS`: override the automatic stripifier budget per
  compatible component.

Notes:

- Mesh conversion always works without LKH, but LKH can improve strip quality
  when installed.
- For OBJ input, normals are verified or generated as needed by the conversion
  pipeline.
- Texture export uses the same image exporter as `tgx_image_cli.py`. OBJ/MTL
  `map_Kd` becomes the material diffuse texture. `Ke` is stored as emissive
  color, and `map_Ke` becomes the material emissive texture in Mesh3Dv2
  `material_extras`. TGX stores this emissive metadata but does not render it
  yet.
- A `Mesh3Dv2` output writes a `material_extras` table only when at least one
  material needs extended emissive data. When the table is present, it has one
  row per material; materials without emissive data receive a neutral row with
  black color, zero strength, `nullptr` texture and zero flags.
- A `Mesh3D` legacy output cannot store emissive material data. Diffuse
  materials and diffuse textures are preserved, but emissive colors and emissive
  textures are omitted.
- When the input is already a TGX mesh header, existing TGX texture headers
  referenced by exported material links are copied next to the generated mesh by
  default. Included textures that are not linked by a material are omitted.
- For new projects, start with default Mesh3Dv2 options, then inspect the output
  with `tgx_info_cli.py` or `tgx_info.py` before tuning parameters.
- For embedded projects, the usual first CLI choice is
  `--compact --compact-compression 0`. Use the classic default mode when runtime
  culling efficiency is more important than static memory size.


## tgx_font_cli.py

Converts TrueType/OpenType fonts to TGX-compatible font headers.

Basic usage:

```bash
python tools/cli_tools/tgx_font_cli.py MyFont.ttf -o my_font.h --name my_font
```

Generate several sizes in one file:

```bash
python tools/cli_tools/tgx_font_cli.py MyFont.ttf -o my_font.h --name my_font --sizes 12,16,24
```

Generate an antialiased ILI9341 v2.3 font preview:

```bash
python tools/cli_tools/tgx_font_cli.py MyFont.ttf -o my_font.h --format ili9341-v23 --aa 4 --preview-png preview.png
```

Options:

- `input`: input font file. `.ttf`, `.otf` and `.ttc` are supported when Pillow
  can load them.
- `-o, --output PATH`: output `.h` path. Required.
- `--name SYMBOL`: C++ font symbol base. The pixel size is always appended. For
  ILI9341 v2.3, the AA depth is also appended, for example
  `my_font_AA4_16`.
- `--format ili9341-v23|ili9341-v1|adafruit-gfx`: output font format.
- `--sizes LIST`: comma-separated pixel sizes, for example `10,12,16`.
- `--chars SPEC`: character preset or ranges.
- `--aa none|2|4|8`: antialiasing level. Forced to `none` for ILI9341 v1 and
  Adafruit GFX output.
- `--layout header|split`: generate one header or a `.h + .cpp` pair.
- `--no-progmem`: omit `PROGMEM`.
- `--preview-png PATH`: also render a PNG preview of the exported character
  grid.

Character selection:

- `none`: export no visible glyphs.
- `all`: export all characters found in the font cmap.
- `ascii`: ASCII range.
- `printable`: printable ASCII characters.
- `latin1`: Latin-1 range.
- `letters-digits`: common letters and digits.
- `letters`: letters only.
- `digits`: digits only.
- explicit ranges such as `32-126` or `0xA0-0xFF`.

Several sizes are written to the same generated file. The preview PNG, when
requested, contains one section per generated size.


## tgx_info_cli.py

Inspects generated TGX assets without modifying them.

Basic usage:

```bash
python tools/cli_tools/tgx_info_cli.py generated_asset.h
```

Generate a PNG preview for an image or font:

```bash
python tools/cli_tools/tgx_info_cli.py generated_font.h --preview font_preview.png
```

Open a PyVista viewer for a mesh:

```bash
python tools/cli_tools/tgx_info_cli.py model_v2.h --view
```

Options:

- `input`: generated TGX `.h`, `.hpp` or `.cpp` file.
- `--preview PATH`: write a PNG preview. Available for images and fonts.
- `--view`: open the interactive PyVista mesh viewer. Available for meshes.

The inspector detects the asset type automatically and searches for associated
`.h/.cpp` files next to the selected file. For meshes, it reports geometry,
materials, diffuse and emissive texture links, Mesh3Dv2 material extras, memory
estimates, chain counts and meshlet information. For images, it reports
dimensions, color type and memory size. For fonts, it reports generated font
objects, glyph counts, memory size, character ranges and antialiasing
information.


## Exit behavior

The converters return `0` on success and non-zero on errors. Errors are printed
to stderr. Long mesh conversions print progress messages so automated scripts
can distinguish a running conversion from a stalled one.
