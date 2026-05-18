from __future__ import annotations

import argparse
import sys
from pathlib import Path

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.mesh3d2_converter.legacy_exporter import LegacyExportResult, export_legacy_mesh3d_header
from tools.mesh3d2_converter.mesh3d_to_mesh3d2 import (
    DEFAULT_NORMAL_QUANT_BITS,
    DEFAULT_TEXCOORD_QUANT_BITS,
    DEFAULT_VERTEX_QUANT_BITS,
    _relative_includes,
    legacy_to_objmesh,
    parse_legacy_header,
    preprocess_legacy_mesh,
)
from tools.mesh3d2_converter.obj_loader import load_obj
from tools.mesh3d2_converter.preprocess import preprocess_mesh
from tools.mesh3d2_converter.progress import step
from tools.mesh3d2_converter.stripifier import DEFAULT_LKH_EXE, solver_stats_text
from tools.mesh3d2_converter.texture_exporter import export_texture_rgb565_header, identifier as texture_identifier


def _detect_source(path: Path) -> str:
    if path.suffix.lower() == ".obj":
        return "obj"
    text = path.read_text(encoding="utf-8", errors="replace")
    if "tgx::Mesh3D<" in text:
        return "legacy"
    raise ValueError(f"cannot detect input type for {path}; use --source obj or --source legacy")


def _parse_texture_symbols(items: list[str]) -> dict[str, str]:
    symbols: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--texture-symbol expects MATERIAL=SYMBOL")
        material, symbol = item.split("=", 1)
        symbols[material] = symbol
    return symbols


def _export_obj_textures(args: argparse.Namespace, mesh, output: Path, texture_symbols: dict[str, str], extra_includes: list[str]) -> None:
    if not args.export_textures:
        return
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
        try:
            include = tex.header.resolve().relative_to(output.parent.resolve())
        except ValueError:
            include = tex.header
        extra_includes.append(str(include).replace("\\", "/"))
        print(f"texture: {material or '[default]'} -> {tex.header} ({tex.width}x{tex.height})")


def _load_input(args: argparse.Namespace):
    input_path = Path(args.input)
    source = args.source if args.source != "auto" else _detect_source(input_path)
    output = Path(args.output)
    color_type = args.color_type
    texture_symbols = _parse_texture_symbols(args.texture_symbol)
    extra_includes = list(args.include)

    if source == "legacy":
        with step("parse legacy Mesh3D", str(input_path)):
            parsed = parse_legacy_header(input_path)
            mesh, legacy_textures, chain = legacy_to_objmesh(parsed, root=args.root, name=args.name)
        texture_symbols = {**legacy_textures, **texture_symbols}
        extra_includes = _relative_includes(parsed.includes, output) + extra_includes
        color_type = color_type or chain[0].color_type
        if args.preprocess_legacy:
            with step("preprocess mesh", f"{len(mesh.triangles)} triangles"):
                mesh = preprocess_legacy_mesh(
                    mesh,
                    vertex_quant_bits=args.vertex_quant_bits,
                    texcoord_quant_bits=args.texcoord_quant_bits,
                    texcoord_wrap=args.texcoord_wrap,
                    normal_quant_bits=args.normal_quant_bits,
                )
        return mesh, source, color_type or "tgx::RGB565", texture_symbols, extra_includes

    with step("load OBJ", str(input_path)):
        mesh = load_obj(input_path)
    with step("preprocess mesh", f"{len(mesh.triangles)} triangles"):
        mesh, prep = preprocess_mesh(
            mesh,
            normalize=args.normalize,
            normalize_size=args.normalize_size,
            dedupe_epsilon=args.dedupe_epsilon,
            force_normals=args.force_normals,
            vertex_quant_bits=args.vertex_quant_bits,
            texcoord_quant_bits=args.texcoord_quant_bits,
            texcoord_wrap=args.texcoord_wrap,
            normal_quant_bits=args.normal_quant_bits,
        )
    _ = prep
    color_type = color_type or "tgx::RGB565"
    _export_obj_textures(args, mesh, output, texture_symbols, extra_includes)
    return mesh, source, color_type, texture_symbols, extra_includes


