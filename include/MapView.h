#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include "DataRepository.h"
#include "MapGeometry.h"
#include <vector>
#include <set>
#include <memory>
#include <string>
#include <functional>
#include <gui/Timer.h>


class SearchAlgorithm;
class TSPAlgorithm;

enum class PanelSide { Left, Right };

// Logical vector-map dimensions shared by generated OSM geometry and graph cities.
constexpr float ORIGINAL_MAP_WIDTH = 1000.0f;
constexpr float ORIGINAL_MAP_HEIGHT = 866.0f;

// MapView: Rendering-only canvas that draws cities and roads
// Data operations are delegated to DataRepository
class MapView : public gui::Canvas
{
    bool _hasExactMinimum = false;
    double _exactMinimum = 0.0;
    bool _benchmarkExact = true;
public:
    void clearExactMinimum() { _hasExactMinimum = false; }
    void setBenchmark(double cost, bool exact) {
        _exactMinimum = cost; _hasExactMinimum = true; _benchmarkExact = exact;
    }

    using CityClickCallback = std::function<void(int cityIdx)>;

    MapView();
    ~MapView() = default;

    // Attach the repository and subscribe to changes
    void setRepository(DataRepository* repo);
    
    // Attach solver for visualization
    void setSolver(SearchAlgorithm* solver);
    
    // Read-only access to underlying repository (for queries)
    DataRepository* repository() const { return _repo; }
    
    // Public method to trigger redraw (wraps protected reDraw)
    // Explicit UI/data changes redraw the current frame. Resize throttling is
    // controlled independently by the resize activity state.
    void refresh() { reDraw(); }
    // Repository geometry changed: rebuild the cached road path, then redraw.
    void refreshData();
    
    // Get current scale factors for coordinate transformation
    float getScaleX() const { return _scaleX; }
    float getScaleY() const { return _scaleY; }

    void startSolutionAnimation();
    void stopSolutionAnimation();
    bool isSolutionAnimating() const { return _solutionAnimating; }

    // returns true if still animating, false if finished
    bool advanceSolutionAnimation(size_t edgesPerTick);

    // Step-tour animation: animates the current tour edge-by-edge between solver steps
    // For SA: animates the single current tour after each step
    // For GA: queues all population tours and shows each member as a full path per frame
    void startStepTourAnimation(const std::vector<int>& tour);
    void queueStepTourAnimations(const std::vector<std::vector<int>>& tours);
    void queueStepTourFrames(const std::vector<std::vector<int>>& tours);
    bool advanceStepTourAnimation(size_t edgesPerTick);
    bool isStepTourAnimating() const { return _stepTourAnimating; }
    void stopStepTourAnimation();
    size_t getStepTourQueueIndex() const { return _stepTourQueueIdx; }
    size_t getStepTourQueueTotal() const { return _stepTourQueueTotal; }

    // Click callbacks for city interactions
    void setOnPrimaryCityClick(CityClickCallback cb) { _onPrimaryCityClick = std::move(cb); }
    void setOnSecondaryCityClick(CityClickCallback cb) { _onSecondaryCityClick = std::move(cb); }



protected:
    bool onTimer(gui::Timer* pTimer) override;
    void onPrimaryButtonPressed(const gui::InputDevice& inputDevice) override;
    void onSecondaryButtonPressed(const gui::InputDevice& inputDevice) override;

    void onDraw(const gui::Rect& rect) override;
    void onResize(const gui::Size& newSize) override;

private:
    void buildVectorMapShapes();
    void buildStaticMapBitmap();
    void buildRoadShape();
    void handleResizeFrame();
    void drawPreparedLandShapes(float logicalLineWidth) const;
    void drawPreparedCoastShapes(float logicalLineWidth) const;
    void drawResizePreview(const gui::Rect& canvasRect) const;
    void drawVectorMap(const gui::Rect& canvasRect) const;
    void drawDisplayOptions(const gui::Rect& canvasRect);
    bool handleDisplayOptionClick(const gui::Point& point);
    void drawAlgorithmState();
    void drawPath(const std::vector<int>& path);
    void drawTSPState();
    void drawTour(const std::vector<int>& tour, td::ColorID color, float lineWidth);
    void drawTourWithVisitOrder(const std::vector<int>& tour, td::ColorID color, float lineWidth, td::ColorID orderColor);
    void drawInfoPanel(const std::vector<std::pair<std::string, std::string>>& lines, PanelSide side) const;
    bool getUnreachableGoalsMessage(std::string& outMessage) const;

    // Animation helpers (main)
    bool buildExpandedTourPath(const std::vector<int>& tour, std::vector<int>& outPath) const;
    void drawAnimatedSolution();
    void drawStepTourAnimation();
    bool startNextQueuedTour();
    int hitTestCity(const gui::Point& p) const;

    // Transform original coordinates to current view coordinates (responsiveness)
    float scaleX(float x) const { return x * _scaleX + _offsetX; }
    float scaleY(float y) const { return y * _scaleY + _offsetY; }
    std::pair<float, float> scalePoint(float x, float y) const { return { scaleX(x), scaleY(y) }; }

    DataRepository* _repo = nullptr;
    SearchAlgorithm* _solver = nullptr;
    MapGeometry _mapGeometry;
    bool _mapLoaded = false;

    // Static map geometry is compiled into natID shapes once. Resizing only
    // applies a graphics transformation instead of rebuilding every path.
    std::vector<std::unique_ptr<gui::Shape>> _landShapes;
    std::unique_ptr<gui::Shape> _borderShape;
    std::unique_ptr<gui::Shape> _seaMaskShape;
    std::vector<std::unique_ptr<gui::Shape>> _islandShapes;
    std::unique_ptr<gui::Shape> _coastlineShape;
    std::unique_ptr<gui::Shape> _roadShape;
    std::unique_ptr<gui::Image> _staticMapBitmap;

    // Current view size + scale/offset (responsiveness)
    gui::Size _currentSize;
    float _scaleX = 1.0f;
    float _scaleY = 1.0f;
    float _offsetX = 0.0f;
    float _offsetY = 0.0f;
    gui::Rect _lastCanvasRect { 0.0, 0.0, ORIGINAL_MAP_WIDTH, ORIGINAL_MAP_HEIGHT };
    gui::Rect _cityNamesToggleRect { 0.0, 0.0, 0.0, 0.0 };
    gui::Rect _mapToggleRect { 0.0, 0.0, 0.0, 0.0 };
    float _topInfoBandHeight = 0.0f;
    float _bottomOptionsBandHeight = 40.0f;
    bool _useCompactMapBands = false;
    bool _showCityNames = true;
    bool _showMapGeometry = true;

    // Animation state (main)
    gui::Timer _solutionTimer;
    gui::Timer _resizeFrameTimer;
    bool _isResizing = false;
    bool _resizeRedrawPending = false;
    unsigned int _resizeStableTicks = 0;
    bool _solutionAnimating = false;
    std::vector<int> _solutionExpandedPath;
    size_t _solutionAnimEdgeCount = 0;

    // Step-tour animation state (per-step path replay for SA/GA)
    bool _stepTourAnimating = false;
    std::vector<int> _stepTourExpandedPath;
    size_t _stepTourEdgeCount = 0;
    std::vector<std::vector<int>> _stepTourQueue;  // remaining tours (GA population)
    size_t _stepTourQueueIdx = 0;   // current tour index (for display)
    size_t _stepTourQueueTotal = 0; // total tours queued (for display)
    bool _stepTourStaticPerMember = false; // GA mode: each member shown as full path

    CityClickCallback _onPrimaryCityClick;
    CityClickCallback _onSecondaryCityClick;

    


};
