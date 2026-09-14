#!/usr/bin/env python3
"""Bounded player-text coverage inventory; not a C++ parser or visual approval.

Checks paired catalogs, concrete display literals in selected application sinks,
and scene-transition failures. Dynamic fragments are reported separately: the
product tests exercise their actual composed values.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', re.MULTILINE)
PLACEHOLDER = re.compile(r"\{([0-3])\}")


def literals(source):
    for token in TOKEN.finditer(source):
        if not token.group().startswith('"'):
            continue
        try:
            yield json.loads(token.group())
        except json.JSONDecodeError:
            # C++ char escapes in non-display/diagnostic constants.
            continue


def call_arguments(source, name):
    """Read balanced calls, ignoring parentheses inside strings/comments."""
    mask = list(source)
    for token in TOKEN.finditer(source):
        mask[token.start():token.end()] = " " * len(token.group())
    masked = "".join(mask)
    for call in re.finditer(r"\b" + re.escape(name) + r"\s*\(", masked):
        start = call.end()
        depth, end = 1, start
        while end < len(masked) and depth:
            depth += (masked[end] == "(") - (masked[end] == ")")
            end += 1
        if depth:
            raise ValueError(f"Unbalanced call: {name}")
        yield source[start:end - 1]


def bank(path):
    result = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        key, sep, value = line.partition("\t")
        if not sep or not key or not value or key in result:
            raise ValueError(f"{path.name}:{number}: invalid or duplicate entry")
        result[key] = value
    return result


def inspect():
    translations, counts = {}, {}
    for en_name, zh_name in (("recovery_text.txt", "recovery_text.zh-CN.txt"),
                            ("interface.en.txt", "interface.zh-CN.txt")):
        en, zh = bank(ROOT / "data/text" / en_name), bank(ROOT / "data/text" / zh_name)
        if en.keys() != zh.keys():
            raise ValueError(f"ID mismatch: {en_name}")
        counts[en_name] = len(en)
        for key, value in en.items():
            if value in translations and translations[value] != zh[key]:
                raise ValueError(f"Conflicting source: {key}")
            if sorted(PLACEHOLDER.findall(value)) != sorted(PLACEHOLDER.findall(zh[key])):
                raise ValueError(f"Placeholder mismatch: {key}")
            translations[value] = zh[key]
    patterns = []
    for source in sorted(translations, key=lambda text: (-len(PLACEHOLDER.sub("", text)), text)):
        if "{0}" not in source:
            continue
        parts = PLACEHOLDER.split(source)
        pattern = "".join("(.*?)" if i % 2 else re.escape(part)
                          for i, part in enumerate(parts))
        patterns.append(re.compile(pattern, re.DOTALL))

    # Physical key names, numbers and product/floor identifiers are deliberately
    # stable across languages. This is not a blanket uppercase-text exemption.
    stable = re.compile(r"(?:[0-9./ +%-]+|[A-Z]|F[0-9]{1,2}|B[1-4]|RF|FPS|"
                        r"MOUSE[1-5]|SPACE|SHIFT|CTRL|TAB|ESC|UP|DOWN|LEFT|RIGHT|"
                        r"PAD (?:UP|DOWN|LEFT|RIGHT|A|B|X|Y|LB|RB|LT|RT|START|BACK|LS|RS)|"
                        r"WRITEOVER-07|English|简体中文|WASD|LMB)")

    def covered(value):
        if not re.search(r"[A-Za-z\u4e00-\u9fff]", value):
            return True
        if value in translations or stable.fullmatch(value):
            return True
        if value.startswith(("> ", "  ")):
            return covered(value[2:])
        for pattern in patterns:
            matched = pattern.fullmatch(value)
            if matched and all(part != value and covered(part) for part in matched.groups()):
                return True
        if " / " in value:
            return all(covered(part) for part in value.split(" / ", 1))
        if "/" in value and all(stable.fullmatch(part) for part in value.split("/")):
            return True
        return False

    candidates = {}
    fragments = {}
    # Deliberately bilingual pre-preference selector, language code, and a
    # perception de-duplication key. None is an untranslated narrative line.
    nontranslated = {"选择语言 / CHOOSE LANGUAGE", "zh-CN", "body-discovery"}
    def collect(source, origin):
        for literal in literals(source):
            for value in literal.splitlines():
                if not value or value in nontranslated or re.fullmatch(r"[a-z0-9_./]+", value):
                    continue  # resource IDs, paths and state-machine keys
                if not re.search(r"[A-Za-z]{2}", value):
                    continue
                # A substring of a dynamically assembled message is not a
                # standalone acceptance case. Keep it visible in the report.
                if value != value.strip() or value.endswith((": ", ":")) or value.startswith(". "):
                    fragments.setdefault(value, []).append(origin)
                    continue
                candidates.setdefault(value, []).append(origin)

    app = (ROOT / "src/app/composition_root.cpp").read_text(encoding="utf-8")
    for name in ("SetSubtitleOnce", "fail_load", "fail_commit",
                 "use_chapter_terminal", "use_act2_terminal"):
        for argument in call_arguments(app, name):
            collect(argument, f"composition_root.cpp:{name}")
    # Objective lambda and ending/local-name presentation have clear boundaries.
    for begin, end in (("render->SetObjectiveSource(", "render->SetChapterClosureSource("),
                       ("product.ending_rows = {", "if (!input.has_focus)")):
        start = app.find(begin)
        finish = app.find(end, start)
        if start < 0 or finish < 0:
            raise ValueError(f"Display inventory boundary moved: {begin}")
        collect(app[start:finish], "composition_root.cpp:display block")
    # All prompt literals, including ternary branches.
    for match in re.finditer(r'"\[F\] (?:\\.|[^"\\])*"', app):
        collect(match.group(), "composition_root.cpp:interaction prompt")
    for filename in ("player_product.h", "player_perception.h", "perception_feed.h", "product_records.h"):
        collect((ROOT / "src/app" / filename).read_text(encoding="utf-8"), filename)
    tower = (ROOT / "src/app/tower_campaign_runtime.cpp").read_text(encoding="utf-8")
    collect(tower[tower.index("std::string TowerCampaignRuntime::Objective"):],
            "tower_campaign_runtime.cpp:objectives/panels/endings")
    scene = json.loads((ROOT / "data/scenes/recovery_scene.json").read_text(encoding="utf-8"))
    for link in scene["transitions"]:
        candidates.setdefault(link["unavailableMessage"], []).append("recovery_scene.json")
    missing = [{"text": text, "sources": sorted(set(origins))}
               for text, origins in sorted(candidates.items()) if not covered(text)]
    return {
        "status": "PASS" if not missing else "INCOMPLETE",
        "scope": "catalog parity and selected concrete player-display literals",
        "catalog_pairs": counts,
        "concrete_literals": len(candidates),
        "covered_literals": len(candidates) - len(missing),
        "missing": missing,
        "dynamic_fragments_require_composed_tests": sorted(fragments),
        "not_proven": ["all possible dynamic combinations", "foreground terminal glyphs",
                       "human language review", "audio listening", "visual acceptance"],
    }


if __name__ == "__main__":
    result = inspect()
    print(json.dumps(result, ensure_ascii=False, indent=2))
    raise SystemExit(0 if result["status"] == "PASS" else 1)
