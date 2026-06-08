from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[4]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools._internal.modules.mesh_pipeline.exporter import MeshletExportResult
from tools._internal.modules.mesh_pipeline.legacy_exporter import LegacyExportResult, export_legacy_mesh3d_header
from tools._internal.modules.mesh_pipeline.mesh import FaceVertex, Material, ObjMesh, Triangle
from tools._internal.modules.mesh_pipeline.mesh3d_to_mesh3d2 import (
    DEFAULT_NORMAL_QUANT_BITS,
    DEFAULT_TEXCOORD_QUANT_BITS,
    DEFAULT_VERTEX_QUANT_BITS,
    _relative_includes,
    legacy_to_objmesh,
    parse_legacy_header,
    preprocess_legacy_mesh,
)
from tools._internal.modules.mesh_pipeline.mesh_inspect import DecodedHeaderMesh, _parse_texture_headers, _parse_texture_image, detect_mesh_type, parse_mesh3d2_header
from tools._internal.modules.mesh_pipeline.obj_loader import load_obj
from tools._internal.modules.mesh_pipeline.pipeline import build_meshlets_for_args, export_common, finalize_meshlets_for_export, print_cull_quality, print_mesh_stats, print_preprocess_stats
from tools._internal.modules.mesh_pipeline.preprocess import preprocess_mesh
from tools._internal.modules.mesh_pipeline.profiles import DEFAULT_MAX_NORMAL_ANGLE_DEG, DEFAULT_TARGET_VERTICES
from tools._internal.modules.mesh_pipeline.progress import step
from tools._internal.modules.mesh_pipeline.stripifier import DEFAULT_LKH_EXE, solver_stats_text

from _internal.modules.image.core import ImageExportOptions, SUPPORTED_COLOR_TYPES, SUPPORTED_LAYOUTS, SUPPORTED_RESAMPLING, export_image, normalize_color_type, parse_resize, sanitize_identifier
from _internal.modules.mesh.textures import (
    IMAGE_EXTENSIONS,
    apply_texture_choices,
    clear_emissive_texture,
    confirm_texture_choices,
    parse_texture_overrides,
    resolve_mesh_textures,
    texture_choices_text,
)


