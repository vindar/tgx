from __future__ import annotations

import difflib
import re
from dataclasses import dataclass, field
from pathlib import Path

from tools._internal.modules.mesh_pipeline.mesh import Material, ObjMesh


IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".tif", ".tiff", ".gif"}
DIFFUSE_ROLES = ("map_kd", "map_basecolor", "map_albedo", "map_diffuse")
EMISSIVE_ROLES = ("map_ke", "map_emissive", "map_emission", "map_emit")
DIFFUSE_HINTS = ("diffuse", "diff", "albedo", "basecolor", "base_color", "color", "col", "_d")
NON_DIFFUSE_HINTS = ("normal", "norm", "bump", "spec", "rough", "metal", "emit", "emiss", "reflection", "refl")
EMISSIVE_HINTS = ("emissive", "emission", "emiss", "emit", "glow", "_e")
NON_EMISSIVE_HINTS = ("normal", "norm", "bump", "spec", "rough", "metal", "diffuse", "diff", "albedo", "basecolor", "base_color")


@dataclass
class TextureCandidate:
    path: Path
    score: float
    reason: str


@dataclass
class TextureChoice:
    material: str
    role: str
    requested: Path | None
    selected: Path | None
    refs: dict[str, Path] = field(default_factory=dict)
    candidates: list[TextureCandidate] = field(default_factory=list)

    @property
    def needs_attention(self) -> bool:
        return self.selected is None or self.requested is None or self.requested.resolve() != self.selected.resolve()


def resolve_mesh_textures(
    mesh: ObjMesh,
    *,
    search_roots: list[Path] | None = None,
    overrides: dict[str, Path] | None = None,
    role: str = "map_kd",
    guess_when_missing: bool = True,
) -> dict[str, TextureChoice]:
    """Resolve one TGX texture per material for a requested material texture role."""

    roots = _search_roots(mesh, search_roots or [])
    images = _collect_images(roots)
    overrides = overrides or {}
    choices: dict[str, TextureChoice] = {}
    role = role.lower()

    used_materials = sorted({tri.material for tri in mesh.triangles})
    for material in used_materials:
        desc = mesh.materials.get(material)
        if desc is None:
            desc = Material(material)
            mesh.materials[material] = desc
        requested_role, requested = _requested_texture(desc.texture_refs, desc.texture_path, role)
        selected: Path | None = None
        candidates: list[TextureCandidate] = []

        if material in overrides:
            selected = overrides[material].expanduser().resolve()
            candidates = [TextureCandidate(selected, 10_000.0, "user override")]
        elif requested is not None:
            selected, candidates = _resolve_requested_texture(requested, material, requested_role, images)
        elif guess_when_missing:
            candidates = _guess_material_texture(material, images, role)
            selected = candidates[0].path if candidates and candidates[0].score >= 0.70 else None

        choices[material] = TextureChoice(
            material=material,
            role=requested_role,
            requested=requested,
            selected=selected,
            refs=dict(desc.texture_refs),
            candidates=candidates,
        )
        if role in EMISSIVE_ROLES:
            if selected is not None:
                desc.emissive_texture_path = selected
                desc.emissive_strength = 1.0
                desc.material_extra_present = True
            else:
                clear_emissive_texture(desc)
        else:
            desc.texture_path = selected

    return choices


def texture_choices_text(choices: dict[str, TextureChoice]) -> str:
    lines: list[str] = []
    for material, choice in choices.items():
        label = material or "[default]"
        requested = str(choice.requested) if choice.requested else "none"
        selected = str(choice.selected) if choice.selected else "none"
        marker = "!" if choice.needs_attention else " "
        lines.append(f"{marker} {label}: {choice.role} requested={requested}")
        if choice.refs:
            refs = ", ".join(f"{role}={path.name}" for role, path in sorted(choice.refs.items()))
            lines.append(f"    refs={refs}")
        lines.append(f"    selected={selected}")
        for candidate in choice.candidates[:5]:
            lines.append(f"    candidate {candidate.score:.3f}: {candidate.path} ({candidate.reason})")
    return "\n".join(lines)


def parse_texture_overrides(items: list[str]) -> dict[str, Path]:
    out: dict[str, Path] = {}
    for item in items:
        if "=" not in item:
            raise ValueError("--texture expects MATERIAL=PATH")
        material, path = item.split("=", 1)
        out[material] = Path(path)
    return out


def confirm_texture_choices(choices: dict[str, TextureChoice]) -> None:
    for choice in choices.values():
        if not choice.needs_attention:
            continue
        label = choice.material or "[default]"
        print(f"\nTexture for material {label}:")
        print(f"  requested: {choice.requested or 'none'}")
        for i, candidate in enumerate(choice.candidates[:8], start=1):
            print(f"  {i}. {candidate.path} ({candidate.reason}, score {candidate.score:.3f})")
        print("  0. no texture")
        print("  p. enter path manually")
        default = "1" if choice.candidates else "0"
        answer = input(f"Select texture [{default}]: ").strip() or default
        if answer == "0":
            choice.selected = None
        elif answer.lower() == "p":
            manual = input("Texture path: ").strip().strip('"')
            choice.selected = Path(manual).expanduser().resolve() if manual else None
        else:
            try:
                index = int(answer) - 1
                choice.selected = choice.candidates[index].path
            except (ValueError, IndexError):
                raise ValueError(f"invalid texture selection for {label}: {answer}")


