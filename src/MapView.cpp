#include "MapView.h"
#include "MapPoint.h"
#include "SearchAlgorithm.h"
#include "TSPAlgorithm.h"
#include <set>
#include <sstream>
#include <iomanip>
#include "SimulatedAnnealingAlgorithm.h"
#include "GeneticAlgorithmTSP.h"
#include "NearestNeighborAlgorithm.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <filesystem>
#include <gui/Context.h>
#include <gui/Transformation.h>


namespace {
    constexpr float SOLUTION_ANIM_INTERVAL_SEC = 0.05f;
    constexpr size_t SOLUTION_ANIM_SEGMENTS_PER_TICK = 1;
    constexpr float RESIZE_FRAME_INTERVAL_SEC = 1.0f / 30.0f;
    constexpr unsigned int RESIZE_STABLE_TICKS_REQUIRED = 4;
    constexpr float RESIZE_SIZE_EPSILON = 0.75f;

}



MapView::MapView()
    : gui::Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::SecondaryClicks })
    , _currentSize(ORIGINAL_MAP_WIDTH, ORIGINAL_MAP_HEIGHT)
    , _solutionTimer(this, SOLUTION_ANIM_INTERVAL_SEC, false)
    , _resizeFrameTimer(this, RESIZE_FRAME_INTERVAL_SEC, false)
{
    enableResizeEvent(true);
    // Use Timer's direct callback API for resize activity. This avoids relying
    // on timer-message routing through Canvas::onTimer, which is not consistent
    // across the natID backends used by this project.
    _resizeFrameTimer.onTimer([this]() {
        handleResizeFrame();
    });

    const td::String mapPath = getResFileName(":exYuBoundaries");
    _mapLoaded = _mapGeometry.load(std::filesystem::path(mapPath.c_str()));
    if (_mapLoaded) {
        buildVectorMapShapes();
        buildStaticMapBitmap();
    }
}

void MapView::setRepository(DataRepository* repo)
{
    _repo = repo;
    buildRoadShape();
}

void MapView::buildRoadShape()
{
    _roadShape.reset();
    if (!_repo) return;

    const auto& cities = _repo->cities();
    const auto& roads = _repo->roads();
    auto roadShape = std::make_unique<gui::Shape>();
    auto path = roadShape->createBezier(1.0f, td::LinePattern::Solid);
    std::set<std::pair<int, int>> drawn;
    bool hasRoad = false;
    for (const auto& road : roads) {
        const int first = std::min(road.fromId, road.toId);
        const int second = std::max(road.fromId, road.toId);
        if (!drawn.insert({ first, second }).second) continue;
        if (first < 0 || second < 0 ||
            first >= static_cast<int>(cities.size()) ||
            second >= static_cast<int>(cities.size())) continue;

        const auto firstCenter = MapPointStyle::getCenter(cities[first].x, cities[first].y);
        const auto secondCenter = MapPointStyle::getCenter(cities[second].x, cities[second].y);
        path.moveTo({ static_cast<gui::CoordType>(firstCenter.first),
                      static_cast<gui::CoordType>(firstCenter.second) });
        path.lineTo({ static_cast<gui::CoordType>(secondCenter.first),
                      static_cast<gui::CoordType>(secondCenter.second) });
        hasRoad = true;
    }
    if (hasRoad) _roadShape = std::move(roadShape);
}

void MapView::refreshData()
{
    buildRoadShape();
    // Repository mutations (including right-click connection toggles) are
    // interactive and must be visible immediately, even if a resize preview
    // happened just before the click.
    reDraw();
}

void MapView::setSolver(SearchAlgorithm* solver)
{
    _solver = solver;
    stopSolutionAnimation();
    stopStepTourAnimation();
}

void MapView::onResize(const gui::Size& newSize)
{
    // Ignore duplicate and sub-pixel layout noise. A few natID backends can
    // repeatedly alternate fractional dimensions during an otherwise idle
    // layout, which must not be interpreted as continuous user resizing.
    const float widthDelta = std::abs(
        static_cast<float>(newSize.width - _currentSize.width));
    const float heightDelta = std::abs(
        static_cast<float>(newSize.height - _currentSize.height));
    if (widthDelta <= RESIZE_SIZE_EPSILON &&
        heightDelta <= RESIZE_SIZE_EPSILON) return;

    _currentSize = newSize;
    _isResizing = true;
    _resizeRedrawPending = true;
    _resizeStableTicks = 0;

    // The timer redraws at most once per frame while sizes arrive and declares
    // resize complete only after several ticks without a new accepted size.
    if (!_resizeFrameTimer.isRunning()) _resizeFrameTimer.start();
}

