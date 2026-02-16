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
#include "GeneticAlgorithmTSP.h"
#include "NearestNeighborAlgorithm.h"
#include <algorithm>
#include <limits>


namespace {
    void dbg(const char* m) { OutputDebugStringA(m); }
    constexpr float SOLUTION_ANIM_INTERVAL_SEC = 0.05f;
    constexpr size_t SOLUTION_ANIM_SEGMENTS_PER_TICK = 1;

}
MapView::MapView()
    : gui::Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::SecondaryClicks })
    , _currentSize(ORIGINAL_MAP_WIDTH, ORIGINAL_MAP_HEIGHT)
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
    stopStepTourAnimation();
}

void MapView::onResize(const gui::Size& newSize)
{
    _currentSize = newSize;
}

void MapView::onPrimaryButtonPressed(const gui::InputDevice& inputDevice)
{
    int idx = hitTestCity(inputDevice.getFramePoint());
    if (idx >= 0 && _onPrimaryCityClick) {
        _onPrimaryCityClick(idx);
    }
}

void MapView::onSecondaryButtonPressed(const gui::InputDevice& inputDevice)
{
    int idx = hitTestCity(inputDevice.getFramePoint());
    if (idx >= 0 && _onSecondaryCityClick) {
        _onSecondaryCityClick(idx);
    }
}

int MapView::hitTestCity(const gui::Point& p) const
{
    if (!_repo) return -1;
    const auto& cities = _repo->cities();

    const float scaledSize = MapPointStyle::Size * _scaleX;
    const float scaledBorder = MapPointStyle::BorderOffset * _scaleX;

    for (int i = static_cast<int>(cities.size()) - 1; i >= 0; --i) {
        const auto& c = cities[i];
        float sx = _offsetX + static_cast<float>(c.x) * _scaleX;
        float sy = _offsetY + static_cast<float>(c.y) * _scaleY;

        float left = sx - scaledBorder;
        float top = sy - scaledBorder;
        float right = sx + scaledSize + scaledBorder;
        float bottom = sy + scaledSize + scaledBorder;

        if (p.x >= left && p.x <= right && p.y >= top && p.y <= bottom) {
            return i;
        }
    }
    return -1;
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

    // Draw warning if not all goals are reachable
    std::string unreachableMsg;
    if (getUnreachableGoalsMessage(unreachableMsg)) {
        gui::DrawableString msg(unreachableMsg.c_str());
        gui::Point pos(static_cast<gui::CoordType>(_offsetX + 10.0f), static_cast<gui::CoordType>(_offsetY + 20.0f));
        msg.draw(pos, gui::Font::ID::SystemNormal, td::ColorID::Red);
    }

    // Draw cities on top using scaled MapPointRenderer
    for (const auto& city : cities) {
        MapPointRenderer::drawScaled(city, _scaleX, _offsetX, _offsetY);
    }
}

