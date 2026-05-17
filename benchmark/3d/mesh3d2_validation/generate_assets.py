from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))

from tools.mesh3d2_converter.cones import apply_visibility_cones, auto_visibility_margin_deg
from tools.mesh3d2_converter.exporter import export_mesh3d2_header
from tools.mesh3d2_converter.legacy_exporter import export_legacy_mesh3d_header
from tools.mesh3d2_converter.meshlets import MeshletBuildOptions, build_meshlets
from tools.mesh3d2_converter.obj_loader import load_obj
from tools.mesh3d2_converter.preprocess import preprocess_mesh
from tools.mesh3d2_converter.quality import fibonacci_sphere
from tools.mesh3d2_converter.validate import validate_mesh_for_export
from tools.mesh3d2_converter.visibility import compute_tgx_visibility


MODELS = [
    ("bunny", Path(r"D:\Programmation\myProjects\tgxmeshlets\bunny.obj")),
    ("dragon", Path(r"D:\Programmation\myProjects\tgxmeshlets\dragon.obj")),
    ("suzanne", Path(r"D:\Programmation\myProjects\tgxmeshlets\suzanne.obj")),
    ("spot", Path(r"D:\Programmation\myProjects\tgxmeshlets\spot.obj")),
]


def main() -> int:
    out = Path(__file__).resolve().parent / "generated"
    out.mkdir(parents=True, exist_ok=True)

    includes = []
    for name, obj in MODELS:
        print(f"Generating {name} from {obj}")
        mesh = load_obj(obj)
        mesh, _stats = preprocess_mesh(mesh, normalize=True, force_normals=False)
        mesh.name = name
        meshlets = build_meshlets(mesh, MeshletBuildOptions())
        views = fibonacci_sphere(512)
        margin = auto_visibility_margin_deg(len(views))
        visibility = compute_tgx_visibility(mesh, meshlets, views, width=384, height=384)
        meshlets = apply_visibility_cones(meshlets, views, visibility, margin_deg=margin)
        validate_mesh_for_export(mesh, meshlets)

        legacy = export_legacy_mesh3d_header(mesh, name=f"legacy_{name}", meshlets=meshlets)
        mesh3d2 = export_mesh3d2_header(mesh, meshlets, name=f"m2_{name}")
        (out / f"legacy_{name}.h").write_text(legacy.header, encoding="utf-8", newline="\n")
        (out / f"m2_{name}.h").write_text(mesh3d2.header, encoding="utf-8", newline="\n")
        includes.append((name, legacy.chains, mesh3d2.stats.meshlets, mesh3d2.stats.chains))
        print(f"  legacy chains={legacy.chains}, Mesh3D2 meshlets={mesh3d2.stats.meshlets}, chains={mesh3d2.stats.chains}")

    lines = ["#pragma once", ""]
    for name, *_ in includes:
        lines.append(f'#include "legacy_{name}.h"')
        lines.append(f'#include "m2_{name}.h"')
    lines.append("")
    lines.append("struct Mesh3D2ValidationModelInfo { const char* name; int legacy_chains; int meshlets; int mesh3d2_chains; };")
    lines.append("static const Mesh3D2ValidationModelInfo mesh3d2_validation_model_info[] = {")
    for name, legacy_chains, meshlets, chains in includes:
        lines.append(f'    {{ "{name}", {legacy_chains}, {meshlets}, {chains} }},')
    lines.append("};")
    (out / "all_meshes.h").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
