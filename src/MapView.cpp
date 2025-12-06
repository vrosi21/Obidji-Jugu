#include "MapView.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <windows.h>

namespace {
    void dbg(const char* m){ OutputDebugStringA(m); }
}

MapView::MapView()
{
    loadCities();
}

void MapView::loadCities()
{
    if (_loaded) return;
    namespace fs = std::filesystem;

    static const char* candidates[] = {
        "res/exYu.json",
        "../res/exYu.json",
        "../../res/exYu.json",
        "DSAI_AI_Project_The_Travelling_Salesman/res/exYu.json",
        "exYu.json"
    };

    auto readFile = [](const fs::path& p)->std::string {
        try { if(!fs::exists(p)) return {}; std::ifstream in(p); if(!in) return {}; std::ostringstream ss; ss<<in.rdbuf(); return ss.str(); } catch(...) { return {}; }
    };

    fs::path cwd = fs::current_path();

    // Also try exe directory similar to other loaders
    char exeBuf[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    fs::path exeDir(exeBuf);
    exeDir = exeDir.parent_path();

    std::string content;
    for (auto c: candidates) {
        content = readFile(cwd / c);
        if (!content.empty()) break;
    }
    if (content.empty()) {
        for (auto c: candidates) {
            content = readFile(exeDir / c);
            if (!content.empty()) break;
        }
    }
    if (content.empty()) {
        dbg("[MapView] exYu.json not found in search paths\n");
        _loaded = true; // prevent repeated attempts
        return;
    }

    // Parse a simple array of objects [{...}, {...}, ...]
    size_t pos = 0;
    size_t arrayStart = content.find('[');
    size_t arrayEnd = content.rfind(']');
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        dbg("[MapView] exYu.json invalid array format\n");
        _loaded = true;
        return;
    }
    pos = arrayStart + 1;
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
        cp.x = static_cast<int>(extractNumber("\"x\""));
        cp.y = static_cast<int>(extractNumber("\"y\""));
        cp.name = extractString("\"name\"");
        cp.colorHex = extractString("\"color\"");
        if (!cp.name.empty()) _cities.push_back(cp);

        pos = objEnd + 1;
    }
    char msg[128]; sprintf_s(msg, "[MapView] Loaded %zu cities from exYu.json\n", _cities.size()); dbg(msg);
    _loaded = true;
    // If framework provides an invalidation method, call it; otherwise rely on next paint.
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
    // Background
    gui::Shape bg;
    bg.createRect(rect);
    bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);

    // Draw each city as a 10x10 square at (x,y)
    const int size = 10;
    for (const auto& c : _cities) {
        gui::Rect r(c.x, c.y, c.x+size, c.y+size); // Using x1,y1,x2,y2 semantics
        gui::Shape s; s.createRect(r);
        s.drawFillAndWire(mapHexToColor(c.colorHex), td::ColorID::Black, 1.0f);
    }
}
