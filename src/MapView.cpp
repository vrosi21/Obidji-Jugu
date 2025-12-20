#include "MapView.h"
#include "JsonService.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <set>
#include <windows.h>

namespace {
    void dbg(const char* m) { OutputDebugStringA(m); }
}

MapView::MapView()
{
    loadCities();
    loadBackground();
}

void MapView::loadCities()
{
    if (_loaded) return;
    
    _jsonPath = JsonService::findJsonFile();
    bool success = JsonService::loadFromJson(_jsonPath, _cities, _roads, _roadsFull);
    
    if (!success) {
        dbg("[MapView] Failed to load cities from JSON\n");
    }
    
    _loaded = true;
}

std::vector<std::string> MapView::getCityNames() const
{
    std::vector<std::string> names;
    names.reserve(_cities.size());
    for (const auto& c : _cities) names.push_back(c.name);
    return names;
}

td::ColorID MapView::mapHexToColor(const std::string& hex) const
{
    if (hex == "#d62728") return td::ColorID::Red;
    if (hex == "#1f77b4") return td::ColorID::Blue;
    if (hex == "#ff7f0e") return td::ColorID::Orange;
    if (hex == "#2ca02c") return td::ColorID::Green;
    if (hex == "#9467bd") return td::ColorID::Magenta;
    if (hex == "#8c564b") return td::ColorID::SandyBrown;
    if (hex == "#e377c2") return td::ColorID::Pink;
    if (hex == "#e63946") return td::ColorID::Red;
    if (hex == "#f4f0bb") return td::ColorID::LightYellow;
    if (hex == "#8338ec") return td::ColorID::Violet;
    return td::ColorID::Gray;
}

void MapView::onDraw(const gui::Rect& rect)
{
    // Background: draw image with desired scaling (1000 x 866.3) if available, else white fill
    if (_bgLoaded && _bgImage.isOK()) {
        const float targetW = 1000.0f;
        const float srcW = 860.0f;
        const float srcH = 745.0f;
        const float targetH = targetW * (srcH / srcW); // 1000 * 745/860 ≈ 866.279
        gui::Rect imgRect(rect.left,
            rect.top,
            rect.left + targetW,
            rect.top + targetH);
        _bgImage.draw(imgRect, gui::Image::AspectRatio::No);
    }
    else {
        gui::Shape bg;
        bg.createRect(rect);
        bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);
    }

    // Draw connection lines first (so they appear under cities)
    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(1, td::LinePattern::Solid);
    std::set<std::pair<int, int>> drawn;
    for (const auto& e : _roads) {
        int a = std::min(e.fromId, e.toId);
        int b = std::max(e.fromId, e.toId);
        if (drawn.insert({ a,b }).second) {
            // find city points by id (id equals index in current data)
            if (a >= 0 && b >= 0 && a < (int)_cities.size() && b < (int)_cities.size()) {
                auto c1 = getPointCenter(_cities[a]);
                auto c2 = getPointCenter(_cities[b]);
                bezier.moveTo({ c1.first, c1.second });
                bezier.lineTo({ c2.first, c2.second });
            }
        }
    }
    bezierShape.drawWire(td::ColorID::Black);

    // Draw cities on top
    const int size = 10;
    const int borderOffset = 3;
    for (const auto& c : _cities) {
        // Determine colors based on visitation_status
        // 0=blocked: border=#e63946, center=#f4f0bb
        // 1=open: border=#f4f0bb, center=#f4f0bb
        // 2=goal: border=#f4f0bb, center=#8338ec
        std::string borderColor = "#f4f0bb";
        std::string centerColor = "#f4f0bb";
        
        if (c.visitation_status == 0) {
            borderColor = "#e63946";
            centerColor = "#f4f0bb";
        } else if (c.visitation_status == 2) {
            borderColor = "#f4f0bb";
            centerColor = "#8338ec";
        }
        
        // Draw border rect (2px wider on each side)
        gui::Rect borderRect(static_cast<int>(c.x) - borderOffset, 
                            static_cast<int>(c.y) - borderOffset,
                            static_cast<int>(c.x) + size + borderOffset, 
                            static_cast<int>(c.y) + size + borderOffset);
        gui::Shape borderShape;
        borderShape.createRect(borderRect);
        borderShape.drawFillAndWire(mapHexToColor(borderColor), td::ColorID::Black, 1.0f);
        
        // Draw center rect
        gui::Rect centerRect(static_cast<int>(c.x), static_cast<int>(c.y),
                            static_cast<int>(c.x) + size, static_cast<int>(c.y) + size);
        gui::Shape centerShape;
        centerShape.createRect(centerRect);
        centerShape.drawFillAndWire(mapHexToColor(centerColor), td::ColorID::Black, 1.0f);
        
        // Draw city name
        td::String c_name = c.name;
        gui::DrawableString str1(c_name);
        str1.draw(gui::Point(c.x, c.y + 10), gui::Font::ID::SystemNormal, td::ColorID::Black);
    }
}

