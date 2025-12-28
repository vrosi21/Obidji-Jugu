#include "MapView.h"
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

    // Draw connection lines first (under cities)
    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(1, td::LinePattern::Solid);
    std::set<std::pair<int, int>> drawn;
    
    for (const auto& e : roads) {
        int a = std::min(e.fromId, e.toId);
        int b = std::max(e.fromId, e.toId);
        if (drawn.insert({ a, b }).second) {
            if (a >= 0 && b >= 0 && a < (int)cities.size() && b < (int)cities.size()) {
                auto c1 = getPointCenter(cities[a]);
                auto c2 = getPointCenter(cities[b]);
                bezier.moveTo({ c1.first, c1.second });
                bezier.lineTo({ c2.first, c2.second });
            }
        }
    }
    bezierShape.drawWire(td::ColorID::Black);

    // Draw cities on top
    const int size = 10;
    const int borderOffset = 3;
    
    for (const auto& c : cities) {
        // Colors based on visitation_status
        td::ColorID borderColor = td::ColorID::LightYellow;
        td::ColorID centerColor = td::ColorID::LightYellow;
        
        if (c.visitation_status == VisitationStatus::Blocked) {
            borderColor = td::ColorID::Red;
            centerColor = td::ColorID::LightYellow;
        } else if (c.visitation_status == VisitationStatus::Goal) {
            borderColor = td::ColorID::LightYellow;
            centerColor = td::ColorID::Violet;
        }
        
        // Border rectangle
        gui::Rect borderRect(static_cast<int>(c.x) - borderOffset, 
                            static_cast<int>(c.y) - borderOffset,
                            static_cast<int>(c.x) + size + borderOffset, 
                            static_cast<int>(c.y) + size + borderOffset);
        gui::Shape borderShape;
        borderShape.createRect(borderRect);
        borderShape.drawFillAndWire(borderColor, td::ColorID::Black, 1.0f);
        
        // Center rectangle
        gui::Rect centerRect(static_cast<int>(c.x), static_cast<int>(c.y),
                            static_cast<int>(c.x) + size, static_cast<int>(c.y) + size);
        gui::Shape centerShape;
        centerShape.createRect(centerRect);
        centerShape.drawFillAndWire(centerColor, td::ColorID::Black, 1.0f);
        
        // City name label
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