void MapView::handleResizeFrame()
{
    if (!_isResizing) {
        _resizeFrameTimer.stop();
        return;
    }

    if (_resizeRedrawPending) {
        _resizeRedrawPending = false;
        _resizeStableTicks = 0;
        reDraw();
    } else {
        ++_resizeStableTicks;
        if (_resizeStableTicks >= RESIZE_STABLE_TICKS_REQUIRED) {
            _resizeFrameTimer.stop();
            _resizeStableTicks = 0;
            _isResizing = false;
            reDraw(); // final full vector frame with all live layers
            return;
        }
    }

    // natID Timer remains active until stop(); the direct callback above keeps
    // observing stable frames without restarting or resetting its interval.
}

void MapView::onPrimaryButtonPressed(const gui::InputDevice& inputDevice)
{
    const gui::Point& point = inputDevice.getFramePoint();
    if (handleDisplayOptionClick(point)) return;

    int idx = hitTestCity(point);
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
    _lastCanvasRect = rect;

    // Keep a small options band available at every splitter position. Very
    // narrow cells use two rows so the controls remain readable and clickable.
    float viewW = static_cast<float>(rect.right - rect.left);
    float viewH = static_cast<float>(rect.bottom - rect.top);
    _bottomOptionsBandHeight = (viewW < 300.0f) ? 62.0f : 40.0f;
    _bottomOptionsBandHeight = std::min(_bottomOptionsBandHeight,
                                        std::max(24.0f, viewH * 0.20f));

    // If width is the limiting dimension and there is enough vertical
    // letterbox space, move the algorithm card into a dedicated full-width
    // header instead of covering the map.
    const float baseMapHeight = std::max(1.0f, viewH - _bottomOptionsBandHeight);
    const float naturalMapHeight = viewW * ORIGINAL_MAP_HEIGHT / ORIGINAL_MAP_WIDTH;
    const float naturalVerticalSpace = baseMapHeight - naturalMapHeight;
    _useCompactMapBands = naturalVerticalSpace >= 76.0f;
    _topInfoBandHeight = (_useCompactMapBands && _solver) ? 92.0f : 0.0f;
    _topInfoBandHeight = std::min(_topInfoBandHeight,
                                  std::max(0.0f, baseMapHeight - 80.0f));

    const float mapAreaTop = static_cast<float>(rect.top) + _topInfoBandHeight;
    const float mapAreaH = std::max(
        1.0f, viewH - _topInfoBandHeight - _bottomOptionsBandHeight);

    // Fit the logical map into the remaining area without stretching it.
    float scale  = std::min(viewW / ORIGINAL_MAP_WIDTH,
                            mapAreaH / ORIGINAL_MAP_HEIGHT);
    float mapW   = ORIGINAL_MAP_WIDTH * scale;
    float mapH   = ORIGINAL_MAP_HEIGHT * scale;

    float offsetX = rect.left + std::max(0.0f, (viewW - mapW) / 2.0f);
    float offsetY = mapAreaTop + std::max(0.0f, (mapAreaH - mapH) / 2.0f);

    // store for other methods
    _scaleX = scale;
    _scaleY = scale;
    _offsetX = offsetX;
    _offsetY = offsetY;

    if (!_showMapGeometry) {
        // Map-off mode intentionally leaves only the road graph and cities.
        gui::Shape::drawRect(rect, td::ColorID::LightBlue);
    } else if (_isResizing && _staticMapBitmap && _staticMapBitmap->isOK()) {
        drawResizePreview(rect);
    } else {
        drawVectorMap(rect);
    }

    if (!_repo) return;

    const auto& cities = _repo->cities();
    // The road network is also a persistent logical-coordinate shape. Resize
    // events transform it in-place instead of rebuilding every segment.
    if (_roadShape) {
        gui::Context context;
        gui::Transformation transform;
        transform.translate(static_cast<gui::CoordType>(_offsetX),
                            static_cast<gui::CoordType>(_offsetY));
        transform.scale(static_cast<gui::CoordType>(_scaleX));
        transform.appendToContext();
        const float logicalLineWidth = std::max(1.0f, 0.5f / std::max(_scaleX, 0.01f));
        _roadShape->drawWire(td::ColorID::Black, logicalLineWidth);
    }

    // Expensive live visualization is skipped only during an actively detected
    // resize and restored by the final stable-frame redraw.
    if (!_isResizing && _solver) {
        drawAlgorithmState();
    }

    // Draw warning if not all goals are reachable
    std::string unreachableMsg;
    if (!_isResizing && getUnreachableGoalsMessage(unreachableMsg)) {
        gui::DrawableString msg(unreachableMsg.c_str());
        gui::Point pos(static_cast<gui::CoordType>(_offsetX + 10.0f), static_cast<gui::CoordType>(_offsetY + 20.0f));
        msg.draw(pos, gui::Font::ID::SystemNormal, td::ColorID::Red);
    }

    // Draw cities on top using scaled MapPointRenderer
    for (size_t i = 0; i < cities.size(); ++i) {
        const auto& city = cities[i];
        // Markers remain useful during resize; text is restored only on the
        // final stable frame to keep interactive window movement inexpensive.
        MapPointRenderer::drawScaled(city, _scaleX, _offsetX, _offsetY,
                                     !_isResizing && _showCityNames);
    }

    drawDisplayOptions(rect);
}

