# Obiđi Jugu — The Travelling Salesman

An interactive desktop application for visualising and solving the **Travelling Salesman Problem (TSP)** on a map of the former Yugoslavia. Built with C++ and the [natID](https://github.com/) GUI framework, the program lets users place cities, draw road connections, tweak algorithm parameters and watch three different solvers work step-by-step or in real-time.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20Visual%20Studio-green)

---

## Features

| Category | Details |
|---|---|
| **Map canvas** | Lightweight offline vector map generated from simplified OpenStreetMap boundaries and coastline; only the 50 editable graph cities are drawn; aspect-preserving responsive scaling |
| **City management** | Add, edit, delete cities; set coordinates, weight and visitation status (Start, Goal, Open, Blocked) |
| **Road connections** | Toggle connections between cities via side panel or right-click on the map; randomise planar-ish connections |
| **Randomisation** | One-click randomise positions, connections, weights, statuses or all at once; randomise GA parameters |
| **Three TSP solvers** | Nearest Neighbor, Simulated Annealing, Genetic Algorithm — all with configurable parameters |
| **Execution control** | Start / Pause auto-run, Step Forward, Step Back, Reset, Show Solution with animated playback |
| **Step-back** | Full undo history for every algorithm — step backwards through each solver decision |
| **Speed control** | Slider (1–10) controls both auto-stepping cadence and animation playback speed |
| **Per-step animation** | SA animates the tour edge-by-edge per step; GA shows each population member per frame |
| **Solution animation** | Animated green trail with gold leading edge and progress percentage |
| **Localisation** | English and Bosnian UI via toolbar flag buttons; persisted in `lang.cfg` |

---

## Algorithms

### Nearest Neighbor (NN)
Greedy constructive heuristic. Starts at the Start city and repeatedly visits the nearest unvisited required city (Goals + Start anchor). Optional **2-opt** post-optimisation reverses tour sub-segments to shorten total distance.

**Parameters:**
- 2-opt enabled (checkbox)
- Improvement cycles (number of 2-opt passes)

### Simulated Annealing (SA)
Metaheuristic that starts from an initial tour (random or NN-seeded) and iteratively applies 2-opt swaps. Worse solutions are accepted with probability $e^{-\Delta / T}$, allowing escape from local minima. Temperature cools geometrically until it reaches a minimum threshold.

**Parameters:**
- Initial temperature
- Cooling rate $\alpha$ (slider, 0.90–0.99)
- Iterations per temperature level
- Initial solution mode (Random / Nearest Neighbor)

### Genetic Algorithm (GA)
Population-based evolutionary optimisation. Maintains a pool of candidate tours and evolves them through selection, crossover and mutation over many generations. Elitism preserves top individuals.

**Parameters:**
- Population size
- Number of generations
- Mutation rate, Crossover rate
- Selection method (Roulette / Tournament / Rank)
- Mutation operator (Swap / Inversion / OX-like)
- Elitism percentage
- Randomise Parameters button

---

## Screenshots

*The application window with the side panel on the right and the vector map canvas on the left, showing cities, roads and algorithm visualisation.*

---

## Prerequisites

| Requirement | Notes |
|---|---|
| **Visual Studio** 2019 or 2022 | "Desktop development with C++" workload |
| **CMake** ≥ 3.18 | Used for project generation |
| **natID** | GUI framework; ensure headers/libs are available at `$HOME/Work/DevEnv/` |
| **Git** | For cloning |
| **Python** ≥ 3.10 | Optional; only needed to refresh the bundled OSM map resources |

---

## Build & Run

```powershell
# Clone
git clone https://github.com/vrosi21/DSAI_AI-Project-Obidji-Jugu.git
cd DSAI_AI-Project-Obidji-Jugu

# Configure (Visual Studio 2022 x64)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build (Debug or Release)
cmake --build build --config Debug

# Run
.\build\Debug\The_Travelling_Salesman.exe
```

The executable expects `res/` to be accessible relative to the working directory (or the exe location). A `lang.cfg` file will be created on first run to persist the UI language.

---

## Usage Guide

### Setting up the map
1. **Add cities** — fill in name and X/Y coordinates in the side panel, click *Add Point*.
2. **Edit cities** — select from the dropdown, change fields, click *Update* or *Delete*.
3. **Set status** — each city is one of: **Start** (exactly one), **Goal** (must visit), **Open** (transit), **Blocked** (excluded).
4. **Connect roads** — use the *Toggle* button or **right-click** two cities on the map.
5. **Randomise** — click *Positions*, *Connections*, *Weights*, *Status* or *All* to randomise aspects of the map.

### Running a solver
1. **Select algorithm** from the dropdown (NN, SA, GA).
2. **Configure parameters** in the algorithm-specific controls that appear.
3. **Step Forward / Step Back** — advance or rewind one solver step at a time.
4. **Start / Pause** — auto-step at the rate controlled by the speed slider.
5. **Show Solution** — complete remaining steps instantly and play an animated solution trail.
6. **Reset** — clear all solver state and return to the initial map.

### Map interactions
- **Left-click** a city → selects it in the side panel.
- **Right-click** a city → toggles a road connection between the selected city and the clicked city.

---

## Project Structure

```
DSAI_AI-Project-Obidji-Jugu/
├── CMakeLists.txt              # Top-level CMake configuration
├── TTS.cmake                   # Project target definition (sources, libs)
├── README.md                   # This file
├── ARCHITECTURE.md             # Detailed architecture documentation
├── old_README.md               # Original brief readme
│
├── include/                    # All header files
│   ├── Application.h           # gui::Application subclass (entry point)
│   ├── MainWindow.h            # gui::Window — title bar, toolbar, central view
│   ├── MainView.h              # Central orchestrator (timers, solver lifecycle)
│   ├── MapView.h               # Canvas — map rendering + animations
│   ├── SidePanelView.h         # Side panel — CRUD controls + algorithm params
│   ├── ToolBar.h               # Language switch toolbar (EN/BA)
│   ├── DataRepository.h        # Data layer (cities, roads, metric closure)
│   ├── DataTypes.h             # CityPoint, RoadEdge, VisitationStatus
│   ├── JsonService.h           # Static JSON I/O service
│   ├── MapGeometry.h           # Offline vector-map geometry model/loader
│   ├── MapPoint.h              # City rendering helpers (colours, shapes)
│   ├── SearchAlgorithm.h       # Base class for BFS/DFS graph search
│   ├── TSPAlgorithm.h          # Base class for TSP solvers (NN, SA, GA)
│   ├── BFSAlgorithm.h          # Breadth-First Search
│   ├── DFSAlgorithm.h          # Depth-First Search
│   ├── NearestNeighborAlgorithm.h  # NN solver + 2-opt
│   ├── SimulatedAnnealingAlgorithm.h  # SA solver
│   └── GeneticAlgorithmTSP.h   # GA solver
│
├── src/                        # Implementation files
│   ├── main.cpp                # Entry point, language config
│   ├── DataRepository.cpp      # CRUD, metric closure (Floyd-Warshall)
│   ├── JsonService.cpp         # JSON parsing / serialisation
│   ├── MapGeometry.cpp         # Compact generated-map parser
│   ├── MapPoint.cpp            # City marker rendering
│   ├── MapView.cpp             # Canvas drawing, animation logic
│   └── SidePanelView.cpp       # Side panel event handlers
│
├── res/                        # Resources
│   ├── exYu.json               # Default city/road dataset
│   ├── main.xml                # Image and file-resource declarations
│   ├── ex_yu_boundaries.map    # Projected boundaries and coastlines
│   ├── osm/                    # Lon/lat boundaries, coastlines + city catalogue
│   ├── DevRes.xml              # Development resource config
│   ├── flag-us.png             # English flag icon
│   ├── flag-bhs.png            # Bosnian flag icon
│   └── tr/                     # Translations
│       ├── EN/main.xml         # English strings
│       └── BA/main.xml         # Bosnian strings
│
└── build/                      # CMake build output (git-ignored)
```

## Refreshing OpenStreetMap data

The application is deliberately offline at runtime. Ready-to-run Overpass QL
queries and a standard-library Python importer are in [`tools/overpass`](tools/overpass/README.md).
The importer reconstructs `outer` and `inner` relation members, stitches the
actual `natural=coastline` ways (including Adriatic islands), simplifies the
geometry, drops islands smaller than three units on the logical 1000 × 866
canvas, retains the 10 largest visible island rings and enforces a hard point
budget. The resulting runtime coastline contains 11 chains and 240 points.
Together with the simplified borders, the complete runtime vector layer has
1,918 vertices, down from 6,252 in the preceding version.

It also produces a catalogue of the OSM `city`/`town` objects that currently
have `population > 15000`. The original 30 editable cities are preserved and a
curated, geographically balanced set of 20 places is added, giving 50 editable
colour-coded square nodes in total. The added set includes Umag (not the Istria
region as a synthetic city). Every added city is connected to at least two nearby
graph cities. The 135-place OSM catalogue remains available as GeoJSON for data
refreshes, but non-editable settlements are no longer written to the runtime map
or drawn as blue reference points.

Map and place data: © OpenStreetMap contributors, [ODbL 1.0](https://www.openstreetmap.org/copyright).

## Resize performance

`MapView` compiles the static land, border, sea-mask, island and coastline
geometry into persistent natID `gui::Shape` objects once at startup. During a
resize it applies only a `gui::Transformation` for translation and uniform
scaling; the approximately 1,900 vector vertices are no longer copied into new
paths for every repaint.

The generated country rings contain 1,665 border segments because each country
must keep a closed polygon for filling. The cached wire layer canonicalises the
segment direction and draws only 1,050 unique borders, eliminating 615 shared
border duplicates. The same cache reduces 121 road records to 92 unique drawn
segments and rebuilds only after repository data changes. City label resources
are cached as well, and marker rectangles use natID's direct primitive draw
calls instead of temporary shapes.

Canvas rendering intentionally stays on the UI thread because the natID graphics
context is thread-affine. Parallelising individual draw calls would introduce
context races and synchronisation overhead; one-time shape compilation plus
framework transformations is the safe runtime optimisation. OSM reconstruction
and geometric simplification remain offline preprocessing work.

Active window resizing has a separate lightweight path:

- resize-triggered redraw requests are coalesced to at most approximately 30 FPS;
- a 140 ms quiet-period timer detects when resizing has ended;
- the expensive land and shared-border layer is shown from a 1000 × 866
  off-screen `gui::Image` bitmap while the window is moving;
- the small sea-mask, island and coastline overlay stays on the display canvas,
  avoiding off-screen colour-mapping differences;
- city labels, solver overlays and warning text are temporarily skipped, while
  roads and editable markers stay visible;
- after resizing stops, the bitmap is replaced by the final vector render.

---

## Contributors

| Name | Student ID |
|---|---|
| Volodymyr Rodin | 20188 |
| Irfan Hadžić | 20013 |
| Kemal Sivro | 20015 |

## Credits

Special thanks to **Prof. Izudin Džafić** for guidance and for the natID framework components used in this project.

---

## License

Academic project — DSAI programme.