@dataclass
class LoadedMesh:
    mesh: ObjMesh
    source: str
    color_type: str
    texture_symbols: dict[str, str]
    emissive_texture_symbols: dict[str, str]
    texture_sources: dict[str, Path]
    texture_source_symbols: dict[str, str]
    emissive_texture_sources: dict[str, Path]
    emissive_texture_source_symbols: dict[str, str]
    extra_includes: list[str]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert OBJ or TGX mesh headers to Mesh3D or Mesh3Dv2.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("input", help="Input .obj, Mesh3D header, or Mesh3Dv2 header")
    parser.add_argument("-o", "--output", required=True, help="Output mesh header path")
    parser.add_argument("--source", choices=("auto", "obj", "mesh3d", "mesh3dv2"), default="auto", help="Input type")
    parser.add_argument("--format", choices=("Mesh3D", "Mesh3Dv2"), default="Mesh3Dv2", help="Output mesh format")
    parser.add_argument("--name", help="Output C++ mesh symbol")
    parser.add_argument("--root", help="Root Mesh3D symbol when a legacy header contains several chains")
    parser.add_argument("--layout", choices=("header", "split"), default="header", help="'header' creates one .h; 'split' creates .h + .cpp")
    parser.add_argument("--color-type", choices=("RGB565", "RGB24", "RGB32", "RGBf", "tgx::RGB565", "tgx::RGB24", "tgx::RGB32", "tgx::RGBf"), help="Output TGX color type")

    clean = parser.add_argument_group("mesh cleanup")
    clean.add_argument("--normalize", action="store_true", help="For OBJ input: center the model and fit it to --normalize-size")
    clean.add_argument("--normalize-size", type=float, default=2.0, help="Normalized bounding-box size")
    normal_mode = clean.add_mutually_exclusive_group()
    normal_mode.add_argument("--force-normals", action="store_true", help="Recompute smooth vertex normals before cleanup")
    normal_mode.add_argument("--remove-normals", action="store_true", help="Drop all normals before cleanup")
    clean.add_argument("--no-cleanup", action="store_true", help="Disable quantize/dedupe cleanup")
    clean.add_argument("--texcoord-wrap", action="store_true", help="Identify UVs modulo 1 during cleanup; only use when this preserves the texture mapping")

    meshlets = parser.add_argument_group("Mesh3Dv2 advanced meshlet options")
    meshlets.add_argument("--compact", action="store_true", help="Build larger Mesh3Dv2 meshlets to minimize static memory while still exporting visibility cones")
    meshlets.add_argument("--compact-compression", type=float, default=None, metavar="0..100", help="Non-adjacent packing strength used with --compact: 0 disables the second pass and is the conservative GUI default; if omitted after --compact, the CLI uses 50; 100 is ultra-compact")
    meshlets.add_argument("--target-vertices", type=int, default=None, help=f"Preferred meshlet vertex count for classic visibility mode; default is {DEFAULT_TARGET_VERTICES}")
    meshlets.add_argument("--max-normal-angle", type=float, default=None, help=f"Maximum normal spread accepted during classic visibility-merge; default is {DEFAULT_MAX_NORMAL_ANGLE_DEG:g}")
    meshlets.add_argument("--visibility-samples", type=int, default=None, help="Number of TGX visibility directions used by visibility-merge; default is 1024")
    meshlets.add_argument("--visibility-size", type=int, default=None, help="Visibility render size used by visibility-merge; default is 1024")
    meshlets.add_argument("--visibility-margin", type=float, default=None, help="Visibility cone margin in degrees; default is automatic")

    textures = parser.add_argument_group("texture export")
    textures.add_argument("--no-textures", action="store_true", help="Do not export or copy material textures")
    textures.add_argument("--texture-format", choices=SUPPORTED_COLOR_TYPES, default="RGB565", help="TGX color type for generated textures")
    textures.add_argument("--texture-size", type=parse_resize, help="Resize all generated textures to WIDTHxHEIGHT")
    textures.add_argument("--texture-resize", action="append", default=[], metavar="MATERIAL=WIDTHxHEIGHT", help="Resize one material texture; overrides --texture-size for that material")
    textures.add_argument("--texture-resample", choices=SUPPORTED_RESAMPLING, default="lanczos", help="Texture resize filter")
    textures.add_argument("--texture-filter", action="append", default=[], metavar="MATERIAL=FILTER", help="Resize filter for one material texture; overrides --texture-resample")
    textures.add_argument("--texture-layout", choices=SUPPORTED_LAYOUTS, default="header", help="Generated texture layout")
    textures.add_argument("--texture-output-dir", help="Directory for generated texture headers; defaults to the mesh output directory")
    textures.add_argument("--texture-search", action="append", default=[], metavar="DIR", help="Extra directory searched when OBJ/MTL texture paths are missing")
    textures.add_argument("--texture", action="append", default=[], metavar="MATERIAL=PATH", help="Use this image or TGX texture header for one material")
    textures.add_argument("--skip-texture", action="append", default=[], metavar="MATERIAL", help="Do not assign a texture to this material")
    textures.add_argument("--emissive-texture", action="append", default=[], metavar="MATERIAL=PATH", help="Use this image or TGX texture header as the emissive texture for one material")
    textures.add_argument("--skip-emissive-texture", action="append", default=[], metavar="MATERIAL", help="Do not assign an emissive texture to this material")
    textures.add_argument("--emissive-texture-resize", action="append", default=[], metavar="MATERIAL=WIDTHxHEIGHT", help="Resize one material emissive texture; overrides --texture-size for that material")
    textures.add_argument("--emissive-texture-filter", action="append", default=[], metavar="MATERIAL=FILTER", help="Resize filter for one material emissive texture; overrides --texture-resample")
    textures.add_argument("--texture-straight-alpha", action="store_true", help="Keep RGB32/RGB64 alpha straight instead of TGX pre-multiplied alpha")
    textures.add_argument("--texture-symbol", action="append", default=[], metavar="MATERIAL=SYMBOL", help="Use an existing texture symbol for a material")
    textures.add_argument("--emissive-texture-symbol", action="append", default=[], metavar="MATERIAL=SYMBOL", help="Use an existing emissive texture symbol for a material")
    textures.add_argument("--texture-name", action="append", default=[], metavar="MATERIAL=NAME", help="Use this generated texture symbol name for one diffuse texture export")
    textures.add_argument("--emissive-texture-name", action="append", default=[], metavar="MATERIAL=NAME", help="Use this generated texture symbol name for one emissive texture export")
    textures.add_argument("--list-textures", action="store_true", help="Print resolved OBJ texture choices and exit")
    textures.add_argument("--confirm-textures", action="store_true", help="Interactively confirm or override OBJ texture choices")
    textures.add_argument("--include", action="append", default=[], help="Extra quoted include to add before the mesh declaration")

    materials = parser.add_argument_group("material overrides")
    materials.add_argument("--material-color", action="append", default=[], metavar="MATERIAL=R,G,B", help="Override one material diffuse color")
    materials.add_argument("--material-ambiant", "--material-ambient", dest="material_ambiant", action="append", default=[], metavar="MATERIAL=VALUE", help="Override one material ambient strength")
    materials.add_argument("--material-diffuse", action="append", default=[], metavar="MATERIAL=VALUE", help="Override one material diffuse strength")
    materials.add_argument("--material-specular", action="append", default=[], metavar="MATERIAL=VALUE", help="Override one material specular strength")
    materials.add_argument("--material-exponent", action="append", default=[], metavar="MATERIAL=VALUE", help="Override one material specular exponent")
    materials.add_argument("--emissive-color", action="append", default=[], metavar="MATERIAL=R,G,B", help="Override one material emissive color")
    materials.add_argument("--emissive-strength", action="append", default=[], metavar="MATERIAL=VALUE", help="Override one material emissive strength")
    materials.add_argument("--material-simple", action="append", default=[], metavar="MATERIAL", help="Force a material to omit MeshMaterialExtra3Dv2 data")
    materials.add_argument("--material-rename", action="append", default=[], metavar="OLD=NEW", help="Rename a material after loading and texture processing")

    strip = parser.add_argument_group("stripifier")
    strip.add_argument("--lkh", default=str(DEFAULT_LKH_EXE), help="Optional patched LKH helper path")
    strip.add_argument("--strip-time-limit", type=float, help="Override automatic stripifier budget, in seconds per compatible component")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        convert(args)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


def convert(args: argparse.Namespace) -> LegacyExportResult | MeshletExportResult:
    output = Path(args.output)

    with step("load input", args.input):
        loaded = _load_input(args, output)
    _apply_material_overrides(args, loaded.mesh)
    _apply_material_renames(args, loaded)

    if args.format == "Mesh3D":
        return _export_mesh3d(args, loaded, output)
    return _export_mesh3dv2(args, loaded, output)


def _load_input(args: argparse.Namespace, output: Path) -> LoadedMesh:
    path = Path(args.input)
    source = _detect_source(path, args.source)
    if source == "obj":
        return _load_obj_input(args, path, output)
    if source == "mesh3d":
        return _load_legacy_input(args, path, output)
    if source == "mesh3dv2":
        return _load_mesh3dv2_input(args, path, output)
    raise ValueError(f"unsupported input source: {source}")


def _detect_source(path: Path, requested: str) -> str:
    if requested != "auto":
        return requested
    if path.suffix.lower() == ".obj":
        return "obj"
    mesh_type = detect_mesh_type(path)
    return "mesh3dv2" if mesh_type == "Mesh3Dv2" else "mesh3d"


