from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

from .meshlets import build_meshlets
from .obj_loader import load_obj
from .preprocess import preprocess_mesh
from .stripifier import DEFAULT_LKH_EXE, strip_stats
from .viewer import show_meshlets, show_meshlets_pyvista
from .texture_exporter import export_texture_rgb565_header, identifier as texture_identifier
from .pipeline import (
    add_build_options as _add_build_options,
    add_visibility_options as _add_visibility_options,
    build_meshlets_for_args,
    export_common,
    finalize_meshlets_for_export,
    maybe_compute_visibility_cones as _maybe_compute_visibility_cones,
    options_from_args as _options,
    print_cull_quality as _print_cull_quality,
    print_mesh_stats as _print_mesh_stats,
    print_preprocess_stats as _print_preprocess_stats,
)


def cmd_stats(args: argparse.Namespace) -> None:
    mesh = load_obj(args.obj)
    if args.meshlets:
        meshlets = build_meshlets(mesh, _options(args, mesh))
        meshlets = _maybe_compute_visibility_cones(args, mesh, meshlets)
    else:
        meshlets = None
    _print_mesh_stats(mesh, meshlets)
    if meshlets is not None:
        _print_cull_quality(meshlets, args.cull_samples, args.meshlet_cost, args.cone_source)


def cmd_view(args: argparse.Namespace) -> None:
    mesh = load_obj(args.obj)
    meshlets = build_meshlets(mesh, _options(args, mesh)) if args.meshlets else None
    if args.meshlets:
        meshlets = _maybe_compute_visibility_cones(args, mesh, meshlets)
        _print_mesh_stats(mesh, meshlets)
        _print_cull_quality(meshlets, args.cull_samples, args.meshlet_cost, args.cone_source)
    if args.viewer == "pyvista":
        if args.only_meshlet is not None and meshlets is not None:
            meshlets = [meshlets[args.only_meshlet]]
        view_dir = np.asarray(args.view_dir, dtype=np.float64) if args.view_dir is not None else None
        camera_pos = np.asarray(args.camera_pos, dtype=np.float64) if args.camera_pos is not None else None
        camera_target = np.asarray(args.camera_target, dtype=np.float64) if args.camera_target is not None else None
        show_meshlets_pyvista(
            mesh,
            meshlets,
            save=args.save,
            color_by=args.color_by,
            view_dir=view_dir,
            cull_by_view=args.cull_view,
            cone_source=args.cone_source,
            camera_pos=camera_pos,
            camera_target=camera_target,
            cull_from_camera=args.cull_from_camera,
            texture=args.texture,
        )
    else:
        if args.cull_view:
            raise ValueError("--cull-view currently requires --viewer pyvista")
        show_meshlets(mesh, meshlets, only_meshlet=args.only_meshlet, max_triangles=args.max_triangles_draw, save=args.save, color_by=args.color_by)
    if args.save:
        print(f"wrote {Path(args.save)}")


def cmd_strips(args: argparse.Namespace) -> None:
    mesh = load_obj(args.obj)
    meshlets = build_meshlets(mesh, _options(args, mesh))
    meshlets = _maybe_compute_visibility_cones(args, mesh, meshlets)
    _print_mesh_stats(mesh, meshlets)
    _print_cull_quality(meshlets, args.cull_samples, args.meshlet_cost, args.cone_source)
    stats = strip_stats(mesh, meshlets, args.lkh)
    print("triangle chains:")
    print(f"  meshlets        : {stats.meshlets}")
    print(f"  triangles       : {stats.triangles}")
    print(f"  LKH runs        : {stats.lkh_runs}")
    print(f"  simple chains   : {stats.simple_chains} ({100.0 * stats.simple_chains / stats.triangles:.2f}% of triangles)")
    print(f"  LKH chains      : {stats.lkh_chains} ({100.0 * stats.lkh_chains / stats.triangles:.2f}% of triangles)")
    print(f"  simple loads    : {stats.simple_vertices_loaded} vertex refs ({stats.simple_vertices_loaded / stats.triangles:.3f} per tri)")
    print(f"  LKH loads       : {stats.lkh_vertices_loaded} vertex refs ({stats.lkh_vertices_loaded / stats.triangles:.3f} per tri)")
    if stats.simple_chains:
        print(f"  chain reduction : {100.0 * (stats.simple_chains - stats.lkh_chains) / stats.simple_chains:.1f}%")
    print(f"  LKH overhead    : {100.0 * stats.lkh_extra_vertex_refs / stats.triangles:.1f}% extra vertex refs vs ideal single streams")


