[out:json][timeout:180];

// Build exact administrative areas first, then search only inside them.
rel
  ["boundary"="administrative"]
  ["admin_level"="2"]
  ["ISO3166-1"~"^(SI|HR|BA|RS|ME|MK|XK)$"]
  ->.countries;

.countries map_to_area -> .country_areas;

// OSM populated places may be nodes, ways or relations. Population is optional
// in OSM, so this query can only return objects where that tag is present.
nwr(area.country_areas)
  ["place"~"^(city|town)$"]
  ["name"]
  ["population"]
  (if: number(t["population"]) > 15000);

out tags center;
