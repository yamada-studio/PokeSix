#!/usr/bin/env python3
"""Validate and install a researched acquisition book.

Input: a filled docs/i18n/acquisition/<version-group>.json. The script checks
every item / move / location identifier against the reference lists, every
"how" and cost unit against the known vocabularies, drops "_" helper keys and
uncertain entries, then writes resources/data/acquisition/<version-group>.json
and docs/i18n/review-acquisition-<version-group>.md.

Usage: scripts/i18n/check_acquisition.py <filled.json> [--dry-run]
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ITEMS = ROOT / "docs/i18n/item-ids.json"
LOCATIONS = ROOT / "docs/i18n/location-ids.json"
OUT_DIR = ROOT / "resources/data/acquisition"
HOWS = {"field", "hidden", "gift", "shop", "exchange", "prize", "reward", "held", "trade",
        "tutor", "other"}
UNITS = {"money", "bp", "coins", "red-shard", "blue-shard", "yellow-shard", "green-shard",
         "heart-scale", "athlete-points", "watts", "league-points"}
FIELDS = {"how", "where", "place", "content", "cost", "detail", "note", "confidence", "source"}


def clean_sources(key, sources, locations, problems):
    kept = []
    for source in sources if isinstance(sources, list) else []:
        if not isinstance(source, dict):
            problems.append(f"{key}: not an object: {source!r}")
            continue
        if source.get("confidence") == "uncertain" or "불확실" in str(source.get("note", "")):
            problems.append(f"{key}: uncertain, dropped: {json.dumps(source, ensure_ascii=False)}")
            continue
        extra = set(source) - FIELDS
        if extra:
            problems.append(f"{key}: unknown fields {sorted(extra)}")
        how = source.get("how")
        if how and how not in HOWS:
            problems.append(f"{key}: unknown how '{how}' (kept as other)")
            source["how"] = "other"
        where = source.get("where")
        if where and where not in locations:
            problems.append(f"{key}: unknown location '{where}' (moved to place)")
            source.setdefault("place", where)
            del source["where"]
        for cost in source.get("cost", []) or []:
            if cost.get("unit") not in UNITS:
                problems.append(f"{key}: unknown cost unit '{cost.get('unit')}'")
        for drop in ("note", "confidence", "source"):
            source.pop(drop, None)
        kept.append(source)
    return kept


def register_resource(group):
    """Add the book to resources/resources.qrc (each file must be listed there)."""
    qrc = ROOT / "resources/resources.qrc"
    text = qrc.read_text(encoding="utf-8")
    line = f"<file>data/acquisition/{group}.json</file>"
    if line in text:
        return
    anchor = "<file>data/place-names.json</file>"
    text = text.replace(anchor, f"{line}\n    {anchor}", 1)
    qrc.write_text(text, encoding="utf-8")


def main(argv):
    dry = "--dry-run" in argv
    files = [Path(a) for a in argv if not a.startswith("--")]
    if len(files) != 1:
        print(__doc__)
        return 2
    data = json.loads(files[0].read_text(encoding="utf-8"))
    group = data["versionGroup"]
    items = json.loads(ITEMS.read_text(encoding="utf-8"))
    locations = json.loads(LOCATIONS.read_text(encoding="utf-8"))
    problems = []
    book = {
        "_comment": [
            "게임(버전 그룹)별 입수 사전 (ui/dex/guidebook.h가 읽는다). 형식은 docs/i18n/README.md 참고.",
            "scripts/i18n/check_acquisition.py로 검증해 넣었다. 출처는 _sources.",
        ],
        "_sources": data.get("_sources", []),
        "versionGroup": group,
        "items": {},
        "tutors": {},
    }
    for key, sources in data.get("items", {}).items():
        if key not in items:
            problems.append(f"unknown item '{key}' (dropped)")
            continue
        kept = clean_sources(key, sources, locations, problems)
        if kept:
            book["items"][key] = kept
    for key, sources in data.get("tutors", {}).items():
        kept = clean_sources(key, sources, locations, problems)
        if kept:
            book["tutors"][key] = kept
    review = ROOT / f"docs/i18n/review-acquisition-{group}.md"
    review.write_text("\n".join([f"# Acquisition review: {group}", "",
                                 f"items {len(book['items'])}, tutors {len(book['tutors'])}", ""]
                                + [f"- {p}" for p in problems]) + "\n", encoding="utf-8")
    if not dry:
        OUT_DIR.mkdir(parents=True, exist_ok=True)
        (OUT_DIR / f"{group}.json").write_text(
            json.dumps(book, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        register_resource(group)
    print(f"{group}: items {len(book['items'])}, tutors {len(book['tutors'])}, "
          f"problems {len(problems)}{' (dry run)' if dry else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