bool MapView::handleDisplayOptionClick(const gui::Point& point)
{
    if (_cityNamesToggleRect.contains(point)) {
        _showCityNames = !_showCityNames;
        reDraw();
        return true;
    }
    if (_mapToggleRect.contains(point)) {
        _showMapGeometry = !_showMapGeometry;
        reDraw();
        return true;
    }
    return false;
}

void MapView::drawDisplayOptions(const gui::Rect& canvasRect)
{
    const float left = static_cast<float>(canvasRect.left);
    const float right = static_cast<float>(canvasRect.right);
    const float bottom = static_cast<float>(canvasRect.bottom);
    const float top = std::max(static_cast<float>(canvasRect.top),
                               bottom - _bottomOptionsBandHeight);
    const float width = std::max(1.0f, right - left);

    gui::Rect barRect(left, top, right, bottom);
    gui::Shape::drawRect(barRect, td::ColorID::WhiteSmoke,
                         td::ColorID::Silver, 1.0f);

    auto drawToggle = [](float x, float y, const char* text, bool checked,
                         float hitWidth, gui::Rect& hitRect) {
        constexpr float boxSize = 15.0f;
        gui::Rect boxRect(x, y, x + boxSize, y + boxSize);
        gui::Shape::drawRect(boxRect,
                             checked ? td::ColorID::DodgerBlue : td::ColorID::White,
                             td::ColorID::DarkSlateGray, 1.0f);

        if (checked) {
            gui::Shape tickShape;
            auto tick = tickShape.createBezier(2.0f, td::LinePattern::Solid);
            tick.moveTo({ static_cast<gui::CoordType>(x + 3.0f),
                          static_cast<gui::CoordType>(y + 8.0f) });
            tick.lineTo({ static_cast<gui::CoordType>(x + 6.5f),
                          static_cast<gui::CoordType>(y + 11.5f) });
            tick.lineTo({ static_cast<gui::CoordType>(x + 12.5f),
                          static_cast<gui::CoordType>(y + 3.5f) });
            tickShape.drawWire(td::ColorID::White);
        }

        gui::DrawableString label(text);
        label.draw({ static_cast<gui::CoordType>(x + boxSize + 6.0f),
                     static_cast<gui::CoordType>(y - 1.0f) },
                   gui::Font::ID::SystemSmaller, td::ColorID::DarkSlateGray);

        hitRect = gui::Rect(x - 4.0f, y - 5.0f,
                            x + hitWidth, y + boxSize + 5.0f);
    };

    const float x = left + 10.0f;
    if (width < 300.0f) {
        drawToggle(x, top + 8.0f, "City names", _showCityNames,
                   std::max(1.0f, width - 16.0f), _cityNamesToggleRect);
        drawToggle(x, top + 35.0f, "Map", _showMapGeometry,
                   std::max(1.0f, width - 16.0f), _mapToggleRect);
    } else {
        drawToggle(x, top + 12.0f, "City names", _showCityNames,
                   122.0f, _cityNamesToggleRect);
        drawToggle(x + 135.0f, top + 12.0f, "Map", _showMapGeometry,
                   78.0f, _mapToggleRect);
    }
}