def _load_obj_input(args: argparse.Namespace, path: Path, output: Path) -> LoadedMesh:
    with step("load OBJ", str(path)):
        mesh = load_obj(path)
    mesh = _preprocess_obj(args, mesh)
    color_type = _color_type(args.color_type, "tgx::RGB565")
    texture_symbols = _parse_texture_symbols(args.texture_symbol)
    emissive_texture_symbols = _parse_texture_symbols(args.emissive_texture_symbol)
    extra_includes = list(args.include)
    if not args.no_textures:
        choices = resolve_mesh_textures(
            mesh,
            search_roots=[Path(p) for p in args.texture_search],
            overrides=parse_texture_overrides(args.texture),
        )
        for material in args.skip_texture:
            if material in choices:
                choices[material].selected = None
                mesh.materials[material].texture_path = None
        print("texture choices:")
        print(texture_choices_text(choices) or "  none")
        emissive_choices = {}
        if args.format == "Mesh3Dv2":
            emissive_choices = resolve_mesh_textures(
                mesh,
                search_roots=[Path(p) for p in args.texture_search],
                overrides=parse_texture_overrides(args.emissive_texture),
                role="map_ke",
                guess_when_missing=False,
            )
            for material in args.skip_emissive_texture:
                if material in emissive_choices:
                    emissive_choices[material].selected = None
                    clear_emissive_texture(mesh.materials[material])
            print("emissive texture choices:")
            print(texture_choices_text(emissive_choices) or "  none")
        if args.confirm_textures:
            confirm_texture_choices(choices)
            apply_texture_choices(mesh, choices)
            if args.format == "Mesh3Dv2":
                confirm_texture_choices(emissive_choices)
                apply_texture_choices(mesh, emissive_choices)
        if args.list_textures:
            raise SystemExit(0)
        _export_obj_textures(args, mesh, output, texture_symbols, extra_includes, role="diffuse")
        if args.format == "Mesh3Dv2":
            _export_obj_textures(args, mesh, output, emissive_texture_symbols, extra_includes, role="emissive")
    return LoadedMesh(mesh, "obj", color_type, texture_symbols, emissive_texture_symbols, {}, {}, {}, {}, extra_includes)


def _load_legacy_input(args: argparse.Namespace, path: Path, output: Path) -> LoadedMesh:
    with step("parse legacy Mesh3D", str(path)):
        parsed = parse_legacy_header(path)
        mesh, legacy_textures, chain = legacy_to_objmesh(parsed, root=args.root, name=args.name)
    if not args.no_cleanup:
        with step("preprocess mesh", f"{len(mesh.triangles)} triangles"):
            mesh = preprocess_legacy_mesh(
                mesh,
                vertex_quant_bits=DEFAULT_VERTEX_QUANT_BITS,
                texcoord_quant_bits=DEFAULT_TEXCOORD_QUANT_BITS,
                texcoord_wrap=args.texcoord_wrap,
                normal_quant_bits=DEFAULT_NORMAL_QUANT_BITS,
                force_normals=args.force_normals,
                remove_normals=args.remove_normals,
            )
    texture_headers = _parse_texture_headers(path.read_text(encoding="utf-8", errors="replace"), path.parent)
    texture_sources = {material: texture_headers[symbol] for material, symbol in legacy_textures.items() if symbol in texture_headers}
    texture_source_symbols = dict(legacy_textures)
    texture_symbols = {**legacy_textures, **_parse_texture_symbols(args.texture_symbol)}
    emissive_texture_symbols = _parse_texture_symbols(args.emissive_texture_symbol)
    texture_include_paths = {p.resolve() for p in texture_headers.values()}
    non_texture_includes = [inc for inc in parsed.includes if not isinstance(inc, Path) or inc.resolve() not in texture_include_paths]
    extra_includes = _relative_includes(non_texture_includes, output) + list(args.include)
    _export_tgx_textures(args, output, texture_symbols, texture_sources, texture_source_symbols, extra_includes)
    if args.format == "Mesh3Dv2":
        _export_tgx_textures(
            args,
            output,
            emissive_texture_symbols,
            {},
            {},
            extra_includes,
            overrides_items=args.emissive_texture,
            skipped_items=args.skip_emissive_texture,
            resize_items=args.emissive_texture_resize,
            filter_items=args.emissive_texture_filter,
            name_items=args.emissive_texture_name,
            label="emissive texture",
            symbol_suffix="emissive_texture",
        )
    color_type = _color_type(args.color_type, chain[0].color_type)
    for material in args.skip_emissive_texture:
        if material in mesh.materials:
            clear_emissive_texture(mesh.materials[material])
    return LoadedMesh(mesh, "mesh3d", color_type, texture_symbols, emissive_texture_symbols, texture_sources, texture_source_symbols, {}, {}, extra_includes)


def _load_mesh3dv2_input(args: argparse.Namespace, path: Path, output: Path) -> LoadedMesh:
    with step("parse Mesh3Dv2", str(path)):
        decoded = parse_mesh3d2_header(path)
    mesh, texture_symbols, emissive_texture_symbols = _decoded_mesh3dv2_to_objmesh(decoded)
    texture_source_symbols = dict(texture_symbols)
    emissive_texture_source_symbols = dict(emissive_texture_symbols)
    if not args.no_cleanup:
        with step("preprocess mesh", f"{len(mesh.triangles)} triangles"):
            mesh = preprocess_legacy_mesh(
                mesh,
                vertex_quant_bits=DEFAULT_VERTEX_QUANT_BITS,
                texcoord_quant_bits=DEFAULT_TEXCOORD_QUANT_BITS,
                texcoord_wrap=args.texcoord_wrap,
                normal_quant_bits=DEFAULT_NORMAL_QUANT_BITS,
                force_normals=args.force_normals,
                remove_normals=args.remove_normals,
            )
    texture_sources = {
        material: decoded.texture_headers[symbol]
        for material, symbol in texture_symbols.items()
        if symbol in decoded.texture_headers
    }
    emissive_texture_sources = {
        material: decoded.texture_headers[symbol]
        for material, symbol in emissive_texture_symbols.items()
        if symbol in decoded.texture_headers
    }
    extra_includes = list(args.include)
    color_type = _color_type(args.color_type, decoded.color_type)
    texture_symbols.update(_parse_texture_symbols(args.texture_symbol))
    emissive_texture_symbols.update(_parse_texture_symbols(args.emissive_texture_symbol))
    for material in args.skip_emissive_texture:
        if material in mesh.materials:
            clear_emissive_texture(mesh.materials[material])
    _export_tgx_textures(args, output, texture_symbols, texture_sources, texture_source_symbols, extra_includes)
    if args.format == "Mesh3Dv2":
        _export_tgx_textures(
            args,
            output,
            emissive_texture_symbols,
            emissive_texture_sources,
            emissive_texture_source_symbols,
            extra_includes,
            overrides_items=args.emissive_texture,
            skipped_items=args.skip_emissive_texture,
            resize_items=args.emissive_texture_resize,
            filter_items=args.emissive_texture_filter,
            name_items=args.emissive_texture_name,
            label="emissive texture",
            symbol_suffix="emissive_texture",
        )
    return LoadedMesh(mesh, "mesh3dv2", color_type, texture_symbols, emissive_texture_symbols, texture_sources, texture_source_symbols, emissive_texture_sources, emissive_texture_source_symbols, extra_includes)


