#include "MapView.h"
#include "MapPoint.h"
#include "SearchAlgorithm.h"
#include "TSPAlgorithm.h"
#include <set>
#include <sstream>
#include <iomanip>
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

void MapView::setSolver(SearchAlgorithm* solver)
{
    _solver = solver;
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
                auto c1 = MapPointStyle::getCenter(cities[a].x, cities[a].y);
                auto c2 = MapPointStyle::getCenter(cities[b].x, cities[b].y);
                bezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
                bezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
            }
        }
    }
    bezierShape.drawWire(td::ColorID::Black);

    // Draw algorithm visualization (path, if found)
    if (_solver) {
        drawAlgorithmState();
    }

    // Draw cities on top using MapPointRenderer
    for (const auto& city : cities) {
        MapPointRenderer::draw(city);
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

void MapView::drawAlgorithmState()
{
    if (!_solver || !_repo) return;

    // Check if this is a TSP algorithm
    TSPAlgorithm* tspSolver = dynamic_cast<TSPAlgorithm*>(_solver);
    if (tspSolver) {
        drawTSPState();
        return;
    }

    // BFS/DFS visualization
    const auto& cities = _repo->cities();
    
    // Build sets for quick lookup
    auto visitedVec = _solver->getVisited();
    std::set<int> visitedSet(visitedVec.begin(), visitedVec.end());
    
    const auto& frontierVec = _solver->getFrontier();
    std::set<int> frontierSet(frontierVec.begin(), frontierVec.end());

    int currentIdx = _solver->getCurrentIdx();
    
    // Draw visited nodes with a green overlay
    for (int idx : visitedSet) {
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            int ix = static_cast<int>(city.x);
            int iy = static_cast<int>(city.y);
            
            gui::Rect overlayRect(
                ix - MapPointStyle::BorderOffset - 2,
                iy - MapPointStyle::BorderOffset - 2,
                ix + MapPointStyle::Size + MapPointStyle::BorderOffset + 2,
                iy + MapPointStyle::Size + MapPointStyle::BorderOffset + 2
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::LightGreen, td::ColorID::DarkGreen, 2.0f);
        }
    }
    
    // Draw frontier nodes with an orange overlay
    for (int idx : frontierSet) {
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            int ix = static_cast<int>(city.x);
            int iy = static_cast<int>(city.y);
            
            gui::Rect overlayRect(
                ix - MapPointStyle::BorderOffset - 2,
                iy - MapPointStyle::BorderOffset - 2,
                ix + MapPointStyle::Size + MapPointStyle::BorderOffset + 2,
                iy + MapPointStyle::Size + MapPointStyle::BorderOffset + 2
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::Orange, td::ColorID::DarkRed, 2.0f);
        }
    }
    
    // Draw current node with special highlight
    if (currentIdx >= 0 && currentIdx < static_cast<int>(cities.size())) {
        const auto& city = cities[currentIdx];
        int ix = static_cast<int>(city.x);
        int iy = static_cast<int>(city.y);
        
        gui::Rect currentRect(
            ix - MapPointStyle::BorderOffset - 4,
            iy - MapPointStyle::BorderOffset - 4,
            ix + MapPointStyle::Size + MapPointStyle::BorderOffset + 4,
            iy + MapPointStyle::Size + MapPointStyle::BorderOffset + 4
        );
        gui::Shape currentOverlay;
        currentOverlay.createRect(currentRect);
        currentOverlay.drawWire(td::ColorID::Blue, 3.0f);
    }
    
    // Draw solution path if found
    const auto& path = _solver->getPath();
    if (!path.empty()) {
        drawPath(path);
    }
}

