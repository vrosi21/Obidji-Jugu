# Architecture Documentation

This document describes the architecture of the Obiđi Jugu (Travelling Salesman) application, including all classes, their purposes, and method descriptions.

---

## Table of Contents

1. [Overview](#overview)
2. [Data Layer](#data-layer)
   - [DataTypes](#datatypes)
   - [DataRepository](#datarepository)
   - [JsonService](#jsonservice)
3. [Rendering Layer](#rendering-layer)
   - [MapPointStyle](#mappointstyle)
   - [MapPointRenderer](#mappointrenderer)
   - [MapView](#mapview)
4. [UI Layer](#ui-layer)
   - [SidePanelView](#sidepanelview)
   - [ToolBar](#toolbar)
5. [Application Layer](#application-layer)
   - [MainView](#mainview)
   - [MainWindow](#mainwindow)
   - [Application](#application)

---

## Overview

The application follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (Application, MainWindow, MainView)                        │
├─────────────────────────────────────────────────────────────┤
│                       UI Layer                              │
│  (SidePanelView, ToolBar)                                   │
├─────────────────────────────────────────────────────────────┤
│                    Rendering Layer                          │
│  (MapView, MapPointStyle, MapPointRenderer)                 │
├─────────────────────────────────────────────────────────────┤
│                      Data Layer                             │
│  (DataRepository, JsonService, DataTypes)                   │
└─────────────────────────────────────────────────────────────┘
```

**Key Design Patterns:**
- **Repository Pattern**: `DataRepository` centralizes all data access and persistence
- **Observer Pattern**: Components subscribe to data changes via callbacks
- **Service Pattern**: `JsonService` provides stateless I/O operations

---

## Data Layer

### DataTypes

**File:** `include/DataTypes.h`

**Purpose:** Defines core domain types used throughout the application.

#### Enum: `VisitationStatus`

Represents the visitation state of a city point.

| Value | Description |
|-------|-------------|
| `Blocked = 0` | City cannot be visited |
| `Open = 1` | City is available for visiting |
| `Goal = 2` | City is a destination/goal |
| `Start = 3` | City is the starting point (only one allowed) |

#### Struct: `CityPoint`

Represents a city/point on the map.

| Field | Type | Description |
|-------|------|-------------|
| `id` | `int` | Unique identifier (-1 if unset) |
| `x` | `double` | X coordinate on map |
| `y` | `double` | Y coordinate on map |
| `weight` | `double` | Weight/cost value for algorithms |
| `name` | `std::string` | Display name of the city |
| `visitation_status` | `VisitationStatus` | Current visitation state |

#### Struct: `RoadEdge`

Lightweight road connection (for rendering).

| Field | Type | Description |
|-------|------|-------------|
| `fromId` | `int` | Source city ID |
| `toId` | `int` | Destination city ID |

#### Struct: `RoadInfo`

Full road information (for persistence and algorithms).

| Field | Type | Description |
|-------|------|-------------|
| `fromId` | `int` | Source city ID |
| `toId` | `int` | Destination city ID |
| `length` | `double` | Road length |
| `travelTimeH` | `double` | Travel time in hours |
| `type` | `std::string` | Road type (e.g., "local", "highway") |
| `bidirectional` | `bool` | Whether road can be traversed both ways |

---

### DataRepository

**File:** `include/DataRepository.h`, `src/DataRepository.cpp`

**Purpose:** Central data store implementing the Repository pattern. Manages all city and road data with automatic JSON persistence and change notifications.

#### Type Aliases

| Name | Type | Description |
|------|------|-------------|
| `ChangeCallback` | `std::function<void()>` | Callback for data change notifications |

#### Constructor/Destructor

| Method | Description |
|--------|-------------|
| `DataRepository()` | Initializes repository and loads data from JSON |
| `~DataRepository()` | Default destructor |

#### City CRUD Operations

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `addCity` | `name`, `x`, `y` | `bool` | Adds a new city with given name and coordinates. Returns false if name is empty or save fails. |
| `updateCity` | `index`, `name`, `x`, `y`, `weight` | `bool` | Updates city at index with new values. Returns false if index invalid or save fails. |
| `updateCityStatus` | `index`, `status` | `bool` | Updates visitation status of city at index. Returns false if index invalid or save fails. |
| `deleteCity` | `index` | `bool` | Deletes city at index and reindexes remaining cities. Also removes associated roads. Returns false if index invalid or save fails. |

#### City Queries

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `getCity` | `index`, `out` | `bool` | Retrieves city data at index into `out` parameter. Returns false if index invalid. |
| `getCityNames` | — | `std::vector<std::string>` | Returns list of all city names in order. |
| `cityCount` | — | `size_t` | Returns total number of cities. |

#### Road CRUD Operations

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `addConnection` | `fromIndex`, `toIndex` | `bool` | Creates a road between two cities. Calculates length automatically. Returns false if indices invalid or save fails. |
| `removeConnection` | `fromIndex`, `toIndex` | `bool` | Removes road between two cities. Returns false if indices invalid or save fails. |

#### Road Queries

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `hasConnection` | `fromIndex`, `toIndex` | `bool` | Checks if a road exists between two cities (either direction). |
| `getConnections` | `index` | `std::vector<int>` | Returns indices of all cities connected to the given city. |
| `getConnectionNames` | `index` | `std::vector<std::string>` | Returns names of all cities connected to the given city. |

#### Direct Access (Read-Only)

| Method | Returns | Description |
|--------|---------|-------------|
| `cities` | `const std::vector<CityPoint>&` | Direct read-only access to city data for rendering. |
| `roads` | `const std::vector<RoadEdge>&` | Direct read-only access to road data for rendering. |

#### Change Notifications

| Method | Parameters | Description |
|--------|------------|-------------|
| `setOnDataChanged` | `callback` | Registers a callback to be invoked when data changes. |

#### Private Methods

| Method | Description |
|--------|-------------|
| `load()` | Loads data from JSON file on construction. |
| `save()` | Persists current data to JSON file. Called after every modification. |
| `nextCityId()` | Calculates the next available city ID. |
| `isValidIndex(index)` | Validates that an index is within bounds. |
| `notifyChange()` | Invokes the registered change callback if set. |

---

### JsonService

**File:** `include/JsonService.h`, `src/JsonService.cpp`

**Purpose:** Stateless service class providing JSON file I/O operations. All methods are static; the class cannot be instantiated.

#### Public Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `loadFromJson` | `jsonPath`, `outCities`, `outRoads`, `outRoadsFull` | `bool` | Loads and parses JSON file, populating output vectors. Returns false on failure. |
| `saveToJson` | `jsonPath`, `cities`, `roadsFull` | `bool` | Writes cities and roads to JSON file. Returns false on failure. |
| `findJsonFile` | — | `std::filesystem::path` | Searches for JSON data file in standard locations (relative to cwd and exe directory). |

#### Private Helper Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `readFileContent` | `path` | `std::string` | Reads entire file content as string. |
| `extractNumber` | `obj`, `key` | `double` | Extracts numeric value from JSON object string. |
| `extractInt` | `obj`, `key` | `int` | Extracts integer value from JSON object string. |
| `extractString` | `obj`, `key` | `std::string` | Extracts string value from JSON object string. |
| `extractBool` | `obj`, `key` | `bool` | Extracts boolean value from JSON object string. |

---

## Rendering Layer

### MapPointStyle

**File:** `include/MapPoint.h`, `src/MapPoint.cpp`

**Purpose:** Configuration and utility struct for map point visual styling.

#### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `Size` | `10` | Inner rectangle size in pixels |
| `BorderOffset` | `3` | Border extends this many pixels beyond inner rect |

#### Static Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `getColors` | `status`, `borderColor`, `centerColor` | `void` | Sets border and center colors based on visitation status. Colors: Blocked=Red/Yellow, Open=Yellow/Yellow, Goal=Yellow/Violet, Start=Yellow/Cyan |
| `getCenter` | `x`, `y` | `std::pair<double, double>` | Calculates center point of a map point for road connection drawing. |

---

### MapPointRenderer

**File:** `include/MapPoint.h`, `src/MapPoint.cpp`

**Purpose:** Renders individual map points (cities) on the canvas.

#### Static Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `draw` | `mapPoint` | `void` | Draws a complete map point with border rectangle, center rectangle, and name label. Uses `MapPointStyle` for sizing and colors. |

---

### MapView

**File:** `include/MapView.h`, `src/MapView.cpp`

**Purpose:** Canvas view responsible for rendering the map, cities, and road connections. Contains no data—delegates all data operations to `DataRepository`.

#### Constructor/Destructor

| Method | Description |
|--------|-------------|
| `MapView()` | Initializes canvas and loads background image. |
| `~MapView()` | Default destructor. |

#### Public Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `setRepository` | `repo` | `void` | Attaches a repository and subscribes to change notifications for automatic redraw. |
| `repository` | — | `DataRepository*` | Returns pointer to attached repository (for queries). |

#### Protected Methods

| Method | Parameters | Description |
|--------|------------|-------------|
| `onDraw` | `rect` | Renders background image, road connection lines, and all city points. |

#### Private Methods

| Method | Description |
|--------|-------------|
| `loadBackground()` | Attempts to load background map image from standard locations. |

---

## UI Layer

### SidePanelView

**File:** `include/SidePanelView.h`, `src/SidePanelView.cpp`

**Purpose:** Side panel containing all UI controls for CRUD operations on cities and roads, status management, and algorithm selection.

#### UI Controls (Public Members)

| Control | Type | Description |
|---------|------|-------------|
| `lblXCoord` / `txtEditXCoord` | Label / NumericEdit | X coordinate input for new points |
| `lblYCoord` / `txtEditYCoord` | Label / NumericEdit | Y coordinate input for new points |
| `lblName` / `lnEditName` | Label / LineEdit | Name input for new points |
| `btnAddPt` | Button | Adds a new point with entered values |
| `lblChoosePoint` / `cmbPoints` | Label / ComboBox | Dropdown to select existing point |
| `lblCurrentCityName` / `lnEditCurrentCityName` | Label / LineEdit | Current point name (editable) |
| `lblCurrentX` / `neCurrentX` | Label / NumericEdit | Current point X coordinate (editable) |
| `lblCurrentY` / `neCurrentY` | Label / NumericEdit | Current point Y coordinate (editable) |
| `lblCurrentWeight` / `neCurrentWeight` | Label / NumericEdit | Current point weight (editable) |
| `btnUpdatePoint` | Button | Updates selected point with edited values |
| `btnDeletePoint` | Button | Deletes selected point |
| `lblConnections` / `lblConnectionsValue` | Labels | Displays current point's connections |
| `lblConnectTo` / `cmbConnectTo` | Label / ComboBox | Dropdown to select connection target |
| `btnToggleConnection` | Button | Adds or removes connection to selected target |
| `lblStatus` / `cmbStatus` | Label / ComboBox | Visitation status dropdown (Blocked/Open/Goal/Start) |
| `lblSolvingSection` | Label | Section header for algorithm controls |
| `cmbSolvingAlgorithm` | ComboBox | Algorithm selection dropdown |
| `btnStartPause` / `btnStepFwd` / `btnStepBwd` | Buttons | Algorithm playback controls |
| `gl` | GridLayout | Layout manager for all controls |

#### Constructor/Destructor

| Method | Description |
|--------|-------------|
| `SidePanelView()` | Initializes all controls and arranges them in grid layout. |
| `~SidePanelView()` | Default destructor. |

#### Public Methods

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `populatePointNames` | `names` | `void` | Fills point dropdown with city names. |
| `populateSolvingAlgorithms` | `names` | `void` | Fills algorithm dropdown with algorithm names. |
| `setRepository` | `repo` | `void` | Connects panel to data repository for operations. |
| `syncSelectionDetails` | — | `void` | Synchronizes all fields with currently selected point. |

#### Protected Methods (Event Handlers)

| Method | Parameters | Returns | Description |
|--------|------------|---------|-------------|
| `onChangedSelection` | `pCB` | `bool` | Handles dropdown selection changes. |
| `onClick` | `pBtn` | `bool` | Handles button clicks. |

#### Private Methods

| Method | Description |
|--------|-------------|
| `handleAddPoint()` | Creates new point from input fields. |
| `handleUpdatePoint()` | Updates selected point with current field values. |
| `handleDeletePoint()` | Deletes currently selected point. |
| `handleToggleConnection()` | Adds or removes road to selected target point. |
| `handleStatusChange()` | Updates visitation status. Ensures only one Start exists. |
| `selectIndexAndUpdate(idx)` | Selects point at index and refreshes all related fields. |
| `updateCurrentNameFromSelection()` | Updates name field from selected point. |
| `updateCurrentCoordsFromSelection()` | Updates coordinate and weight fields from selected point. |
| `updateConnectionsLabel()` | Updates connections display label. |
| `populateConnectToCombo()` | Fills connection target dropdown (excludes current point). |
| `populateStatusCombo()` | Fills status dropdown with all visitation statuses. |
| `updateStatusFromSelection()` | Sets status dropdown to match selected point's status. |

---

### ToolBar

**File:** `include/ToolBar.h`

**Purpose:** Application toolbar with language selection buttons.

#### Members

| Member | Type | Description |
|--------|------|-------------|
| `_imgEN` | gui::Image | English flag icon |
| `_imgBA` | gui::Image | Bosnian flag icon |

#### Constructor

| Method | Description |
|--------|-------------|
| `ToolBar()` | Creates toolbar with EN and BA language buttons. Uses menuID=255, actionIDs 10 (EN) and 20 (BA). |

---

## Application Layer

### MainView

**File:** `include/MainView.h`

**Purpose:** Main content view that composes the map view and side panel, and owns the central `DataRepository` instance.

#### Members

| Member | Type | Description |
|--------|------|-------------|
| `_hlayout` | gui::HorizontalLayout | Layout arranging map and panel side by side |
| `_repo` | DataRepository | Central data store (owned by MainView) |
| `_mapView` | MapView | Map rendering canvas |
| `_sidePanel` | SidePanelView | CRUD controls panel |

#### Constructor

| Method | Description |
|--------|-------------|
| `MainView()` | Initializes layout, sets size limits, arranges views, and wires repository to both MapView and SidePanelView. |

---

### MainWindow

**File:** `include/MainWindow.h`

**Purpose:** Top-level application window containing toolbar and main view.

#### Members

| Member | Type | Description |
|--------|------|-------------|
| `_toolBar` | ToolBar | Language selection toolbar |
| `_mainView` | MainView | Main content area |
| `windowWidth` / `windowHeight` | int | Window dimensions |

#### Constructor/Destructor

| Method | Description |
|--------|-------------|
| `MainWindow()` | Creates 1500x866 window with title "Obiđi Jugu", sets toolbar and central view. |
| `~MainWindow()` | Default destructor. |

#### Protected Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `shouldClose()` | `bool` | Always returns true to allow window closing. |
| `onActionItem(aiDesc)` | `bool` | Handles toolbar actions for language switching (EN/BA). Persists choice to `lang.cfg` and restarts app. |

---

### Application

**File:** `include/Application.h`

**Purpose:** Application entry point wrapper. Creates the initial window.

#### Constructor

| Method | Parameters | Description |
|--------|------------|-------------|
| `Application` | `argc`, `argv` | Initializes GUI application with command-line arguments. |

#### Protected Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `createInitialWindow()` | `gui::Window*` | Creates and returns new `MainWindow` instance. |

---

## Data Flow Diagram

```
┌─────────────┐     changes      ┌──────────────┐
│ SidePanelView│ ──────────────> │ DataRepository │
│  (UI CRUD)   │ <────────────── │  (Data Store) │
└─────────────┘    callback      └──────────────┘
                                        │
                                        │ callback
                                        ▼
                                 ┌─────────────┐
                                 │   MapView   │
                                 │ (Rendering) │
                                 └─────────────┘
                                        │
                                        │ uses
                                        ▼
                               ┌────────────────┐
                               │ MapPointRenderer│
                               │ MapPointStyle  │
                               └────────────────┘
```

1. User interacts with **SidePanelView** controls
2. SidePanelView calls **DataRepository** methods (add/update/delete)
3. DataRepository persists changes via **JsonService** and notifies listeners
4. **MapView** receives change notification and redraws
5. MapView uses **MapPointRenderer** and **MapPointStyle** for city rendering