def _preprocess_obj(args: argparse.Namespace, mesh: ObjMesh) -> ObjMesh:
    with step("preprocess mesh", f"{len(mesh.triangles)} triangles"):
        if args.no_cleanup:
            vertex_bits = -1
            texcoord_bits = -1
            normal_bits = -1
        else:
            vertex_bits = DEFAULT_VERTEX_QUANT_BITS
            texcoord_bits = DEFAULT_TEXCOORD_QUANT_BITS
            normal_bits = DEFAULT_NORMAL_QUANT_BITS
        mesh, stats = preprocess_mesh(
            mesh,
            normalize=args.normalize,
            normalize_size=args.normalize_size,
            dedupe_epsilon=1e-8,
            force_normals=args.force_normals,
            remove_normals=args.remove_normals,
            vertex_quant_bits=vertex_bits,
            texcoord_quant_bits=texcoord_bits,
            texcoord_wrap=args.texcoord_wrap,
            normal_quant_bits=normal_bits,
        )
    print_preprocess_stats(stats)
    return mesh


def _export_mesh3d(args: argparse.Namespace, loaded: LoadedMesh, output: Path) -> LegacyExportResult:
    name = args.name or output.stem
    with step("build Mesh3D strips", f"{len(loaded.mesh.triangles)} triangles"):
        result = export_legacy_mesh3d_header(
            loaded.mesh,
            name=name,
            color_type=loaded.color_type,
            use_lkh=True,
            lkh_exe=args.lkh,
            lkh_time_limit_s=args.strip_time_limit,
            texture_symbols=loaded.texture_symbols,
            extra_includes=loaded.extra_includes,
            split_by_material=True,
            header_filename=output.name,
            single_header=args.layout == "header",
        )
    output.parent.mkdir(parents=True, exist_ok=True)
    with step("write header", str(output)):
        with output.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(result.header)
    if result.source is not None:
        source = output.with_suffix(".cpp")
        with step("write source", str(source)):
            with source.open("w", encoding="utf-8", newline="\n") as handle:
                handle.write(result.source)
    print(f"stripifier choices: {solver_stats_text()}")
    print(f"wrote {output}")
    print("Mesh3D output:")
    print(f"  source        : {loaded.source}")
    print(f"  symbol        : {name}")
    print(f"  color type    : {loaded.color_type}")
    print(f"  triangles     : {result.triangles}")
    print(f"  submeshes     : {result.meshes}")
    print(f"  chains        : {result.chains} ({100.0 * result.chains / max(1, result.triangles):.2f}% of triangles)")
    print(f"  textures      : {len(loaded.texture_symbols)}")
    return result


def _export_mesh3dv2(args: argparse.Namespace, loaded: LoadedMesh, output: Path) -> MeshletExportResult:
    _prepare_meshlet_args(args)
    meshlets, _ = build_meshlets_for_args(args, loaded.mesh, source=loaded.source)
    meshlets, cone_source = finalize_meshlets_for_export(args, loaded.mesh, meshlets)
    result = export_common(
        args,
        loaded.mesh,
        meshlets,
        output=output,
        color_type=loaded.color_type,
        cone_source=cone_source,
        texture_symbols=loaded.texture_symbols,
        emissive_texture_symbols=loaded.emissive_texture_symbols,
        extra_includes=loaded.extra_includes,
    )
    print_mesh_stats(loaded.mesh, meshlets)
    print_cull_quality(meshlets, 128, 3.0, cone_source)
    print(f"wrote {output}")
    print("Mesh3Dv2 output:")
    print(f"  source        : {loaded.source}")
    print(f"  symbol        : {args.name or output.stem}")
    print(f"  color type    : {loaded.color_type}")
    print(f"  meshlets      : {result.stats.meshlets}")
    print(f"  chains        : {result.stats.chains} ({100.0 * result.stats.chains / max(1, result.stats.triangles):.2f}% of triangles)")
    print(f"  payload       : {result.stats.payload_bytes} bytes")
    print(f"  static memory : {result.stats.static_bytes} bytes, excluding texture images")
    print(f"  textures      : {len(loaded.texture_symbols)}")
    print(f"  emissive textures: {len(loaded.emissive_texture_symbols)}")
    return result


