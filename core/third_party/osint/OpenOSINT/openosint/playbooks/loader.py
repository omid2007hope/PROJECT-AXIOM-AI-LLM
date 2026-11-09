# openosint/playbooks/loader.py
"""Load and validate playbook recipe YAML files into frozen dataclasses."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PlaybookStep:
    id: str
    tool: str
    section: str


@dataclass(frozen=True)
class Recipe:
    name: str
    label: str
    target_type: str
    steps: tuple[PlaybookStep, ...]


_RECIPES_DIR = Path(__file__).parent / "recipes"


def load_recipe(name_or_path: str) -> Recipe:
    """
    Load a recipe by name or explicit file path.

    A bare name (e.g. ``"domain"``) resolves to
    ``openosint/playbooks/recipes/<name>.yaml``.  An explicit path is used
    as-is.  Raises ``ValueError`` on any validation failure.
    """
    try:
        import yaml  # type: ignore[import]
    except ImportError as exc:
        raise ImportError(
            "PyYAML is required for playbooks. Install with: pip install pyyaml"
        ) from exc

    path = Path(name_or_path)
    if not path.suffix:
        path = _RECIPES_DIR / f"{name_or_path}.yaml"

    if not path.exists():
        raise ValueError(
            f"Recipe not found: '{name_or_path}'. "
            f"Built-in recipes are in {_RECIPES_DIR}."
        )

    with path.open(encoding="utf-8") as fh:
        raw = yaml.safe_load(fh)

    return _validate(raw, path)


def _validate(raw: object, path: Path) -> Recipe:
    if not isinstance(raw, dict):
        raise ValueError(f"{path}: expected a YAML mapping at the top level.")

    for field in ("name", "label", "target_type"):
        value = raw.get(field)
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"{path}: '{field}' must be a non-empty string.")

    steps_raw = raw.get("steps")
    if not isinstance(steps_raw, list) or not steps_raw:
        raise ValueError(f"{path}: 'steps' must be a non-empty list.")

    from openosint.playbooks.runner import TOOL_MAP  # avoid circular at module load

    seen_ids: set[str] = set()
    steps: list[PlaybookStep] = []

    for i, step in enumerate(steps_raw):
        if not isinstance(step, dict):
            raise ValueError(f"{path}: step {i} must be a mapping.")
        for field in ("id", "tool", "section"):
            value = step.get(field)
            if not isinstance(value, str) or not value.strip():
                raise ValueError(
                    f"{path}: step {i} — '{field}' must be a non-empty string."
                )
        sid = step["id"]
        if sid in seen_ids:
            raise ValueError(f"{path}: duplicate step id '{sid}'.")
        seen_ids.add(sid)

        tool = step["tool"]
        if tool not in TOOL_MAP:
            raise ValueError(
                f"{path}: step '{sid}' references unknown tool '{tool}'. "
                f"Known tools: {sorted(TOOL_MAP)}."
            )

        steps.append(PlaybookStep(id=sid, tool=tool, section=step["section"]))

    return Recipe(
        name=raw["name"],
        label=raw["label"],
        target_type=raw["target_type"],
        steps=tuple(steps),
    )