def convert(args: argparse.Namespace) -> LegacyExportResult:
    output = Path(args.output)
    with step("load input", args.input):
        mesh, source, color_type, texture_symbols, extra_includes = _load_input(args)
    name = args.name or output.stem
    with step("build legacy Mesh3D strips", f"{len(mesh.triangles)} triangles"):
        result = export_legacy_mesh3d_header(
            mesh,
            name=name,
            color_type=color_type,
            use_lkh=True,
            lkh_exe=args.lkh,
            lkh_time_limit_s=args.lkh_time_limit,
            texture_symbols=texture_symbols,
            extra_includes=extra_includes,
            split_by_material=not args.no_split_by_material,
        )
    output.parent.mkdir(parents=True, exist_ok=True)
    with step("write header", str(output)):
        with output.open("w", encoding="utf-8", newline="\n") as f:
            f.write(result.header)
    print(f"stripifier choices: {solver_stats_text()}")
    print(f"wrote {output}")
    print("Mesh3D output:")
    print(f"  source        : {source}")
    print(f"  symbol        : {name}")
    print(f"  color type    : {color_type}")
    print(f"  triangles     : {result.triangles}")
    print(f"  submeshes     : {result.meshes}")
    print(f"  components    : {result.components}")
    print(f"  chains        : {result.chains} ({100.0 * result.chains / max(1, result.triangles):.2f}% of triangles)")
    print(f"  face words    : {result.face_words}")
    print(f"  textures      : {len(texture_symbols)}")
    print("  stripifiers   : enabled")
    if args.lkh_time_limit is not None:
        print(f"  time override : {args.lkh_time_limit:g}s per component")
    else:
        print("  time budget   : automatic per component")
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Convert OBJ or legacy TGX Mesh3D headers into optimized legacy Mesh3D headers")
    parser.add_argument("input", help="input .obj or legacy Mesh3D .h")
    parser.add_argument("-o", "--output", required=True, help="output Mesh3D header")
    parser.add_argument("--source", choices=("auto", "obj", "legacy"), default="auto")
    parser.add_argument("--root", help="root Mesh3D symbol for legacy chained inputs")
    parser.add_argument("--name", help="output C++ Mesh3D symbol")
    parser.add_argument("--color-type", help="override tgx color type; defaults to source color type or tgx::RGB565")
    parser.add_argument("--lkh", default=str(DEFAULT_LKH_EXE))
    parser.add_argument("--lkh-time-limit", type=float, default=None, help="override the automatic stripifier budget, in seconds per compatible connected component")
    parser.add_argument("--no-split-by-material", action="store_true", help="write one Mesh3D instead of one linked Mesh3D per material")

    parser.add_argument("--no-preprocess-legacy", dest="preprocess_legacy", action="store_false", help="keep legacy vertex/normal/UV indices exactly as-is")
    parser.add_argument("--vertex-quant-bits", type=int, default=DEFAULT_VERTEX_QUANT_BITS, help="cleanup: merge vertices snapped to this many bounding-box bits; negative disables")
    parser.add_argument("--texcoord-quant-bits", type=int, default=DEFAULT_TEXCOORD_QUANT_BITS, help="cleanup: snap UVs to 1/(2^bits); negative disables")
    parser.add_argument("--texcoord-wrap", action="store_true", help="identify UVs modulo 1 during UV cleanup; only use when known to preserve mapping")
    parser.add_argument("--normal-quant-bits", type=int, default=DEFAULT_NORMAL_QUANT_BITS, help="cleanup: merge normals after signed fixed-point snapping; negative disables")

    parser.add_argument("--normalize", action="store_true", help="OBJ input: normalize to a centered box")
    parser.add_argument("--normalize-size", type=float, default=2.0, help="OBJ input: normalized box size")
    parser.add_argument("--dedupe-epsilon", type=float, default=1e-8, help="OBJ input: merge nearly identical vertices/UVs/normals")
    parser.add_argument("--force-normals", action="store_true", help="OBJ input: recompute flat normals")
    parser.add_argument("--texture-symbol", action="append", default=[], metavar="MATERIAL=SYMBOL", help="link a material to an existing tgx::Image symbol")
    parser.add_argument("--include", action="append", default=[], help="extra quoted include to add to the generated header")
    parser.add_argument("--export-textures", action="store_true", help="OBJ input: convert map_Kd textures to RGB565 TGX headers")
    parser.add_argument("--texture-output-dir", help="OBJ input: directory for generated texture headers; defaults to output header directory")
    parser.add_argument("--texture-size", nargs=2, type=int, metavar=("W", "H"), help="OBJ input: resize exported textures")
    parser.add_argument("--texture-require-pow2", action="store_true", help="OBJ input: require power-of-two textures")
    parser.set_defaults(preprocess_legacy=True)
    args = parser.parse_args(argv)
    convert(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
