# Project Defense Guide — The Travelling Salesman (Obiđi Jugu)

> This document explains how every framework concept (layouts, views, windows, canvas, resources, CMake) is concretely implemented in our TSP project. Use it to prepare for defending your implementation decisions.

---

## Table of Contents

1. [Application Lifecycle](#1-application-lifecycle)
2. [Window — MainWindow](#2-window--mainwindow)
3. [Layout — SplitterLayout in MainView](#3-layout--splitterlayout-in-mainview)
4. [View — SidePanelView with GridLayout](#4-view--sidepanelview-with-gridlayout)
5. [ViewScroller — SidePanelScroller](#5-viewscroller--sidepanelscroller)
6. [Canvas — MapView](#6-canvas--mapview)
7. [Resource System — DevRes.xml & main.xml](#7-resource-system--devresxml--mainxml)
8. [Translation / i18n](#8-translation--i18n)
9. [Persistent Settings — IAppProperties](#9-persistent-settings--iappproperties)
10. [Data Layer — Repository & JSON](#10-data-layer--repository--json)
11. [Algorithm Architecture](#11-algorithm-architecture)
12. [Solver Orchestration in MainView](#12-solver-orchestration-in-mainview)
13. [Responsive Design & Sizing](#13-responsive-design--sizing)
14. [CMake Build Configuration](#14-cmake-build-configuration)
15. [Cross-Platform Considerations](#15-cross-platform-considerations)
16. [Architecture Diagram](#16-architecture-diagram)

---

## 1. Application Lifecycle

**Files:** `include/Application.h`, `src/main.cpp`

The app starts in `main()`, which creates our `Application` subclass, reads the saved language preference, initializes the framework, and enters the event loop:

```cpp
// src/main.cpp
int main(int argc, const char* argv[]) {
    Application app(argc, argv);
    auto appProperties = app.getProperties();
    td::String trLang = appProperties->getValue("translation", "EN");
    app.init(trLang);
    return app.run();
}
```

`Application` extends `gui::Application` and overrides one method — `createInitialWindow()` — which returns our `MainWindow`:

```cpp
// include/Application.h
class Application : public gui::Application {
protected:
    gui::Window* createInitialWindow() override {
        return new MainWindow();
    }
public:
    Application(int argc, const char** argv)
        : gui::Application(argc, argv) {}
};
```

**Why this matters:** The framework controls the lifecycle. We never call `new MainWindow()` manually from `main()` — the framework calls `createInitialWindow()` at the right time after resources and translations are loaded.

---

## 2. Window — MainWindow

**File:** `include/MainWindow.h`

`MainWindow` extends `gui::Window` and configures the top-level window.

```cpp
class MainWindow : public gui::Window {
    ToolBar _toolBar;
    MainView _mainView;
public:
    MainWindow() : gui::Window(gui::Size(1500, 866)) {
        setTitle("Obiđi Jugu");
        setToolBar(_toolBar);
        setCentralView(&_mainView, Frame::FixSizes::FixMin);
    }
};
```

### Key decisions explained:

| Code | Why |
|------|-----|
| `gui::Size(1500, 866)` | Initial window dimensions — wide enough for map + side panel |
| `setToolBar(_toolBar)` | Attaches language-switching toolbar at top |
| `setCentralView(&_mainView, FixMin)` | **FixMin** locks the minimum window size to `MainView`'s declared size limits (700×600). The user can resize but never smaller |
| `shouldClose() → true` | Allows the window to close without confirmation |

### Toolbar Action Handling (Language Switch):

```cpp
bool onActionItem(gui::ActionItemDescriptor& aiDesc) override {
    auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
    if (menuID == 255) {
        const char* lang = nullptr;
        if (actionID == 10) lang = "EN";
        else if (actionID == 20) lang = "BA";
        if (lang) {
            auto appProperties = getAppProperties();
            appProperties->setValue("translation", lang);
            gui::getApplication()->restart();  // reload with new language
        }
    }
    return false;
}
```

**Framework concept:** `onActionItem` is the signal handler for toolbar/menu actions. The `menuID=255` convention is used by the natGUI framework for toolbar buttons. `getApplication()->restart()` reloads the entire app with the new translation — the preference survives because it was saved via `IAppProperties`.

---

## 3. Layout — SplitterLayout in MainView

**File:** `include/MainView.h`

`MainView` extends `gui::View` and uses a `gui::SplitterLayout` to divide the screen into two resizable panes:

```cpp
class MainView : public gui::View {
    gui::SplitterLayout _splitter{
        gui::SplitterLayout::Orientation::Horizontal,
        gui::SplitterLayout::AuxiliaryCell::Second
    };
    MapView _mapView;
    SidePanelScroller _sidePanelScroller;
    // ...
public:
    MainView() {
        setMargins(0, 0, 0, 0);
        _splitter.setContent(_mapView, _sidePanelScroller);
        setLayout(&_splitter);
    }
};
```

### Why SplitterLayout?

| Decision | Reason |
|----------|--------|
| `Orientation::Horizontal` | Map on the left, controls on the right — side by side |
| `AuxiliaryCell::Second` | The **right pane** (side panel) is collapsible. The user can drag to minimize it, giving more space to the map |
| `setMargins(0,0,0,0)` | No padding — the map fills edge to edge |

**The splitter makes the UI responsive:** when the window is wide, both panes have space. When narrow, the user can collapse the side panel. The map always fills the available height (see Canvas section).

---

## 4. View — SidePanelView with GridLayout

**Files:** `include/SidePanelView.h`, `src/SidePanelView.cpp`

`SidePanelView` is a `gui::View` using a `gui::GridLayout` with 37 rows × 5 columns, populated via `gui::GridComposer`:

```cpp
class SidePanelView : public gui::View {
    gui::GridLayout _gl;     // 37 rows, 5 columns
    // 40+ member controls: Labels, LineEdits, NumericEdits, ComboBoxes, Buttons, Sliders...
public:
    SidePanelView()
        : _gl(37, 5),
          lblAddSection(tr(">> Add point <<")),
          // ... 40+ initializers using tr() for i18n ...
    {
        gui::GridComposer gc(_gl);

        // Each appendRow starts a new row (auto-tracks position)
        gc.appendRow(lblAddSection, 0);
        gc.appendRow(lblName);   gc.appendCol(lnEditName, 0);
        gc.appendRow(lblXCoord); gc.appendCol(txtEditXCoord);
              gc.appendCol(lblYCoord); gc.appendCol(txtEditYCoord);
        gc.appendRow(btnAddPt, 0);
        // ... ~35 more rows of controls ...

        setLayout(&_gl);
    }
};
```

### Why GridLayout + GridComposer?

- **GridLayout(37, 5)** — the side panel has many rows of controls. A grid guarantees alignment across rows.
- **GridComposer** — avoids manually counting `insert(row, col, ctrl)` for 37 rows. `appendRow/appendCol` auto-advances position.
- **Column span** — `gc.appendRow(ctrl, 0)` with span `0` means "span to the end" (full-width headers/buttons).

### Control sections in the side panel:

| Section | Controls | Purpose |
|---------|----------|---------|
| Add Point | Name, X, Y inputs + button | Create new cities |
| Edit Point | ComboBox + fields + Update/Delete | Modify/remove selected city |
| Connections | Connected-to label, ComboBox, Toggle | Manage road connections |
| Randomize | 5 buttons in HorizontalLayout | Randomize positions/connections/weights/status |
| Solve | Algorithm ComboBox, params, Start/Step/Reset | Run TSP algorithms |

### Algorithm-specific fields visibility:

```cpp
void updateAlgorithmFieldsVisibility() {
    int selIdx = cmbSolvingAlgorithm.getSelectedIndex();
    bool isNN = (selIdx == 2);
    bool isSA = (selIdx == 3);
    bool isGA = (selIdx == 4);

    // Hide/show NN fields
    lblImprovementCycles.hide(!isNN, true);
    // Hide/show SA fields (temperature, cooling rate, etc.)
    // Hide/show GA fields (population, mutation, crossover, etc.)
}
```

Controls are dynamically hidden/shown based on the selected algorithm using `ctrl.hide(bool, recalcLayout)`.

---

## 5. ViewScroller — SidePanelScroller

**File:** `include/SidePanelScroller.h`

The side panel has many controls that may not fit in a small window. `SidePanelScroller` wraps it in a scrollable container:

```cpp
class SidePanelScroller : public gui::ViewScroller {
public:
    SidePanelView panel;
    SidePanelScroller()
        : gui::ViewScroller(Type::NoScroll, Type::ScrollAndAutoHide)
    {
        setContentView(&panel);
    }
};
```

| Parameter | Value | Why |
|-----------|-------|-----|
| Horizontal | `NoScroll` | Side panel width is managed by the splitter — no horizontal scroll needed |
| Vertical | `ScrollAndAutoHide` | Scrollbar appears only when the panel is taller than the visible area; hidden when there's enough space |

**In MainView**, the scroller is the right pane of the splitter. A reference alias provides direct access to the inner panel:

```cpp
SidePanelScroller _sidePanelScroller;
SidePanelView& _sidePanel = _sidePanelScroller.panel; // convenient alias
```

---

## 6. Canvas — MapView

**Files:** `include/MapView.h`, `src/MapView.cpp`

`MapView` is our custom 2D rendering surface — a `gui::Canvas` that draws the map, cities, roads, and algorithm visualization.

### Construction:

```cpp
MapView()
    : gui::Canvas({
        gui::InputDevice::Event::PrimaryClicks,
        gui::InputDevice::Event::SecondaryClicks,
        gui::InputDevice::Event::Zoom
      })
    , _bgImage(":bgMap")   // load background map from resources
{
    enableResizeEvent(true);   // REQUIRED for onResize to fire
    _bgLoaded = _bgImage.isOK();
}
```

| Setup | Purpose |
|-------|---------|
| `PrimaryClicks` | Left-click to select a city |
| `SecondaryClicks` | Right-click to toggle road connections |
| `Zoom` | Scroll wheel / pinch zoom (future use) |
| `enableResizeEvent(true)` | **Critical** — without this, `onResize()` is never called and the canvas can't track its dimensions |
| `gui::Image(":bgMap")` | Loads the map background via the resource system |

### Height-based scaling (onDraw):

```cpp
void onDraw(const gui::Rect& rect) override {
    float viewH = static_cast<float>(rect.bottom - rect.top);
    float scale  = viewH / ORIGINAL_MAP_HEIGHT;  // always fill height
    float mapW   = ORIGINAL_MAP_WIDTH * scale;
    float offsetX = rect.left + std::max(0.0f, (viewW - mapW) / 2.0f);

    // Draw background image
    gui::Rect imgRect(offsetX, 0, offsetX + mapW, viewH);
    _bgImage.draw(imgRect, gui::Image::AspectRatio::No);

    // Draw roads (using gui::Shape bezier paths)
    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(scaledLineWidth, td::LinePattern::Solid);
    for (each road) {
        bezier.moveTo(scaledPoint1);
        bezier.lineTo(scaledPoint2);
    }
    bezierShape.drawWire(td::ColorID::Black);

    // Draw algorithm state (tour edges in green)
    if (_solver) drawAlgorithmState();

    // Draw cities on top (colored circles with labels)
    for (const auto& city : cities) {
        MapPointRenderer::drawScaled(city, scale, offsetX, offsetY);
    }
}
```

**Why height-based scaling?** The map always fills the full canvas height. When the side panel expands (taking horizontal space), the map extends beyond the visible area to the right — the side panel visually covers the overflow. This prevents the map from shrinking when the panel opens.

### Click handling:

```cpp
void onPrimaryButtonPressed(const gui::InputDevice& inputDevice) override {
    int idx = hitTestCity(inputDevice.getFramePoint());
    if (idx >= 0 && _onPrimaryCityClick)
        _onPrimaryCityClick(idx);  // callback to MainView
}
```

`hitTestCity()` converts the click point to map coordinates using the current scale/offset and checks if it falls within any city's bounding box.

### Drawing primitives used:

| Framework API | Usage in MapView |
|---------------|-----------------|
| `gui::Image::draw(rect)` | Background map |
| `gui::Shape::createBezier()` | Road connections (lines), tour edges |
| `gui::Shape::createCircle()` | City markers |
| `gui::DrawableString::draw()` | City names, warning messages |
| `gui::Shape::drawFillAndWire()` | Filled circles for cities |
| `gui::Shape::drawWire()` | Road lines |

---

## 7. Resource System — DevRes.xml & main.xml

**Files:** `res/DevRes.xml`, `res/main.xml`

### DevRes.xml — Master configuration:

```xml
<DevRes regionals="Work/Common/Regionals">
    <Langs>
        <Lang id="EN" name="English"/>
        <Lang id="BA" name="Bosanski"/>
    </Langs>
    <Config path="Work/Common/ResConfigs/natGUI.xml"/>
    <ArtWork>
        <Config path=":main.xml"/>           <!-- our app's resources -->
    </ArtWork>
    <Translation>
        <Config path="Work/Common/ResConfigs/tr/%s/natGUI.xml"/>
        <Config path=":tr/%s/main.xml"/>     <!-- our app's translations -->
    </Translation>
</DevRes>
```

### main.xml — App resource declarations:

```xml
<DevRes>
    <Images>
        <Res id="flagEN" path=":flag-us.png"/>
        <Res id="flagBA" path=":flag-bhs.png"/>
        <Res id="bgMap" path=":assets/yugoslavia.png"/>
    </Images>
    <FileNames>
        <Res id="exYu" path=":exYu.json"/>
    </FileNames>
</DevRes>
```

### How resources are used in code:

| Resource ID | Declared In | Used By | Code |
|-------------|-------------|---------|------|
| `flagEN` | `<Images>` | `ToolBar.h` | `gui::Image _imgEN(":flagEN")` |
| `flagBA` | `<Images>` | `ToolBar.h` | `gui::Image _imgBA(":flagBA")` |
| `bgMap` | `<Images>` | `MapView.h` | `gui::Image _bgImage(":bgMap")` |
| `exYu` | `<FileNames>` | `MainView.h` | `td::String path = getResFileName(":exYu")` |

### The `:` prefix explained:

- `:flag-us.png` → resolved to `<projectRoot>/res/flag-us.png`
- `:assets/yugoslavia.png` → resolved to `<projectRoot>/res/assets/yugoslavia.png`
- `:exYu.json` → resolved to `<projectRoot>/res/exYu.json`

The path resolution is handled by the framework using the `-devResPath` argument (set by CMake via `setIDEPropertiesForGUIExecutable`). This makes resource loading **build-location-independent** — it doesn't matter where the build folder is.

---

## 8. Translation / i18n

**Files:** `res/tr/EN/main.xml`, `res/tr/BA/main.xml`

### Translation files:

```xml
<!-- res/tr/EN/main.xml -->
<Translations>
    <Res id="Algorithm:" tr="Algorithm:"/>
    <Res id="Start/Pause" tr="Start/Pause"/>
    <Res id="Add Point" tr="Add Point"/>
</Translations>

<!-- res/tr/BA/main.xml -->
<Translations>
    <Res id="Algorithm:" tr="Algoritam:"/>
    <Res id="Start/Pause" tr="Start/Pauza"/>
    <Res id="Add Point" tr="Dodaj tačku"/>
</Translations>
```

### Usage — `tr()` function:

Every `gui::NatObject` subclass has access to `tr()`:

```cpp
gui::Label lblAlgorithm(tr("Algorithm:"));   // → "Algorithm:" or "Algoritam:"
gui::Button btnAddPt(tr("Add Point"));       // → "Add Point" or "Dodaj tačku"
```

### Language selection flow:

1. **On startup:** `main.cpp` reads `IAppProperties → "translation"` (default `"EN"`) → calls `app.init(trLang)`
2. **User clicks toolbar flag:** `MainWindow::onActionItem` → saves new language to `IAppProperties` → calls `gui::getApplication()->restart()`
3. **On restart:** `main.cpp` reads the updated property → initializes with new language

---

## 9. Persistent Settings — IAppProperties

**Used in:** `src/main.cpp`, `include/MainWindow.h`

`IAppProperties` provides OS-native persistent storage:
- **Windows:** Registry
- **macOS:** `.plist` file (Application Support)
- **Linux:** Settings/config file

```cpp
// Reading (main.cpp):
auto props = app.getProperties();
td::String lang = props->getValue("translation", "EN");

// Writing (MainWindow.h):
auto props = getAppProperties();
props->setValue("translation", "BA");
```

**Why IAppProperties instead of a file?** We originally used a `lang.cfg` file, but it broke on macOS because the current working directory differs when launching from Finder vs. terminal. `IAppProperties` is the framework's cross-platform solution — works everywhere without path issues.

---

## 10. Data Layer — Repository & JSON

**Files:** `include/DataRepository.h`, `src/DataRepository.cpp`, `include/JsonService.h`, `src/JsonService.cpp`

### DataRepository (Repository Pattern):

```cpp
class DataRepository {
    std::vector<CityPoint> _cities;
    std::vector<RoadEdge> _roads;
    ChangeCallback _onDataChanged;
    // ...
public:
    void init(const std::filesystem::path& sourceJsonPath);
    // City CRUD
    bool addCity(const std::string& name, double x, double y);
    bool updateCity(int index, ...);
    bool deleteCity(int index);
    // Road CRUD
    bool addConnection(int fromIndex, int toIndex);
    bool removeConnection(int fromIndex, int toIndex);
    // Metric closure (all-pairs shortest paths)
    double getMetricDistance(int i, int j) const;
};
```

### Key design decisions:

| Decision | Implementation | Why |
|----------|---------------|-----|
| **Session-only edits** | `save()` is a no-op | Edits stay in memory; restarting reloads pristine source data |
| **Change notifications** | `_onDataChanged` callback | MapView redraws automatically when data changes |
| **Metric closure** | Floyd-Warshall all-pairs shortest paths | TSP needs distances between any two cities via road network |
| **Cross-platform paths** | `std::filesystem::path` | Works on Windows, macOS, Linux |

### Data flow:

```
res/exYu.json  →  JsonService::loadFromJson()  →  DataRepository._cities / _roads
                                                         ↓
                                          recomputeMetricClosure()
                                                         ↓
                                          _metricDist[][] / _metricPrev[][]
```

### Data types (`include/DataTypes.h`):

```cpp
enum class VisitationStatus : int {
    Blocked = 0,   // excluded from TSP
    Open = 1,      // optional transit node
    Goal = 2,      // MUST visit
    Start = 3      // tour anchor
};

struct CityPoint {
    int id; double x, y, weight;
    std::string name;
    VisitationStatus visitation_status;
};

struct RoadEdge { int fromId, toId; };
```

---

## 11. Algorithm Architecture

**Files:** `include/SearchAlgorithm.h`, `include/TSPAlgorithm.h`, `include/NearestNeighborAlgorithm.h`, `include/SimulatedAnnealingAlgorithm.h`, `include/GeneticAlgorithmTSP.h`, `include/BFSAlgorithm.h`, `include/DFSAlgorithm.h`

### Class hierarchy:

```
SearchAlgorithm          (abstract: step/stepBack, frontier-based graph search)
  ├── BFSAlgorithm       (breadth-first: Start→Goal pathfinding)
  ├── DFSAlgorithm       (depth-first: Start→Goal pathfinding)
  └── TSPAlgorithm       (abstract: tour optimization over required cities)
        ├── NearestNeighborAlgorithm   (greedy + optional 2-opt)
        ├── SimulatedAnnealingAlgorithm (metaheuristic, temperature-based)
        └── GeneticAlgorithmTSP        (evolutionary, population-based)
```

### SearchAlgorithm base:

```cpp
class SearchAlgorithm {
public:
    virtual void reset(DataRepository* repo);  // initialize with data
    virtual bool step();                        // execute one algorithm step
    virtual bool stepBack();                    // undo one step (history-based)
    bool isFinished() const;
    const std::vector<int>& getPath() const;   // result path
protected:
    AlgorithmState _state;
    std::vector<AlgorithmState> _history;       // for step-back
    virtual int getNextFromFrontier() = 0;      // BFS=queue, DFS=stack
    virtual void expandFrontier(int node) = 0;
};
```

### TSPAlgorithm layer:

```cpp
class TSPAlgorithm : public SearchAlgorithm {
protected:
    TSPState _tspState;                         // tour, bestTour, lengths
    std::vector<TSPSnapshot> _tspHistory;       // for step-back with subclass state
    std::vector<int> _requiredCities;           // Goal + Start cities
    virtual void initializeTSP() = 0;           // subclass-specific init
    virtual bool stepTSP() = 0;                 // subclass-specific step
    double calculateTourLength(const std::vector<int>& tour); // uses metric closure
    std::any saveSubclassState() const;          // snapshot for undo
    void restoreSubclassState(const std::any&);
};
```

### Algorithm-specific logic:

| Algorithm | `stepTSP()` does | Key parameters |
|-----------|------------------|----------------|
| **Nearest Neighbor** | Picks closest unvisited city, adds to tour. After construction: optional 2-opt improvement cycles | `enable2Opt`, `improvementCycles` |
| **Simulated Annealing** | Randomly swaps two cities in tour. Accepts worse solutions with probability $e^{-\Delta/T}$. Cools temperature each iteration | `initialTemp`, `minTemp`, `coolingRate`, `iterationsPerTemperature`, `initialSolutionMode` |
| **Genetic Algorithm** | Evolves a population of tours via selection, crossover, mutation. One generation per step | `populationSize`, `mutationRate`, `maxGenerations`, `crossoverRate`, `selectionMethod`, `mutationOperator`, `elitismPercent` |

### State snapshot for step-back:

Each algorithm uses `std::any` for subclass-specific snapshots:
- **NN:** `tuple<set<int>, int, int>` — unvisited set, current city, remaining 2-opt cycles
- **SA:** `pair<double, int>` — temperature, iterations at current temp
- **GA:** `struct{int generation, vector<vector<int>> population, vector<double> fitness}`

---

## 12. Solver Orchestration in MainView

**File:** `include/MainView.h`

`MainView` is the **controller** that wires everything together. It owns the `DataRepository`, creates/destroys solvers, and drives the animation timer.

### Solver creation:

```cpp
void selectAlgorithm(int algorithmIdx) {
    _currentAlgorithmIdx = algorithmIdx;
    switch (algorithmIdx) {
        case 0: _solver = std::make_unique<BFSAlgorithm>(); break;
        case 1: _solver = std::make_unique<DFSAlgorithm>(); break;
        case 2: _solver = std::make_unique<NearestNeighborAlgorithm>(
                    _sidePanel.is2OptEnabled(),
                    _sidePanel.getImprovementCycles()); break;
        case 3: _solver = std::make_unique<SimulatedAnnealingAlgorithm>(
                    _sidePanel.getInitialTemperature(), ...); break;
        case 4: _solver = std::make_unique<GeneticAlgorithmTSP>(
                    _sidePanel.getPopulationSize(), ...); break;
    }
    _solver->reset(&_repo);
    _mapView.setSolver(_solver.get());
}
```

**Every time Start/Reset/Step is pressed**, the solver is recreated with fresh parameters from the side panel. This ensures all control values take effect.

### Timer-driven execution:

```cpp
gui::Timer _timer;               // fires at SOLVER_STEP_INTERVAL
gui::Timer _solutionTimer;       // fires at SOLUTION_ANIM_INTERVAL

// Start button: begin auto-stepping
_timer.start();
_running = true;

// Timer callback:
bool onTimer(gui::Timer* pTimer) override {
    if (!_solver->isFinished()) {
        _solver->step();          // advance algorithm
        _mapView.refresh();       // redraw canvas
        _timer.start();           // schedule next tick
    }
}
```

### Action handlers:

| Action | What happens |
|--------|-------------|
| **Start/Pause** | Creates fresh solver with current params → starts timer → locks SA/GA controls |
| **Step Forward** | Creates fresh solver if needed → calls `step()` once → refreshes canvas |
| **Step Back** | Calls `stepBack()` → refreshes canvas |
| **Reset** | Stops timer → creates fresh solver → refreshes canvas |
| **Show Solution** | Runs solver to completion → starts edge-by-edge animation |

---

## 13. Responsive Design & Sizing

### Size limits chain:

```
MainView (700×600 min)
  ├── MapView (400 min width) ← fills height via scale
  └── SidePanelScroller (300×200 min)
        └── SidePanelView (scrollable content)

MainWindow: setCentralView(&_mainView, FixSizes::FixMin)
  → Window cannot resize below 700×600
```

### How it works:

1. **MainView** declares `setSizeLimits(700, UseAsMin, 600, UseAsMin)`
2. **MainWindow** uses `FixSizes::FixMin` → the window enforces this minimum
3. **SplitterLayout** divides space between MapView and SidePanelScroller
4. **MapView** always scales to fill height: `scale = viewH / ORIGINAL_MAP_HEIGHT`
5. **SidePanelScroller** shows vertical scrollbar when panel content exceeds visible height

### Why height-based scaling?

The map image has fixed original dimensions. If we scaled by `min(scaleX, scaleY)`, resizing the window would shrink the map height when the side panel takes horizontal space. Instead, we always fill the height and let the map extend rightward — the side panel visually covers the overflow area.

---

## 14. CMake Build Configuration

**Files:** `CMakeLists.txt`, `TTS.cmake`

### CMakeLists.txt — Project entry point:

```cmake
cmake_minimum_required(VERSION 3.18)
set(SOLUTION_NAME The_Travelling_Salesman)
project(${SOLUTION_NAME})

# Resolve paths
set(HOME_ROOT $ENV{HOME})
if (WIN32)
    string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()
set(WORK_ROOT ${HOME_ROOT}/Work)

# Include framework layers
include(${WORK_ROOT}/DevEnv/Common.cmake)    # C++20, output paths, utilities
include(${WORK_ROOT}/DevEnv/natGUI.cmake)    # natGUI library paths
include(TTS.cmake)                           # our project config
```

### TTS.cmake — Project-specific build:

```cmake
set(PROJECT_NAME The_Travelling_Salesman)

# Gather all source and header files
file(GLOB PROJECT_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB PROJECT_INCS     ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
set(PROJECT_PLIST           ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)
file(GLOB PROJECT_INC_TD   ${MY_INC}/td/*.h)
file(GLOB PROJECT_INC_GUI  ${MY_INC}/gui/*.h)

# Create executable with all sources + framework headers (for IDE navigation)
add_executable(${PROJECT_NAME} ${PROJECT_INCS} ${PROJECT_SOURCES}
               ${PROJECT_INC_TD} ${PROJECT_INC_GUI})

# Include path for our headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include)

# IDE source groups (Visual Studio filters / Xcode groups)
source_group("inc"         FILES ${PROJECT_INCS})
source_group("inc\\td"     FILES ${PROJECT_INC_TD})
source_group("inc\\gui"    FILES ${PROJECT_INC_GUI})
source_group("src"         FILES ${PROJECT_SOURCES})

# Link: debug and release variants of both libraries
target_link_libraries(${PROJECT_NAME}
    debug     ${MU_LIB_DEBUG}     debug     ${NATGUI_LIB_DEBUG}
    optimized ${MU_LIB_RELEASE}   optimized ${NATGUI_LIB_RELEASE})

# Platform-specific setup
setTargetPropertiesForGUIApp(${PROJECT_NAME} ${PROJECT_PLIST})
setIDEPropertiesForGUIExecutable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${PROJECT_NAME})
```

### What each function does:

| Function | Windows | macOS |
|----------|---------|-------|
| `setTargetPropertiesForGUIApp` | `WIN32_EXECUTABLE TRUE` (no console) | `MACOSX_BUNDLE TRUE` + sets Info.plist |
| `setIDEPropertiesForGUIExecutable` | Sets VS debugger args: `-devResPath=<path>` | Sets Xcode scheme: `-devResPath`, `DYLD_LIBRARY_PATH` |
| `setPlatformDLLPath` | Adds `~/other_bin` to VS debugger PATH | No-op |

---

## 15. Cross-Platform Considerations

| Concern | Solution |
|---------|----------|
| **Debug output** | `fprintf(stderr, ...)` — no Windows-specific APIs |
| **String formatting** | `snprintf()` — not `sprintf_s` (MSVC-only) |
| **File paths** | `std::filesystem::path` everywhere |
| **Language persistence** | `IAppProperties` — not file-based (CWD varies on macOS) |
| **Resource loading** | natGUI resource system with `:` prefix — not hardcoded paths |
| **GUI app bundle** | CMake handles `WIN32_EXECUTABLE` / `MACOSX_BUNDLE` automatically |
| **Library linking** | CMake variables `MU_LIB_DEBUG/RELEASE`, `NATGUI_LIB_DEBUG/RELEASE` resolve per platform |

---

## 16. Architecture Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                      Application (main.cpp)                   │
│  IAppProperties → language → app.init(lang) → app.run()      │
└──────────────────────────┬───────────────────────────────────┘
                           │ createInitialWindow()
┌──────────────────────────▼───────────────────────────────────┐
│                      MainWindow                               │
│  gui::Window(1500×866) + ToolBar + setCentralView(FixMin)    │
│  onActionItem → language switch → IAppProperties + restart    │
└──────────────────────────┬───────────────────────────────────┘
                           │ centralView
┌──────────────────────────▼───────────────────────────────────┐
│                       MainView                                │
│  gui::View + SplitterLayout(Horizontal, AuxSecond)           │
│  Owns: DataRepository, unique_ptr<SearchAlgorithm>, Timers   │
│  Orchestrates: solver creation, timer stepping, animations    │
├─────────────────────┬────────────────────────────────────────┤
│     MapView         │        SidePanelScroller               │
│  gui::Canvas        │  gui::ViewScroller(NoH, AutoV)         │
│  onDraw: map bg,    │        ┌──────────────────┐            │
│    roads, cities,   │        │ SidePanelView    │            │
│    tour edges       │        │ gui::View        │            │
│  onClick: select    │        │ GridLayout(37×5) │            │
│  Height-based scale │        │ + GridComposer   │            │
│                     │        │ 40+ controls     │            │
└─────────────────────┴────────┴──────────────────┴────────────┘
                           │ uses
┌──────────────────────────▼───────────────────────────────────┐
│                    DataRepository                             │
│  Session-only edits (save = no-op)                           │
│  Floyd-Warshall metric closure                               │
│  CRUD: cities, roads                                         │
│  ChangeCallback → MapView.reDraw()                           │
└──────────────────────────┬───────────────────────────────────┘
                           │ loaded by
┌──────────────────────────▼───────────────────────────────────┐
│                    JsonService                                │
│  Static load/save for exYu.json                              │
│  std::filesystem, snprintf, fprintf(stderr)                  │
└──────────────────────────────────────────────────────────────┘

Algorithm Hierarchy:
  SearchAlgorithm (step/stepBack/frontier)
    ├── BFS (queue-based)
    ├── DFS (stack-based)
    └── TSPAlgorithm (tour optimization + metric closure)
          ├── NearestNeighbor (greedy + 2-opt)
          ├── SimulatedAnnealing (temperature + random swaps)
          └── GeneticAlgorithm (population + crossover + mutation)
```
