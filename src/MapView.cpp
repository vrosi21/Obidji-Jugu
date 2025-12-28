#include "MapView.h"
#include "MapPoint.h"
#include <set>
#include <windows.h>
#include <filesystem>

namespace {
    void dbg(const char* m) { OutputDebugStringA(m); }
}

MapView::MapView()
{
    loadBackground();
}

void MapView::setRepository(DataRepository* repo)
{
    _repo = repo;
    if (_repo) {
        _repo->setOnDataChanged([this]() { reDraw(); });
    }
}

void MapView::onDraw(const gui::Rect& rect)
{
    // Draw background
    if (_bgLoaded && _bgImage.isOK()) {
        const float targetW = 1000.0f;
        const float srcW = 860.0f;
        const float srcH = 745.0f;
        const float targetH = targetW * (srcH / srcW);
        gui::Rect imgRect(rect.left, rect.top, rect.left + targetW, rect.top + targetH);
        _bgImage.draw(imgRect, gui::Image::AspectRatio::No);
    } else {
        gui::Shape bg;
        bg.createRect(rect);
        bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);
    }

    if (!_repo) return;

    const auto& cities = _repo->cities();
    const auto& roads = _repo->roads();
    int startPointId = _repo->getStartPointId();

    // Draw connection lines first (under cities)
    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(1, td::LinePattern::Solid);
    std::set<std::pair<int, int>> drawn;
    
    for (const auto& e : roads) {
        int a = std::min(e.fromId, e.toId);
        int b = std::max(e.fromId, e.toId);
        if (drawn.insert({ a, b }).second) {
            if (a >= 0 && b >= 0 && a < (int)cities.size() && b < (int)cities.size()) {
                auto c1 = MapPointStyle::getCenter(cities[a].x, cities[a].y);
                auto c2 = MapPointStyle::getCenter(cities[b].x, cities[b].y);
                bezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
                bezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
            }
        }
    }
    bezierShape.drawWire(td::ColorID::Black);

    // Draw cities on top using MapPointRenderer
    for (const auto& city : cities) {
        MapPointRenderer::draw(city, startPointId);
    }
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

    auto exists = [](const fs::path& p) { 
        try { return fs::exists(p); } 
        catch (...) { return false; } 
    };

    fs::path cwd;
    try { cwd = fs::current_path(); }
    catch (...) { cwd = fs::path("."); }

    char exeBuf[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    fs::path exeDir = fs::path(exeBuf).parent_path();

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
        _bgLoaded = _bgImage.isOK();
        dbg(_bgLoaded ? "[MapView] Background loaded\n" : "[MapView] Failed to load background\n");
    } else {
        dbg("[MapView] Background image not found\n");
    }
}