def _prepare_meshlet_args(args: argparse.Namespace) -> None:
    args.mesh_format = "mesh3dv2"
    if args.compact:
        args.profile = "compact"
    else:
        args.profile = "visibility_merge"
    if args.compact_compression is not None and not args.compact:
        raise ValueError("--compact-compression can only be used with --compact")
    if args.compact and args.target_vertices is not None:
        raise ValueError("--target-vertices is only available in classic visibility mode; use --compact-compression with --compact")
    if args.compact and args.max_normal_angle is not None:
        raise ValueError("--max-normal-angle is only available in classic visibility mode; use --compact-compression with --compact")
    compression = 50.0 if args.compact_compression is None else float(args.compact_compression)
    if compression < 0.0 or compression > 100.0:
        raise ValueError("--compact-compression must be between 0 and 100")
    args.visibility_pack_compression = compression if args.compact else None
    args.builder = None
    args.max_vertices = None
    args.max_triangles = None
    args.max_normal_angle = args.max_normal_angle
    args.radius_weight = None
    args.normal_weight = None
    args.vertex_weight = None
    args.stop_score = None
    args.compatible_edge_weight = None
    args.incompatible_edge_penalty = None
    args.compatible_merge_weight = None
    args.visibility_merge_samples = args.visibility_samples
    args.visibility_merge_size = args.visibility_size
    args.visibility_merge_margin = args.visibility_margin
    args.visibility_merge_cone_weight = None
    args.merge_passes = None
    args.smooth_passes = None
    args.merge_max_normal_angle = None
    args.visibility_cones = False
    args.visibility_helper = None
    args.keep_visibility_files = False
    args.cone_source = "visibility"
    args.sort_by_material = True
    args.merge_small_materials = True
    args.single_header = args.layout == "header"


def _export_obj_textures(args: argparse.Namespace, mesh: ObjMesh, output: Path, texture_symbols: dict[str, str], extra_includes: list[str], *, role: str = "diffuse") -> None:
    texture_dir = Path(args.texture_output_dir) if args.texture_output_dir else output.parent
    texture_dir.mkdir(parents=True, exist_ok=True)
    if role == "emissive":
        material_resizes = _parse_material_resizes(args.emissive_texture_resize)
        material_filters = _parse_material_filters(args.emissive_texture_filter)
        material_names = _parse_texture_symbols(args.emissive_texture_name)
        skipped = set(args.skip_emissive_texture)
        path_attr = "emissive_texture_path"
        symbol_suffix = "emissive_texture"
        label = "emissive texture"
    else:
        material_resizes = _parse_material_resizes(args.texture_resize)
        material_filters = _parse_material_filters(args.texture_filter)
        material_names = _parse_texture_symbols(args.texture_name)
        skipped = set(args.skip_texture)
        path_attr = "texture_path"
        symbol_suffix = "texture"
        label = "texture"
    used_materials = {tri.material for tri in mesh.triangles}
    for material, desc in sorted(mesh.materials.items()):
        if material not in used_materials:
            continue
        texture_path = getattr(desc, path_attr)
        if material in skipped or not texture_path or material in texture_symbols:
            continue
        symbol = _generated_texture_symbol(args, output, material, symbol_suffix, material_names)
        resize = material_resizes.get(material, args.texture_size)
        resample = material_filters.get(material, args.texture_resample)
        selected = Path(texture_path).expanduser().resolve()
        if selected.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
            source_symbol = _first_tgx_image_symbol(selected)
            if not source_symbol:
                raise ValueError(f"could not find a tgx::Image object in {selected}")
            source_color = _tgx_texture_color_type(selected, source_symbol)
            can_copy = (
                resize is None
                and args.texture_layout == "header"
                and source_color == normalize_color_type(args.texture_format)
                and symbol == source_symbol
            )
            if can_copy:
                header_path, copied = _copy_tgx_texture_header(selected, texture_dir)
                _append_unique_include(extra_includes, _relative_include(header_path, output.parent))
                print(f"{label}: {material or '[default]'} -> {header_path} (copied TGX texture)")
                if copied is not None:
                    print(f"{label} source: {copied}")
                texture_symbols[material] = source_symbol
            else:
                result = _export_tgx_texture_from_header(
                    selected,
                    source_symbol,
                    symbol,
                    texture_dir,
                    args.texture_format,
                    args.texture_layout,
                    resize,
                    resample,
                    not args.texture_straight_alpha,
                )
                _append_unique_include(extra_includes, _relative_include(result.header_path, output.parent))
                texture_symbols[material] = result.object_name
                print(f"{label}: {material or '[default]'} -> {result.header_path} ({result.width}x{result.height}, {result.color_type}, {resample})")
        elif selected.suffix.lower() in IMAGE_EXTENSIONS:
            result = export_image(
                ImageExportOptions(
                    input_path=selected,
                    output_dir=texture_dir,
                    color_type=args.texture_format,
                    object_name=symbol,
                    output_base=symbol,
                    layout=args.texture_layout,
                    resize=resize,
                    resample=resample,
                    flip_y=True,
                    progmem=True,
                    premultiply_alpha=not args.texture_straight_alpha,
                )
            )
            texture_symbols[material] = result.object_name
            _append_unique_include(extra_includes, _relative_include(result.header_path, output.parent))
            print(f"{label}: {material or '[default]'} -> {result.header_path} ({result.width}x{result.height}, {result.color_type}, {resample})")
        else:
            raise ValueError(f"unsupported {label} input for material {material or '[default]'}: {selected}")