std::pair<gui::CoordType, gui::CoordType> MapView::getPointCenter(const CityPoint& p) const
{
    return { static_cast<gui::CoordType>(p.x + 5), static_cast<gui::CoordType>(p.y + 5) };
}

void MapView::loadBackground()
{
    if (_bgLoaded) return;
    namespace fs = std::filesystem;

    static const char* candidates[] = {
        "res/assets/yugoslavia.png",
        "./res/assets/yugoslavia.png",
        "../res/assets/yugoslavia.png"
    };

    auto exists = [](const fs::path& p) { try { return fs::exists(p); } catch (...) { return false; } };

    fs::path cwd;
    try { cwd = fs::current_path(); }
    catch (...) { cwd = fs::path("."); }

    // Also try exe directory
    char exeBuf[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    fs::path exeDir(exeBuf);
    exeDir = exeDir.parent_path();

    std::string toLoad;
    for (auto c : candidates) {
        fs::path p = cwd / c;
        if (exists(p)) { toLoad = p.string(); break; }
    }
    if (toLoad.empty()) {
        for (auto c : candidates) {
            fs::path p = exeDir / c;
            if (exists(p)) { toLoad = p.string(); break; }
        }
    }

    if (!toLoad.empty()) {
        _bgImage.load(toLoad.c_str());
        if (_bgImage.isOK()) {
            _bgLoaded = true;
            OutputDebugStringA((std::string("[MapView] Loaded background image: ") + toLoad + "\n").c_str());
        }
        else {
            OutputDebugStringA((std::string("[MapView] Failed to load background: ") + toLoad + "\n").c_str());
        }
    }
    else {
        OutputDebugStringA("[MapView] Background image not found in search paths\n");
        _bgLoaded = false;
    }
}

bool MapView::getCity(int index, CityPoint& out) const
{
    if (index < 0 || index >= static_cast<int>(_cities.size())) return false;
    out = _cities[static_cast<size_t>(index)];
    return !out.name.empty();
}

int MapView::nextCityId() const
{
    int maxId = -1;
    for (const auto& c : _cities) {
        maxId = std::max(maxId, c.id);
    }
    if (maxId < 0) {
        return static_cast<int>(_cities.size());
    }
    return maxId + 1;
}

bool MapView::saveJson() const
{
    namespace fs = std::filesystem;
    fs::path pathToSave = _jsonPath.empty() ? JsonService::findJsonFile() : _jsonPath;
    return JsonService::saveToJson(pathToSave, _cities, _roadsFull);
}

bool MapView::addCity(const std::string& name, double x, double y)
{
    if (name.empty()) return false;
    CityPoint cp;
    cp.id = nextCityId();
    cp.name = name;
    cp.x = x;
    cp.y = y;
    cp.weight = 0.0;
    cp.visitation_status = 1;

    _cities.push_back(cp);
    bool saved = saveJson();
    if (!saved) {
        _cities.pop_back();
        return false;
    }
    reDraw();
    return true;
}

bool MapView::updateCity(int index, const std::string& name, double x, double y)
{
    if (index < 0 || index >= static_cast<int>(_cities.size())) return false;
    if (name.empty()) return false;

    CityPoint backup = _cities[static_cast<size_t>(index)];
    _cities[static_cast<size_t>(index)].name = name;
    _cities[static_cast<size_t>(index)].x = x;
    _cities[static_cast<size_t>(index)].y = y;

    bool saved = saveJson();
    if (!saved) {
        _cities[static_cast<size_t>(index)] = backup;
        return false;
    }
    reDraw();
    return true;
}

bool MapView::deleteCity(int index)
{
    if (index < 0 || index >= static_cast<int>(_cities.size())) return false;
    auto backupCities = _cities;
    auto backupRoads = _roads;
    auto backupRoadsFull = _roadsFull;

    _cities.erase(_cities.begin() + index);
    for (size_t i = 0; i < _cities.size(); ++i) {
        _cities[i].id = static_cast<int>(i);
    }

    auto adjustId = [index](int id) {
        if (id > index) return id - 1;
        return id;
        };

    std::vector<RoadEdge> newRoads;
    std::vector<RoadInfo> newRoadsFull;
    for (const auto& r : _roads) {
        if (r.fromId == index || r.toId == index) continue;
        RoadEdge nr{ adjustId(r.fromId), adjustId(r.toId) };
        newRoads.push_back(nr);
    }
    for (const auto& r : _roadsFull) {
        if (r.fromId == index || r.toId == index) continue;
        RoadInfo nr = r;
        nr.fromId = adjustId(r.fromId);
        nr.toId = adjustId(r.toId);
        newRoadsFull.push_back(nr);
    }
    _roads.swap(newRoads);
    _roadsFull.swap(newRoadsFull);

    bool saved = saveJson();
    if (!saved) {
        _cities.swap(backupCities);
        _roads.swap(backupRoads);
        _roadsFull.swap(backupRoadsFull);
        return false;
    }
    reDraw();
    return true;
}

std::vector<int> MapView::getConnections(int index) const
{
    std::set<int> ids;
    if (index < 0 || index >= static_cast<int>(_cities.size())) return {};
    for (const auto& r : _roads) {
        if (r.fromId == index && r.toId >= 0 && r.toId < static_cast<int>(_cities.size())) ids.insert(r.toId);
        if (r.toId == index && r.fromId >= 0 && r.fromId < static_cast<int>(_cities.size())) ids.insert(r.fromId);
    }
    return std::vector<int>(ids.begin(), ids.end());
}

std::vector<std::string> MapView::getConnectionNames(int index) const
{
    std::vector<std::string> names;
    for (int i : getConnections(index)) {
        if (i >= 0 && i < static_cast<int>(_cities.size())) {
            names.push_back(_cities[static_cast<size_t>(i)].name);
        }
    }
    return names;
}

bool MapView::addConnection(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0) return false;
    if (fromIndex >= static_cast<int>(_cities.size()) || toIndex >= static_cast<int>(_cities.size())) return false;
    if (fromIndex == toIndex) return false;

    auto exists = [&](int a, int b)->bool {
        for (const auto& r : _roads) {
            if ((r.fromId == a && r.toId == b) || (r.fromId == b && r.toId == a)) return true;
        }
        return false;
        };
    if (exists(fromIndex, toIndex)) return true;

    double dx = _cities[static_cast<size_t>(fromIndex)].x - _cities[static_cast<size_t>(toIndex)].x;
    double dy = _cities[static_cast<size_t>(fromIndex)].y - _cities[static_cast<size_t>(toIndex)].y;
    double length = std::sqrt(dx * dx + dy * dy);
    double travel = length / 60.0; // arbitrary scaling for demo

    _roads.push_back({ fromIndex, toIndex });
    RoadInfo ri;
    ri.fromId = fromIndex;
    ri.toId = toIndex;
    ri.length = length;
    ri.travelTimeH = travel;
    ri.type = "local";
    ri.bidirectional = true;
    _roadsFull.push_back(ri);

    bool saved = saveJson();
    if (!saved) {
        _roads.pop_back();
        _roadsFull.pop_back();
        return false;
    }
    reDraw();
    return true;
}

