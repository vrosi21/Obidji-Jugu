#!/usr/bin/env python3
"""Build the offline vector-map resources used by Obidji Jugu.

The boundary input is raw Overpass JSON and the city input is a GeoJSON export
from the queries in tools/overpass. The generated runtime map contains projected
canvas coordinates, while generated GeoJSON retains longitude/latitude.
Only Python's standard library is required.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import unicodedata
from pathlib import Path
from typing import Any, Iterable


COUNTRIES = {
    "SI": "Slovenia",
    "HR": "Croatia",
    "BA": "Bosnia and Herzegovina",
    "RS": "Serbia",
    "ME": "Montenegro",
    "MK": "North Macedonia",
    "XK": "Kosovo",
}

GRAPH_CITY_ALIASES = {
    "Beograd": ("Beograd",),
    "Zagreb": ("Zagreb",),
    "Skoplje": ("Skoplje", "Skopje", "Скопје"),
    "Sarajevo": ("Sarajevo",),
    "Ljubljana": ("Ljubljana",),
    "Niš": ("Niš", "Nis", "Ниш"),
    "Novi Sad": ("Novi Sad", "Нови Сад"),
    "Split": ("Split",),
    "Banja Luka": ("Banja Luka", "Бања Лука"),
    "Podgorica": ("Podgorica", "Подгорица"),
    "Kragujevac": ("Kragujevac", "Крагујевац"),
    "Priština": ("Priština", "Pristina", "Prishtinë", "Приштина"),
    "Rijeka": ("Rijeka",),
    "Subotica": ("Subotica", "Суботица"),
    "Maribor": ("Maribor",),
    "Peć": ("Peć", "Pec", "Pejë", "Пећ"),
    "Osijek": ("Osijek",),
    "Tuzla": ("Tuzla",),
    "Pančevo": ("Pančevo", "Pancevo", "Панчево"),
    "Zrenjanin": ("Zrenjanin", "Зрењанин"),
    "Zenica": ("Zenica",),
    "Čačak": ("Čačak", "Cacak", "Чачак"),
    "Bitola": ("Bitola", "Битола"),
    "Tetovo": ("Tetovo", "Тетово"),
    "Pula": ("Pula",),
    "Smederevo": ("Smederevo", "Смедерево"),
    "Kumanovo": ("Kumanovo", "Куманово"),
    "Mostar": ("Mostar",),
    "Valjevo": ("Valjevo", "Ваљево"),
    "Šabac": ("Šabac", "Sabac", "Шабац"),
    "Slavonski Brod": ("Slavonski Brod", "Sllavonski Brod"),
}

NAME_KEYS = (
    "int_name",
    "name:bs",
    "name:hr",
    "name:sr-Latn",
    "name:sl",
    "name:sq",
    "name:en",
    "name",
)

# The default graph deliberately keeps the original 30 cities and adds this
# hand-picked geographic spread. Fallback coordinates are used only for towns
# absent from the population > 15,000 Overpass catalogue.
EDITABLE_CITY_SELECTION = (
    {"name": "Dubrovnik", "aliases": ("Dubrovnik",)},
    {"name": "Zadar", "aliases": ("Zadar",)},
    {"name": "Umag", "aliases": ("Umag",), "lon": 13.52389, "lat": 45.43139, "population": 6750},
    {"name": "Duvno", "aliases": ("Duvno", "Tomislavgrad"), "lon": 17.22515, "lat": 43.71849, "population": 5760},
    {"name": "Neum", "aliases": ("Neum",), "lon": 17.61560, "lat": 42.92300, "population": 3013},
    {"name": "Bihać", "aliases": ("Bihać", "Bihac")},
    {"name": "Velika Kladuša", "aliases": ("Velika Kladuša", "Velika Kladusa"), "lon": 15.80579, "lat": 45.18497, "population": 4520},
    {"name": "Cazin", "aliases": ("Cazin",), "lon": 15.94306, "lat": 44.96694, "population": 14387},
    {"name": "Goražde", "aliases": ("Goražde", "Gorazde")},
    {"name": "Bijeljina", "aliases": ("Bijeljina",)},
    {"name": "Bugojno", "aliases": ("Bugojno",)},
    {"name": "Doboj", "aliases": ("Doboj",)},
    {"name": "Brčko", "aliases": ("Brčko", "Brcko")},
    {"name": "Priboj", "aliases": ("Priboj",), "lon": 19.52580, "lat": 43.58360, "population": 14015},
    {"name": "Novi Pazar", "aliases": ("Novi Pazar",)},
    {"name": "Vranje", "aliases": ("Vranje",)},
    {"name": "Nikšić", "aliases": ("Nikšić", "Niksic")},
    {"name": "Herceg Novi", "aliases": ("Herceg Novi", "Herceg-Novi"), "lon": 18.53750, "lat": 42.45306, "population": 19536},
    {"name": "Makarska", "aliases": ("Makarska",), "lon": 17.02000, "lat": 43.29300, "population": 13834},
    {"name": "Varaždin", "aliases": ("Varaždin", "Varazdin")},
)


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def endpoint_key(point: tuple[float, float]) -> tuple[float, float]:
    return round(point[0], 7), round(point[1], 7)


def stitch_way_members(members: list[dict[str, Any]], role: str) -> list[list[list[float]]]:
    segments: list[list[tuple[float, float]]] = []
    for member in members:
        if member.get("type") != "way" or member.get("role") != role:
            continue
        geometry = member.get("geometry") or []
        points = [(float(p["lon"]), float(p["lat"])) for p in geometry if "lon" in p and "lat" in p]
        if len(points) >= 2:
            segments.append(points)

    endpoint_index: dict[tuple[float, float], set[int]] = {}
    for index, segment in enumerate(segments):
        endpoint_index.setdefault(endpoint_key(segment[0]), set()).add(index)
        endpoint_index.setdefault(endpoint_key(segment[-1]), set()).add(index)

    unused = set(range(len(segments)))
    rings: list[list[list[float]]] = []
    while unused:
        current_index = min(unused)
        unused.remove(current_index)
        ring = list(segments[current_index])

        while endpoint_key(ring[0]) != endpoint_key(ring[-1]):
            candidates = endpoint_index.get(endpoint_key(ring[-1]), set()) & unused
            if not candidates:
                raise ValueError(f"Cannot close an OSM relation {role} ring near {ring[-1]}")
            next_index = min(candidates)
            unused.remove(next_index)
            segment = segments[next_index]
            if endpoint_key(segment[0]) == endpoint_key(ring[-1]):
                ring.extend(segment[1:])
            else:
                ring.extend(reversed(segment[:-1]))

        rings.append([[lon, lat] for lon, lat in ring])
    return rings


def signed_ring_area(ring: list[list[float]]) -> float:
    area = 0.0
    previous = ring[-1]
    for current in ring:
        area += float(previous[0]) * float(current[1]) - float(current[0]) * float(previous[1])
        previous = current
    return area * 0.5


def overpass_relations_to_features(data: dict[str, Any]) -> list[dict[str, Any]]:
    result = []
    for relation in data.get("elements", []):
        tags = relation.get("tags", {})
        iso = tags.get("ISO3166-1")
        if relation.get("type") != "relation" or iso not in COUNTRIES:
            continue

        outers = stitch_way_members(relation.get("members", []), "outer")
        inners = stitch_way_members(relation.get("members", []), "inner")
        if not outers:
            raise ValueError(f"Relation {relation.get('id')} ({iso}) has no closed outer rings")

        polygons: list[list[list[list[float]]]] = [[outer] for outer in outers]
        for inner in inners:
            point = inner[0]
            containing = [
                index for index, polygon in enumerate(polygons)
                if point_in_ring(float(point[0]), float(point[1]), polygon[0])
            ]
            if not containing:
                raise ValueError(f"Relation {relation.get('id')} ({iso}) has an unassigned inner ring")
            selected = min(containing, key=lambda index: abs(signed_ring_area(polygons[index][0])))
            polygons[selected].append(inner)

        geometry_type = "Polygon" if len(polygons) == 1 else "MultiPolygon"
        result.append(
            {
                "type": "Feature",
                "properties": {**tags, "@id": f"relation/{relation.get('id')}"},
                "geometry": {
                    "type": geometry_type,
                    "coordinates": polygons[0] if geometry_type == "Polygon" else polygons,
                },
                "id": f"relation/{relation.get('id')}",
            }
        )
    return result


def boundary_features(data: dict[str, Any]) -> list[dict[str, Any]]:
    source_features = (
        data.get("features", [])
        if data.get("type") == "FeatureCollection"
        else overpass_relations_to_features(data)
    )
    result = []
    for feature in source_features:
        props = feature.get("properties", {})
        geometry = feature.get("geometry") or {}
        iso = props.get("ISO3166-1")
        if iso in COUNTRIES and geometry.get("type") in ("Polygon", "MultiPolygon"):
            result.append(feature)
    found = {f["properties"]["ISO3166-1"] for f in result}
    missing = set(COUNTRIES) - found
    if missing:
        raise ValueError(f"Boundary export is missing: {', '.join(sorted(missing))}")
    return sorted(result, key=lambda f: list(COUNTRIES).index(f["properties"]["ISO3166-1"]))


def polygons_of(feature: dict[str, Any]) -> list[list[list[list[float]]]]:
    geometry = feature["geometry"]
    if geometry["type"] == "Polygon":
        return [geometry["coordinates"]]
    return geometry["coordinates"]


def all_boundary_points(features: Iterable[dict[str, Any]]) -> Iterable[tuple[float, float]]:
    for feature in features:
        for polygon in polygons_of(feature):
            for ring in polygon:
                for lon, lat, *_ in ring:
                    yield float(lon), float(lat)


def point_segment_distance(point: tuple[float, float], start: tuple[float, float], end: tuple[float, float]) -> float:
    px, py = point
    ax, ay = start
    bx, by = end
    dx, dy = bx - ax, by - ay
    if dx == 0.0 and dy == 0.0:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def rdp_open(points: list[tuple[float, float]], tolerance: float) -> list[tuple[float, float]]:
    if len(points) <= 2:
        return points
    distance, index = max(
        (point_segment_distance(point, points[0], points[-1]), idx)
        for idx, point in enumerate(points[1:-1], start=1)
    )
    if distance <= tolerance:
        return [points[0], points[-1]]
    left = rdp_open(points[: index + 1], tolerance)
    right = rdp_open(points[index:], tolerance)
    return left[:-1] + right


def simplify_closed_ring(raw_ring: list[list[float]], tolerance: float) -> list[list[float]]:
    points = [(float(p[0]), float(p[1])) for p in raw_ring]
    if len(points) > 1 and points[0] == points[-1]:
        points.pop()
    if len(points) < 4:
        return [[x, y] for x, y in points + points[:1]]

    first = min(range(len(points)), key=lambda i: (points[i][0], points[i][1]))
    opposite = max(
        range(len(points)),
        key=lambda i: (points[i][0] - points[first][0]) ** 2 + (points[i][1] - points[first][1]) ** 2,
    )
    if first > opposite:
        first, opposite = opposite, first

    arc_a = points[first : opposite + 1]
    arc_b = points[opposite:] + points[: first + 1]
    simplified = rdp_open(arc_a, tolerance)[:-1] + rdp_open(arc_b, tolerance)[:-1]
    if len(simplified) < 3:
        simplified = points
    simplified.append(simplified[0])
    return [[round(x, 7), round(y, 7)] for x, y in simplified]


def simplify_boundaries(features: list[dict[str, Any]], tolerance: float) -> dict[str, Any]:
    output = {"type": "FeatureCollection", "features": []}
    for source in features:
        props = source["properties"]
        simplified_polygons = []
        for polygon in polygons_of(source):
            simplified_polygons.append([simplify_closed_ring(ring, tolerance) for ring in polygon])
        geometry_type = source["geometry"]["type"]
        coordinates: Any = simplified_polygons[0] if geometry_type == "Polygon" else simplified_polygons
        output["features"].append(
            {
                "type": "Feature",
                "properties": {
                    "name": COUNTRIES[props["ISO3166-1"]],
                    "country_code": props["ISO3166-1"],
                    "osm_id": props.get("@id", source.get("id", "")),
                    "source": "OpenStreetMap contributors",
                },
                "geometry": {"type": geometry_type, "coordinates": coordinates},
            }
        )
    return output


def coastline_chains(data: dict[str, Any], tolerance: float) -> list[dict[str, Any]]:
    """Join directed OSM coastline ways into continuous open/closed chains."""
    segments: list[list[tuple[float, float]]] = []
    for element in data.get("elements", []):
        if element.get("type") != "way" or element.get("tags", {}).get("natural") != "coastline":
            continue
        geometry = element.get("geometry") or []
        points = [(float(p["lon"]), float(p["lat"])) for p in geometry if "lon" in p and "lat" in p]
        if len(points) >= 2:
            segments.append(points)

    starts: dict[tuple[float, float], set[int]] = {}
    ends: dict[tuple[float, float], set[int]] = {}
    for index, segment in enumerate(segments):
        starts.setdefault(endpoint_key(segment[0]), set()).add(index)
        ends.setdefault(endpoint_key(segment[-1]), set()).add(index)

    unused = set(range(len(segments)))
    chains: list[dict[str, Any]] = []
    while unused:
        open_starts = [
            index for index in unused
            if not (ends.get(endpoint_key(segments[index][0]), set()) & unused)
        ]
        current_index = min(open_starts) if open_starts else min(unused)
        unused.remove(current_index)
        chain = list(segments[current_index])

        while endpoint_key(chain[0]) != endpoint_key(chain[-1]):
            forward = starts.get(endpoint_key(chain[-1]), set()) & unused
            reverse = ends.get(endpoint_key(chain[-1]), set()) & unused
            if forward:
                next_index = min(forward)
                segment = segments[next_index]
            elif reverse:
                # Defensive fallback for a locally reversed OSM way.
                next_index = min(reverse)
                segment = list(reversed(segments[next_index]))
            else:
                break
            unused.remove(next_index)
            chain.extend(segment[1:])

        closed = endpoint_key(chain[0]) == endpoint_key(chain[-1])
        if closed:
            simplified = simplify_closed_ring([[x, y] for x, y in chain], tolerance)
        else:
            simplified = [[round(x, 7), round(y, 7)] for x, y in rdp_open(chain, tolerance)]
        if len(simplified) >= (4 if closed else 2):
            chains.append({"closed": closed, "coordinates": simplified})

    chains.sort(key=lambda chain: (not chain["closed"], -len(chain["coordinates"])))
    return chains


def coastline_geojson(chains: list[dict[str, Any]], timestamp: Any, tolerance: float) -> dict[str, Any]:
    return {
        "type": "FeatureCollection",
        "metadata": {
            "source": "OpenStreetMap contributors",
            "license": "ODbL 1.0",
            "osm_base_timestamp": timestamp,
            "simplification_tolerance_degrees": tolerance,
        },
        "features": [
            {
                "type": "Feature",
                "properties": {"closed": chain["closed"]},
                "geometry": {"type": "LineString", "coordinates": chain["coordinates"]},
            }
            for chain in chains
        ],
    }


def mercator(lon: float, lat: float) -> tuple[float, float]:
    latitude = math.radians(max(-85.05112878, min(85.05112878, lat)))
    return math.radians(lon), math.log(math.tan(math.pi / 4.0 + latitude / 2.0))


class CanvasProjection:
    def __init__(self, features: list[dict[str, Any]], width: float, height: float, padding: float):
        projected = [mercator(lon, lat) for lon, lat in all_boundary_points(features)]
        xs = [p[0] for p in projected]
        ys = [p[1] for p in projected]
        self.min_x, self.max_x = min(xs), max(xs)
        self.min_y, self.max_y = min(ys), max(ys)
        usable_w = width - 2.0 * padding
        usable_h = height - 2.0 * padding
        self.scale = min(usable_w / (self.max_x - self.min_x), usable_h / (self.max_y - self.min_y))
        drawn_w = (self.max_x - self.min_x) * self.scale
        drawn_h = (self.max_y - self.min_y) * self.scale
        self.offset_x = (width - drawn_w) / 2.0
        self.offset_y = (height - drawn_h) / 2.0

    def point(self, lon: float, lat: float) -> tuple[float, float]:
        x, y = mercator(lon, lat)
        return (
            self.offset_x + (x - self.min_x) * self.scale,
            self.offset_y + (self.max_y - y) * self.scale,
        )


def filter_coastlines_for_canvas(
    chains: list[dict[str, Any]],
    projection: CanvasProjection,
    minimum_island_size: float,
) -> list[dict[str, Any]]:
    """Keep the mainland coast and only islands visible on the logical canvas."""
    retained = []
    for chain in chains:
        if not chain["closed"]:
            retained.append(chain)
            continue
        projected = [projection.point(float(p[0]), float(p[1])) for p in chain["coordinates"]]
        width = max(p[0] for p in projected) - min(p[0] for p in projected)
        height = max(p[1] for p in projected) - min(p[1] for p in projected)
        if max(width, height) >= minimum_island_size:
            retained.append(chain)
    return retained


def limit_chain_points(chain: dict[str, Any], maximum_points: int) -> dict[str, Any]:
    coordinates = chain["coordinates"]
    if len(coordinates) <= maximum_points:
        return chain
    if chain["closed"]:
        core = coordinates[:-1] if coordinates[0] == coordinates[-1] else coordinates
        target = max(3, maximum_points - 1)
        sampled = [core[(index * len(core)) // target] for index in range(target)]
        sampled.append(sampled[0])
    else:
        target = max(2, maximum_points)
        sampled = [
            coordinates[round(index * (len(coordinates) - 1) / (target - 1))]
            for index in range(target)
        ]
    return {"closed": chain["closed"], "coordinates": sampled}


def constrain_coastline_complexity(
    chains: list[dict[str, Any]],
    projection: CanvasProjection,
    maximum_islands: int,
    maximum_mainland_points: int,
    maximum_island_points: int,
) -> list[dict[str, Any]]:
    """Apply a hard canvas-safe point budget while retaining the largest islands."""
    mainland = [chain for chain in chains if not chain["closed"]]
    island_rows = []
    for chain in chains:
        if not chain["closed"]:
            continue
        projected = [projection.point(float(p[0]), float(p[1])) for p in chain["coordinates"]]
        width = max(p[0] for p in projected) - min(p[0] for p in projected)
        height = max(p[1] for p in projected) - min(p[1] for p in projected)
        island_rows.append((max(width, height), width * height, chain))
    island_rows.sort(key=lambda row: (row[0], row[1]), reverse=True)

    retained = [limit_chain_points(chain, maximum_island_points) for _, _, chain in island_rows[:maximum_islands]]
    retained.extend(limit_chain_points(chain, maximum_mainland_points) for chain in mainland)
    return retained


def write_runtime_map(
    path: Path,
    simplified: dict[str, Any],
    coastlines: list[dict[str, Any]],
    projection: CanvasProjection,
) -> int:
    point_count = 0
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write("EXYU_MAP_V2\n")
        for feature in simplified["features"]:
            props = feature["properties"]
            stream.write(f"COUNTRY {props['country_code']}\n")
            for polygon in polygons_of(feature):
                stream.write("POLYGON\n")
                for ring_index, ring in enumerate(polygon):
                    projected = [projection.point(float(p[0]), float(p[1])) for p in ring]
                    stream.write(f"RING {1 if ring_index else 0} {len(projected)}\n")
                    for x, y in projected:
                        stream.write(f"{x:.3f} {y:.3f}\n")
                    point_count += len(projected)
                stream.write("ENDPOLYGON\n")
            stream.write("ENDCOUNTRY\n")
        for coastline in coastlines:
            projected = [projection.point(float(p[0]), float(p[1])) for p in coastline["coordinates"]]
            stream.write(f"COASTLINE {1 if coastline['closed'] else 0} {len(projected)}\n")
            for x, y in projected:
                stream.write(f"{x:.3f} {y:.3f}\n")
            point_count += len(projected)
        stream.write("END\n")
    return point_count


def point_in_ring(lon: float, lat: float, ring: list[list[float]]) -> bool:
    inside = False
    previous = ring[-1]
    for current in ring:
        x1, y1 = float(previous[0]), float(previous[1])
        x2, y2 = float(current[0]), float(current[1])
        if (y1 > lat) != (y2 > lat):
            crossing = (x2 - x1) * (lat - y1) / (y2 - y1) + x1
            if lon < crossing:
                inside = not inside
        previous = current
    return inside


def point_in_polygon(lon: float, lat: float, polygon: list[list[list[float]]]) -> bool:
    return bool(polygon) and point_in_ring(lon, lat, polygon[0]) and not any(
        point_in_ring(lon, lat, hole) for hole in polygon[1:]
    )


def country_for_point(lon: float, lat: float, boundaries: list[dict[str, Any]]) -> str:
    # Kosovo is checked before Serbia so disputed/overlapping source geometries do
    # not make the result depend on relation order.
    ordered = sorted(boundaries, key=lambda f: f["properties"]["ISO3166-1"] != "XK")
    for feature in ordered:
        if any(point_in_polygon(lon, lat, polygon) for polygon in polygons_of(feature)):
            return feature["properties"]["ISO3166-1"]
    return ""


def parse_population(raw: Any) -> int | None:
    if raw is None:
        return None
    match = re.search(r"\d+", str(raw).replace(" ", "").replace(",", ""))
    return int(match.group(0)) if match else None


def preferred_name(props: dict[str, Any]) -> str:
    for key in NAME_KEYS:
        value = props.get(key)
        if value:
            return str(value)
    return ""


def normalize_name(value: str) -> str:
    text = unicodedata.normalize("NFKD", value.casefold())
    return "".join(ch for ch in text if ch.isalnum() and not unicodedata.combining(ch))


def feature_names(props: dict[str, Any]) -> set[str]:
    names = set()
    for key, value in props.items():
        if (key == "name" or key == "int_name" or key.startswith("name:")) and value:
            names.add(normalize_name(str(value)))
    return names


def dedupe_city_features(features: list[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = {}
    for feature in features:
        props = feature.get("properties", {})
        geometry = feature.get("geometry") or {}
        if geometry.get("type") != "Point":
            continue
        population = parse_population(props.get("population"))
        if population is None or population <= 15000 or props.get("place") not in ("city", "town"):
            continue
        lon, lat = map(float, geometry["coordinates"][:2])
        # Wikidata is the strongest cross-object identity. When it is missing,
        # the rounded location prevents unrelated same-name towns from being
        # collapsed while still merging a place node and its local relation.
        key = props.get("wikidata") or (
            f"name:{normalize_name(preferred_name(props))}:{round(lon, 1):.1f}:{round(lat, 1):.1f}"
        )
        grouped.setdefault(str(key), []).append(feature)

    selected = []
    for group in grouped.values():
        # A place node is normally the best map point. Fill any missing metadata
        # from the matching administrative relation.
        group.sort(key=lambda f: (not str(f.get("id", f.get("properties", {}).get("@id", ""))).startswith("node/"),))
        chosen = json.loads(json.dumps(group[0]))
        merged = chosen.setdefault("properties", {})
        for candidate in group[1:]:
            for key, value in candidate.get("properties", {}).items():
                if not merged.get(key) and value not in (None, ""):
                    merged[key] = value
        selected.append(chosen)
    return selected


def build_city_catalog(city_data: dict[str, Any], boundaries: list[dict[str, Any]]) -> dict[str, Any]:
    output = {"type": "FeatureCollection", "features": []}
    for feature in dedupe_city_features(city_data.get("features", [])):
        props = feature.get("properties", {})
        lon, lat = map(float, feature["geometry"]["coordinates"][:2])
        code = country_for_point(lon, lat, boundaries)
        if code not in COUNTRIES:
            continue
        raw_id = str(feature.get("id", props.get("@id", "")))
        osm_type, _, osm_numeric_id = raw_id.partition("/")
        output["features"].append(
            {
                "type": "Feature",
                "properties": {
                    "name": preferred_name(props),
                    "country": COUNTRIES[code],
                    "country_code": code,
                    "population": parse_population(props.get("population")),
                    "population_date": props.get("population:date"),
                    "place": props.get("place"),
                    "osm_id": int(osm_numeric_id) if osm_numeric_id.isdigit() else osm_numeric_id,
                    "osm_type": osm_type,
                    "wikidata": props.get("wikidata"),
                    "source_population": props.get("source:population"),
                },
                "geometry": {"type": "Point", "coordinates": [lon, lat]},
            }
        )
    output["features"].sort(key=lambda f: (f["properties"]["country_code"], f["properties"]["name"]))
    return output


def find_graph_city_feature(name: str, city_features: list[dict[str, Any]]) -> dict[str, Any] | None:
    wanted = {normalize_name(alias) for alias in GRAPH_CITY_ALIASES.get(name, (name,))}
    candidates = [feature for feature in city_features if wanted & feature_names(feature.get("properties", {}))]
    if not candidates:
        return None
    candidates.sort(key=lambda f: (not str(f.get("id", f.get("properties", {}).get("@id", ""))).startswith("node/"),))
    return candidates[0]


def graph_city_name_keys(name: str) -> set[str]:
    return {normalize_name(alias) for alias in GRAPH_CITY_ALIASES.get(name, (name,))}


def find_selected_city_feature(selection: dict[str, Any], city_catalog: dict[str, Any]) -> dict[str, Any] | None:
    wanted = {normalize_name(alias) for alias in selection["aliases"]}
    candidates = [
        feature for feature in city_catalog.get("features", [])
        if normalize_name(str(feature.get("properties", {}).get("name") or "")) in wanted
    ]
    candidates.sort(key=lambda feature: feature.get("properties", {}).get("osm_type") != "node")
    return candidates[0] if candidates else None


def graph_road(city_a: dict[str, Any], city_b: dict[str, Any]) -> dict[str, Any]:
    length = math.hypot(float(city_a["x"]) - float(city_b["x"]), float(city_a["y"]) - float(city_b["y"]))
    return {
        "from": int(city_a["id"]),
        "to": int(city_b["id"]),
        "length": round(length, 2),
        "travel_time_h": round(length / 60.0, 3),
        "type": "local",
        "bidirectional": True,
    }


def update_graph(
    path: Path,
    city_data: dict[str, Any],
    city_catalog: dict[str, Any],
    projection: CanvasProjection,
    base_city_count: int,
) -> tuple[int, int, int]:
    with path.open("r", encoding="utf-8-sig") as stream:
        graph = json.load(stream)

    cities = graph.setdefault("cities", [])
    roads = graph.setdefault("roads", [])
    if len(cities) < base_city_count:
        raise ValueError(f"Graph has only {len(cities)} cities; expected at least {base_city_count}")

    # Always return to the original graph before recreating the curated layer.
    del cities[base_city_count:]
    roads[:] = [
        road for road in roads
        if int(road["from"]) < base_city_count and int(road["to"]) < base_city_count
    ]

    updated = 0
    missing = []
    features = city_data.get("features", [])
    for city in cities:
        feature = find_graph_city_feature(city["name"], features)
        if feature is None:
            missing.append(city["name"])
            continue
        lon, lat = map(float, feature["geometry"]["coordinates"][:2])
        x, y = projection.point(lon, lat)
        # CityPoint stores the marker's top-left corner; geographic coordinates
        # describe its center.
        city["x"] = round(x - 5.0, 2)
        city["y"] = round(y - 5.0, 2)
        updated += 1
    if missing:
        raise ValueError(f"No OSM match for graph cities: {', '.join(missing)}")

    original_city_count = len(cities)
    used_names = set()
    for city in cities:
        used_names.update(graph_city_name_keys(str(city["name"])))

    added_indices = []
    for selection in EDITABLE_CITY_SELECTION:
        display_name = str(selection["name"])
        normalized = normalize_name(display_name)
        if normalized in used_names:
            raise ValueError(f"Selected editable city already exists in base graph: {display_name}")

        feature = find_selected_city_feature(selection, city_catalog)
        if feature is not None:
            props = feature.get("properties", {})
            lon, lat = map(float, feature["geometry"]["coordinates"][:2])
            population = int(props.get("population") or selection.get("population") or 0)
        else:
            if "lon" not in selection or "lat" not in selection:
                raise ValueError(f"No OSM catalogue match or fallback coordinate for {display_name}")
            lon = float(selection["lon"])
            lat = float(selection["lat"])
            population = int(selection.get("population") or 0)

        x, y = projection.point(lon, lat)
        city_id = len(cities)
        cities.append(
            {
                "id": city_id,
                "x": round(x - 5.0, 2),
                "y": round(y - 5.0, 2),
                "name": display_name,
                "weight": round(population / 10000.0, 1),
                "visitation_status": 1,
            }
        )
        used_names.add(normalized)
        added_indices.append(city_id)

    road_pairs = {
        tuple(sorted((int(road["from"]), int(road["to"]))))
        for road in roads
    }
    roads_added = 0
    for city_index in added_indices:
        city = cities[city_index]
        original_neighbors = sorted(
            range(original_city_count),
            key=lambda index: math.hypot(
                float(city["x"]) - float(cities[index]["x"]),
                float(city["y"]) - float(cities[index]["y"]),
            ),
        )
        all_neighbors = sorted(
            (index for index in range(len(cities)) if index != city_index),
            key=lambda index: math.hypot(
                float(city["x"]) - float(cities[index]["x"]),
                float(city["y"]) - float(cities[index]["y"]),
            ),
        )
        selected_neighbors = []
        if original_neighbors:
            selected_neighbors.append(original_neighbors[0])
        for neighbor in all_neighbors:
            if neighbor not in selected_neighbors:
                selected_neighbors.append(neighbor)
            if len(selected_neighbors) == 2:
                break
        for neighbor in selected_neighbors:
            pair = tuple(sorted((city_index, neighbor)))
            if pair in road_pairs:
                continue
            roads.append(graph_road(city, cities[neighbor]))
            road_pairs.add(pair)
            roads_added += 1

    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(graph, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    return updated, len(added_indices), roads_added


def write_json(path: Path, value: dict[str, Any]) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--boundaries", required=True, type=Path, help="Raw Overpass JSON from ex_yu_boundaries.overpassql")
    parser.add_argument("--cities", required=True, type=Path, help="GeoJSON exported from ex_yu_cities_15000.overpassql")
    parser.add_argument("--output-dir", type=Path, default=Path("res/osm"))
    parser.add_argument("--runtime-map", type=Path, default=Path("res/ex_yu_boundaries.map"))
    parser.add_argument("--graph-json", type=Path, help="Optional exYu.json whose city coordinates should be aligned")
    parser.add_argument("--tolerance", type=float, default=0.03, help="Boundary simplification tolerance in degrees")
    parser.add_argument(
        "--coastline-tolerance",
        type=float,
        default=0.004,
        help="Coastline simplification tolerance in degrees (smaller keeps more island detail)",
    )
    parser.add_argument(
        "--minimum-island-size",
        type=float,
        default=3.0,
        help="Discard islands smaller than this many units on the 1000 x 866 logical canvas",
    )
    parser.add_argument(
        "--maximum-islands",
        type=int,
        default=10,
        help="Maximum number of largest island rings retained in the runtime map",
    )
    parser.add_argument(
        "--maximum-mainland-points",
        type=int,
        default=160,
        help="Hard point budget for each open mainland coastline",
    )
    parser.add_argument(
        "--maximum-island-points",
        type=int,
        default=8,
        help="Hard point budget for each retained island ring",
    )
    parser.add_argument(
        "--base-graph-city-count",
        type=int,
        default=30,
        help="Keep this many original graph cities before adding the curated editable-city list",
    )
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    args.runtime_map.parent.mkdir(parents=True, exist_ok=True)

    raw_boundaries = load_json(args.boundaries)
    boundaries = boundary_features(raw_boundaries)
    simplified = simplify_boundaries(boundaries, args.tolerance)
    simplified["metadata"] = {
        "source": "OpenStreetMap contributors",
        "license": "ODbL 1.0",
        "osm_base_timestamp": raw_boundaries.get("osm3s", {}).get("timestamp_osm_base"),
        "simplification_tolerance_degrees": args.tolerance,
    }
    projection = CanvasProjection(boundaries, 1000.0, 866.0, 24.0)
    coastlines = coastline_chains(raw_boundaries, args.coastline_tolerance)
    coastlines = filter_coastlines_for_canvas(coastlines, projection, args.minimum_island_size)
    coastlines = constrain_coastline_complexity(
        coastlines,
        projection,
        args.maximum_islands,
        args.maximum_mainland_points,
        args.maximum_island_points,
    )
    if not coastlines:
        raise ValueError("Boundary input contains no natural=coastline ways; use the current boundary query")
    city_data = load_json(args.cities)
    if city_data.get("type") != "FeatureCollection":
        raise ValueError("The city input must be a GeoJSON FeatureCollection")
    city_catalog = build_city_catalog(city_data, boundaries)
    city_catalog["metadata"] = {
        "source": "OpenStreetMap contributors",
        "license": "ODbL 1.0",
        "osm_base_timestamp": city_data.get("timestamp"),
        "selection": "place in (city, town) and population > 15000",
    }

    boundary_path = args.output_dir / "ex_yu_boundaries.geojson"
    coastline_path = args.output_dir / "ex_yu_coastlines.geojson"
    cities_path = args.output_dir / "ex_yu_cities_15000.geojson"
    write_json(boundary_path, simplified)
    write_json(
        coastline_path,
        coastline_geojson(
            coastlines,
            raw_boundaries.get("osm3s", {}).get("timestamp_osm_base"),
            args.coastline_tolerance,
        ),
    )
    write_json(cities_path, city_catalog)
    point_count = write_runtime_map(args.runtime_map, simplified, coastlines, projection)

    graph_count = 0
    graph_added = 0
    graph_roads_added = 0
    if args.graph_json:
        graph_count, graph_added, graph_roads_added = update_graph(
            args.graph_json,
            city_data,
            city_catalog,
            projection,
            args.base_graph_city_count,
        )

    print(f"Generated {len(simplified['features'])} country geometries ({point_count} runtime vertices).")
    coastline_point_count = sum(len(chain["coordinates"]) for chain in coastlines)
    print(f"Generated {len(coastlines)} coastline chains ({coastline_point_count} points).")
    print("Runtime settlement reference points are disabled; only editable graph cities are drawn.")
    print(f"Generated {len(city_catalog['features'])} unique city/town records with population > 15000.")
    if args.graph_json:
        print(f"Aligned {graph_count} graph cities with their OSM coordinates.")
        print(f"Added {graph_added} editable graph cities and {graph_roads_added} connecting roads.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
