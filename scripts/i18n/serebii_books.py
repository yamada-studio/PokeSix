#!/usr/bin/env python3
"""Build per-game acquisition request books from parsed Serebii rows.

Input: the rows JSON produced by serebii_items.py and the PKHeX location
name table (norm(english) -> [korean, japanese, source]).
Output: docs/i18n/acquisition/<version-group>.json gets its "items" filled
(existing entries — e.g. the blog-sourced Platinum/HGSS TMs and tutors — are
kept and win; Serebii only adds items they do not list). A coverage report
is printed per game.

Place resolution order:
 1. PokéAPI location identifier (region-aware name/slug match) -> "where"
 2. Species name (held items)            -> how=held, place = Korean species
 3. PKHeX game-text Korean name          -> place = {ko, en}
 4. Known feature/detail translations    -> place = Korean label
 5. Raw English text (left for review)

Usage: scripts/i18n/serebii_books.py <rows.json> <pkhex-loc-table.json>
"""
import collections
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/i18n/acquisition"

REGION = {"kanto": "Kanto", "johto": "Johto", "hoenn": "Hoenn", "sinnoh": "Sinnoh",
          "unova": "Unova", "kalos": "Kalos", "alola": "Alola", "galar": "Galar",
          "hisui": "Hisui", "paldea": "Paldea", "sevii": "Kanto", "kitakami": "Paldea",
          "terarium": "Paldea", "isleofarmor": "Galar", "crowntundra": "Galar",
          "lumiosecity": "Kalos"}

# 자주 나오는 장소/컨텐츠 글자(공식 한국어 표기가 확실한 것만). 나머지는 영어로 남기고 검토 목록에.
PLACES_KO = {
    "Max Raid Battles": "맥스 레이드배틀", "6 Star Raids :": "6성 테라 레이드배틀",
    "Festival Plaza": "페스서클", "Talk to Following Pokémon": "동행 포켓몬이 주워 온다",
    "Cram-o-Matic": "우라오스의 합체 머신(마스터도장)",
    "Vermillion City": "갈색시티", "Victory Road B2W2": "챔피언로드",
    "Grand Underground": "지하대동굴", "Poké Pelago": "포켓리조트",
    "Super Training": "슈퍼트레이닝", "Battle Maison": "배틀하우스",
    "Battle Tower": "배틀타워", "Battle Subway": "배틀서브웨이",
    "Battle Tree": "배틀트리", "Battle Royal Dome": "배틀로열돔",
    "Join Avenue": "조인애버뉴", "Black City": "블랙시티", "White Forest": "화이트포레스트",
    "Pickup": "특성 픽업", "Loto-ID": "ID 추첨",
}

DETAILS_KO = {
    "With Dowsing Machine": ("hidden", "다우징머신"),
    "With Dowsing MCHN": ("hidden", "다우징머신"),
    "Dustcloud": ("field", "먼지구름"),
    "Hidden": ("hidden", ""),
    "Berry Master": ("gift", "나무열매 명인"),
    "Berry Master's Wife": ("gift", "나무열매 명인의 아내"),
    "Mom's Savings": ("gift", "엄마의 저금"),
    "Pokémon News Press": ("gift", "포켓몬 신문사"),
    "Come 1st in Bug Catching Contest": ("prize", "벌레잡기대회 1등"),
    "Souvenir": ("gift", "기념품"),
    "From Ranger": ("gift", "레인저에게 받기"),
    "Daily Berry": ("gift", "매일 한 번"),
    "Winter Only": ("", "겨울에만"),
    "Spring Only": ("", "봄에만"), "Summer Only": ("", "여름에만"), "Autumn Only": ("", "가을에만"),
}
SHARD = {"Red": "red-shard", "Blue": "blue-shard", "Yellow": "yellow-shard",
         "Green": "green-shard"}