bool MapView::getUnreachableGoalsMessage(std::string& outMessage) const
{
    outMessage.clear();
    if (!_repo) return false;

    const auto& cities = _repo->cities();
    if (cities.empty()) return false;

    std::vector<int> goals;
    goals.reserve(cities.size());

    int startIdx = -1;
    for (int i = 0; i < static_cast<int>(cities.size()); ++i) {
        if (cities[i].visitation_status == VisitationStatus::Blocked) continue;
        if (cities[i].visitation_status == VisitationStatus::Start) {
            startIdx = i;
        }
        if (cities[i].visitation_status == VisitationStatus::Goal) {
            goals.push_back(i);
        }
    }

    std::vector<std::string> issues;

    if (startIdx < 0) {
        issues.push_back("No Start selected");
    }
    if (goals.empty()) {
        issues.push_back("No Goal selected");
    }

    if (startIdx >= 0 && !goals.empty()) {
        int unreachableCount = 0;
        int reachableCount = 0;
        for (int g : goals) {
            double d = _repo->getMetricDistance(startIdx, g);
            if (std::isfinite(d)) {
                reachableCount++;
            } else {
                unreachableCount++;
            }
        }

        if (reachableCount == 0) {
            issues.push_back("No goals reachable");
        } else if (unreachableCount > 0) {
            std::ostringstream oss;
            oss << "Unreachable goals: " << unreachableCount;
            issues.push_back(oss.str());
        }
    }

    if (issues.empty()) return false;

    std::ostringstream joined;
    for (size_t i = 0; i < issues.size(); ++i) {
        if (i > 0) joined << " | ";
        joined << issues[i];
    }
    outMessage = joined.str();
    return true;
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
        
    }

    if (_stepTourAnimating) {
        drawStepTourAnimation();
        
    }

    TSPAlgorithm* tspSolver = dynamic_cast<TSPAlgorithm*>(_solver);
    if (!tspSolver || !_repo) return;

    const auto& cities = _repo->cities();
    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;

    const auto& currentTour = tspSolver->getTour();
    const auto& bestTour = tspSolver->getBestTour();
    double currentLength = tspSolver->getTourLength();
    double bestLength = tspSolver->getBestLength();

    // --- Determine algorithm type for specialised rendering ---
    auto* nn = dynamic_cast<NearestNeighborAlgorithm*>(tspSolver);
    auto* sa = dynamic_cast<SimulatedAnnealingAlgorithm*>(tspSolver);
    auto* ga = dynamic_cast<GeneticAlgorithmTSP*>(tspSolver);

    // ============================================================
    //  TOUR LINES
    // ============================================================
    if (nn) {
        // NN: single constructive path in vivid blue with visit-order labels
        if (!currentTour.empty()) {
            drawTourWithVisitOrder(currentTour, td::ColorID::DodgerBlue, 3.0f * avgScale, td::ColorID::DodgerBlue);
        }
    } else {
        // SA / GA: show best-so-far as thin dashed reference, current as thick orange
        if (bestTour.size() >= 2 && bestTour != currentTour) {
            drawTour(bestTour, td::ColorID::SteelBlue, 1.5f * avgScale);
        }
        if (!currentTour.empty()) {
            drawTour(currentTour, td::ColorID::DarkOrange, 2.5f * avgScale);
        }
    }

    // ============================================================
    //  CITY HIGHLIGHTS
    // ============================================================
    for (size_t i = 0; i < currentTour.size(); ++i) {
        int idx = currentTour[i];
        if (idx < 0 || idx >= static_cast<int>(cities.size())) continue;
        const auto& city = cities[idx];
        float sx = _offsetX + static_cast<float>(city.x) * _scaleX;
        float sy = _offsetY + static_cast<float>(city.y) * _scaleY;

        float pad = 2.0f * avgScale;
        gui::Rect overlayRect(
            sx - scaledBorder - pad,
            sy - scaledBorder - pad,
            sx + scaledSize + scaledBorder + pad,
            sy + scaledSize + scaledBorder + pad
        );
        gui::Shape overlay;
        overlay.createRect(overlayRect);

        if (nn) {
            // NN head (last city added) gets bright highlight
            if (i == currentTour.size() - 1 && !tspSolver->isComplete()) {
                overlay.drawFillAndWire(td::ColorID::Gold, td::ColorID::OrangeRed, 2.5f * avgScale);
            } else if (i == 0) {
                overlay.drawFillAndWire(td::ColorID::LightSkyBlue, td::ColorID::DarkBlue, 1.5f * avgScale);
            } else {
                overlay.drawWire(td::ColorID::DodgerBlue, 1.2f * avgScale);
            }
        } else {
            // SA/GA: subtle ring around tour members
            if (i == 0) {
                overlay.drawFillAndWire(td::ColorID::LightSalmon, td::ColorID::DarkOrange, 1.5f * avgScale);
            } else {
                overlay.drawWire(td::ColorID::DarkOrange, 1.0f * avgScale);
            }
        }
    }

    // ============================================================
    //  INFO PANEL  (multi-line with background)
    // ============================================================
    std::vector<std::pair<std::string, std::string>> infoLines;

    auto fmtCost = [](double v) -> std::string {
        if (v >= std::numeric_limits<double>::max() || !std::isfinite(v)) return "--";
        std::ostringstream o; o << std::fixed << std::setprecision(1) << v; return o.str();
    };

    if (ga) {
        infoLines.push_back({"Algorithm", "Genetic Algorithm"});
        infoLines.push_back({"Generation", std::to_string(ga->getGeneration()) + " / " + std::to_string(ga->getMaxGenerations())});
        infoLines.push_back({"Best cost", fmtCost(bestLength)});
        infoLines.push_back({"Current cost", fmtCost(currentLength)});
    } else if (sa) {
        infoLines.push_back({"Algorithm", "Simulated Annealing"});
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        std::ostringstream ts; ts << std::fixed << std::setprecision(2) << sa->getTemperature();
        infoLines.push_back({"Temperature", ts.str()});
        infoLines.push_back({"Best cost", fmtCost(bestLength)});
        infoLines.push_back({"Current cost", fmtCost(currentLength)});
    } else if (nn) {
        infoLines.push_back({"Algorithm", "Nearest Neighbor"});
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        infoLines.push_back({"Tour cost", fmtCost(currentLength)});
        if (nn->is2OptEnabled()) {
            if (nn->isIn2OptPhase()) {
                infoLines.push_back({"2-opt", "improving (" + std::to_string(nn->getRemaining2OptCycles()) + " left)"});
            } else {
                infoLines.push_back({"2-opt", "enabled"});
            }
        }
    } else {
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        infoLines.push_back({"Cost", fmtCost(currentLength)});
    }

    drawInfoPanel(infoLines, PanelSide::Right);
}