void MapView::buildVectorMapShapes()
{
    _landShapes.clear();
    _borderShape.reset();
    _seaMaskShape.reset();
    _islandShapes.clear();
    _coastlineShape.reset();
    if (!_mapLoaded || _mapGeometry.empty()) return;

    constexpr float logicalLineWidth = 1.15f;
    auto toPoint = [](const MapCoordinate& point) {
        return gui::Point(static_cast<gui::CoordType>(point.x),
                          static_cast<gui::CoordType>(point.y));
    };

    // Filled country polygons are static and can be compiled once by natID.
    for (const auto& country : _mapGeometry.countries()) {
        for (const auto& polygon : country.polygons) {
            for (const auto& ring : polygon.rings) {
                if (ring.isHole || ring.points.size() < 3) continue;
                std::vector<gui::Point> points;
                points.reserve(ring.points.size());
                for (const auto& point : ring.points) points.push_back(toPoint(point));
                auto land = std::make_unique<gui::Shape>();
                land->createPolygon(points.data(), points.size(), logicalLineWidth);
                _landShapes.push_back(std::move(land));
            }
        }
    }

    // Country relations contain the same shared edge in both directions. Build
    // one persistent border path and keep each undirected segment only once.
    using PointKey = std::pair<int, int>;
    using EdgeKey = std::pair<PointKey, PointKey>;
    auto pointKey = [](const MapCoordinate& point) -> PointKey {
        return { static_cast<int>(std::lround(point.x * 1000.0f)),
                 static_cast<int>(std::lround(point.y * 1000.0f)) };
    };
    std::set<EdgeKey> uniqueEdges;
    _borderShape = std::make_unique<gui::Shape>();
    auto borderPath = _borderShape->createBezier(logicalLineWidth, td::LinePattern::Solid);
    bool hasBorder = false;
    for (const auto& country : _mapGeometry.countries()) {
        for (const auto& polygon : country.polygons) {
            for (const auto& ring : polygon.rings) {
                if (ring.points.size() < 2) continue;
                for (size_t i = 1; i < ring.points.size(); ++i) {
                    const auto& start = ring.points[i - 1];
                    const auto& end = ring.points[i];
                    auto startKey = pointKey(start);
                    auto endKey = pointKey(end);
                    if (endKey < startKey) std::swap(startKey, endKey);
                    if (!uniqueEdges.insert({ startKey, endKey }).second) continue;
                    borderPath.moveTo(toPoint(start));
                    borderPath.lineTo(toPoint(end));
                    hasBorder = true;
                }
            }
        }
    }
    if (!hasBorder) _borderShape.reset();

    const MapCoastline* mainlandCoast = nullptr;
    for (const auto& coastline : _mapGeometry.coastlines()) {
        if (!coastline.closed && (!mainlandCoast || coastline.points.size() > mainlandCoast->points.size())) {
            mainlandCoast = &coastline;
        }
    }
    if (mainlandCoast && mainlandCoast->points.size() >= 2) {
        std::vector<gui::Point> points;
        points.reserve(mainlandCoast->points.size() + 4);
        for (const auto& point : mainlandCoast->points) points.push_back(toPoint(point));
        const auto& start = mainlandCoast->points.front();
        const auto& end = mainlandCoast->points.back();
        points.emplace_back(static_cast<gui::CoordType>(end.x), ORIGINAL_MAP_HEIGHT);
        points.emplace_back(0.0f, ORIGINAL_MAP_HEIGHT);
        points.emplace_back(0.0f, 0.0f);
        points.emplace_back(static_cast<gui::CoordType>(start.x), 0.0f);
        _seaMaskShape = std::make_unique<gui::Shape>();
        _seaMaskShape->createPolygon(points.data(), points.size(), 0.0f);
    }

    for (const auto& coastline : _mapGeometry.coastlines()) {
        if (!coastline.closed || coastline.points.size() < 3) continue;
        std::vector<gui::Point> points;
        points.reserve(coastline.points.size());
        for (const auto& point : coastline.points) points.push_back(toPoint(point));
        auto island = std::make_unique<gui::Shape>();
        island->createPolygon(points.data(), points.size(), logicalLineWidth);
        _islandShapes.push_back(std::move(island));
    }

    _coastlineShape = std::make_unique<gui::Shape>();
    auto coastlinePath = _coastlineShape->createBezier(logicalLineWidth, td::LinePattern::Solid);
    bool hasCoastline = false;
    for (const auto& coastline : _mapGeometry.coastlines()) {
        if (coastline.points.size() < 2) continue;
        coastlinePath.moveTo(toPoint(coastline.points.front()));
        for (size_t i = 1; i < coastline.points.size(); ++i) {
            coastlinePath.lineTo(toPoint(coastline.points[i]),
                                 coastline.closed && i + 1 == coastline.points.size());
        }
        hasCoastline = true;
    }
    if (!hasCoastline) _coastlineShape.reset();
}