bool MapView::removeConnection(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0) return false;
    if (fromIndex >= static_cast<int>(_cities.size()) || toIndex >= static_cast<int>(_cities.size())) return false;
    if (fromIndex == toIndex) return false;

    auto backupRoads = _roads;
    auto backupRoadsFull = _roadsFull;

    auto isPair = [&](int a, int b, int x, int y)->bool {
        return (a == x && b == y) || (a == y && b == x);
        };

    _roads.erase(std::remove_if(_roads.begin(), _roads.end(), [&](const RoadEdge& r) {
        return isPair(r.fromId, r.toId, fromIndex, toIndex);
        }), _roads.end());

    _roadsFull.erase(std::remove_if(_roadsFull.begin(), _roadsFull.end(), [&](const RoadInfo& r) {
        return isPair(r.fromId, r.toId, fromIndex, toIndex);
        }), _roadsFull.end());

    bool saved = saveJson();
    if (!saved) {
        _roads.swap(backupRoads);
        _roadsFull.swap(backupRoadsFull);
        return false;
    }
    reDraw();
    return true;
}

bool MapView::hasConnection(int fromIndex, int toIndex) const
{
    if (fromIndex < 0 || toIndex < 0) return false;
    if (fromIndex >= static_cast<int>(_cities.size()) || toIndex >= static_cast<int>(_cities.size())) return false;
    if (fromIndex == toIndex) return false;

    for (const auto& r : _roads) {
        if ((r.fromId == fromIndex && r.toId == toIndex) || (r.fromId == toIndex && r.toId == fromIndex)) {
            return true;
        }
    }
    return false;
}