void MapView::drawInfoPanel(const std::vector<std::pair<std::string, std::string>>& lines,
    PanelSide side) const
{
    if (lines.empty()) return;

    float margin = 8.0f;
    float lineH = 18.0f;
    float panelW = 260.0f;
    float panelH = lineH * static_cast<float>(lines.size()) + 12.0f;

    float mapLeft = _offsetX;
    float mapRight = _offsetX + ORIGINAL_MAP_WIDTH * _scaleX;

    float panelX = (side == PanelSide::Left)
        ? (mapLeft + margin)
        : (mapRight - panelW - margin);

    float panelY = _offsetY + margin;

    gui::Rect bgRect(
        (gui::CoordType)panelX,
        (gui::CoordType)panelY,
        (gui::CoordType)(panelX + panelW),
        (gui::CoordType)(panelY + panelH)
    );

    gui::Shape bgShape;
    bgShape.createRect(bgRect);
    bgShape.drawFillAndWire(td::ColorID::WhiteSmoke, td::ColorID::Silver, 1.0f);

    float textX = panelX + 8.0f;
    float textY = panelY + 6.0f;
    for (const auto& kv : lines) {
        std::string txt = kv.first + ":  " + kv.second;
        gui::DrawableString ds(txt.c_str());
        gui::Point pos((gui::CoordType)textX, (gui::CoordType)textY);
        ds.draw(pos, gui::Font::ID::SystemNormal, td::ColorID::DarkSlateGray);
        textY += lineH;
    }
}
void MapView::drawTourWithVisitOrder(const std::vector<int>& tour, td::ColorID color, float lineWidth, td::ColorID orderColor)
{
    // Draw the tour edges
    drawTour(tour, color, lineWidth);

    if (!_repo || tour.empty()) return;
    const auto& cities = _repo->cities();

    /*
    // Draw visit-order numbers near each city
    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    for (size_t i = 0; i < tour.size(); ++i) {
        int idx = tour[i];
        if (idx < 0 || idx >= static_cast<int>(cities.size())) continue;

        auto cc = MapPointStyle::getScaledCenter(cities[idx].x, cities[idx].y, _scaleX, _offsetX, _offsetY);
        std::string numStr = std::to_string(i + 1);
        gui::DrawableString ds(numStr.c_str());
        gui::Point pos(
            static_cast<gui::CoordType>(cc.first + scaledSize * 0.5f + 2.0f),
            static_cast<gui::CoordType>(cc.second - scaledSize * 0.6f)
        );
        ds.draw(pos, gui::Font::ID::SystemNormal, orderColor);
    }
    */
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

    const std::vector<int>* tourToUse = nullptr;
    if (bestTour.size() >= 2) {
        tourToUse = &bestTour;
    } else if (curTour.size() >= 2) {
        tourToUse = &curTour;
    } else {
        return;
    }

    std::vector<int> expanded;
    if (!buildExpandedTourPath(*tourToUse, expanded)) return;

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

    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;
    const size_t currentEdgeIdx = edgeCount - 1;

    // --- Completed trail: thick forest green ---
    if (currentEdgeIdx > 0) {
        float trailWidth = 4.0f * avgScale;
        if (trailWidth < 2.0f) trailWidth = 2.0f;
        gui::Shape completedShape;
        auto completed = completedShape.createBezier(static_cast<gui::CoordType>(trailWidth), td::LinePattern::Solid);

        for (size_t i = 0; i < currentEdgeIdx; ++i) {
            int u = _solutionExpandedPath[i];
            int v = _solutionExpandedPath[i + 1];
            if (isBlocked(u) || isBlocked(v)) continue;

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            completed.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            completed.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
        }
        completedShape.drawWire(td::ColorID::ForestGreen);
    }

    // --- Leading edge: thicker gold line ---
    {
        int u = _solutionExpandedPath[currentEdgeIdx];
        int v = _solutionExpandedPath[currentEdgeIdx + 1];
        if (!isBlocked(u) && !isBlocked(v)) {
            float headWidth = 5.0f * avgScale;
            if (headWidth < 3.0f) headWidth = 3.0f;
            gui::Shape currentShape;
            auto curr = currentShape.createBezier(static_cast<gui::CoordType>(headWidth), td::LinePattern::Solid);

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            curr.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            curr.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
            currentShape.drawWire(td::ColorID::Gold);

            // Leading-city highlight in gold
            auto cc = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            float ix = static_cast<float>(cc.first) - (scaledSize * 0.5f);
            float iy = static_cast<float>(cc.second) - (scaledSize * 0.5f);

            float pad = 4.0f * avgScale;
            gui::Rect overlayRect(
                ix - scaledBorder - pad,
                iy - scaledBorder - pad,
                ix + scaledSize + scaledBorder + pad,
                iy + scaledSize + scaledBorder + pad
            );

            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::Gold, td::ColorID::DarkOrange, 2.0f * avgScale);
        }
    }

    // --- Progress label ---
    {
        int pct = static_cast<int>((100.0 * edgeCount) / maxEdges);
        std::string progress = "Solution: " + std::to_string(pct) + "%";
        gui::DrawableString ds(progress.c_str());
        gui::Point pos(static_cast<gui::CoordType>(_offsetX + 10.0f), static_cast<gui::CoordType>(_offsetY + 20.0f));
        ds.draw(pos, gui::Font::ID::SystemNormal, td::ColorID::ForestGreen);
    }
}