def apply_texture_choices(mesh: ObjMesh, choices: dict[str, TextureChoice]) -> None:
    for material, choice in choices.items():
        if material in mesh.materials:
            if choice.role in EMISSIVE_ROLES:
                if choice.selected is not None:
                    mesh.materials[material].emissive_strength = 1.0
                    mesh.materials[material].emissive_texture_path = choice.selected
                    mesh.materials[material].material_extra_present = True
                else:
                    clear_emissive_texture(mesh.materials[material])
            else:
                mesh.materials[material].texture_path = choice.selected


def clear_emissive_texture(material: Material) -> None:
    material.emissive_texture_path = None
    if material.emissive == (0.0, 0.0, 0.0) and material.material_extra_flags == 0:
        material.emissive_strength = 0.0
        material.material_extra_present = False


def _search_roots(mesh: ObjMesh, extra: list[Path]) -> list[Path]:
    roots: list[Path] = []
    if mesh.source_path is not None:
        roots.append(mesh.source_path.resolve().parent)
    for desc in mesh.materials.values():
        for path in desc.texture_refs.values():
            roots.append(path.resolve().parent)
    roots.extend(path.expanduser().resolve() for path in extra)

    unique: list[Path] = []
    seen: set[Path] = set()
    for root in roots:
        if root in seen or not root.exists():
            continue
        seen.add(root)
        unique.append(root)
    return unique


def _collect_images(roots: list[Path]) -> list[Path]:
    images: list[Path] = []
    seen: set[Path] = set()
    for root in roots:
        if root.is_file() and root.suffix.lower() in IMAGE_EXTENSIONS:
            candidates = [root]
        else:
            candidates = [p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in IMAGE_EXTENSIONS]
        for path in candidates:
            resolved = path.resolve()
            if resolved not in seen:
                seen.add(resolved)
                images.append(resolved)
    return images


def _requested_texture(refs: dict[str, Path], texture_path: Path | None, preferred_role: str) -> tuple[str, Path | None]:
    if preferred_role in refs:
        return preferred_role, refs[preferred_role]
    roles = EMISSIVE_ROLES if preferred_role in EMISSIVE_ROLES else DIFFUSE_ROLES
    for role in roles:
        if role in refs:
            return role, refs[role]
    if preferred_role in DIFFUSE_ROLES and texture_path is not None:
        return "map_kd", texture_path
    if preferred_role in EMISSIVE_ROLES:
        return preferred_role, None
    return preferred_role, None


def _resolve_requested_texture(requested: Path, material: str, role: str, images: list[Path]) -> tuple[Path | None, list[TextureCandidate]]:
    requested = requested.expanduser().resolve()
    if requested.exists():
        return requested, [TextureCandidate(requested, 1.0, "exact path")]

    same_dir = [p for p in images if p.parent == requested.parent]
    candidates = _score_candidates(requested, material, role, same_dir or images)
    selected = candidates[0].path if candidates and candidates[0].score >= 0.60 else None
    return selected, candidates


def _guess_material_texture(material: str, images: list[Path], role: str = "map_kd") -> list[TextureCandidate]:
    pseudo = Path(material or "texture")
    return _score_candidates(pseudo, material, role, images)


def _score_candidates(requested: Path, material: str, role: str, images: list[Path]) -> list[TextureCandidate]:
    req_stem = _norm(requested.stem)
    req_name = _norm(requested.name)
    mat = _norm(material)
    role = role.lower()
    out: list[TextureCandidate] = []

    for image in images:
        stem = _norm(image.stem)
        name = _norm(image.name)
        stem_ratio = difflib.SequenceMatcher(None, req_stem, stem).ratio() if req_stem else 0.0
        name_ratio = difflib.SequenceMatcher(None, req_name, name).ratio() if req_name else 0.0
        material_ratio = difflib.SequenceMatcher(None, mat, stem).ratio() if mat else 0.0
        score = max(stem_ratio, name_ratio * 0.96, material_ratio * 0.80)
        reasons = [f"name similarity {score:.2f}"]

        if requested.suffix and image.suffix.lower() == requested.suffix.lower():
            score += 0.05
            reasons.append("same extension")
        if image.parent == requested.parent:
            score += 0.12
            reasons.append("same directory")
        if role in DIFFUSE_ROLES:
            if any(h in stem for h in DIFFUSE_HINTS):
                score += 0.12
                reasons.append("diffuse-like name")
            if any(h in stem for h in NON_DIFFUSE_HINTS):
                score -= 0.25
                reasons.append("non-diffuse-like name")
        elif role in EMISSIVE_ROLES:
            if any(h in stem for h in EMISSIVE_HINTS):
                score += 0.18
                reasons.append("emissive-like name")
            if any(h in stem for h in NON_EMISSIVE_HINTS):
                score -= 0.20
                reasons.append("non-emissive-like name")
        if req_stem and (stem.startswith(req_stem) or req_stem.startswith(stem)):
            score += 0.15
            reasons.append("prefix match")

        out.append(TextureCandidate(image, max(0.0, score), ", ".join(reasons)))

    out.sort(key=lambda c: (-c.score, len(str(c.path))))
    return out


def _norm(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", text.lower())