void MapView::drawPreparedLandShapes(float logicalLineWidth) const
{
    for (const auto& land : _landShapes) land->drawFill(td::ColorID::WhiteSmoke);
    if (_borderShape) _borderShape->drawWire(td::ColorID::DarkSlateGray, logicalLineWidth);
}

void MapView::drawPreparedCoastShapes(float logicalLineWidth) const
{
    if (_seaMaskShape) _seaMaskShape->drawFill(td::ColorID::LightBlue);
    for (const auto& island : _islandShapes) island->drawFill(td::ColorID::WhiteSmoke);
    if (_coastlineShape) _coastlineShape->drawWire(td::ColorID::DarkSlateGray, logicalLineWidth);
}

void MapView::buildStaticMapBitmap()
{
    _staticMapBitmap.reset();
    if (!_mapLoaded || _mapGeometry.empty() || _landShapes.empty()) return;

    auto bitmap = std::make_unique<gui::Image>(
        gui::Size(static_cast<gui::CoordType>(ORIGINAL_MAP_WIDTH),
                  static_cast<gui::CoordType>(ORIGINAL_MAP_HEIGHT)));
    if (!bitmap->isOK()) return;

    bitmap->startDrawingContext(true, td::ColorID::LightBlue);
    // natID's off-screen backend resolves a large LightBlue polygon differently
    // from the display backend. Cache only the expensive land/border layer;
    // the small coastline overlay is drawn live with the correct display color.
    drawPreparedLandShapes(1.15f);
    bitmap->releaseDrawingContext();
    _staticMapBitmap = std::move(bitmap);
}

void MapView::drawResizePreview(const gui::Rect& canvasRect) const
{
    gui::Shape::drawRect(canvasRect, td::ColorID::LightBlue);
    if (!_staticMapBitmap || !_staticMapBitmap->isOK()) {
        drawVectorMap(canvasRect);
        return;
    }

    gui::Rect mapRect(
        static_cast<gui::CoordType>(_offsetX),
        static_cast<gui::CoordType>(_offsetY),
        static_cast<gui::CoordType>(_offsetX + ORIGINAL_MAP_WIDTH * _scaleX),
        static_cast<gui::CoordType>(_offsetY + ORIGINAL_MAP_HEIGHT * _scaleY));
    // The target rectangle has the same 1000:866 aspect as the bitmap.
    _staticMapBitmap->draw(mapRect, gui::Image::AspectRatio::No);

    gui::Context context;
    gui::Transformation transform;
    transform.translate(static_cast<gui::CoordType>(_offsetX),
                        static_cast<gui::CoordType>(_offsetY));
    transform.scale(static_cast<gui::CoordType>(_scaleX));
    transform.appendToContext();
    const float logicalLineWidth = std::max(1.15f, 0.65f / std::max(_scaleX, 0.01f));
    drawPreparedCoastShapes(logicalLineWidth);
}