def cmd_export(args: argparse.Namespace) -> None:
    mesh = load_obj(args.obj)
    mesh, prep = preprocess_mesh(
        mesh,
        normalize=args.normalize,
        normalize_size=args.normalize_size,
        dedupe_epsilon=args.dedupe_epsilon,
        force_normals=args.force_normals,
    )
    meshlets, _ = build_meshlets_for_args(args, mesh, source="obj")
    meshlets, cone_source = finalize_meshlets_for_export(args, mesh, meshlets)

    texture_symbols: dict[str, str] = {}
    for item in args.texture_symbol:
        if "=" not in item:
            raise ValueError("--texture-symbol expects MATERIAL=SYMBOL")
        material, symbol = item.split("=", 1)
        texture_symbols[material] = symbol
    extra_includes = list(args.include)
    output = Path(args.output)

    if args.export_textures:
        texture_dir = Path(args.texture_output_dir) if args.texture_output_dir else output.parent
        resize = tuple(args.texture_size) if args.texture_size else None
        for material, desc in sorted(mesh.materials.items()):
            if not desc.texture_path or material in texture_symbols:
                continue
            tex_symbol = texture_identifier(f"{(args.name or output.stem)}_{material or 'default'}_texture")
            tex = export_texture_rgb565_header(
                desc.texture_path,
                texture_dir,
                symbol=tex_symbol,
                resize=resize,
                require_pow2=args.texture_require_pow2,
            )
            texture_symbols[material] = tex.symbol
            rel = tex.header.name if tex.header.parent == output.parent else str(tex.header)
            extra_includes.append(rel.replace("\\", "/"))
            print(f"texture: {material or '[default]'} -> {tex.header} ({tex.width}x{tex.height})")

    result = export_common(
        args,
        mesh,
        meshlets,
        output=output,
        color_type=args.color_type,
        texture_symbols=texture_symbols,
        extra_includes=extra_includes,
    )

    _print_preprocess_stats(prep)
    _print_mesh_stats(mesh, meshlets)
    _print_cull_quality(meshlets, args.cull_samples, args.meshlet_cost, cone_source)
    stats = result.stats
    print("Mesh3D2 export:")
    print(f"  output          : {output}")
    print(f"  color type      : {args.color_type}")
    print(f"  materials       : {stats.materials}")
    if texture_symbols:
        print(f"  texture links   : {len(texture_symbols)}")
    print(f"  chains          : {stats.chains} ({100.0 * stats.chains / stats.triangles:.2f}% of triangles)")
    print(f"  vertex refs     : {stats.vertex_refs_loaded} ({stats.vertex_refs_loaded / stats.triangles:.3f} per tri)")
    print(f"  payload         : {stats.payload_bytes} bytes ({stats.payload_words} uint32 words)")
    print(f"  LKH chains      : {'no' if args.no_lkh else 'yes'}")


def _ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value or default


