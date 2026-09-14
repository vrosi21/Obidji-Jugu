# OSM / Overpass podaci za vektorsku mapu

Aplikacija **ne poziva Overpass pri pokretanju**. Upiti služe samo za povremeno
osvježavanje izvornog skupa podataka, a aplikacija koristi mali lokalni vektorski
resurs. Tako rad aplikacije ostaje brz, determinističan i moguć bez interneta.

Podaci: © OpenStreetMap contributors, dostupni pod ODbL 1.0.

## 1. Granice država

Kopiraj sadržaj `ex_yu_boundaries.overpassql` u
[Overpass Turbo](https://overpass-turbo.eu/) i pritisni **Run**. Upit bira
administrativne relacije nivoa 2 za kodove `SI`, `HR`, `BA`, `RS`, `ME`, `MK` i
`XK`, njihove member ways te `natural=coastline` ways unutar tih država.
Završni `out body geom` vraća stvarnu geometriju; bez `body` Turbo može pri
izvozu napraviti samo bounding-box pravougaonike. Rezultat je velik (deseci MB),
pa query koristi timeout od 300 sekundi.

Za ovaj rezultat izaberi **Export → Data → raw OSM data → download**. Direktni
Turbo GeoJSON konverter ponekad pogrešno spoji neporedane way članove složenih
državnih multipolygona. Projektna skripta zato čita njihove `outer`/`inner` uloge,
spaja zajedničke krajnje nodeove u zatvorene prstenove i tek onda generiše
ispravan GeoJSON. Usmjereni coastline ways se zasebno spajaju u jednu otvorenu
kontinentalnu obalu i zatvorene prstenove stvarnih jadranskih otoka. Time se ne
crtaju pojednostavljeni pomorski dijelovi administrativnih granica kao kopno.

## 2. Gradovi iznad 15.000 stanovnika

Na isti način pokreni `ex_yu_cities_15000.overpassql`. `map_to_area` pretvara
državne relacije u Overpass područja, pa se `node`, `way` i `relation` objekti
traže isključivo unutar tih država. Uključeni su `place=city` i `place=town`, a
uslov `number(t["population"]) > 15000` radi samo nad objektima koji zaista imaju
OSM `population` tag.

OSM nije popis stanovništva: `population` može nedostajati ili biti zastario.
Zato rezultat znači „svi odgovarajući OSM objekti koje trenutni podaci mogu
identifikovati“, a ne garantovano kompletan službeni spisak naselja.

## Pregled i izvoz u Overpass Turbo

1. Nakon izvršavanja izaberi **Data → Show data** za sirovi Overpass JSON.
2. Za granice izaberi **Export → Data → raw OSM data → download**; za gradove
   izaberi **download/copy as GeoJSON**.
3. Sačuvaj rezultate kao, na primjer, `boundaries.json` i `cities.geojson`.
4. Iz korijena projekta pokreni:

```powershell
python tools/build_osm_map_data.py `
  --boundaries boundaries.json `
  --cities cities.geojson `
  --output-dir res/osm `
  --runtime-map res/ex_yu_boundaries.map `
  --graph-json res/exYu.json `
  --base-graph-city-count 30
```

Skripta:

- provjeri da svih sedam država imaju Polygon/MultiPolygon geometriju;
- pojednostavi administrativne granice i obalu odvojeno za brzo crtanje,
  izostavlja otoke manje od tri jedinice na logičkom canvasu i ograničava
  obalu na 240 tačaka (jedna kontinentalna linija i 10 najvećih otoka);
- generiše `res/osm/ex_yu_boundaries.geojson` sa `[longitude, latitude]` tačkama;
- generiše `res/osm/ex_yu_coastlines.geojson` sa otvorenom kontinentalnom obalom
  i zatvorenim prstenovima otoka;
- deduplicira node/relation zapise gradova i dodjeljuje državu point-in-polygon
  provjerom;
- generiše katalog `res/osm/ex_yu_cities_15000.geojson`;
- zadržava svih 135 trenutno pronađenih naselja samo u GeoJSON katalogu;
- projektuje mapu i čvorove grafa u isti 1000 × 866 koordinatni sistem;
- zadržava originalnih 30 editabilnih gradova i dodaje tačno 20 ručno odabranih,
  geografski raspoređenih mjesta (uključujući Umag, a ne sintetički grad
  „Istra“), te svakom novom čvoru daje najmanje dvije obližnje veze. Ostala
  naselja se ne upisuju u runtime mapu i ne crtaju se kao plave tačke.

Podrazumijevani `--tolerance 0.03`, `--coastline-tolerance 0.004`,
`--minimum-island-size 3.0`, `--maximum-mainland-points 160`,
`--maximum-islands 10` i `--maximum-island-points 8` namjerno ograničavaju
kompletnu runtime geometriju na 1.918 tačaka, od čega je 240 obalnih. Budžeti se
mogu povećati za detaljniji izvoz, ali previše geometrije može prekoračiti
praktične limite natID canvasa.

## Kako dobiti samo koordinate jedne granice

U generisanom GeoJSON-u pronađi feature sa željenim `country_code`. Za
`Polygon`, `geometry.coordinates[0]` je vanjski prsten oblika
`[[lon1, lat1], [lon2, lat2], ...]`; naredni prstenovi su rupe. Za
`MultiPolygon`, prvo se prolazi kroz svaki polygon, a zatim njegove prstenove.
Ne treba ručno spajati raw `nodes`, `ways` i `relations`: projektna skripta to
radi automatski iz `members[].role` i `members[].geometry` polja sirovog
Overpass JSON-a.

Minimalan primjer očekivanog izlaza (vrijednosti su samo primjer strukture):

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {"country_code": "BA", "name": "Bosnia and Herzegovina"},
      "geometry": {
        "type": "MultiPolygon",
        "coordinates": [[[[15.7, 45.2], [15.8, 45.1], [15.7, 45.2]]]]
      }
    },
    {
      "type": "Feature",
      "properties": {
        "name": "Sarajevo", "country_code": "BA", "population": 275524,
        "osm_type": "node", "osm_id": 2021709163
      },
      "geometry": {"type": "Point", "coordinates": [18.4131, 43.8563]}
    }
  ]
}
```

Aktuelna sintaksa i značenje tagova dokumentovani su na OSM Wiki stranicama za
[Overpass QL](https://wiki.openstreetmap.org/wiki/OverpassQL),
[`place`](https://wiki.openstreetmap.org/wiki/Key%3Aplace) i
[`population`](https://wiki.openstreetmap.org/wiki/Key%3Apopulation).