def _export_tgx_textures(
    args: argparse.Namespace,
    output: Path,
    texture_symbols: dict[str, str],
    texture_sources: dict[str, Path],
    texture_source_symbols: dict[str, str],
    extra_includes: list[str],
    *,
    all_texture_headers: dict[str, Path] | None = None,
    overrides_items: list[str] | None = None,
    skipped_items: list[str] | None = None,
    resize_items: list[str] | None = None,
    filter_items: list[str] | None = None,
    name_items: list[str] | None = None,
    label: str = "texture",
    symbol_suffix: str = "texture",
) -> None:
    if args.no_textures:
        texture_symbols.clear()
        return

    texture_dir = Path(args.texture_output_dir) if args.texture_output_dir else output.parent
    texture_dir.mkdir(parents=True, exist_ok=True)
    overrides = parse_texture_overrides(args.texture if overrides_items is None else overrides_items)
    skipped = set(args.skip_texture if skipped_items is None else skipped_items)
    material_resizes = _parse_material_resizes(args.texture_resize if resize_items is None else resize_items)
    material_filters = _parse_material_filters(args.texture_filter if filter_items is None else filter_items)
    material_names = _parse_texture_symbols(args.texture_name if name_items is None else name_items)
    exported_symbols: set[str] = set()
    exported_source_symbols: set[str] = set()
    exported_headers: set[Path] = set()
    skipped_source_symbols: set[str] = set()
    synthesized_symbols: set[str] = set()

    for material, name in material_names.items():
        if material in texture_symbols:
            texture_symbols[material] = sanitize_identifier(name)

    for material, selected in sorted(overrides.items()):
        if material in skipped or material in texture_symbols or selected is None:
            continue
        texture_symbols[material] = _generated_texture_symbol(args, output, material, symbol_suffix, material_names)
        synthesized_symbols.add(material)

    for material in sorted(list(texture_symbols)):
        if material in skipped:
            source_symbol = texture_source_symbols.get(material, texture_symbols[material])
            if source_symbol:
                skipped_source_symbols.add(source_symbol)
            del texture_symbols[material]
            continue
        symbol = texture_symbols[material]
        source_symbol = "" if material in synthesized_symbols else texture_source_symbols.get(material, symbol)
        selected = overrides.get(material, texture_sources.get(material))
        if selected is None:
            continue
        selected = selected.expanduser().resolve()
        resize = material_resizes.get(material, args.texture_size)
        resample = material_filters.get(material, args.texture_resample)
        if symbol in exported_symbols:
            continue

        if selected.suffix.lower() in (".h", ".hpp", ".cpp", ".cxx", ".cc"):
            source_color = _tgx_texture_color_type(selected, source_symbol)
            source_symbol_available = bool(source_symbol) and _parse_texture_image(selected, source_symbol) is not None
            can_copy = (
                source_symbol_available
                and resize is None
                and args.texture_layout == "header"
                and source_color == normalize_color_type(args.texture_format)
                and symbol == source_symbol
            )
            if can_copy:
                header_path, copied = _copy_tgx_texture_header(selected, texture_dir)
                _append_unique_include(extra_includes, _relative_include(header_path, output.parent))
                exported_headers.add(header_path.resolve())
                print(f"{label}: {material or '[default]'} -> {header_path} (copied TGX texture)")
                if copied is not None:
                    print(f"{label} source: {copied}")
            else:
                result = _export_tgx_texture_from_header(
                    selected,
                    source_symbol,
                    symbol,
                    texture_dir,
                    args.texture_format,
                    args.texture_layout,
                    resize,
                    resample,
                    not args.texture_straight_alpha,
                )
                _append_unique_include(extra_includes, _relative_include(result.header_path, output.parent))
                exported_headers.add(result.header_path.resolve())
                print(f"{label}: {material or '[default]'} -> {result.header_path} ({result.width}x{result.height}, {result.color_type}, {resample})")
        elif selected.suffix.lower() in IMAGE_EXTENSIONS:
            result = export_image(
                ImageExportOptions(
                    input_path=selected,
                    output_dir=texture_dir,
                    color_type=args.texture_format,
                    object_name=symbol,
                    output_base=symbol,
                    layout=args.texture_layout,
                    resize=resize,
                    resample=resample,
                    flip_y=True,
                    progmem=True,
                    premultiply_alpha=not args.texture_straight_alpha,
                )
            )
            _append_unique_include(extra_includes, _relative_include(result.header_path, output.parent))
            exported_headers.add(result.header_path.resolve())
            print(f"{label}: {material or '[default]'} -> {result.header_path} ({result.width}x{result.height}, {result.color_type}, {resample})")
        else:
            raise ValueError(f"unsupported {label} input for material {material or '[default]'}: {selected}")
        exported_symbols.add(symbol)
        if source_symbol:
            exported_source_symbols.add(source_symbol)

    for symbol, path in sorted((all_texture_headers or {}).items()):
        if symbol in exported_symbols or symbol in exported_source_symbols or symbol in skipped_source_symbols:
            continue
        header_path, copied = _copy_tgx_texture_header(path, texture_dir)
        if header_path.resolve() in exported_headers:
            continue
        _append_unique_include(extra_includes, _relative_include(header_path, output.parent))
        exported_headers.add(header_path.resolve())
        print(f"{label}: {symbol} -> {header_path} (copied TGX texture)")
        if copied is not None:
            print(f"{label} source: {copied}")


def _generated_texture_symbol(args: argparse.Namespace, output: Path, material: str, suffix: str, names: dict[str, str] | None = None) -> str:
    if names and material in names:
        return sanitize_identifier(names[material])
    return sanitize_identifier(f"{(args.name or output.stem)}_{material or 'default'}_{suffix}")


def _copy_tgx_texture_header(path: Path, texture_dir: Path) -> tuple[Path, Path | None]:
    header = path if path.suffix.lower() in (".h", ".hpp") else path.with_suffix(".h")
    if not header.exists():
        raise FileNotFoundError(str(header))
    copied_source: Path | None = None
    dest_header = texture_dir / header.name
    if header.resolve() != dest_header.resolve():
        shutil.copy2(header, dest_header)
    for source in (header.with_suffix(".cpp"), header.with_suffix(".cxx"), header.with_suffix(".cc")):
        if not source.exists():
            continue
        dest_source = texture_dir / source.name
        if source.resolve() != dest_source.resolve():
            shutil.copy2(source, dest_source)
        copied_source = dest_source
        break
    return dest_header, copied_source


def _tgx_texture_color_type(path: Path, symbol: str) -> str:
    header = path if path.suffix.lower() in (".h", ".hpp") else path.with_suffix(".h")
    raw = header.read_text(encoding="utf-8", errors="replace") if header.exists() else ""
    source = header.with_suffix(".cpp")
    if source.exists():
        raw += "\n" + source.read_text(encoding="utf-8", errors="replace")
    raw = re.sub(r"/\*.*?\*/", "", raw, flags=re.S)
    raw = re.sub(r"//.*?$", "", raw, flags=re.M)
    match = re.search(rf"(?:static\s+)?(?:extern\s+)?(?:const\s+)?tgx::Image<\s*tgx::(\w+)\s*>\s+{re.escape(symbol)}\s*(?:\(|;)", raw)
    if not match:
        match = re.search(r"(?:static\s+)?(?:extern\s+)?(?:const\s+)?tgx::Image<\s*tgx::(\w+)\s*>\s+\w+\s*(?:\(|;)", raw)
    if not match:
        raise ValueError(f"could not determine TGX texture color type from {header}")
    return normalize_color_type(match.group(1))