void MapView::drawPath(const std::vector<int>& path)
{
    if (path.size() < 2 || !_repo) return;

    const auto& cities = _repo->cities();

    // Draw path edges in blue
    gui::Shape pathShape;
    auto pathBezier = pathShape.createBezier(3, td::LinePattern::Solid);
    
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int a = path[i];
        int b = path[i + 1];
        
        if (a >= 0 && b >= 0 && a < static_cast<int>(cities.size()) && b < static_cast<int>(cities.size())) {
            auto c1 = MapPointStyle::getCenter(cities[a].x, cities[a].y);
            auto c2 = MapPointStyle::getCenter(cities[b].x, cities[b].y);
            pathBezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
            pathBezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
        }
    }
    pathShape.drawWire(td::ColorID::Blue);
}

void MapView::drawTSPState()
{
    TSPAlgorithm* tspSolver = dynamic_cast<TSPAlgorithm*>(_solver);
    if (!tspSolver || !_repo) return;

    const auto& cities = _repo->cities();

    // Draw current tour in cyan
    const auto& currentTour = tspSolver->getTour();
    if (!currentTour.empty()) {
        drawTour(currentTour, td::ColorID::Cyan, 2.0f);
    }

    // Draw best tour in green (if different and exists)
    const auto& bestTour = tspSolver->getBestTour();
    if (!bestTour.empty() && bestTour != currentTour) {
        drawTour(bestTour, td::ColorID::Green, 3.0f);
    }

    // Highlight cities in the current tour
    for (size_t i = 0; i < currentTour.size(); ++i) {
        int idx = currentTour[i];
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            int ix = static_cast<int>(city.x);
            int iy = static_cast<int>(city.y);
            
            gui::Rect overlayRect(
                ix - MapPointStyle::BorderOffset - 2,
                iy - MapPointStyle::BorderOffset - 2,
                ix + MapPointStyle::Size + MapPointStyle::BorderOffset + 2,
                iy + MapPointStyle::Size + MapPointStyle::BorderOffset + 2
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            
            // First city in tour gets special color
            if (i == 0) {
                overlay.drawFillAndWire(td::ColorID::Cyan, td::ColorID::DarkBlue, 2.0f);
            } else {
                overlay.drawFillAndWire(td::ColorID::LightGreen, td::ColorID::DarkGreen, 1.5f);
            }
        }
    }

    // Display tour length info
    double currentLength = tspSolver->getTourLength();
    double bestLength = tspSolver->getBestLength();
    int step = tspSolver->getCurrentStep();

    std::ostringstream info;
    info << std::fixed << std::setprecision(1);
    info << "Step: " << step;
    if (currentLength < std::numeric_limits<double>::max()) {
        info << "  Current: " << currentLength;
    }
    if (bestLength < std::numeric_limits<double>::max()) {
        info << "  Best: " << bestLength;
    }

    gui::DrawableString infoStr(info.str().c_str());
    gui::Point textPos(10, 10);
    infoStr.draw(textPos, gui::Font::ID::SystemNormal, td::ColorID::Black);
}

void MapView::drawTour(const std::vector<int>& tour, td::ColorID color, float lineWidth)
{
    if (tour.size() < 2 || !_repo) return;

    const auto& cities = _repo->cities();

    gui::Shape tourShape;
    auto tourBezier = tourShape.createBezier(static_cast<gui::CoordType>(lineWidth), td::LinePattern::Solid);
    
    // Draw each TSP edge along the underlying road shortest path
    for (size_t i = 0; i < tour.size(); ++i) {
        int a = tour[i];
        int b = tour[(i + 1) % tour.size()];  // Wrap around to close the tour
        if (a < 0 || b < 0 || a >= static_cast<int>(cities.size()) || b >= static_cast<int>(cities.size())) continue;

        std::vector<int> path;
        if (_repo->getShortestRoadPath(a, b, path) && path.size() >= 2) {
            for (size_t k = 0; k + 1 < path.size(); ++k) {
                int u = path[k];
                int v = path[k + 1];
                auto c1 = MapPointStyle::getCenter(cities[u].x, cities[u].y);
                auto c2 = MapPointStyle::getCenter(cities[v].x, cities[v].y);
                tourBezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
                tourBezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
            }
        }
    }
    tourShape.drawWire(color);
}