// ================================================================
//  Step-Tour Animation (per-step path replay for SA / GA)
// ================================================================

void MapView::startStepTourAnimation(const std::vector<int>& tour)
{
    stopStepTourAnimation();
    _stepTourStaticPerMember = false;
    if (!_repo || tour.size() < 2) return;

    std::vector<int> expanded;
    if (!buildExpandedTourPath(tour, expanded)) return;

    _stepTourExpandedPath = std::move(expanded);
    _stepTourEdgeCount = 1;
    _stepTourAnimating = true;
    _stepTourQueue.clear();
    _stepTourQueueIdx = 1;
    _stepTourQueueTotal = 1;
}

void MapView::queueStepTourAnimations(const std::vector<std::vector<int>>& tours)
{
    stopStepTourAnimation();
    _stepTourStaticPerMember = false;
    if (!_repo || tours.empty()) return;

    _stepTourQueue.clear();
    _stepTourQueueTotal = tours.size();
    _stepTourQueueIdx = 0;

    // Put all tours in the queue, start the first one
    for (auto& t : tours) {
        _stepTourQueue.push_back(t);
    }

    startNextQueuedTour();
}

void MapView::queueStepTourFrames(const std::vector<std::vector<int>>& tours)
{
    stopStepTourAnimation();
    _stepTourStaticPerMember = true;
    if (!_repo || tours.empty()) return;

    _stepTourQueue.clear();
    _stepTourQueueTotal = tours.size();
    _stepTourQueueIdx = 0;

    for (const auto& t : tours) {
        _stepTourQueue.push_back(t);
    }

    startNextQueuedTour();
}

bool MapView::startNextQueuedTour()
{
    while (!_stepTourQueue.empty()) {
        std::vector<int> tour = _stepTourQueue.front();
        _stepTourQueue.erase(_stepTourQueue.begin());
        _stepTourQueueIdx = _stepTourQueueTotal - _stepTourQueue.size();

        if (tour.size() < 2) continue;

        std::vector<int> expanded;
        if (!buildExpandedTourPath(tour, expanded)) continue;

        _stepTourExpandedPath = std::move(expanded);
        _stepTourEdgeCount = _stepTourStaticPerMember
            ? (_stepTourExpandedPath.size() > 1 ? _stepTourExpandedPath.size() - 1 : 0)
            : 1;
        _stepTourAnimating = true;
        return true;
    }

    // No more tours
    _stepTourAnimating = false;
    _stepTourExpandedPath.clear();
    _stepTourEdgeCount = 0;
    return false;
}

bool MapView::advanceStepTourAnimation(size_t edgesPerTick)
{
    if (!_stepTourAnimating) return false;

    // GA mode: each queued member is shown as a complete path for one frame period,
    // then we switch to the next member.
    if (_stepTourStaticPerMember) {
        return startNextQueuedTour();
    }

    if (_stepTourExpandedPath.size() < 2) {
        // Try next queued tour
        return startNextQueuedTour();
    }

    const size_t maxEdges = _stepTourExpandedPath.size() - 1;
    if (_stepTourEdgeCount >= maxEdges) {
        // Current tour animation done, try next
        return startNextQueuedTour();
    }

    size_t next = _stepTourEdgeCount + edgesPerTick;
    _stepTourEdgeCount = (next > maxEdges) ? maxEdges : next;

    if (_stepTourEdgeCount >= maxEdges) {
        // Current tour fully drawn, try next
        return startNextQueuedTour();
    }
    return true;
}