void MapView::drawVectorMap(const gui::Rect& canvasRect) const
{
    gui::Shape::drawRect(canvasRect, td::ColorID::LightBlue);

    if (!_mapLoaded || _mapGeometry.empty() || _landShapes.empty()) {
        gui::Rect mapRect(
            static_cast<gui::CoordType>(_offsetX),
            static_cast<gui::CoordType>(_offsetY),
            static_cast<gui::CoordType>(_offsetX + ORIGINAL_MAP_WIDTH * _scaleX),
            static_cast<gui::CoordType>(_offsetY + ORIGINAL_MAP_HEIGHT * _scaleY));
        gui::Shape::drawRect(mapRect, td::ColorID::WhiteSmoke,
                            td::ColorID::DarkSlateGray, 1.0f);
        return;
    }

    gui::Context context;
    gui::Transformation transform;
    transform.translate(static_cast<gui::CoordType>(_offsetX),
                        static_cast<gui::CoordType>(_offsetY));
    transform.scale(static_cast<gui::CoordType>(_scaleX));
    transform.appendToContext();

    // Compensate for very small windows so the transformed stroke stays visible.
    const float logicalLineWidth = std::max(1.15f, 0.65f / std::max(_scaleX, 0.01f));
    drawPreparedLandShapes(logicalLineWidth);
    drawPreparedCoastShapes(logicalLineWidth);
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
    

    TSPAlgorithm* tspSolver = dynamic_cast<TSPAlgorithm*>(_solver);
    if (!tspSolver || !_repo) return;

    const auto& cities = _repo->cities();
    float avgScale = _scaleX;
    float scaledSize = MapPointStyle::Size * avgScale;
    float scaledBorder = MapPointStyle::BorderOffset * avgScale;

    const auto& currentTour = tspSolver->getTour();
    const auto& bestTour = tspSolver->getBestTour();
    // Match the route table: unweighted distance along the expanded road route,
    // including the return leg, rather than the solver's weighted objective.
    const auto& distanceTour = bestTour.empty() ? currentTour : bestTour;
    double totalDistance = 0.0;
    for (size_t i = 0; i < distanceTour.size(); ++i) {
        std::vector<int> leg;
        if (!_repo->getShortestRoadPath(distanceTour[i],
                distanceTour[(i + 1) % distanceTour.size()], leg)) {
            totalDistance = std::numeric_limits<double>::infinity();
            break;
        }
        for (size_t j = 1; j < leg.size(); ++j)
            totalDistance += _repo->getMetricDistance(leg[j - 1], leg[j]);
    }

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
            drawTour(bestTour, td::ColorID::Blue, 1.5f * avgScale);
        }
        if (!currentTour.empty()) {
            drawTour(currentTour, td::ColorID::Green, 2.5f * avgScale);
        }
    }

    if (_solutionAnimating) {
        drawAnimatedSolution();

    }

    if (_stepTourAnimating) {
        drawStepTourAnimation();

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
        infoLines.push_back({"Total distance", fmtCost(totalDistance)});
    } else if (sa) {
        infoLines.push_back({"Algorithm", "Simulated Annealing"});
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        std::ostringstream ts; ts << std::fixed << std::setprecision(2) << sa->getTemperature();
        infoLines.push_back({"Temperature", ts.str()});
        infoLines.push_back({"Total distance", fmtCost(totalDistance)});
    } else if (nn) {
        infoLines.push_back({"Algorithm", "Nearest Neighbor"});
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        infoLines.push_back({"Total distance", fmtCost(totalDistance)});
        if (nn->is2OptEnabled()) {
            if (nn->isIn2OptPhase()) {
                infoLines.push_back({"2-opt", "improving (" + std::to_string(nn->getRemaining2OptCycles()) + " left)"});
            } else {
                infoLines.push_back({"2-opt", "enabled"});
            }
        }
    } else {
        infoLines.push_back({"Step", std::to_string(tspSolver->getCurrentStep())});
        infoLines.push_back({"Total distance", fmtCost(totalDistance)});
    }

    drawInfoPanel(infoLines, PanelSide::Right);
}

