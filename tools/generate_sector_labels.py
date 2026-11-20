#!/usr/bin/env python3
"""Generate CoVCL sector ceiling labels ([[sector_vertical_limit]]) from a EuroScope .ese file.

Usage:
    python tools/generate_sector_labels.py path/to/VVXX.ese > tools/sector_labels.toml
    python tools/generate_sector_labels.py path/to/VVXX.ese --write-config config.toml

Rule used for each airspace SECTOR (top, bottom, OWNER list o1:o2:...:on):
  position oi controls the sector when it is online and none of o1..o(i-1) are online.
  That maps exactly onto CoFrance's format:
      controller_me = oi, default = true, controller_not_others = [o1 .. o(i-1)]
Only radar positions of VCLvACC (callsigns starting VV, VD, VL, VCL, DNA and ending _APP/_CTR)
get labels, and only for radar sectors (first owner is an _APP/_CTR). Centre positions skip
approach (TMA) sectors unless --include-tma is given. Labels sit at the centre of the sector's border lines.
"""
import sys, re, argparse
from collections import defaultdict, OrderedDict

VCL_PREFIXES = ("VV", "VD", "VL", "VCL", "DNA")
RADAR_SUFFIXES = ("_APP", "_CTR")
INCLUDE_TMA = False
MARKER = "# ===== GENERATED SECTOR LABELS (tools/generate_sector_labels.py) ====="


def dms_to_dec(s):
    m = re.match(r"([NSEW])(\d+)\.(\d+)\.(\d+)\.(\d+)", s.strip())
    if not m:
        return None
    h, d, mi, se, ms = m.groups()
    v = int(d) + int(mi) / 60 + (int(se) + int(ms) / 1000) / 3600
    return -v if h in "SW" else v


def dec_to_dms(v, pos, neg, width):
    h = pos if v >= 0 else neg
    v = abs(v)
    d = int(v); m = int((v - d) * 60); s = (v - d - m / 60) * 3600
    sec = int(s); ms = int(round((s - sec) * 1000))
    if ms == 1000:
        sec, ms = sec + 1, 0
    return f"{h}{d:0{width}d}.{m:02d}.{sec:02d}.{ms:03d}"


def fl(ft, top):
    if top and ft >= 66000:
        return "UNL"
    if not top and ft <= 0:
        return "SFC"
    return f"{ft // 100:03d}"


def parse(path):
    text = open(path, encoding="latin-1").read().splitlines()
    section = None
    positions = {}          # position id -> callsign
    lines = {}              # sectorline id -> list of (lat, lon)
    sectors = []
    cur_line = cur_sector = None
    for raw in text:
        line = raw.strip()
        if not line or line.startswith(";"):
            continue
        if line.startswith("["):
            section = line
            continue
        if section == "[POSITIONS]":
            f = line.split(":")
            if len(f) > 3:
                positions[f[3]] = f[0]
        elif section == "[AIRSPACE]":
            key, _, rest = line.partition(":")
            if key == "SECTORLINE":
                cur_line = rest.strip(); lines[cur_line] = []; cur_sector = None
            elif key == "COORD" and cur_line is not None:
                lat, lon = rest.split(":")[:2]
                a, b = dms_to_dec(lat), dms_to_dec(lon)
                if a is not None and b is not None:
                    lines[cur_line].append((a, b))
            elif key == "SECTOR":
                f = rest.split(":")
                cur_sector = {"name": f[0], "bottom": int(f[1]), "top": int(f[2]), "owners": [], "border": []}
                sectors.append(cur_sector); cur_line = None
            elif key == "OWNER" and cur_sector:
                cur_sector["owners"] = [o for o in rest.split(":") if o]
            elif key == "BORDER" and cur_sector:
                cur_sector["border"] = [b for b in rest.split(":") if b]
            elif key in ("CIRCLE_SECTORLINE",):
                cur_line = None
    return positions, lines, sectors


def is_vcl_radar(pos_id, positions):
    cs = positions.get(pos_id, "")
    return cs.startswith(VCL_PREFIXES) and cs.endswith(RADAR_SUFFIXES)


def generate(path):
    positions, lines, sectors = parse(path)
    # (me, sector display name, not_others) -> merged vertical range + points
    groups = OrderedDict()
    for s in sectors:
        pts = [p for b in s["border"] for p in lines.get(b, [])]
        if not pts or s["top"] <= 500 or not s["owners"]:
            continue
        first = positions.get(s["owners"][0], "")
        # Tower/ground sectors are skipped: labels are for radar airspace only
        if not first.endswith(RADAR_SUFFIXES):
            continue
        tma_sector = first.endswith("_APP")
        base = re.sub(r"[·\xb7]\d{3}[·\xb7]\d{3}$", "", s["name"]).replace("\xb7", " ")
        for i, me in enumerate(s["owners"]):
            if not is_vcl_radar(me, positions):
                continue
            # Centre positions only get labels for en-route sectors, unless --include-tma
            if tma_sector and positions.get(me, "").endswith("_CTR") and not INCLUDE_TMA:
                continue
            not_others = tuple(s["owners"][:i])
            key = (me, base, not_others)
            g = groups.setdefault(key, {"bottom": s["bottom"], "top": s["top"], "pts": []})
            g["bottom"] = min(g["bottom"], s["bottom"]); g["top"] = max(g["top"], s["top"])
            g["pts"] += pts
    out = [MARKER, f"# Source: {path.split('/')[-1].split(chr(92))[-1]}", ""]
    for (me, base, not_others), g in groups.items():
        lat = sum(p[0] for p in g["pts"]) / len(g["pts"])
        lon = sum(p[1] for p in g["pts"]) / len(g["pts"])
        size = 30 if positions.get(me, "").endswith("_CTR") else 20
        out += [
            "[[sector_vertical_limit]]",
            f'#comment = "{base} as {positions.get(me, me)}"',
            f'text = "{fl(g["top"], True)}\\r\\n{fl(g["bottom"], False)}"',
            f'position = ["{dec_to_dms(lat, "N", "S", 3)}", "{dec_to_dms(lon, "E", "W", 3)}"]',
            f"size = {size}",
            f'controller_me = "{me}"',
            "controller_others = []",
            "controller_not_others = [" + ", ".join(f'"{o}"' for o in not_others) + "]",
            "default = true",
            "",
        ]
    return "\n".join(out), len(groups)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("ese")
    ap.add_argument("--include-tma", action="store_true", help="also label approach (TMA) sectors for centre positions when they cover them top-down")
    ap.add_argument("--write-config", help="replace the generated block at the end of this config.toml")
    a = ap.parse_args()
    INCLUDE_TMA = a.include_tma
    block, n = generate(a.ese)
    if a.write_config:
        cfg = open(a.write_config, encoding="utf-8").read()
        head = cfg.split(MARKER)[0].rstrip() + "\n\n"
        open(a.write_config, "w", encoding="utf-8", newline="\n").write(head + block)
        print(f"Wrote {n} labels to {a.write_config}", file=sys.stderr)
    else:
        print(block)
