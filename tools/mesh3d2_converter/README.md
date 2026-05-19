# TGX Meshlet Converter

Experimental Python tooling for building and inspecting TGX meshlet meshes from Wavefront `.obj` files.

Current scope:

- load triangular or polygonal OBJ files,
- triangulate polygon faces,
- split triangles into connected meshlets under the current meshlet local-index limits,
- visualize the mesh or meshlets with matplotlib,
- print basic meshlet quality statistics.
- estimate gross and net meshlet-cone culling efficiency across view directions,
- compute visibility cones with an offline TGX C++ helper,
- export a TGX `Mesh3Dv2` C++ header,
- convert an existing TGX `Mesh3D` header to a meshlet format while reusing the same texture headers.

This is still an experimental converter. The generated meshlet files are meant to
coexist with the existing `Mesh3D` format while the new runtime path is being validated.

## Python

Use the local Python 3.12 environment:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 stats D:\Programmation\myProjects\tgxmeshlets\spot.obj --meshlets
```

Save a meshlet preview image:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 view D:\Programmation\myProjects\tgxmeshlets\spot.obj --meshlets --save tmp\spot_meshlets.png
```

The default meshlet profile is `visibility_merge`. It balances culling quality,
meshlet test overhead and stripification, using the MCU-tuned defaults:

```text
builder        : visibility_merge
target vertices: 30
max triangles  : 127
max normal cone: 90 degrees
merge max cone : 48 degrees
visibility views: 1024 at 1024x1024
```

The `meshlet cost` is used only for the quality estimate. It subtracts a fixed per-meshlet runtime overhead from the gross number of culled triangles, making it easier to compare many small meshlets against fewer large meshlets. The tuning experiments used a cost of 3 triangle-equivalent units, which favors fewer, well-connected meshlets on MCU targets.

Interactive view:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 view D:\Programmation\myProjects\tgxmeshlets\spot.obj --meshlets
```

Compute true visibility cones using TGX itself, then inspect a fixed-view culling result:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 view D:\Programmation\myProjects\tgxmeshlets\bunny.obj --meshlets --visibility-cones --cone-source visibility --viewer pyvista --view-dir 0 0 1 --cull-view
```

The default visibility-cone pass uses 2048 orthographic views rendered at 768x768 by the TGX helper. The default margin is automatic: it is based on the average angular coverage of the sampled views, with a small safety allowance for non-perfect sampling and raster effects.

Export a normalized model as a `Mesh3Dv2<RGB565>` header. Without an explicit
`--profile`, the converter uses the `visibility_merge` defaults above:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 export D:\Programmation\myProjects\tgxmeshlets\bunny.obj -o tmp\bunny_mesh3dv2.h --name bunny_mesh3dv2 --normalize
```

The export command:

- removes duplicate vertex, normal and texture-coordinate entries,
- snaps vertices, UVs and normals to conservative grids before duplicate removal,
- recenters and rescales the model when `--normalize` is used,
- regenerates smooth normals if the OBJ has no complete normal set, or when `--force-normals` is used,
- removes degenerate triangles,
- splits the mesh into meshlets,
- encodes local triangle chains into the 8-bit meshlet face stream,
- writes a 32-bit aligned payload array,
- prints meshlet, culling, strip and payload statistics.

## Triangle Stripifier Helpers

The converter uses `tools.mesh3d2_converter.stripifier` as the single stripification module.
It receives one strict compatible connected component, where adjacency means a shared edge
with matching `(vertex, texcoord, normal)` corners, and returns triangle chains.

Build the optional external helpers with:

```powershell
py -3.12 tools\mesh3d2_converter\setup_stripifiers.py
```

The module always keeps a fast greedy fallback, runs the bundled GA-EAX helper when built,
and also runs the patched sparse LKH helper when an LKH 2.x source tree is available under
`tools\mesh3d2_converter\external_libs\LKH`. The only high-level tuning knobs are the total thread count and a time
budget; the module chooses small, medium and large component profiles internally.

For a guided conversion, use the interactive wizard:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 wizard model.obj
```

Textures can be linked by symbol name if they already exist:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 export model.obj -o model_mesh3dv2.h --texture-symbol Body=model_body_texture --include model_body_texture.h
```

They can also be generated automatically from OBJ/MTL `map_Kd` entries:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 export model.obj -o model_mesh3dv2.h --profile auto --export-textures --texture-size 128 128
```

If no texture symbol is provided or generated for a material, that material is exported with
a `nullptr` texture and its diffuse material color.

Interactive textured preview is available with PyVista:

```powershell
py -3.12 -m tools.mesh3d2_converter.tgx_mesh3d2 view model.obj --meshlets --profile auto --viewer pyvista --texture
```

## Converting Existing Mesh3D Headers

Existing TGX `Mesh3D` headers can be converted directly without going back to the original
OBJ file:

```powershell
py -3.12 -m tools.mesh3d2_converter.mesh3d_to_mesh3d2 examples\Teensy4\3D\mars\falcon\falcon_vs.h -o falcon_vs_m2.h --root falcon_vs_1 --name falcon_vs_m2
```

The legacy converter:

- decodes the original 16-bit `Mesh3D` triangle-chain stream,
- keeps the exact same vertex, normal and texture-coordinate indices,
- preserves mesh material colors and lighting coefficients,
- links generated meshlet materials to the original texture image symbols,
- writes relative includes to the original texture headers,
- builds meshlets and 8-bit local face streams for the new runtime path.

The default `visibility_merge` profile keeps meshlets inside a single material and
groups exported meshlets by material so the runtime lazy material loader changes
texture/color state as rarely as possible. The older `auto` profile is still
available explicitly for quick heuristic conversions.

For best culling quality, compute visibility cones from the complete mesh, not from each
material independently:

```powershell
py -3.12 -m tools.mesh3d2_converter.mesh3d_to_mesh3d2 model.h -o model_m2.h --root model_1 --visibility-cones --visibility-samples 1024 --visibility-size 512
```

The visibility helper renders the whole mesh and only uses the meshlet assignment as a color
ID, so one material/submesh can occlude another during offline cone construction.

When the legacy header contains a single mesh chain, `--root` is optional. For files with
several independent chains, pass the first `Mesh3D` symbol explicitly.

The current implementation uses packages available in the local `tgxmesh3d2` Python environment: `numpy`, `matplotlib`, `scipy`, `scikit-learn`, `numba`, `trimesh`, `meshio`, `pyvista` and `vtk`.
