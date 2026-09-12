# Architecture — Obiđi Jugu (The Travelling Salesman)

Detailed technical documentation of the project's class hierarchy, data flow, rendering pipeline, algorithm infrastructure and timer/animation state machines.

---

## Table of Contents

1. [High-Level Architecture](#1-high-level-architecture)
2. [Layered Dependency Map](#2-layered-dependency-map)
3. [Class Catalogue](#3-class-catalogue)
4. [Data Layer](#4-data-layer)
5. [Algorithm Layer](#5-algorithm-layer)
6. [UI Layer](#6-ui-layer)
7. [Rendering Pipeline](#7-rendering-pipeline)
8. [Timer & Animation State Machine](#8-timer--animation-state-machine)
9. [Event & Callback Wiring](#9-event--callback-wiring)
10. [File Index](#10-file-index)

---

## 1. High-Level Architecture

The application follows a **Model-View-Controller** pattern layered on top of the natID GUI framework:

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                          │
│  main.cpp → Application → MainWindow → MainView            │
├────────────────┬────────────────────┬───────────────────────┤
│    MODEL       │    VIEW            │    CONTROLLER          │
│                │                    │                        │
│ DataRepository │ MapView (Canvas)   │ MainView (orchestrator)│
│ JsonService    │ SidePanelView      │   • timer management   │
│ DataTypes      │ MapPoint           │   • solver lifecycle   │
│                │ ToolBar            │   • callback routing   │
├────────────────┴────────────────────┴───────────────────────┤
│                     ALGORITHMS                               │
│                                                              │
│  SearchAlgorithm (BFS, DFS)                                  │
│  TSPAlgorithm (NearestNeighbor, SimulatedAnnealing, GA)      │
└──────────────────────────────────────────────────────────────┘
```

### Data Flow Summary

```
exYu.json ──load──► JsonService ──► DataRepository ──► MapView (rendering)
                                        │              SidePanelView (CRUD)
                                        │
                                        └──► TSPAlgorithm (solver reads cities/roads)
                                                 │
                                                 └──► MainView (timer drives steps)
                                                          │
                                                          └──► MapView (draws state)
```

---

## 2. Layered Dependency Map

```
                    ┌──────────────┐
                    │  Application │  (include/Application.h)
                    └──────┬───────┘
                           │ creates
                    ┌──────▼───────┐
                    │  MainWindow  │  (include/MainWindow.h)
                    │  ┌─ToolBar   │  (include/ToolBar.h)
                    └──┼───────────┘
                       │ setCentralView
                    ┌──▼───────────────────────────────────────────┐
                    │                MainView                       │
                    │  (include/MainView.h — 611 lines)            │
                    │                                               │
                    │  owns:  DataRepository  _repo                 │
                    │         MapView         _mapView              │
                    │         SidePanelView   _sidePanel            │
                    │         Timer           _timer, _solutionTimer│
                    │         unique_ptr<SearchAlgorithm> _solver   │
                    └──┬────────────┬────────────┬─────────────────┘
                       │            │            │
          ┌────────────▼──┐  ┌─────▼──────┐  ┌──▼───────────────┐
          │  DataRepository│  │  MapView   │  │  SidePanelView   │
          │  (71 lines .h) │  │  (136+1040)│  │  (231+1217 lines)│
          └───┬────────────┘  └────────────┘  └──────────────────┘
              │ uses
          ┌───▼──────────┐
          │  JsonService  │  (38+239 lines)
          └───┬──────────-┘
              │ reads/writes
          ┌───▼──────┐
          │ exYu.json│
          └──────────┘
```

---

## 3. Class Catalogue

### 3.1 Application Bootstrap

| Class | Header | Lines | Role |
|---|---|---|---|
| `Application` | `Application.h` | 21 | Subclass of `gui::Application`; creates `MainWindow` |
| `MainWindow` | `MainWindow.h` | 66 | Top-level window; hosts `ToolBar` + `MainView`; handles language switch actions |
| `ToolBar` | `ToolBar.h` | 22 | Two flag buttons (EN/BA) wired to `menuID=255` actions |

### 3.2 Data Layer

| Class/Struct | Header | Lines | Role |
|---|---|---|---|
| `CityPoint` | `DataTypes.h` | 38 | City record: id, x, y, weight, name, `VisitationStatus` |
| `RoadEdge` | `DataTypes.h` | — | Lightweight edge (fromId, toId) |
| `RoadInfo` | `DataTypes.h` | — | Full road info (length, travel time, type, bidirectional) |
| `VisitationStatus` | `DataTypes.h` | — | Enum: `Blocked(0)`, `Open(1)`, `Goal(2)`, `Start(3)` |
| `DataRepository` | `DataRepository.h` | 71 | Central data store: CRUD for cities/roads, all-pairs shortest path (metric closure), change notifications |
| `JsonService` | `JsonService.h` | 38 | Static service: load/save `exYu.json`, find file in search paths |

### 3.3 Algorithm Layer

| Class | Header | Lines | Base | Role |
|---|---|---|---|---|
| `SearchAlgorithm` | `SearchAlgorithm.h` | 201 | — | Abstract base: `step()`, `stepBack()`, `reset()`, visited/frontier/path |
| `BFSAlgorithm` | `BFSAlgorithm.h` | 75 | `SearchAlgorithm` | FIFO queue BFS |
| `DFSAlgorithm` | `DFSAlgorithm.h` | 77 | `SearchAlgorithm` | LIFO stack DFS |
| `TSPAlgorithm` | `TSPAlgorithm.h` | 241 | `SearchAlgorithm` | Abstract TSP base: tour state, history snapshots, `saveSubclassState()` / `restoreSubclassState()` |
| `NearestNeighborAlgorithm` | `NearestNeighborAlgorithm.h` | 173 | `TSPAlgorithm` | Greedy NN + optional 2-opt |
| `SimulatedAnnealingAlgorithm` | `SimulatedAnnealingAlgorithm.h` | 186 | `TSPAlgorithm` | SA with configurable cooling |
| `GeneticAlgorithmTSP` | `GeneticAlgorithmTSP.h` | 352 | `TSPAlgorithm` | GA with selection, crossover, mutation, elitism |

### 3.4 UI Layer

| Class | Header | Impl | Lines | Role |
|---|---|---|---|---|
| `MainView` | `MainView.h` | (header-only) | 611 | Central orchestrator: wires data↔UI, manages solver lifecycle, drives timers |
| `MapView` | `MapView.h` | `MapView.cpp` | 136 + 1040 | Canvas: draws map, cities, roads, algorithm state, animations |
| `SidePanelView` | `SidePanelView.h` | `SidePanelView.cpp` | 231 + 1217 | Side panel: CRUD forms, algorithm parameter inputs, solver control buttons |
| `MapPointRenderer` | `MapPoint.h` | `MapPoint.cpp` | 32 + 115 | Static helpers for drawing colour-coded city markers |

---

## 4. Data Layer

### 4.1 DataRepository

The repository is the **single source of truth** for all map data. It owns the city and road arrays, provides CRUD operations, and maintains an **all-pairs shortest path matrix** (metric closure) via Floyd-Warshall.

```
DataRepository
├── _cities : vector<CityPoint>
├── _roads  : vector<RoadEdge>         (lightweight edges for rendering)
├── _roadsFull : vector<RoadInfo>      (full info for serialisation)
├── _metricDist : vector<vector<double>>   (all-pairs shortest distances)
├── _metricPrev : vector<vector<int>>      (predecessor matrix for path reconstruction)
├── _onDataChanged : ChangeCallback
│
├── City CRUD: addCity, updateCity, updateCityStatus, deleteCity
├── Road CRUD: addConnection, removeConnection
├── Queries:   getMetricDistance(i,j), getShortestRoadPath(i,j,outPath)
│              getLargestConnectedComponent()
│              cities(), roads(), getCityNames()
└── Internal:  load() → JsonService, save() → JsonService
               recomputeMetricClosure() → Floyd-Warshall
```

**Change notification flow:**

```
Any CRUD operation
    └──► notifyChange()
           ├──► recomputeMetricClosure()   (updates distance/predecessor matrices)
           └──► _onDataChanged()           (callback → MainView resets solver, clears canvas)
```

### 4.2 JsonService

Stateless static class for reading/writing the `exYu.json` file. Contains hand-rolled JSON parsing (no external library dependency). Searches multiple candidate paths (CWD, exe directory) to locate the data file.

### 4.3 Metric Closure

`DataRepository::recomputeMetricClosure()` runs **Floyd-Warshall** on the road graph to compute:
- `_metricDist[i][j]` — shortest weighted distance between cities `i` and `j`
- `_metricPrev[i][j]` — predecessor on the shortest path from `i` to `j`

This lets TSP algorithms treat the city graph as a **complete weighted graph** where edge costs reflect shortest road paths, not just Euclidean distance.

`getShortestRoadPath(a, b, outPath)` reconstructs the actual road-hop sequence, which is used to render tour edges along the road network rather than as straight lines.

---

## 5. Algorithm Layer

### 5.1 Class Hierarchy

```
SearchAlgorithm (abstract)
├── BFSAlgorithm        ← FIFO queue, level-by-level
├── DFSAlgorithm        ← LIFO stack, depth-first
│
└── TSPAlgorithm (abstract)
    ├── NearestNeighborAlgorithm
    ├── SimulatedAnnealingAlgorithm
    └── GeneticAlgorithmTSP
```

### 5.2 SearchAlgorithm Base

Provides the generic graph-search lifecycle:

```
reset(repo)          → find Start/Goal, clear state
step()               → save snapshot, get next node, check goal, mark visited, expand frontier
stepBack()           → pop last snapshot from history
canStepBack()        → !history.empty()
isComplete()         → state.finished
getVisited()         → set of visited node indices
getFrontier()        → current frontier for visualisation
getPath()            → path from start to goal (after goal found)
```

**State snapshot:** `AlgorithmState` captures `visited`, `frontier`, `parent` map, `currentIdx`, `finished`, `foundGoal`.

### 5.3 TSPAlgorithm Base

Extends `SearchAlgorithm` with TSP-specific concepts:

```
TSPState
├── tour : vector<int>          (current tour — indices of required cities)
├── bestTour : vector<int>      (best tour found so far)
├── tourLength : double         (current tour cost)
├── bestLength : double         (best cost found)
├── currentStep : int
└── finished : bool

TSPSnapshot
├── baseState : TSPState
└── subclassState : std::any    (opaque subclass-specific state)
```

**Key design: polymorphic state snapshots.** Each subclass overrides `saveSubclassState()` and `restoreSubclassState()` to capture/restore its own fields alongside the base `TSPState`:

| Algorithm | Saved Fields |
|---|---|
| NN | `_unvisited` (set), `_currentCity`, `_remaining2OptCycles` |
| SA | `_temperature`, `_iterAtCurrentTemp` |
| GA | `_generation`, `_population`, `_fitness` |

This ensures step-back restores the **complete** algorithm state, not just the base tour data.

**Required cities:** TSP algorithms only visit non-Blocked Goal cities plus the Start city as anchor. Open cities are not directly in the tour but may appear in expanded road paths.

### 5.4 Nearest Neighbor

```
initializeTSP()
├── Build _unvisited set from _requiredCities
├── Start from _startCityIdx
└── Clear tour, set tourLength = 0

performStep()
├── If _unvisited empty:
│   ├── If 2-opt enabled: applyBestTwoOptMove(), decrement cycles
│   └── Else: finalize (set finished)
│
├── Find nearest unvisited city by calculateTransitionCost()
├── Add to tour, remove from _unvisited, update _currentCity
└── If last city visited: finalize partial tour
```

**Cost function:** `Cost(A→B) = EuclideanDistance(A,B) × Weight(B)`

### 5.5 Simulated Annealing

```
initializeTSP()
├── Set temperature = initialTemp
├── Generate initial tour (random or NN-seeded)
└── Set bestTour = initial tour

performStep()
├── If temperature ≤ minTemp: finished
├── Generate neighbour via random 2-opt swap
├── Calculate ΔE = newLength - currentLength
├── Accept if ΔE < 0, or with probability e^(-ΔE/T)
├── Cool: iterAtCurrentTemp++; if >= iterationsPerTemp: T *= α
└── Return (T > minTemp)
```

### 5.6 Genetic Algorithm

```
initializeTSP()
├── Generate populationSize random tours
├── Calculate fitness for each (1/tourLength)
└── updateBest()

performStep()
├── If generation ≥ maxGenerations: finished
├── Elitism: copy top-N individuals to new population
├── Fill remaining via:
│   ├── Selection: Roulette / Tournament / Rank
│   ├── Crossover: Order Crossover (OX) with probability crossoverRate
│   └── Mutation: Swap / Inversion with probability mutationRate
├── Replace population, recalculate fitness
├── updateBest()
└── generation++
```

---

## 6. UI Layer

### 6.1 MainView — Central Orchestrator

`MainView` is the heart of the application. It owns the data repository, both views, both timers, and the current solver. It is entirely header-only (611 lines).

```
MainView : gui::View
│
├── Layout:  SplitterLayout (MapView | SidePanelView)
│
├── Data:    DataRepository _repo
│            unique_ptr<SearchAlgorithm> _solver
│
├── Timers:  _timer          (SOLVER_STEP_INTERVAL = 0.5s, drives auto-stepping)
│            _solutionTimer  (SOLUTION_ANIM_INTERVAL = 0.05s, drives animations)
│
├── State:   _running              (auto-step active)
│            _solutionRunning      (solution animation active)
│            _stepAnimRunning      (step-tour animation active)
│            _currentAlgorithmIdx  (0=NN, 1=SA, 2=GA)
│
├── Actions: handleSolverAction(action, algoIdx)
│            ├── 0  → Start / Pause toggle
│            ├── 1  → Step Forward
│            ├── -1 → Step Back
│            ├── 2  → Algorithm Changed (re-create solver)
│            ├── 3  → Show Solution (fast-forward + animate)
│            └── 4  → Reset
│
├── Solver:  selectAlgorithm(idx)    → create + reset solver
│            startSolver()           → start _timer auto-stepping
│            stopSolver()            → stop _timer
│            stepForward()           → single step + trigger animation
│            stepBackward()          → undo step + restore state
│
├── Timer:   onTimer(_timer)         → advance solver / play step-tour anim
│            onTimer(_solutionTimer) → advance solution animation
│                                      OR advance manual step animation
│
└── Wiring:  _repo.onDataChanged → reset solver, clear canvas
             _mapView.onPrimaryCityClick → select in side panel
             _mapView.onSecondaryCityClick → toggle connection
             _sidePanel.solverCallback → handleSolverAction
```

### 6.2 SidePanelView

Grid-based form with five sections:

```
SidePanelView : gui::View
│
├── Section 1: ADD NEW POINT
│   └── Name, X, Y → btnAddPt
│
├── Section 2: EDIT SELECTED POINT
│   └── Dropdown, Name, X, Y, Weight, Status → btnUpdate, btnDelete
│
├── Section 3: CONNECTIONS
│   └── Connected-to label, Connect-to dropdown → btnToggle
│
├── Section 4: RANDOMIZE
│   └── btnPositions, btnConnections, btnWeights, btnStatus, btnAll
│
└── Section 5: SOLVE
    ├── Algorithm dropdown (NN / SA / GA)
    ├── Algorithm-specific parameters (shown/hidden dynamically):
    │   ├── NN: 2-opt checkbox, improvement cycles
    │   ├── SA: temperature, cooling rate slider, iterations/temp, initial solution
    │   └── GA: population, generations, mutation/crossover rates, selection, operator, elitism
    ├── btnStartPause, btnStepFwd, btnStepBwd
    ├── btnShowSolution, btnReset
    └── Animation speed slider (1–10)
```

**Key callbacks:**
- `onChangedSelection(ComboBox*)` — handles city selection, status changes, algorithm switch (shows/hides parameter controls)
- `onClick(Button*)` — routes to handler methods; solver actions fire `_solverCallback(action, algoIdx)`

### 6.3 MapView

Canvas-based renderer with multiple drawing layers:

```
MapView : gui::Canvas
│
├── onDraw(rect)
│   ├── Draw generated offline OSM vector geometry
│   │   ├── Sea, administrative land polygons and inland borders
│   │   └── Coastline sea mask and the largest Adriatic islands
│   │       (persistent gui::Shape cache + gui::Transformation on resize)
│   ├── Draw road connections (black bezier lines)
│   ├── Draw algorithm state (if solver attached):
│   │   ├── BFS/DFS: visited (green), frontier (orange), current (blue), path
│   │   └── TSP: drawTSPState() → see Rendering Pipeline
│   ├── Draw unreachable-goals warning (red text)
│   └── Draw city markers on top (MapPointRenderer)
│
├── Click handlers:
│   ├── onPrimaryButtonPressed → hitTestCity → _onPrimaryCityClick
│   └── onSecondaryButtonPressed → hitTestCity → _onSecondaryCityClick
│
├── Animation APIs:
│   ├── Solution animation:  startSolutionAnimation(), advanceSolutionAnimation()
│   ├── Step-tour animation: startStepTourAnimation() (SA edge-by-edge)
│   │                        queueStepTourFrames() (GA full-path per member)
│   │                        advanceStepTourAnimation()
│   └── Stop methods:        stopSolutionAnimation(), stopStepTourAnimation()
│
└── Coordinate system:
    ├── Logical: 1000×866 (shared projected coordinates for boundaries and cities)
    └── Scaled:  aspect-preserving fit + centering offsets for current window size
```

`MapGeometry` loads the `EXYU_MAP_V2` resource `ex_yu_boundaries.map`, generated
from OSM administrative relation members and `natural=coastline` ways by
`tools/build_osm_map_data.py`. Overpass is not contacted by the application.
`MapView` first draws administrative land, masks the maritime parts with the open
mainland coastline and refills 10 retained coastline island rings. Only the 50
editable graph cities are drawn; the separate OSM catalogue is not part of the
runtime rendering path. The generated `res/osm/*.geojson` files retain
longitude/latitude for inspection and future import features, while the compact
runtime file avoids adding a JSON/GIS dependency to the renderer.

Static map paths are constructed once by `buildVectorMapShapes()`. Its border
cache treats edges as undirected, quantised endpoint pairs, reducing 1,665 ring
edges to 1,050 unique wire segments. `buildRoadShape()` similarly caches 92
unique road segments from 121 records and rebuilds only when repository geometry
changes. `gui::Transformation` performs the resize translation/scaling without
reallocating the underlying native shapes. Drawing is deliberately not dispatched
to worker threads because the natID graphics context is owned by the UI thread.

During an active resize, `onResize()` coalesces explicit repaint requests through
a 30 FPS one-shot timer. `onDraw()` uses a pre-rendered 1000 × 866 `gui::Image`
for the expensive land and shared-border layer and omits labels, solver overlays
and warnings. The lightweight sea-mask, island and coastline overlay remains a
live display-context vector layer so its colours exactly match normal rendering.
A second one-shot timer treats 140 ms without another resize event as completion
and requests one final full vector redraw. The bitmap is a temporary resize
preview; normal rendering remains resolution-independent vector output.

---

## 7. Rendering Pipeline

### 7.1 TSP State Rendering

When a TSP solver is attached, `drawTSPState()` dispatches to one of three rendering modes:

```
drawTSPState()
│
├── IF solution animation active → drawAnimatedSolution()
│   ├── Expanded path: tour edges resolved to road-hop sequences
│   ├── Completed trail: thick ForestGreen bezier
│   ├── Leading edge: thicker Gold bezier + Gold city highlight
│   └── Progress label: "Solution: XX%"
│
├── ELSE IF step-tour animation active → drawStepTourAnimation()
│   ├── Completed trail: DarkOrange bezier
│   ├── Leading edge: Gold bezier + Gold city highlight
│   └── Info panel: member index (GA) or path progress
│
└── ELSE → static state rendering
    ├── NN:
    │   ├── Tour path: DodgerBlue with visit-order numbers
    │   ├── Head city: Gold fill (last added, if not complete)
    │   └── Start city: LightSkyBlue fill
    │
    ├── SA / GA:
    │   ├── Best tour (ghost): thin SteelBlue (if different from current)
    │   ├── Current tour: thick DarkOrange
    │   ├── Start city: LightSalmon fill
    │   └── Other cities: DarkOrange wire ring
    │
    └── Info panel (WhiteSmoke background, DarkSlateGray text):
        ├── NN:  Algorithm, Step, Tour cost, 2-opt status
        ├── SA:  Algorithm, Step, Temperature, Best cost, Current cost
        └── GA:  Algorithm, Generation/MaxGen, Best cost, Current cost
```

### 7.2 Colour Palette

| Element | Colour | Usage |
|---|---|---|
| NN tour path | `DodgerBlue` | Construction path with visit-order numbers |
| SA/GA current tour | `DarkOrange` | Thick tour line |
| SA/GA best tour | `SteelBlue` | Thin ghost reference line |
| NN head city | `Gold` / `OrangeRed` | Last-added city highlight |
| Solution trail | `ForestGreen` | Animated solution playback |
| Leading edge | `Gold` | Current animation front |
| City markers | Status-based | Blocked=Red/Yellow, Goal=Yellow/Violet, Start=Yellow/Cyan, Open=Yellow/Yellow |
| Info panel bg | `WhiteSmoke` / `Silver` | Semi-transparent overlay |

### 7.3 Tour Path Expansion

Tour edges between required cities are **expanded** along the actual road network:

```
buildExpandedTourPath(tour) → outPath
│
├── For each consecutive pair (tour[i], tour[i+1]):
│   └── repo.getShortestRoadPath(a, b, leg)
│       → returns intermediate road-hop cities
│
└── Concatenate all legs → smooth path along roads
```

This means the rendered path follows the road connections rather than drawing straight lines between distant cities.

---

## 8. Timer & Animation State Machine

The application uses **two timers** managed by `MainView`:

```
┌──────────────────────────────────────────────────────────────┐
│                      Timer Architecture                       │
│                                                               │
│  _timer (0.5s interval)          _solutionTimer (0.05s)       │
│  ┌─────────────────────┐         ┌─────────────────────────┐  │
│  │ Auto-step solver     │         │ Solution animation      │  │
│  │                      │         │ OR                      │  │
│  │ IF _stepAnimRunning: │         │ Manual step-tour replay │  │
│  │   drive step-tour    │         │                         │  │
│  │   animation instead  │         │                         │  │
│  │   of stepping solver │         │                         │  │
│  └──────────────────────┘         └─────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 8.1 State Transitions

```
                    ┌──────────┐
                    │  IDLE    │  (no solver running, no animation)
                    └──┬───┬──┘
           Start/Pause │   │ Step Forward/Back
                    ┌──▼───┘──┐
        ┌───────────│ RUNNING  │◄────────────────────────┐
        │           └──┬───┬──┘                          │
        │    step done │   │ step-tour animation started  │
        │              │   ▼                              │
        │              │  ┌────────────────┐              │
        │              │  │ STEP_ANIM_RUN  │──anim done──►│
        │              │  │ (SA/GA replay) │              │
        │              │  └────────────────┘              │
        │              │
        │   solver complete
        │              │
        │              ▼
        │         ┌──────────────┐
        │         │ SHOW_SOLUTION│  (auto-triggered after completion)
        │         │ (animation)  │
        │         └──────┬───────┘
        │                │ animation done
        │                ▼
        │           ┌────────┐
        └──Pause──► │ PAUSED │ ◄── Step Back (from any state)
                    └────────┘
```

### 8.2 Speed Control

The speed slider (1–10) affects two aspects:

```
Speed slider value → getTicksPerAdvanceFromSpeed()
    speed 1  → 10 ticks between advances (slowest)
    speed 10 → 1 tick between advances  (fastest)

Speed slider value → getStepAnimEdgesPerTick()
    speed 1  → 1 edge per animation tick
    speed 10 → 10 edges per animation tick
```

### 8.3 Step-Tour Animation Modes

| Mode | Algorithm | Behaviour | Entry Point |
|---|---|---|---|
| Edge-by-edge | SA | Draws tour progressively, edge by edge | `startStepTourAnimation(tour)` |
| Full-frame queue | GA | Shows each population member as a complete path, one per speed tick | `queueStepTourFrames(tours)` |

---

## 9. Event & Callback Wiring

### 9.1 Startup Wiring

```
MainView constructor:
│
├── _mapView.setRepository(&_repo)
├── _sidePanel.setRepository(&_repo)
├── _sidePanel.populatePointNames(_repo.getCityNames())
│
├── _mapView.setOnPrimaryCityClick(λ → _sidePanel.selectPointIndex)
├── _mapView.setOnSecondaryCityClick(λ → toggle connection)
│
├── _sidePanel.setSolverCallback(λ → handleSolverAction)
│
├── _repo.setOnDataChanged(λ → stopAllExecution, reset solver, clear canvas)
│
└── selectAlgorithm(0)  → create initial NN solver
```

### 9.2 Data Change Flow

```
User action (add/edit/delete city, toggle connection, change status)
    │
    ▼
DataRepository::notifyChange()
    ├── recomputeMetricClosure()       ← Floyd-Warshall rebuild
    └── _onDataChanged()
         │
         ▼
    MainView callback:
         ├── stopAllExecution()        ← stop timers, clear animation
         ├── _solver->reset(&_repo)    ← reinitialise with new data
         ├── _mapView.setSolver(nullptr) ← clear canvas
         ├── _mapView.refresh()
         └── refreshStepButtons()

    SidePanelView callback:
         └── reDraw()                  ← refresh side panel display
```

### 9.3 Solver Action Dispatch

```
SidePanelView::onClick(button)
    └── _solverCallback(actionCode, algorithmIdx)
         │
         ▼
    MainView::handleSolverAction(action, algorithmIdx)
         │
         ├── 0: Start/Pause  → startSolver() / stopSolver()
         ├── 1: Step Forward  → stepForward()
         ├── -1: Step Back    → stepBackward()
         ├── 2: Algo Changed  → selectAlgorithm(idx)
         ├── 3: Show Solution → fast-forward + startSolutionAnimation()
         └── 4: Reset         → stopAllExecution() + solver->reset()
```

---

## 10. File Index

### Headers (`include/`)

| File | Lines | Description |
|---|---|---|
| `Application.h` | 21 | `gui::Application` subclass; creates `MainWindow` |
| `MainWindow.h` | 66 | Top-level window; sets toolbar, central view; handles language action items |
| `MainView.h` | 611 | **Core orchestrator**: owns repo, views, timers, solver; header-only |
| `MapView.h` | 136 | Canvas header: animation APIs, click callbacks, drawing state |
| `SidePanelView.h` | 231 | Side panel header: all GUI controls, callback types, getters |
| `ToolBar.h` | 22 | Two-button language toolbar |
| `DataRepository.h` | 71 | Data store: CRUD, metric closure, change notifications |
| `DataTypes.h` | 38 | `CityPoint`, `RoadEdge`, `RoadInfo`, `VisitationStatus` |
| `JsonService.h` | 38 | Static JSON I/O (load/save/find) |
| `MapPoint.h` | 32 | `MapPointStyle` + `MapPointRenderer` for city markers |
| `SearchAlgorithm.h` | 201 | Abstract graph search base: step/back/visited/frontier/path |
| `TSPAlgorithm.h` | 241 | Abstract TSP base: tour state, snapshot system, required cities |
| `BFSAlgorithm.h` | 75 | BFS with FIFO queue |
| `DFSAlgorithm.h` | 77 | DFS with LIFO stack |
| `NearestNeighborAlgorithm.h` | 173 | Greedy NN + optional 2-opt improvement |
| `SimulatedAnnealingAlgorithm.h` | 186 | SA with configurable cooling schedule |
| `GeneticAlgorithmTSP.h` | 352 | GA: selection, crossover, mutation, elitism |

### Sources (`src/`)

| File | Lines | Description |
|---|---|---|
| `main.cpp` | 41 | Entry point; reads `lang.cfg`, initialises `Application` |
| `DataRepository.cpp` | 406 | CRUD implementations, Floyd-Warshall metric closure |
| `JsonService.cpp` | 239 | Hand-rolled JSON parser/writer, file search |
| `MapGeometry.cpp` | — | Compact offline vector-map resource parser |
| `MapPoint.cpp` | 115 | City marker drawing (colour logic, scaled rendering) |
| `MapView.cpp` | 1040 | Canvas rendering: TSP state, animations, info panel, tours |
| `SidePanelView.cpp` | 1217 | All side panel event handlers, randomisation logic |

### Resources (`res/`)

| File | Description |
|---|---|
| `exYu.json` | Default dataset: 50 editable cities and 121 connected roads across ex-Yugoslavia |
| `ex_yu_boundaries.map` | `EXYU_MAP_V2`: projected, aggressively simplified boundaries and coastlines |
| `osm/ex_yu_boundaries.geojson` | Simplified country polygons in `[lon, lat]` coordinates |
| `osm/ex_yu_coastlines.geojson` | Stitched mainland coastline and detailed closed island rings |
| `osm/ex_yu_cities_15000.geojson` | Deduplicated OSM city/town catalogue with country, population and OSM identity |
| `main.xml` | Image resource declarations (flag icons) |
| `DevRes.xml` | Development resource configuration |
| `flag-us.png` | English flag icon for toolbar |
| `flag-bhs.png` | Bosnian flag icon for toolbar |
| `tr/EN/main.xml` | English UI strings |
| `tr/BA/main.xml` | Bosnian UI strings |

### Build

| File | Description |
|---|---|
| `CMakeLists.txt` | Top-level CMake: loads DevEnv, natGUI, includes `TTS.cmake` |
| `TTS.cmake` | Project target: globs sources, sets include paths, links natID libs |

---

**Total codebase: ~5,600 lines** across 23 source/header files + 8 resource files.
