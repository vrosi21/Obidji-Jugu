import json
import os
import re
import sys
from typing import List, Dict, Tuple

INPUT_FILE = os.path.join(os.path.dirname(__file__), 'cities.txt')
OUTPUT_FILE = os.path.join(os.path.dirname(__file__), 'exYu.json')

# Very small parser for the simple table format in cities.txt
# Each line: | idx | City | Country | Population | lat N | lon E |
# Population has commas; lat/lon have degree symbols and N/E suffix.

COUNTRY_COLORS = {
    'Serbia': '#d62728',
    'Croatia': '#1f77b4',
    'North Macedonia': '#ff7f0e',
    'Bosnia & Herzegovina': '#2ca02c',
    'Slovenia': '#9467bd',
    'Montenegro': '#8c564b',
    'Kosovo': '#e377c2',
}

LAT_RE = re.compile(r"([0-9]+(?:\.[0-9]+)?)\s*[°º]?\s*([NS])", re.IGNORECASE)
LON_RE = re.compile(r"([0-9]+(?:\.[0-9]+)?)\s*[°º]?\s*([EW])", re.IGNORECASE)


def parse_population(token: str) -> float:
    token = token.strip().replace(',', '')
    try:
        return float(token)
    except ValueError:
        return 0.0


def parse_deg(token: str, re_pat: re.Pattern, pos_sign: str) -> float:
    m = re_pat.search(token)
    if not m:
        return 0.0
    val = float(m.group(1))
    hemi = m.group(2).upper()
    if hemi != pos_sign:
        val = -val
    return val


LINE_RE = re.compile(
    r"^\s*\|\s*"               # leading pipe
    r"(?P<idx>\d+)\s*\|\s*"    # index
    r"(?P<city>[^|]+?)\s*\|\s*" # city
    r"(?P<country>[^|]+?)\s*\|\s*" # country
    r"(?P<pop>[^|]+?)\s*\|\s*"  # population
    r"(?P<lat>[^|]+?)\s*\|\s*"  # latitude token
    r"(?P<lon>[^|]+?)\s*\|\s*$" # longitude token then trailing pipe
)


def _sanitize_line(line: str) -> str:
    # Remove BOM, normalize spaces and degree symbol variants
    line = line.lstrip('\ufeff').replace('\xa0', ' ')
    line = line.replace('º', '°')
    return line


def load_rows(path: str) -> List[Dict]:
    rows: List[Dict] = []
    failures: List[Tuple[int, str, str]] = []
    with open(path, 'r', encoding='utf-8') as f:
        for i, raw in enumerate(f, start=1):
            line = _sanitize_line(raw.rstrip('\r\n'))
            if not line.strip():
                continue
            m = LINE_RE.match(line)
            if not m:
                # try relaxed split fallback
                core = line.strip().strip('|')
                parts = [p.strip() for p in core.split('|')]
                if len(parts) < 6:
                    failures.append((i, f"too_few_parts:{len(parts)}", line))
                    continue
                try:
                    _, city, country, pop_s, lat_s, lon_s = parts[:6]
                except Exception as e:
                    failures.append((i, f"unpack_error:{e}", line))
                    continue
            else:
                city = m.group('city').strip()
                country = m.group('country').strip()
                pop_s = m.group('pop').strip()
                lat_s = m.group('lat').strip()
                lon_s = m.group('lon').strip()

            pop = parse_population(pop_s)
            lat = parse_deg(lat_s, LAT_RE, 'N')
            lon = parse_deg(lon_s, LON_RE, 'E')
            rows.append({
                'city': city,
                'country': country,
                'population': pop,
                'lat': lat,
                'lon': lon,
            })

    if not rows:
        print("[cities_to_exYu] No rows parsed. Diagnostics:")
        print(f"  File: {path}")
        try:
            with open(path, 'rb') as fb:
                head = fb.read(200)
                print(f"  First 200 bytes (raw): {head!r}")
        except Exception as e:
            print(f"  Could not read raw bytes: {e}")
        print(f"  Failures logged: {len(failures)} (showing up to 5)")
        for n, (ln, reason, preview) in enumerate(failures[:5], start=1):
            print(f"   [{n}] line {ln} reason={reason} preview={preview}")
    return rows


def compute_bounds(rows: List[Dict]) -> Tuple[float, float, float, float]:
    if not rows:
        raise ValueError("No rows parsed from input; check input format and encoding.")
    min_lat = min(r['lat'] for r in rows)
    max_lat = max(r['lat'] for r in rows)
    min_lon = min(r['lon'] for r in rows)
    max_lon = max(r['lon'] for r in rows)
    return min_lat, max_lat, min_lon, max_lon


def scale_points(rows: List[Dict]) -> List[Dict]:
    # Compute width/height in degrees using lon/lat spans
    min_lat, max_lat, min_lon, max_lon = compute_bounds(rows)
    span_x = max_lon - min_lon
    span_y = max_lat - min_lat
    max_span = max(span_x, span_y)
    if max_span == 0:
        scale = 1.0
    else:
        scale = 900.0 / max_span

    out = []
    for r in rows:
        # Convert to pixel coordinates with padding; origin top-left, so invert Y: larger lat => lower Y
        x = (r['lon'] - min_lon) * scale + 50.0
        y = (max_lat - r['lat']) * scale + 50.0
        weight = r['population'] / 10000.0
        color = COUNTRY_COLORS.get(r['country'], '#7f7f7f')
        out.append({
            'x': round(x, 2),
            'y': round(y, 2),
            'name': r['city'],
            'weight': round(weight, 4),
            'color': color,
        })
    return out


def main():
    try:
        rows = load_rows(INPUT_FILE)
        if not rows:
            raise ValueError("Parsed zero rows from input.")
        scaled = scale_points(rows)
        with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
            json.dump(scaled, f, ensure_ascii=False, indent=2)
        print(f"Wrote {len(scaled)} points to {OUTPUT_FILE}")
    except Exception as e:
        print("[cities_to_exYu] ERROR:", e)
        print("[cities_to_exYu] Hint: ensure file encoding is UTF-8 and lines follow '| idx | City | Country | Population | lat | lon |'.")
        sys.exit(1)


if __name__ == '__main__':
    main()