void MapView::drawInfoPanel(const std::vector<std::pair<std::string, std::string>>& lines,
    PanelSide side) const
{
    if (lines.empty()) return;

    const float mapLeft = _offsetX;
    const float mapRight = _offsetX + ORIGINAL_MAP_WIDTH * _scaleX;
    const float mapTop = _offsetY;
    const float mapBottom = _offsetY + ORIGINAL_MAP_HEIGHT * _scaleY;
    const float mapWidth = std::max(1.0f, mapRight - mapLeft);
    const float mapHeight = std::max(1.0f, mapBottom - mapTop);

    gui::Font::ID infoFont = gui::Font::ID::SystemNormal;
    float lineH = 18.0f;
    if (mapWidth < 650.0f) {
        infoFont = gui::Font::ID::SystemSmallest;
        lineH = 13.0f;
    } else if (mapWidth < 900.0f) {
        infoFont = gui::Font::ID::SystemSmaller;
        lineH = 15.0f;
    }

    float panelX = 0.0f;
    float panelY = 0.0f;
    float panelW = 0.0f;
    float panelH = 0.0f;
    float paddingX = 0.0f;
    float paddingY = 0.0f;

    const bool useFullWidthHeader =
        side == PanelSide::Right && _useCompactMapBands && _topInfoBandHeight >= 50.0f;

    if (useFullWidthHeader) {
        // In a narrow splitter cell the map is width-limited, leaving useful
        // vertical space. Put the main algorithm card in that top band and
        // use the full cell width instead of covering the map.
        const float canvasLeft = static_cast<float>(_lastCanvasRect.left);
        const float canvasRight = static_cast<float>(_lastCanvasRect.right);
        const float canvasTop = static_cast<float>(_lastCanvasRect.top);
        const float canvasWidth = std::max(1.0f, canvasRight - canvasLeft);
        const float margin = std::clamp(canvasWidth * 0.015f, 4.0f, 8.0f);

        panelX = canvasLeft + margin;
        panelY = canvasTop + margin;
        panelW = std::max(1.0f, canvasWidth - 2.0f * margin);
        paddingX = std::clamp(panelW * 0.025f, 5.0f, 10.0f);
        paddingY = 5.0f;
        const float wantedPanelH =
            lineH * static_cast<float>(lines.size()) + 2.0f * paddingY;
        panelH = std::min(wantedPanelH,
                          std::max(1.0f, _topInfoBandHeight - 2.0f * margin));
    } else {
        // Normal/wide mode keeps the compact overlay inside the map.
        const float margin = std::clamp(mapWidth * 0.012f, 4.0f, 12.0f);
        const float maxPanelW = std::max(1.0f, mapWidth - 2.0f * margin);
        panelW = std::min(
            std::clamp(mapWidth * 0.27f, 150.0f, 280.0f), maxPanelW);
        paddingX = std::clamp(panelW * 0.03f, 4.0f, 8.0f);
        paddingY = std::clamp(panelW * 0.02f, 3.0f, 6.0f);
        const float wantedPanelH =
            lineH * static_cast<float>(lines.size()) + 2.0f * paddingY;
        panelH = std::min(
            wantedPanelH, std::max(1.0f, mapHeight - 2.0f * margin));

        panelX = (side == PanelSide::Left)
            ? (mapLeft + margin)
            : (mapRight - panelW - margin);
        panelX = std::clamp(panelX, mapLeft,
                            std::max(mapLeft, mapRight - panelW));
        panelY = std::clamp(
            mapTop + margin, mapTop, std::max(mapTop, mapBottom - panelH));
    }

    gui::Rect bgRect(
        (gui::CoordType)panelX,
        (gui::CoordType)panelY,
        (gui::CoordType)(panelX + panelW),
        (gui::CoordType)(panelY + panelH)
    );

    gui::Shape bgShape;
    bgShape.createRect(bgRect);
    bgShape.drawFillAndWire(td::ColorID::WhiteSmoke, td::ColorID::Silver, 1.0f);

    float textX = panelX + paddingX;
    float textY = panelY + paddingY;
    for (const auto& kv : lines) {
        std::string txt = kv.first + ":  " + kv.second;
        gui::DrawableString ds(txt.c_str());
        gui::Point pos((gui::CoordType)textX, (gui::CoordType)textY);
        ds.draw(pos, infoFont, td::ColorID::DarkSlateGray);
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

        if (!_isResizing) reDraw();
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
        completedShape.drawWire(td::ColorID::Blue);
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
        completedShape.drawWire(td::ColorID::Green);
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
            overlay.drawFillAndWire(td::ColorID::Gold, td::ColorID::Red, 1.5f * avgScale);
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





