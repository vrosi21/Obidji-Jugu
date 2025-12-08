#include "MapView.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <set>
#include <windows.h>

namespace {
    void dbg(const char* m){ OutputDebugStringA(m); }
}

MapView::MapView()
{
    loadCities();
    loadBackground();
}

void MapView::loadCities()
{
    if (_loaded) return;
    namespace fs = std::filesystem;

    static const char* candidates[] = {
        "../res/exYu.json"
    };

    auto readFile = [](const fs::path& p)->std::string {
        try { if(!fs::exists(p)) return {}; std::ifstream in(p); if(!in) return {}; std::ostringstream ss; ss<<in.rdbuf(); return ss.str(); } catch(...) { return {}; }
    };

    fs::path cwd = fs::current_path();
    fs::path foundPath;

    // Also try exe directory similar to other loaders
    char exeBuf[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    fs::path exeDir(exeBuf);
    exeDir = exeDir.parent_path();

    std::string content;
    for (auto c: candidates) {
        fs::path p = cwd / c;
        content = readFile(p);
        if (!content.empty()) { foundPath = p; break; }
    }
    if (content.empty()) {
        for (auto c: candidates) {
            fs::path p = exeDir / c;
            content = readFile(p);
            if (!content.empty()) { foundPath = p; break; }
        }
    }
    if (foundPath.empty()) {
        foundPath = cwd / candidates[0];
    }
    _jsonPath = foundPath;
    if (content.empty()) {
        dbg("[MapView] exYu.json not found in search paths\n");
        _loaded = true; // prevent repeated attempts
        return;
    }

    // Parse cities array: [{...}, {...}, ...]
    size_t citiesKey = content.find("\"cities\"");
    size_t roadsKey = content.find("\"roads\"");
    size_t arrayStart = (citiesKey == std::string::npos) ? content.find('[') : content.find('[', citiesKey);
    size_t arrayEnd = std::string::npos;
    if (roadsKey != std::string::npos && roadsKey > arrayStart) {
        arrayEnd = content.rfind(']', roadsKey);
    }
    if (arrayEnd == std::string::npos) {
        arrayEnd = content.rfind(']');
    }
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayStart >= arrayEnd) {
        dbg("[MapView] exYu.json invalid array format\n");
        _loaded = true;
        return;
    }
    size_t pos = arrayStart + 1;
    while (pos < arrayEnd) {
        size_t objStart = content.find('{', pos);
        if (objStart == std::string::npos || objStart > arrayEnd) break;
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos || objEnd > arrayEnd) break;
        std::string obj = content.substr(objStart, objEnd - objStart + 1);

        auto extractNumber = [&](const char* key)->double {
            size_t k = obj.find(key);
            if (k == std::string::npos) return 0.0;
            size_t colon = obj.find(':', k);
            size_t n1 = obj.find_first_of("0123456789-.", colon);
            if (n1 == std::string::npos) return 0.0;
            size_t n2 = obj.find_first_not_of("0123456789.-", n1);
            return std::atof(obj.substr(n1, n2 - n1).c_str());
        };
        auto extractInt = [&](const char* key)->int {
            return static_cast<int>(extractNumber(key));
        };
        auto extractString = [&](const char* key)->std::string {
            size_t k = obj.find(key);
            if (k == std::string::npos) return {};
            size_t colon = obj.find(':', k);
            size_t q1 = obj.find('"', colon);
            size_t q2 = obj.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) return {};
            return obj.substr(q1 + 1, q2 - q1 - 1);
        };

        CityPoint cp;
        int parsedId = extractInt("\"id\"");
        cp.id = parsedId >= 0 ? parsedId : static_cast<int>(_cities.size());
        cp.x = extractNumber("\"x\"");
        cp.y = extractNumber("\"y\"");
        cp.name = extractString("\"name\"");
        cp.weight = extractNumber("\"weight\"");
        cp.colorHex = extractString("\"color\"");
        if (cp.colorHex.empty()) cp.colorHex = "#d62728";
        if (!cp.name.empty()) {
            if (cp.id == static_cast<int>(_cities.size())) {
                _cities.push_back(cp);
            } else if (cp.id >= 0 && cp.id < 10000) { // guard against runaway ids
                if (_cities.size() <= static_cast<size_t>(cp.id)) {
                    _cities.resize(static_cast<size_t>(cp.id) + 1);
                }
                _cities[static_cast<size_t>(cp.id)] = cp;
            } else {
                _cities.push_back(cp);
            }
        }

        pos = objEnd + 1;
    }
    char msg[128]; sprintf_s(msg, "[MapView] Loaded %zu cities from exYu.json\n", _cities.size()); dbg(msg);
    // Parse roads array for diagnostics (no drawing)
    if (roadsKey != std::string::npos) {
        size_t rStart = content.find('[', roadsKey);
        size_t rEnd = content.find(']', rStart == std::string::npos ? 0 : rStart);
        if (rStart != std::string::npos && rEnd != std::string::npos) {
            size_t rpos = rStart + 1;
            while (rpos < rEnd) {
                size_t oStart = content.find('{', rpos);
                if (oStart == std::string::npos || oStart > rEnd) break;
                size_t oEnd = content.find('}', oStart);
                if (oEnd == std::string::npos || oEnd > rEnd) break;
                std::string obj = content.substr(oStart, oEnd - oStart + 1);
                auto extractInt = [&](const char* key)->int {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return -1;
                    size_t colon = obj.find(':', k);
                    size_t n1 = obj.find_first_of("0123456789-", colon);
                    if (n1 == std::string::npos) return -1;
                    size_t n2 = obj.find_first_not_of("0123456789-", n1);
                    return std::atoi(obj.substr(n1, n2 - n1).c_str());
                };
                auto extractDouble = [&](const char* key)->double {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return 0.0;
                    size_t colon = obj.find(':', k);
                    size_t n1 = obj.find_first_of("0123456789-.", colon);
                    if (n1 == std::string::npos) return 0.0;
                    size_t n2 = obj.find_first_not_of("0123456789.-", n1);
                    return std::atof(obj.substr(n1, n2 - n1).c_str());
                };
                auto extractString = [&](const char* key)->std::string {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return {};
                    size_t colon = obj.find(':', k);
                    size_t q1 = obj.find('"', colon);
                    size_t q2 = obj.find('"', q1 + 1);
                    if (q1 == std::string::npos || q2 == std::string::npos) return {};
                    return obj.substr(q1 + 1, q2 - q1 - 1);
                };
                auto extractBool = [&](const char* key)->bool {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return true;
                    size_t colon = obj.find(':', k);
                    size_t t = obj.find("true", colon);
                    size_t f = obj.find("false", colon);
                    if (t != std::string::npos && t < obj.size() && (t < f || f == std::string::npos)) return true;
                    if (f != std::string::npos && f < obj.size()) return false;
                    return true;
                };
                int from = extractInt("\"from\"");
                int to = extractInt("\"to\"");
                if (from >= 0 && to >= 0) {
                    _roads.push_back({from, to});
                    RoadInfo ri;
                    ri.fromId = from;
                    ri.toId = to;
                    ri.length = extractDouble("\"length\"");
                    ri.travelTimeH = extractDouble("\"travel_time_h\"");
                    ri.type = extractString("\"type\"");
                    ri.bidirectional = extractBool("\"bidirectional\"");
                    _roadsFull.push_back(ri);
                }
                rpos = oEnd + 1;
            }
        }
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
    } else {
        gui::Shape bg;
        bg.createRect(rect);
        bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);
    }

    // Draw each city as a 10x10 square at (x,y)
    const int size = 10;
    for (const auto& c : _cities) {
        gui::Rect r(static_cast<int>(c.x), static_cast<int>(c.y),
                    static_cast<int>(c.x)+size, static_cast<int>(c.y)+size); // Using x1,y1,x2,y2 semantics
        gui::Shape s; s.createRect(r);
        s.drawFillAndWire(mapHexToColor(c.colorHex), td::ColorID::Black, 1.0f);
    }

    // Draw connection lines from center to center
    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(1, td::LinePattern::Solid);
    std::set<std::pair<int,int>> drawn;
    for (const auto& e : _roads) {
        int a = std::min(e.fromId, e.toId);
        int b = std::max(e.fromId, e.toId);
        if (drawn.insert({a,b}).second) {
            // find city points by id (id equals index in current data)
            if (a >= 0 && b >= 0 && a < (int)_cities.size() && b < (int)_cities.size()) {
                auto c1 = getPointCenter(_cities[a]);
                auto c2 = getPointCenter(_cities[b]);
                bezier.moveTo({c1.first, c1.second});
                bezier.lineTo({c2.first, c2.second});
            }
        }
    }
    bezierShape.drawWire(td::ColorID::Black);
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

    auto exists = [](const fs::path& p){ try { return fs::exists(p); } catch(...) { return false; } };

    fs::path cwd;
    try { cwd = fs::current_path(); } catch(...) { cwd = fs::path("."); }

    // Also try exe directory
    char exeBuf[MAX_PATH] = {0};
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
        } else {
            OutputDebugStringA((std::string("[MapView] Failed to load background: ") + toLoad + "\n").c_str());
        }
    } else {
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
    fs::path pathToSave = _jsonPath.empty() ? resolveDefaultJsonPath() : _jsonPath;
    std::ofstream out(pathToSave, std::ios::trunc);
    if (!out) {
        dbg("[MapView] Failed to open exYu.json for writing\n");
        return false;
    }

    out << "{\n";
    out << "  \"cities\": [\n";
    for (size_t i = 0; i < _cities.size(); ++i) {
        const auto& c = _cities[i];
        out << "    {\"id\":" << c.id
            << ",\"x\":" << std::fixed << std::setprecision(2) << c.x
            << ",\"y\":" << std::fixed << std::setprecision(2) << c.y
            << ",\"name\":\"" << c.name << "\""
            << ",\"weight\":" << std::fixed << std::setprecision(1) << c.weight
            << ",\"color\":\"" << (c.colorHex.empty() ? "#d62728" : c.colorHex) << "\"}";
        if (i + 1 < _cities.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"roads\": [\n";
    for (size_t i = 0; i < _roadsFull.size(); ++i) {
        const auto& r = _roadsFull[i];
        out << "    {\"from\":" << r.fromId
            << ",\"to\":" << r.toId
            << ",\"length\":" << std::fixed << std::setprecision(2) << r.length
            << ",\"travel_time_h\":" << std::fixed << std::setprecision(3) << r.travelTimeH
            << ",\"type\":\"" << r.type << "\""
            << ",\"bidirectional\":" << (r.bidirectional ? "true" : "false") << "}";
        if (i + 1 < _roadsFull.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

std::filesystem::path MapView::resolveDefaultJsonPath() const
{
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path();
    return cwd / "../res/exYu.json";
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
    cp.colorHex = "#d62728";

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
        RoadEdge nr{adjustId(r.fromId), adjustId(r.toId)};
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
        if (r.fromId == index && r.toId >=0 && r.toId < static_cast<int>(_cities.size())) ids.insert(r.toId);
        if (r.toId == index && r.fromId >=0 && r.fromId < static_cast<int>(_cities.size())) ids.insert(r.fromId);
    }
    return std::vector<int>(ids.begin(), ids.end());
}

std::vector<std::string> MapView::getConnectionNames(int index) const
{
    std::vector<std::string> names;
    for (int i : getConnections(index)) {
        if (i >=0 && i < static_cast<int>(_cities.size())) {
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
    double length = std::sqrt(dx*dx + dy*dy);
    double travel = length / 60.0; // arbitrary scaling for demo

    _roads.push_back({fromIndex, toIndex});
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

    _roads.erase(std::remove_if(_roads.begin(), _roads.end(), [&](const RoadEdge& r){
        return isPair(r.fromId, r.toId, fromIndex, toIndex);
    }), _roads.end());

    _roadsFull.erase(std::remove_if(_roadsFull.begin(), _roadsFull.end(), [&](const RoadInfo& r){
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