def _export_tgx_texture_from_header(
    path: Path,
    source_symbol: str,
    output_symbol: str,
    texture_dir: Path,
    color_type: str,
    layout: str,
    resize: tuple[int, int] | None,
    resample: str,
    premultiply_alpha: bool,
):
    header = path if path.suffix.lower() in (".h", ".hpp") else path.with_suffix(".h")
    texture = _parse_texture_image(header, source_symbol)
    if texture is None:
        fallback_symbol = _first_tgx_image_symbol(header)
        if fallback_symbol:
            texture = _parse_texture_image(header, fallback_symbol)
    if texture is None:
        raise ValueError(f"could not parse TGX texture {source_symbol!r} from {header}")
    with tempfile.TemporaryDirectory(prefix="tgx_texture_") as tmp:
        image_path = Path(tmp) / f"{output_symbol}.png"
        Image.fromarray(texture.rgb, "RGB").save(image_path)
        return export_image(
            ImageExportOptions(
                input_path=image_path,
                output_dir=texture_dir,
                color_type=color_type,
                object_name=output_symbol,
                output_base=output_symbol,
                layout=layout,
                resize=resize,
                resample=resample,
                flip_y=True,
                progmem=True,
                premultiply_alpha=premultiply_alpha,
            )
        )


def _first_tgx_image_symbol(path: Path) -> str:
    header = path if path.suffix.lower() in (".h", ".hpp") else path.with_suffix(".h")
    raw = header.read_text(encoding="utf-8", errors="replace") if header.exists() else ""
    source = header.with_suffix(".cpp")
    if source.exists():
        raw += "\n" + source.read_text(encoding="utf-8", errors="replace")
    raw = re.sub(r"/\*.*?\*/", "", raw, flags=re.S)
    raw = re.sub(r"//.*?$", "", raw, flags=re.M)
    match = re.search(r"(?:static\s+)?(?:extern\s+)?(?:const\s+)?tgx::Image<[^>]+>\s+(\w+)\s*(?:\(|;)", raw)
    return match.group(1) if match else ""


def _append_unique_include(includes: list[str], include: str) -> None:
    if include not in includes:
        includes.append(include)


def _decoded_mesh3dv2_to_objmesh(decoded: DecodedHeaderMesh) -> tuple[ObjMesh, dict[str, str], dict[str, str]]:
    vertices: list[np.ndarray] = []
    texcoords: list[np.ndarray] = []
    normals: list[np.ndarray] = []
    triangles: list[Triangle] = []
    materials: dict[str, Material] = {}
    texture_symbols: dict[str, str] = {}
    emissive_texture_symbols: dict[str, str] = {}

    for index, material in enumerate(decoded.materials):
        name = material.name or f"mat{index}"
        materials[name] = Material(
            name=name,
            diffuse=material.color,
            emissive=material.emissive_color,
            emissive_strength=material.emissive_strength,
            material_extra_flags=material.material_extra_flags,
            material_extra_present=material.material_extra_present,
            ambiant_strength=material.ambiant,
            diffuse_strength=material.diffuse,
            specular_strength=material.specular,
            specular_exponent=material.exponent,
        )
        if material.texture_symbol:
            texture_symbols[name] = material.texture_symbol
        if material.emissive_texture_symbol:
            emissive_texture_symbols[name] = material.emissive_texture_symbol

    for meshlet in decoded.meshlets:
        material_name = decoded.materials[meshlet.material_index].name if meshlet.material_index < len(decoded.materials) else ""
        v_base = len(vertices)
        vt_base = len(texcoords)
        vn_base = len(normals)
        vertices.extend(np.asarray(v, dtype=np.float64) for v in meshlet.vertices)
        texcoords.extend(np.asarray(vt, dtype=np.float64) for vt in meshlet.texcoords)
        normals.extend(np.asarray(vn, dtype=np.float64) for vn in meshlet.normals)
        for t_index, tri in enumerate(meshlet.triangles):
            tri_tex = meshlet.triangle_texcoords[t_index] if t_index < len(meshlet.triangle_texcoords) else (-1, -1, -1)
            tri_norm = meshlet.triangle_normals[t_index] if t_index < len(meshlet.triangle_normals) else (-1, -1, -1)
            corners = tuple(
                FaceVertex(
                    v_base + tri[i],
                    vt_base + tri_tex[i] if tri_tex[i] >= 0 else -1,
                    vn_base + tri_norm[i] if tri_norm[i] >= 0 else -1,
                )
                for i in range(3)
            )
            triangles.append(Triangle(corners, material_name, f"meshlet_{meshlet.index}"))  # type: ignore[arg-type]

    tex_array = np.asarray(texcoords, dtype=np.float64).reshape((-1, 2)) if texcoords else np.zeros((0, 2), dtype=np.float64)
    norm_array = np.asarray(normals, dtype=np.float64).reshape((-1, 3)) if normals else np.zeros((0, 3), dtype=np.float64)
    return (
        ObjMesh(
            vertices=np.asarray(vertices, dtype=np.float64).reshape((-1, 3)),
            texcoords=tex_array,
            normals=norm_array,
            triangles=triangles,
            name=decoded.name or decoded.symbol,
            materials=materials,
            source_path=decoded.path,
        ),
        texture_symbols,
        emissive_texture_symbols,
    )


