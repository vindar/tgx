from __future__ import annotations

import difflib
import re
from dataclasses import dataclass
from pathlib import Path


DIFFUSE_ROLES = (
    "map_kd",
    "map_basecolor",
    "map_base_color",
    "map_albedo",
    "map_diffuse",
    "map_color",
    "map_colour",
)
EMISSIVE_ROLES = (
    "map_ke",
    "map_emissive",
    "map_emission",
    "map_emit",
    "map_emissivecolor",
    "map_emissive_color",
)

DIFFUSE_HINTS = ("diffuse", "diff", "albedo", "basecolor", "basecolour", "color", "colour", "col", "kd")
NON_DIFFUSE_HINTS = (
    "normal",
    "norm",
    "nrm",
    "bump",
    "height",
    "displacement",
    "disp",
    "specular",
    "spec",
    "gloss",
    "roughness",
    "rough",
    "rgh",
    "metallic",
    "metalness",
    "metal",
    "emissive",
    "emission",
    "emiss",
    "emit",
    "glow",
    "ao",
    "occlusion",
    "ambientocclusion",
    "orm",
    "arm",
    "mask",
    "opacity",
    "alpha",
    "cavity",
    "reflection",
    "refl",
)
EMISSIVE_HINTS = ("emissive", "emission", "emiss", "emit", "glow", "ke")
NON_EMISSIVE_HINTS = (
    "normal",
    "norm",
    "nrm",
    "bump",
    "height",
    "displacement",
    "disp",
    "specular",
    "spec",
    "gloss",
    "roughness",
    "rough",
    "rgh",
    "metallic",
    "metalness",
    "metal",
    "diffuse",
    "diff",
    "albedo",
    "basecolor",
    "basecolour",
    "color",
    "colour",
    "col",
    "kd",
    "ao",
    "orm",
    "arm",
)

_ONE_LETTER_DIFFUSE_SUFFIXES = {"d", "c"}
_ONE_LETTER_EMISSIVE_SUFFIXES = {"e"}
_ROLE_WORDS = (
    set(DIFFUSE_HINTS)
    | set(NON_DIFFUSE_HINTS)
    | set(EMISSIVE_HINTS)
    | set(NON_EMISSIVE_HINTS)
    | _ONE_LETTER_DIFFUSE_SUFFIXES
    | _ONE_LETTER_EMISSIVE_SUFFIXES
)


@dataclass(frozen=True)
class ScoredTextureCandidate:
    path: Path
    score: float
    reason: str