def cmd_wizard(args: argparse.Namespace) -> None:
    obj = args.obj or _ask("OBJ file")
    if not obj:
        raise ValueError("an OBJ file is required")
    obj_path = Path(obj)
    default_name = obj_path.stem + "_mesh3d2"
    name = _ask("C++ mesh symbol", default_name)
    output = _ask("Output header", str(Path.cwd() / (name + ".h")))
    profile = _ask("Meshlet profile (auto/smooth/fast/coarse/custom)", "auto")
    normalize = _ask("Normalize to [-1,1] box (y/n)", "y").lower().startswith("y")
    visibility = _ask("Compute TGX visibility cones (y/n)", "y").lower().startswith("y")
    textures = _ask("Export OBJ material textures (y/n)", "y").lower().startswith("y")
    show = _ask("Open interactive preview after export (y/n)", "y").lower().startswith("y")

    export_args = argparse.Namespace(
        obj=str(obj_path),
        output=output,
        name=name,
        color_type=args.color_type,
        normalize=normalize,
        normalize_size=2.0,
        dedupe_epsilon=1e-9,
        force_normals=False,
        no_lkh=False,
        lkh=str(DEFAULT_LKH_EXE),
        texture_symbol=[],
        export_textures=textures,
        texture_output_dir=None,
        texture_size=None,
        texture_require_pow2=False,
        include=[],
        cull_samples=128,
        meshlet_cost=3.0,
        profile=profile,
        builder="greedy",
        target_vertices=52,
        max_vertices=127,
        max_triangles=70,
        max_normal_angle=42.0,
        radius_weight=7.0,
        normal_weight=18.0,
        vertex_weight=2.0,
        stop_score=4.8,
        merge_passes=1,
        smooth_passes=0,
        merge_max_normal_angle=48.0,
        visibility_cones=visibility,
        visibility_samples=2048,
        visibility_size=768,
        visibility_margin=-1.0,
        visibility_helper=None,
        keep_visibility_files=False,
        cone_source="visibility" if visibility else "normal",
    )
    cmd_export(export_args)

    if show:
        view_args = argparse.Namespace(
            obj=str(obj_path),
            meshlets=True,
            only_meshlet=None,
            max_triangles_draw=None,
            save=None,
            viewer="pyvista",
            color_by="meshlet",
            cull_samples=128,
            meshlet_cost=3.0,
            view_dir=None,
            camera_pos=None,
            camera_target=None,
            cull_view=False,
            cull_from_camera=False,
            texture=True,
            profile=profile,
            builder="greedy",
            target_vertices=52,
            max_vertices=127,
            max_triangles=70,
            max_normal_angle=42.0,
            radius_weight=7.0,
            normal_weight=18.0,
            vertex_weight=2.0,
            stop_score=4.8,
            merge_passes=1,
            smooth_passes=0,
            merge_max_normal_angle=48.0,
            visibility_cones=False,
            visibility_samples=2048,
            visibility_size=768,
            visibility_margin=-1.0,
            visibility_helper=None,
            keep_visibility_files=False,
            cone_source="normal",
        )
        cmd_view(view_args)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="TGX Mesh3D2 OBJ conversion helper")
    sub = parser.add_subparsers(dest="command", required=True)

    p_stats = sub.add_parser("stats", help="load an OBJ and print mesh/meshlet statistics")
    p_stats.add_argument("obj")
    p_stats.add_argument("--meshlets", action="store_true")
    p_stats.add_argument("--cull-samples", type=int, default=128)
    p_stats.add_argument("--meshlet-cost", type=float, default=2.0)
    _add_build_options(p_stats)
    _add_visibility_options(p_stats)
    p_stats.set_defaults(func=cmd_stats)

    p_view = sub.add_parser("view", help="visualize an OBJ mesh, optionally colored by meshlet")
    p_view.add_argument("obj")
    p_view.add_argument("--meshlets", action="store_true")
    p_view.add_argument("--only-meshlet", type=int)
    p_view.add_argument("--max-triangles-draw", type=int)
    p_view.add_argument("--save")
    p_view.add_argument("--viewer", choices=("matplotlib", "pyvista"), default="matplotlib")
    p_view.add_argument("--color-by", choices=("meshlet", "normal_angle", "cull_angle", "visibility_angle", "visibility_samples", "radius", "triangles", "vertices"), default="meshlet")
    p_view.add_argument("--cull-samples", type=int, default=128)
    p_view.add_argument("--meshlet-cost", type=float, default=2.0)
    p_view.add_argument("--view-dir", nargs=3, type=float, metavar=("X", "Y", "Z"))
    p_view.add_argument("--camera-pos", nargs=3, type=float, metavar=("X", "Y", "Z"))
    p_view.add_argument("--camera-target", nargs=3, type=float, metavar=("X", "Y", "Z"))
    p_view.add_argument("--cull-view", action="store_true")
    p_view.add_argument("--cull-from-camera", action="store_true")
    p_view.add_argument("--texture", action="store_true", help="show OBJ/MTL textures in the PyVista viewer when texture coordinates are available")
    _add_build_options(p_view)
    _add_visibility_options(p_view)
    p_view.set_defaults(func=cmd_view)

    p_strips = sub.add_parser("strips", help="build meshlets and compare simple vs LKH triangle chains")
    p_strips.add_argument("obj")
    p_strips.add_argument("--lkh", default=str(DEFAULT_LKH_EXE))
    p_strips.add_argument("--cull-samples", type=int, default=128)
    p_strips.add_argument("--meshlet-cost", type=float, default=2.0)
    _add_build_options(p_strips)
    _add_visibility_options(p_strips)
    p_strips.set_defaults(func=cmd_strips)

    p_export = sub.add_parser("export", help="convert an OBJ mesh to a TGX Mesh3D2 C++ header")
    p_export.add_argument("obj")
    p_export.add_argument("-o", "--output", required=True)
    p_export.add_argument("--name")
    p_export.add_argument("--color-type", choices=("tgx::RGB565", "tgx::RGB32", "tgx::RGBf"), default="tgx::RGB565")
    p_export.add_argument("--normalize", action="store_true", help="center the model and fit its largest extent to --normalize-size")
    p_export.add_argument("--normalize-size", type=float, default=2.0)
    p_export.add_argument("--dedupe-epsilon", type=float, default=1e-9)
    p_export.add_argument("--force-normals", action="store_true")
    p_export.add_argument("--no-lkh", action="store_true")
    p_export.add_argument("--lkh", default=str(DEFAULT_LKH_EXE))
    p_export.add_argument("--texture-symbol", action="append", default=[], metavar="MATERIAL=SYMBOL", help="link an OBJ material to an existing tgx::Image symbol")
    p_export.add_argument("--export-textures", action="store_true", help="convert map_Kd textures from OBJ materials to RGB565 TGX headers")
    p_export.add_argument("--texture-output-dir", help="directory for generated texture headers; defaults to the mesh header directory")
    p_export.add_argument("--texture-size", nargs=2, type=int, metavar=("W", "H"), help="resize all exported textures to W x H")
    p_export.add_argument("--texture-require-pow2", action="store_true", help="fail when exported textures are not power-of-two sized")
    p_export.add_argument("--include", action="append", default=[], help="extra header to include before the Mesh3D2 declaration")
    p_export.add_argument("--cull-samples", type=int, default=128)
    p_export.add_argument("--meshlet-cost", type=float, default=2.0)
    _add_build_options(p_export)
    _add_visibility_options(p_export)
    p_export.set_defaults(func=cmd_export)

    p_wizard = sub.add_parser("wizard", help="interactive OBJ to Mesh3D2 conversion")
    p_wizard.add_argument("obj", nargs="?")
    p_wizard.add_argument("--color-type", choices=("tgx::RGB565", "tgx::RGB32", "tgx::RGBf"), default="tgx::RGB565")
    p_wizard.set_defaults(func=cmd_wizard)

    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