def norm(s):
    return re.sub(r"[^a-z0-9]", "", (s or "").replace("é", "e").replace("’", "'").lower())


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    rows = json.loads(Path(argv[0]).read_text(encoding="utf-8"))
    pkhex = json.loads(Path(argv[1]).read_text(encoding="utf-8"))
    locations = json.loads((ROOT / "docs/i18n/location-ids.json").read_text(encoding="utf-8"))
    species = json.loads((ROOT / "docs/i18n/species-en-ko.json").read_text(encoding="utf-8"))

    by_name = collections.defaultdict(list)
    for ident, v in locations.items():
        by_name[norm(v["en"])].append((ident, v["region"]))
        by_name[norm(ident)].append((ident, v["region"]))

    def resolve(row):
        region = REGION.get(row["region"], "")
        base = re.sub(r"\s*\(.*\)$", "", row["place_en"]).strip()
        for key in (norm(row["place_en"]), norm(base), norm(row["place_slug"])):
            if not key:
                continue
            candidates = by_name.get(key, [])
            same = [c for c in candidates if c[1] == region]
            if same:
                return same[0][0]
            if len(candidates) == 1:
                return candidates[0][0]
        m = re.match(r"route(\d+)$", norm(base))
        if m and region:
            candidate = f"{region.lower()}-route-{m.group(1)}"
            if candidate in locations:
                return candidate
        return None

    def source_of(row, stats):
        source = {}
        base = re.sub(r"\s*\(.*\)$", "", row["place_en"]).strip()
        extra = row["place_en"][len(base):].strip(" ()")  # 괄호 속 세부("Vert Sector 8")
        ident = resolve(row)
        if ident:
            source["where"] = ident
            stats["where"] += 1
        elif base in species and species[base]:
            source["how"] = "held"
            source["place"] = {"ko": f"야생 {species[base]}", "en": f"Wild {base}"}
            stats["held"] += 1
        elif norm(base) in pkhex:
            source["place"] = {"ko": pkhex[norm(base)][0], "en": base}
            stats["pkhex"] += 1
        elif base in PLACES_KO:
            source["place"] = {"ko": PLACES_KO[base], "en": base}
            stats["table"] += 1
        else:
            source["place"] = base  # 영어 그대로 — 검토 대상
            stats["english"] += 1
        details = []
        if extra:
            details.append(extra)
        for piece in filter(None, (p.strip() for p in row["detail_en"].split(";"))):
            m = re.match(r"(\d[\d,]*)\+? Steps", piece)
            if m:
                source.setdefault("how", "other")
                details.append(f"포켓워커 {m.group(1)}걸음 이상")
                continue
            m = re.match(r"Trade for (Red|Blue|Yellow|Green) Shard", piece)
            if m:
                source["how"] = "exchange"
                source["cost"] = [{"amount": 1, "unit": SHARD[m.group(1)]}]
                continue
            if piece in DETAILS_KO:
                how, korean = DETAILS_KO[piece]
                if how and "how" not in source:
                    source["how"] = how
                if korean:
                    details.append(korean)
                continue
            details.append(piece)  # 번역 안 된 조건 — 영어 그대로
        if details:
            source["detail"] = " · ".join(details)
        return source

    books = collections.defaultdict(lambda: collections.defaultdict(list))
    stats_by_group = collections.defaultdict(collections.Counter)
    for row in rows:
        for group in row["version_groups"]:
            entry = source_of(row, stats_by_group[group])
            if entry not in books[group][row["item"]]:
                books[group][row["item"]].append(entry)

    for group, items in sorted(books.items()):
        path = OUT / f"{group}.json"
        book = json.loads(path.read_text(encoding="utf-8")) if path.exists() else {
            "versionGroup": group, "_sources": [], "items": {}, "tutors": {}}
        source_note = "Serebii.net ItemDex (https://www.serebii.net/itemdex/) — 장소는 PokéAPI/게임 텍스트로 한국어화"
        if source_note not in book.get("_sources", []):
            book.setdefault("_sources", []).append(source_note)
        kept = 0
        for item, sources in sorted(items.items()):
            if item in book["items"] and book["items"][item]:
                kept += 1  # 블로그 등 기존(한국어) 항목이 이긴다
                continue
            book["items"][item] = sources
        book["items"] = dict(sorted(book["items"].items()))
        path.write_text(json.dumps(book, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
        s = stats_by_group[group]
        print(f"{group:34} items {len(book['items']):4} (kept {kept:3}) "
              f"where {s['where']:5} held {s['held']:4} pkhex {s['pkhex']:4} "
              f"table {s['table']:3} english {s['english']:4}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
