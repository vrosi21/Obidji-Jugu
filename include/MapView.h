#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include "DataRepository.h"
#include <vector>
#include <set>

class SearchAlgorithm;
class TSPAlgorithm;

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

protected:
    void onDraw(const gui::Rect& rect) override;

private:
    void loadBackground();
    void drawAlgorithmState();
    void drawPath(const std::vector<int>& path);
    void drawTSPState();
    void drawTour(const std::vector<int>& tour, td::ColorID color, float lineWidth);
    
    DataRepository* _repo = nullptr;
    SearchAlgorithm* _solver = nullptr;
    gui::Image _bgImage;
    bool _bgLoaded = false;
};