def _parse_texture_symbols(items: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--texture-symbol expects MATERIAL=SYMBOL")
        material, symbol = item.split("=", 1)
        out[material] = symbol
    return out


def _parse_material_resizes(items: list[str]) -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--texture-resize expects MATERIAL=WIDTHxHEIGHT")
        material, size_text = item.split("=", 1)
        size = parse_resize(size_text)
        if size is None:
            raise ValueError("--texture-resize expects MATERIAL=WIDTHxHEIGHT")
        out[material] = size
    return out


def _parse_material_filters(items: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--texture-filter expects MATERIAL=FILTER")
        material, filter_name = item.split("=", 1)
        filter_name = filter_name.strip().lower()
        if filter_name not in SUPPORTED_RESAMPLING:
            raise ValueError("--texture-filter for {} must be one of: {}".format(material or "[default]", ", ".join(SUPPORTED_RESAMPLING)))
        out[material] = filter_name
    return out


def _parse_material_rgb_items(items: list[str], option: str) -> dict[str, tuple[float, float, float]]:
    out: dict[str, tuple[float, float, float]] = {}
    for item in items:
        if "=" not in item:
            raise ValueError(f"{option} expects MATERIAL=R,G,B")
        material, text = item.split("=", 1)
        parts = [part.strip() for part in re.split(r"[,;]", text) if part.strip()]
        if len(parts) != 3:
            raise ValueError(f"{option} for {material or '[default]'} expects R,G,B")
        values = tuple(float(part) for part in parts)
        if any(value < 0.0 or value > 1.0 for value in values):
            raise ValueError(f"{option} for {material or '[default]'} must use values between 0 and 1")
        out[material] = values  # type: ignore[assignment]
    return out


def _parse_material_float_items(items: list[str], option: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for item in items:
        if "=" not in item:
            raise ValueError(f"{option} expects MATERIAL=VALUE")
        material, text = item.split("=", 1)
        value = float(text)
        if value < 0.0:
            raise ValueError(f"{option} for {material or '[default]'} must be non-negative")
        out[material] = value
    return out


def _parse_material_int_items(items: list[str], option: str) -> dict[str, int]:
    out: dict[str, int] = {}
    for item in items:
        if "=" not in item:
            raise ValueError(f"{option} expects MATERIAL=VALUE")
        material, text = item.split("=", 1)
        value = int(round(float(text)))
        if value < 1 or value > 128:
            raise ValueError(f"{option} for {material or '[default]'} must be between 1 and 128")
        out[material] = value
    return out


def _ensure_material(mesh: ObjMesh, material: str) -> Material:
    if material not in mesh.materials:
        mesh.materials[material] = Material(material)
    return mesh.materials[material]


def _refresh_material_extra_present(material: Material) -> None:
    material.material_extra_present = (
        material.emissive != (0.0, 0.0, 0.0)
        or material.emissive_strength != 0.0
        or material.emissive_texture_path is not None
        or material.material_extra_flags != 0
    )


def _apply_material_overrides(args: argparse.Namespace, mesh: ObjMesh) -> None:
    colors = _parse_material_rgb_items(args.material_color, "--material-color")
    ambiants = _parse_material_float_items(args.material_ambiant, "--material-ambiant")
    diffuses = _parse_material_float_items(args.material_diffuse, "--material-diffuse")
    speculars = _parse_material_float_items(args.material_specular, "--material-specular")
    exponents = _parse_material_int_items(args.material_exponent, "--material-exponent")
    emissive_colors = _parse_material_rgb_items(args.emissive_color, "--emissive-color")
    emissive_strengths = _parse_material_float_items(args.emissive_strength, "--emissive-strength")

    for material in args.material_simple:
        desc = _ensure_material(mesh, material)
        clear_emissive_texture(desc)
        desc.emissive = (0.0, 0.0, 0.0)
        desc.emissive_strength = 0.0
        desc.material_extra_flags = 0
        desc.material_extra_present = False
    for material, color in colors.items():
        _ensure_material(mesh, material).diffuse = color
    for material, value in ambiants.items():
        _ensure_material(mesh, material).ambiant_strength = value
    for material, value in diffuses.items():
        _ensure_material(mesh, material).diffuse_strength = value
    for material, value in speculars.items():
        _ensure_material(mesh, material).specular_strength = value
    for material, value in exponents.items():
        _ensure_material(mesh, material).specular_exponent = value
    for material, color in emissive_colors.items():
        desc = _ensure_material(mesh, material)
        desc.emissive = color
        _refresh_material_extra_present(desc)
    for material, value in emissive_strengths.items():
        desc = _ensure_material(mesh, material)
        desc.emissive_strength = value
        setattr(desc, "_emissive_strength_overridden", True)
        _refresh_material_extra_present(desc)


def _parse_material_renames(items: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--material-rename expects OLD=NEW")
        old, new = item.split("=", 1)
        out[old] = new
    return out


def _rename_key(mapping: dict[str, object], old: str, new: str) -> None:
    if old in mapping:
        mapping[new] = mapping.pop(old)


def _apply_material_renames(args: argparse.Namespace, loaded: LoadedMesh) -> None:
    renames = _parse_material_renames(args.material_rename)
    if not renames:
        return
    mesh = loaded.mesh
    for triangle in mesh.triangles:
        triangle.material = renames.get(triangle.material, triangle.material)
    for old, new in renames.items():
        if old == new:
            continue
        _rename_key(mesh.materials, old, new)
        if new in mesh.materials:
            mesh.materials[new].name = new
        _rename_key(loaded.texture_symbols, old, new)
        _rename_key(loaded.emissive_texture_symbols, old, new)
        _rename_key(loaded.texture_sources, old, new)
        _rename_key(loaded.texture_source_symbols, old, new)
        _rename_key(loaded.emissive_texture_sources, old, new)
        _rename_key(loaded.emissive_texture_source_symbols, old, new)


def _color_type(value: str | None, default: str) -> str:
    if not value:
        return default
    return value if value.startswith("tgx::") else f"tgx::{value}"


def _relative_include(path: Path, base: Path) -> str:
    try:
        return str(path.resolve().relative_to(base.resolve())).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def _relative_texture_includes(decoded: DecodedHeaderMesh, output: Path) -> list[str]:
    return [_relative_include(path, output.parent) for path in sorted(decoded.texture_headers.values())]
