#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include "DataRepository.h"
#include <vector>
#include <set>
#include <string>
#include <gui/Timer.h>


class SearchAlgorithm;
class TSPAlgorithm;

// Original design dimensions (from JSON coordinates and background image)
constexpr float ORIGINAL_MAP_WIDTH = 1000.0f;
constexpr float ORIGINAL_MAP_HEIGHT = 866.0f;

// MapView: Rendering-only canvas that draws cities and roads
// Data operations are delegated to DataRepository
class MapView : public gui::Canvas
{
public:
    MapView();
    ~MapView() = default;

    // Attach the repository and subscribe to changes
    void setRepository(DataRepository* repo);
    
    // Attach solver for visualization
    void setSolver(SearchAlgorithm* solver);
    
    // Read-only access to underlying repository (for queries)
    DataRepository* repository() const { return _repo; }
    
    // Public method to trigger redraw (wraps protected reDraw)
    void refresh() { reDraw(); }
    
    // Get current scale factors for coordinate transformation
    float getScaleX() const { return _scaleX; }
    float getScaleY() const { return _scaleY; }

    void startSolutionAnimation();
    void stopSolutionAnimation();
    bool isSolutionAnimating() const { return _solutionAnimating; }

    // returns true if still animating, false if finished
    bool advanceSolutionAnimation(size_t edgesPerTick);



protected:
    bool onTimer(gui::Timer* pTimer) override;

    void onDraw(const gui::Rect& rect) override;
    void onResize(const gui::Size& newSize) override;

private:
    void loadBackground();
    void drawAlgorithmState();
    void drawPath(const std::vector<int>& path);
    void drawTSPState();
    void drawTour(const std::vector<int>& tour, td::ColorID color, float lineWidth);
    bool getUnreachableGoalsMessage(std::string& outMessage) const;

    // Animation helpers (main)
    bool buildExpandedTourPath(const std::vector<int>& tour, std::vector<int>& outPath) const;
    void drawAnimatedSolution();

    // Transform original coordinates to current view coordinates (responsiveness)
    float scaleX(float x) const { return x * _scaleX + _offsetX; }
    float scaleY(float y) const { return y * _scaleY + _offsetY; }
    std::pair<float, float> scalePoint(float x, float y) const { return { scaleX(x), scaleY(y) }; }

    DataRepository* _repo = nullptr;
    SearchAlgorithm* _solver = nullptr;
    gui::Image _bgImage;
    bool _bgLoaded = false;

    // Current view size + scale/offset (responsiveness)
    gui::Size _currentSize;
    float _scaleX = 1.0f;
    float _scaleY = 1.0f;
    float _offsetX = 0.0f;
    float _offsetY = 0.0f;

    // Animation state (main)
    gui::Timer _solutionTimer;
    bool _solutionAnimating = false;
    std::vector<int> _solutionExpandedPath;
    size_t _solutionAnimEdgeCount = 0;

    


};