void MapView::stopStepTourAnimation()
{
    _stepTourAnimating = false;
    _stepTourExpandedPath.clear();
    _stepTourEdgeCount = 0;
    _stepTourQueue.clear();
    _stepTourQueueIdx = 0;
    _stepTourQueueTotal = 0;
    _stepTourStaticPerMember = false;
}

void MapView::drawStepTourAnimation()
{
    if (!_stepTourAnimating || _stepTourExpandedPath.size() < 2 || !_repo) return;

    const auto& cities = _repo->cities();
    auto isBlocked = [&](int idx) {
        return idx < 0 || idx >= static_cast<int>(cities.size()) ||
               cities[idx].visitation_status == VisitationStatus::Blocked;
    };
    const size_t maxEdges = _stepTourExpandedPath.size() - 1;
    const size_t edgeCount = (_stepTourEdgeCount > maxEdges) ? maxEdges : _stepTourEdgeCount;
    if (edgeCount == 0) return;

    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;
    const size_t currentEdgeIdx = edgeCount - 1;

    // --- Completed trail: dark orange ---
    if (currentEdgeIdx > 0) {
        float trailWidth = 3.0f * avgScale;
        if (trailWidth < 1.5f) trailWidth = 1.5f;
        gui::Shape completedShape;
        auto completed = completedShape.createBezier(static_cast<gui::CoordType>(trailWidth), td::LinePattern::Solid);

        for (size_t i = 0; i < currentEdgeIdx; ++i) {
            int u = _stepTourExpandedPath[i];
            int v = _stepTourExpandedPath[i + 1];
            if (isBlocked(u) || isBlocked(v)) continue;

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            completed.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            completed.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
        }
        completedShape.drawWire(td::ColorID::DarkOrange);
    }

    // --- Leading edge: thicker gold ---
    {
        int u = _stepTourExpandedPath[currentEdgeIdx];
        int v = _stepTourExpandedPath[currentEdgeIdx + 1];
        if (!isBlocked(u) && !isBlocked(v)) {
            float headWidth = 4.0f * avgScale;
            if (headWidth < 2.0f) headWidth = 2.0f;
            gui::Shape currentShape;
            auto curr = currentShape.createBezier(static_cast<gui::CoordType>(headWidth), td::LinePattern::Solid);

            auto c1 = MapPointStyle::getScaledCenter(cities[u].x, cities[u].y, _scaleX, _offsetX, _offsetY);
            auto c2 = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            curr.moveTo({ (gui::CoordType)c1.first, (gui::CoordType)c1.second });
            curr.lineTo({ (gui::CoordType)c2.first, (gui::CoordType)c2.second });
            currentShape.drawWire(td::ColorID::Gold);

            // Leading-city highlight
            auto cc = MapPointStyle::getScaledCenter(cities[v].x, cities[v].y, _scaleX, _offsetX, _offsetY);
            float ix = static_cast<float>(cc.first) - (scaledSize * 0.5f);
            float iy = static_cast<float>(cc.second) - (scaledSize * 0.5f);

            float pad = 3.0f * avgScale;
            gui::Rect overlayRect(
                ix - scaledBorder - pad,
                iy - scaledBorder - pad,
                ix + scaledSize + scaledBorder + pad,
                iy + scaledSize + scaledBorder + pad
            );
            gui::Shape overlay;
            overlay.createRect(overlayRect);
            overlay.drawFillAndWire(td::ColorID::Gold, td::ColorID::DarkOrange, 1.5f * avgScale);
        }
    }

    // --- Info panel ---
    {
        std::vector<std::pair<std::string, std::string>> infoLines;
        int pct = static_cast<int>((100.0 * edgeCount) / maxEdges);

        if (_stepTourQueueTotal > 1) {
            infoLines.push_back({ "Member", std::to_string(_stepTourQueueIdx) + " / " + std::to_string(_stepTourQueueTotal) });
            if (_solver) {
                if (auto* ga = dynamic_cast<GeneticAlgorithmTSP*>(_solver)) {
                    infoLines.push_back({ "Generation", std::to_string(ga->getGeneration()) + " / " + std::to_string(ga->getMaxGenerations()) });
                }
            }
        }
        infoLines.push_back({ "Path", std::to_string(pct) + "%" });
        drawInfoPanel(infoLines, PanelSide::Left);
    }
}





