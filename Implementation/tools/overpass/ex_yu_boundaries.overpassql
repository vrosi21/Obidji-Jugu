[out:json][timeout:300];

// Current countries on the territory of former Yugoslavia.
// "body" is important: relation members are required to reconstruct polygons.
rel
  ["boundary"="administrative"]
  ["admin_level"="2"]
  ["ISO3166-1"~"^(SI|HR|BA|RS|ME|MK|XK)$"]
  ->.countries;

// Include member ways and detailed coastlines. The importer uses the relation
// members for inland borders and natural=coastline along the Adriatic/islands.
way(r.countries)->.country_boundary_ways;
.countries map_to_area -> .country_areas;
way(area.country_areas)["natural"="coastline"]->.coastlines;

(.countries; .country_boundary_ways; .coastlines;);
out body geom;
