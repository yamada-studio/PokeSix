#!/usr/bin/env python3
"""Parse cached Serebii itemdex pages into per-game acquisition rows.

Input: a directory of <pokeapi-item-identifier>.html pages
(https://www.serebii.net/itemdex/<slug>.shtml, fetched politely beforehand).
Output (stdout or --out): JSON list of rows
  {"item", "game_label", "version_groups", "region", "place_slug",
   "place_en", "detail_en"}
Only the "Locations" table is read. Game labels map to PokéAPI version groups.
"""
import html
import json
import re
import sys
from pathlib import Path

# Serebii game label (first cell text) -> PokéAPI version groups
GAMES = {
    "Red": ["red-blue"], "Blue": ["red-blue"], "Yellow": ["yellow"],
    "Gold": ["gold-silver"], "Silver": ["gold-silver"], "Crystal": ["crystal"],
    "Ruby": ["ruby-sapphire"], "Sapphire": ["ruby-sapphire"], "Emerald": ["emerald"],
    "FireRed": ["firered-leafgreen"], "LeafGreen": ["firered-leafgreen"],
    "Colosseum": ["colosseum"], "XD": ["xd"],
    "Diamond": ["diamond-pearl"], "Pearl": ["diamond-pearl"], "Platinum": ["platinum"],
    "HeartGold": ["heartgold-soulsilver"], "SoulSilver": ["heartgold-soulsilver"],
    "PokéWalker": ["heartgold-soulsilver"],
    "Black": ["black-white"], "White": ["black-white"],
    "Black 2": ["black-2-white-2"], "White 2": ["black-2-white-2"],
    "X": ["x-y"], "Y": ["x-y"],
    "Omega Ruby": ["omega-ruby-alpha-sapphire"], "Alpha Sapphire": ["omega-ruby-alpha-sapphire"],
    "Sun": ["sun-moon"], "Moon": ["sun-moon"],
    "Ultra Sun": ["ultra-sun-ultra-moon"], "Ultra Moon": ["ultra-sun-ultra-moon"],
    "Let's Go, Pikachu!": ["lets-go-pikachu-lets-go-eevee"],
    "Let's Go, Eevee!": ["lets-go-pikachu-lets-go-eevee"],
    "Sword": ["sword-shield"], "Shield": ["sword-shield"],
    "Isle of Armor": ["the-isle-of-armor"], "The Isle of Armor": ["the-isle-of-armor"],
    "Crown Tundra": ["the-crown-tundra"], "The Crown Tundra": ["the-crown-tundra"],
    "Brilliant Diamond": ["brilliant-diamond-shining-pearl"],
    "Shining Pearl": ["brilliant-diamond-shining-pearl"],
    "Legends: Arceus": ["legends-arceus"], "Legends Arceus": ["legends-arceus"],
    "Scarlet": ["scarlet-violet"], "Violet": ["scarlet-violet"],
    "The Teal Mask": ["the-teal-mask"], "The Indigo Disk": ["the-indigo-disk"],
    "Legends: Z-A": ["legends-za"], "Legends Z-A": ["legends-za"],
}

ROW = re.compile(r"<tr>(.*?)</tr>", re.S)
CELL = re.compile(r'<td class="([^"]*)"[^>]*>(.*?)</td>', re.S)
LINK = re.compile(r'<a href="([^"]*)">(.*?)</a>', re.S)


def text(fragment):
    return " ".join(html.unescape(re.sub(r"<[^>]+>", " ", fragment)).split())


def split_entries(cell):
    """Split the info cell on commas that are outside tags and parentheses."""
    parts, depth, tag, buf = [], 0, False, []
    for ch in cell:
        if ch == "<":
            tag = True
        elif ch == ">":
            tag = False
        elif not tag and ch == "(":
            depth += 1
        elif not tag and ch == ")":
            depth = max(0, depth - 1)
        if ch == "," and not tag and depth == 0:
            parts.append("".join(buf))
            buf = []
        else:
            buf.append(ch)
    parts.append("".join(buf))
    return [p for p in parts if text(p)]


def parse(path):
    raw = path.read_bytes().decode("latin-1")
    try:
        raw = raw.encode("latin-1").decode("utf-8")
    except UnicodeDecodeError:
        pass
    start = raw.find(">Locations<")
    if start < 0:
        return []
    end = raw.find("</table>", start)
    rows = []
    for row in ROW.findall(raw[start:end]):
        cells = CELL.findall(row)
        if len(cells) < 2:
            continue
        info = cells[-1][1]
        labels = [text(c[1]) for c in cells[:-1]]
        groups = sorted({g for label in labels for g in GAMES.get(label, [])})
        if not groups:
            continue
        for entry in split_entries(info):
            link = LINK.search(entry)
            detail = re.findall(r"<i>\s*\((.*?)\)\s*</i>", entry, re.S)
            place_en, region, slug = text(entry), "", ""
            if link:
                place_en = text(link.group(2))
                m = re.search(r"/pokearth/([^/]+)/([^./#]+)", link.group(1))
                if m:
                    region, slug = m.group(1), m.group(2)
            else:
                place_en = text(re.sub(r"<i>.*?</i>", "", entry, flags=re.S))
            rows.append({
                "item": path.stem, "game_label": " / ".join(labels), "version_groups": groups,
                "region": region, "place_slug": slug, "place_en": place_en,
                "detail_en": "; ".join(text(d) for d in detail),
            })
    return rows


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    folder = Path(argv[0])
    out = None
    if "--out" in argv:
        out = Path(argv[argv.index("--out") + 1])
    rows = []
    for page in sorted(folder.glob("*.html")):
        rows.extend(parse(page))
    data = json.dumps(rows, ensure_ascii=False, indent=1)
    if out:
        out.write_text(data, encoding="utf-8")
    else:
        print(data)
    print(f"{len(rows)} rows from {len(list(folder.glob('*.html')))} pages", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
