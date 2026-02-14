#include "MapView.h"
#include "MapPoint.h"
#include "SearchAlgorithm.h"
#include "TSPAlgorithm.h"
#include <set>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <filesystem>
#include "SimulatedAnnealingAlgorithm.h"
#include <algorithm>
#include <limits>


namespace {
    void dbg(const char* m) { OutputDebugStringA(m); }
    constexpr float SOLUTION_ANIM_INTERVAL_SEC = 0.05f;
    constexpr size_t SOLUTION_ANIM_SEGMENTS_PER_TICK = 1;

}
MapView::MapView()
    : _currentSize(ORIGINAL_MAP_WIDTH, ORIGINAL_MAP_HEIGHT)
    , _solutionTimer(this, SOLUTION_ANIM_INTERVAL_SEC, false)
{
    enableResizeEvent(true);
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
    stopSolutionAnimation();
}

void MapView::onResize(const gui::Size& newSize)
{
    _currentSize = newSize;
}

void MapView::onDraw(const gui::Rect& rect)
{
    // Compute uniform scale and offsets to preserve proportions and center the map
    float viewW = static_cast<float>(rect.right - rect.left);
    float viewH = static_cast<float>(rect.bottom - rect.top);
    float scaleX = viewW / ORIGINAL_MAP_WIDTH;
    float scaleY = viewH / ORIGINAL_MAP_HEIGHT;
    float scale = std::min(scaleX, scaleY);
    float offsetX = rect.left + (viewW - ORIGINAL_MAP_WIDTH * scale) / 2.0f;
    float offsetY = rect.top + (viewH - ORIGINAL_MAP_HEIGHT * scale) / 2.0f;

    // store for other methods
    _scaleX = scale;
    _scaleY = scale;
    _offsetX = offsetX;
    _offsetY = offsetY;

    // Draw background - scaled and centered
    if (_bgLoaded && _bgImage.isOK()) {
        gui::Rect imgRect(static_cast<gui::CoordType>(offsetX), static_cast<gui::CoordType>(offsetY),
                          static_cast<gui::CoordType>(offsetX + ORIGINAL_MAP_WIDTH * scale),
                          static_cast<gui::CoordType>(offsetY + ORIGINAL_MAP_HEIGHT * scale));
        _bgImage.draw(imgRect, gui::Image::AspectRatio::No);
    } else {
        gui::Shape bg;
        bg.createRect(rect);
        bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);
    }

    if (!_repo) return;

    const auto& cities = _repo->cities();
    const auto& roads = _repo->roads();

    // Draw connection lines first (under cities) - scaled
    float avgScale = _scaleX; // uniform scale
    float scaledLineWidth = 1.0f * avgScale;
    if (scaledLineWidth < 0.5f) scaledLineWidth = 0.5f;

    gui::Shape bezierShape;
    auto bezier = bezierShape.createBezier(scaledLineWidth, td::LinePattern::Solid);
    std::set<std::pair<int, int>> drawn;
    
    for (const auto& e : roads) {
        int a = std::min(e.fromId, e.toId);
        int b = std::max(e.fromId, e.toId);
        if (drawn.insert({ a, b }).second) {
            if (a >= 0 && b >= 0 && a < static_cast<int>(cities.size()) && b < static_cast<int>(cities.size())) {
                auto c1 = MapPointStyle::getScaledCenter(cities[a].x, cities[a].y, _scaleX, _offsetX, _offsetY);
                auto c2 = MapPointStyle::getScaledCenter(cities[b].x, cities[b].y, _scaleX, _offsetX, _offsetY);
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

    // Draw cities on top using scaled MapPointRenderer
    for (const auto& city : cities) {
        MapPointRenderer::drawScaled(city, _scaleX, _offsetX, _offsetY);
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
    
    // Calculate scaled sizes
    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;
    float scaledLineWidth = 2.0f * avgScale;
    if (scaledLineWidth < 1.0f) scaledLineWidth = 1.0f;
    
    // Draw visited nodes with a green overlay
    for (int idx : visitedSet) {
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            float sx = _offsetX + static_cast<float>(city.x) * _scaleX;
            float sy = _offsetY + static_cast<float>(city.y) * _scaleX;
            
            gui::Rect overlayRect(
                sx - scaledBorder - 2 * avgScale,
                sy - scaledBorder - 2 * avgScale,
                sx + scaledSize + scaledBorder + 2 * avgScale,
                sy + scaledSize + scaledBorder + 2 * avgScale
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::LightGreen, td::ColorID::DarkGreen, scaledLineWidth);
        }
    }
    
    // Draw frontier nodes with an orange overlay
    for (int idx : frontierSet) {
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            float sx = _offsetX + static_cast<float>(city.x) * _scaleX;
            float sy = _offsetY + static_cast<float>(city.y) * _scaleX;
            
            gui::Rect overlayRect(
                sx - scaledBorder - 2 * avgScale,
                sy - scaledBorder - 2 * avgScale,
                sx + scaledSize + scaledBorder + 2 * avgScale,
                sy + scaledSize + scaledBorder + 2 * avgScale
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::Orange, td::ColorID::DarkRed, scaledLineWidth);
        }
    }
    
    // Draw current node with special highlight
    if (currentIdx >= 0 && currentIdx < static_cast<int>(cities.size())) {
        const auto& city = cities[currentIdx];
        float sx = _offsetX + static_cast<float>(city.x) * _scaleX;
        float sy = _offsetY + static_cast<float>(city.y) * _scaleX;
        
        gui::Rect currentRect(
            sx - scaledBorder - 4 * avgScale,
            sy - scaledBorder - 4 * avgScale,
            sx + scaledSize + scaledBorder + 4 * avgScale,
            sy + scaledSize + scaledBorder + 4 * avgScale
        );
        gui::Shape currentOverlay;
        currentOverlay.createRect(currentRect);
        currentOverlay.drawWire(td::ColorID::Blue, 3.0f * avgScale);
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
    auto isBlocked = [&](int idx) {
        return idx < 0 || idx >= static_cast<int>(cities.size()) ||
               cities[idx].visitation_status == VisitationStatus::Blocked;
    };

    // Draw path edges in blue - scaled
    float avgScale = _scaleX;
    float scaledLineWidth = 3.0f * avgScale;
    if (scaledLineWidth < 1.5f) scaledLineWidth = 1.5f;
    
    gui::Shape pathShape;
    auto pathBezier = pathShape.createBezier(scaledLineWidth, td::LinePattern::Solid);
    
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        int a = path[i];
        int b = path[i + 1];
        
        if (!isBlocked(a) && !isBlocked(b)) {
            auto c1 = MapPointStyle::getScaledCenter(cities[a].x, cities[a].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[b].x, cities[b].y, _scaleX, _offsetX, _offsetY);
            pathBezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
            pathBezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
        }
    }
    pathShape.drawWire(td::ColorID::Blue);
}

void MapView::drawTSPState()
{
    if (_solutionAnimating) {
        drawAnimatedSolution();
        return;
    }

    TSPAlgorithm* tspSolver = dynamic_cast<TSPAlgorithm*>(_solver);
    if (!tspSolver || !_repo) return;

    const auto& cities = _repo->cities();
    
    // Calculate scaled sizes
    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;
    float scaledLineWidth = 2.0f * avgScale;
    if (scaledLineWidth < 1.0f) scaledLineWidth = 1.0f;

    // Draw current tour in cyan
    const auto& currentTour = tspSolver->getTour();
    if (!currentTour.empty()) {
        drawTour(currentTour, td::ColorID::Cyan, 2.0f * avgScale);
    }

    // Draw best tour in green (if different and exists)
    const auto& bestTour = tspSolver->getBestTour();
    if (!bestTour.empty() && bestTour != currentTour) {
        drawTour(bestTour, td::ColorID::Green, 3.0f * avgScale);
    }

    // Highlight cities in the current tour
    for (size_t i = 0; i < currentTour.size(); ++i) {
        int idx = currentTour[i];
        if (idx >= 0 && idx < static_cast<int>(cities.size())) {
            const auto& city = cities[idx];
            float sx = _offsetX + static_cast<float>(city.x) * _scaleX;
            float sy = _offsetY + static_cast<float>(city.y) * _scaleY;

            
            gui::Rect overlayRect(
                sx - scaledBorder - 2 * avgScale,
                sy - scaledBorder - 2 * avgScale,
                sx + scaledSize + scaledBorder + 2 * avgScale,
                sy + scaledSize + scaledBorder + 2 * avgScale
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            
            // First city in tour gets special color
            if (i == 0) {
                overlay.drawFillAndWire(td::ColorID::Cyan, td::ColorID::DarkBlue, scaledLineWidth);
            } else {
                overlay.drawFillAndWire(td::ColorID::LightGreen, td::ColorID::DarkGreen, scaledLineWidth * 0.75f);
            }
        }
    }

    // Display tour length info - scaled position
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
    gui::Point textPos(static_cast<gui::CoordType>(_offsetX + 10 * _scaleX), static_cast<gui::CoordType>(_offsetY + 10 * _scaleX));
    infoStr.draw(textPos, gui::Font::ID::SystemNormal, td::ColorID::Black);
}

void MapView::drawTour(const std::vector<int>& tour, td::ColorID color, float lineWidth)
{
    if (tour.size() < 2 || !_repo) return;

    const auto& cities = _repo->cities();
    auto isBlocked = [&](int idx) {
        return idx < 0 || idx >= static_cast<int>(cities.size()) ||
               cities[idx].visitation_status == VisitationStatus::Blocked;
    };

    // Scale the line width appropriately
    float scaledLineWidth = lineWidth;
    if (scaledLineWidth < 1.0f) scaledLineWidth = 1.0f;

    gui::Shape tourShape;
    auto tourBezier = tourShape.createBezier(static_cast<gui::CoordType>(scaledLineWidth), td::LinePattern::Solid);
    
    // Draw each TSP edge along the underlying road shortest path
    for (size_t i = 0; i < tour.size(); ++i) {
        int a = tour[i];
        int b = tour[(i + 1) % tour.size()];  // Wrap around to close the tour
        if (isBlocked(a) || isBlocked(b)) continue;

        std::vector<int> path;
        if (_repo->getShortestRoadPath(a, b, path) && path.size() >= 2) {
            for (size_t k = 0; k + 1 < path.size(); ++k) {
                int u = path[k];
                int v = path[k + 1];
                if (isBlocked(u) || isBlocked(v)) continue;
                auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
                auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
                tourBezier.moveTo({ static_cast<gui::CoordType>(c1.first), static_cast<gui::CoordType>(c1.second) });
                tourBezier.lineTo({ static_cast<gui::CoordType>(c2.first), static_cast<gui::CoordType>(c2.second) });
            }
        }
    }
    tourShape.drawWire(color);
}


bool MapView::onTimer(gui::Timer* pTimer)
{
    if (pTimer == &_solutionTimer && _solutionAnimating) {
        const size_t maxEdges = (_solutionExpandedPath.size() > 1) ? (_solutionExpandedPath.size() - 1) : 0;

        if (_solutionAnimEdgeCount < maxEdges) {
            size_t next = _solutionAnimEdgeCount + SOLUTION_ANIM_SEGMENTS_PER_TICK;
            _solutionAnimEdgeCount = (next > maxEdges) ? maxEdges : next;
        }

        if (_solutionExpandedPath.size() < 2 || _solutionAnimEdgeCount >= (_solutionExpandedPath.size() - 1)) {
            _solutionAnimating = false;
            _solutionTimer.stop();
        }

        reDraw();
        if (_solutionAnimating) _solutionTimer.start(); // one-shot timer -> restart
        return true;
    }
    return false;
}




void MapView::startSolutionAnimation()
{
    if (!_repo || !_solver) return;

    auto* tsp = dynamic_cast<TSPAlgorithm*>(_solver);
    if (!tsp) return;

    const auto& bestTour = tsp->getBestTour();
    const auto& curTour = tsp->getTour();
    const auto& tourToUse = (!bestTour.empty()) ? bestTour : curTour;

    if (tourToUse.size() < 2) return;

    std::vector<int> expanded;
    if (!buildExpandedTourPath(tourToUse, expanded)) return;

    _solutionExpandedPath = std::move(expanded);
    _solutionAnimEdgeCount = 1;     
    _solutionAnimating = true;
}

void MapView::stopSolutionAnimation()
{
    _solutionAnimating = false;
    _solutionExpandedPath.clear();
    _solutionAnimEdgeCount = 0;
}

bool MapView::advanceSolutionAnimation(size_t edgesPerTick)
{
    if (!_solutionAnimating) return false;
    if (_solutionExpandedPath.size() < 2) { stopSolutionAnimation(); return false; }

    const size_t maxEdges = _solutionExpandedPath.size() - 1;
    if (_solutionAnimEdgeCount >= maxEdges) {
        _solutionAnimating = false;
        return false;
    }

    size_t next = _solutionAnimEdgeCount + edgesPerTick;
    _solutionAnimEdgeCount = (next > maxEdges) ? maxEdges : next;

    if (_solutionAnimEdgeCount >= maxEdges) {
        _solutionAnimating = false;
        return false;
    }
    return true;
}

bool MapView::buildExpandedTourPath(const std::vector<int>& tour, std::vector<int>& outPath) const
{
    outPath.clear();
    if (!_repo || tour.size() < 2) return false;

    const auto& cities = _repo->cities();
    auto isBlocked = [&](int idx) {
        return idx < 0 || idx >= static_cast<int>(cities.size()) ||
               cities[idx].visitation_status == VisitationStatus::Blocked;
    };

    for (size_t i = 0; i < tour.size(); ++i) {
        int a = tour[i];
        int b = tour[(i + 1) % tour.size()];
        if (isBlocked(a) || isBlocked(b)) return false;

        std::vector<int> leg;
        if (!_repo->getShortestRoadPath(a, b, leg) || leg.size() < 2) return false;

        for (int idx : leg) {
            if (isBlocked(idx)) return false;
        }

        if (outPath.empty()) outPath.insert(outPath.end(), leg.begin(), leg.end());
        else outPath.insert(outPath.end(), leg.begin() + 1, leg.end()); 
    }

    return outPath.size() >= 2;
}

void MapView::drawAnimatedSolution()
{
    if (!_solutionAnimating || _solutionExpandedPath.size() < 2 || !_repo) return;

    const auto& cities = _repo->cities();
    auto isBlocked = [&](int idx) {
        return idx < 0 || idx >= static_cast<int>(cities.size()) ||
               cities[idx].visitation_status == VisitationStatus::Blocked;
    };
    const size_t maxEdges = _solutionExpandedPath.size() - 1;
    const size_t edgeCount = (_solutionAnimEdgeCount > maxEdges) ? maxEdges : _solutionAnimEdgeCount;
    if (edgeCount == 0) return;

    const size_t currentEdgeIdx = edgeCount - 1;

    if (currentEdgeIdx > 0) {
        gui::Shape completedShape;
        auto completed = completedShape.createBezier(4, td::LinePattern::Solid);

        for (size_t i = 0; i < currentEdgeIdx; ++i) {
            int u = _solutionExpandedPath[i];
            int v = _solutionExpandedPath[i + 1];
            if (isBlocked(u) || isBlocked(v)) continue;

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            completed.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            completed.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
        }
        completedShape.drawWire(td::ColorID::Red);
    }

    {
        int u = _solutionExpandedPath[currentEdgeIdx];
        int v = _solutionExpandedPath[currentEdgeIdx + 1];
        if (!isBlocked(u) && !isBlocked(v)) {
            gui::Shape currentShape;
            auto curr = currentShape.createBezier(6, td::LinePattern::Solid);

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            curr.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            curr.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
            currentShape.drawWire(td::ColorID::Green);

            float avgScale = _scaleX;
            float scaledSize = MapPointStyle::Size * avgScale;
            float scaledBorder = MapPointStyle::BorderOffset * avgScale;

            auto cc = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            float ix = cc.first - (scaledSize * 0.5f);   // ili koristi tačno kako ti je MapPointStyle centar definisan
            float iy = cc.second - (scaledSize * 0.5f);

            gui::Rect overlayRect(
                ix - scaledBorder - 4 * avgScale,
                iy - scaledBorder - 4 * avgScale,
                ix + scaledSize + scaledBorder + 4 * avgScale,
                iy + scaledSize + scaledBorder + 4 * avgScale
            );

            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::LightGreen, td::ColorID::DarkGreen, 2.0f);
        }
    }
}