def score_texture_candidates(
    *,
    requested: Path | None,
    material: str,
    role: str,
    images: list[Path],
    refs: dict[str, Path] | None = None,
    mesh_name: str = "",
) -> list[ScoredTextureCandidate]:
    """Rank image files by how likely they are to be the texture for *role*.

    The score intentionally mixes several weak signals. OBJ/MTL files in the wild
    are often inconsistent, so no single clue is reliable enough on its own.
    """

    role = role.lower()
    target_roles = set(EMISSIVE_ROLES if role in EMISSIVE_ROLES else DIFFUSE_ROLES)
    requested = requested.expanduser().resolve() if requested is not None else None
    requested_parent = requested.parent if requested is not None else None
    requested_stem = _compact(requested.stem) if requested is not None else ""
    requested_name = _compact(requested.name) if requested is not None else ""
    material_key = _compact(material)
    mesh_key = _compact(mesh_name)
    ref_roles = _ref_roles_by_path(refs or {})

    out: list[ScoredTextureCandidate] = []
    for image in images:
        path = image.expanduser().resolve()
        stem_tokens = _tokens(path.stem)
        material_tokens = set(_tokens(material))
        mesh_tokens = set(_tokens(mesh_name))
        role_tokens = [token for token in stem_tokens if token not in material_tokens and token not in mesh_tokens]
        role_compact = _compact(" ".join(role_tokens))
        stem_compact = _compact(path.stem)
        name_compact = _compact(path.name)
        clean_compact = _compact(" ".join(token for token in stem_tokens if token not in _ROLE_WORDS))

        score = 0.0
        reasons: list[str] = []

        ref_role = ref_roles.get(path)
        if ref_role in target_roles:
            score += 1.20
            reasons.append(f"MTL {ref_role}")
        elif ref_role is not None:
            score -= 1.00
            reasons.append(f"MTL non-target {ref_role}")

        if requested is not None:
            name_score = max(
                _ratio(requested_stem, stem_compact),
                0.96 * _ratio(requested_name, name_compact),
            )
            if name_score > 0.0:
                score += 0.55 * name_score
                reasons.append(f"requested-name {name_score:.2f}")
            if requested.suffix and path.suffix.lower() == requested.suffix.lower():
                score += 0.05
                reasons.append("same extension")
            if requested_parent is not None and path.parent == requested_parent:
                score += 0.20
                reasons.append("same directory")
            if requested_stem and (stem_compact.startswith(requested_stem) or requested_stem.startswith(stem_compact)):
                score += 0.18
                reasons.append("requested prefix")

        material_score = max(_ratio(material_key, stem_compact), _ratio(material_key, clean_compact))
        if material_score > 0.0:
            score += 0.32 * material_score
            reasons.append(f"material-name {material_score:.2f}")
        if material_key and material_key == clean_compact:
            score += 0.42
            reasons.append("material exact after suffix removal")
        elif material_key and clean_compact and (clean_compact.startswith(material_key) or material_key.startswith(clean_compact)):
            score += 0.18
            reasons.append("material prefix")

        mesh_score = max(_ratio(mesh_key, stem_compact), _ratio(mesh_key, clean_compact))
        if mesh_score > 0.0:
            score += 0.14 * mesh_score
            reasons.append(f"mesh-name {mesh_score:.2f}")
        if mesh_key and mesh_key == clean_compact:
            score += 0.18
            reasons.append("mesh exact after suffix removal")

        if role in EMISSIVE_ROLES:
            positive = _keyword_score(role_tokens, role_compact, EMISSIVE_HINTS, _ONE_LETTER_EMISSIVE_SUFFIXES, 0.55, reasons, "emissive-like")
            negative = _keyword_score(role_tokens, role_compact, NON_EMISSIVE_HINTS, set(), 0.80, reasons, "non-emissive")
            score += positive - negative
        else:
            positive = _keyword_score(role_tokens, role_compact, DIFFUSE_HINTS, _ONE_LETTER_DIFFUSE_SUFFIXES, 0.48, reasons, "diffuse-like")
            negative = _keyword_score(role_tokens, role_compact, NON_DIFFUSE_HINTS, set(), 0.85, reasons, "non-diffuse")
            score += positive - negative
        if negative > 0.0 and positive <= 0.0 and ref_role not in target_roles:
            score = min(score, 0.55)

        if not reasons:
            reasons.append("no strong signal")
        out.append(ScoredTextureCandidate(path, max(0.0, score), ", ".join(reasons)))

    out.sort(key=lambda candidate: (-candidate.score, len(str(candidate.path)), str(candidate.path).lower()))
    return out


def _ref_roles_by_path(refs: dict[str, Path]) -> dict[Path, str]:
    out: dict[Path, str] = {}
    for role, path in refs.items():
        try:
            out[path.expanduser().resolve()] = role.lower()
        except Exception:
            continue
    return out


def _keyword_score(
    tokens: list[str],
    compact: str,
    words: tuple[str, ...],
    one_letter_suffixes: set[str],
    weight: float,
    reasons: list[str],
    label: str,
) -> float:
    hits = 0
    for word in words:
        if word in tokens or (len(word) > 3 and word in compact):
            hits += 1
    if tokens and tokens[-1] in one_letter_suffixes:
        hits += 1
    if not hits:
        return 0.0
    reasons.append(label)
    return weight * min(1.0, 0.65 + 0.25 * (hits - 1))


def _tokens(text: str) -> list[str]:
    text = re.sub(r"([a-z])([A-Z])", r"\1 \2", text)
    text = re.sub(r"([A-Za-z])([0-9])", r"\1 \2", text)
    text = re.sub(r"([0-9])([A-Za-z])", r"\1 \2", text)
    return [part.lower() for part in re.split(r"[^A-Za-z0-9]+", text) if part]


def _compact(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", text.lower())


def _ratio(a: str, b: str) -> float:
    if not a or not b:
        return 0.0
    if a == b:
        return 1.0
    return difflib.SequenceMatcher(None, a, b).ratio()
