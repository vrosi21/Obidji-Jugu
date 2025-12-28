#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include "DataRepository.h"
#include <vector>

// MapView: Rendering-only canvas that draws cities and roads
// Data operations are delegated to DataRepository
class MapView : public gui::Canvas
{
public:
    MapView();
    ~MapView() = default;

    // Attach the repository and subscribe to changes
    void setRepository(DataRepository* repo);
    
    // Read-only access to underlying repository (for queries)
    DataRepository* repository() const { return _repo; }

protected:
    void onDraw(const gui::Rect& rect) override;

private:
    void loadBackground();
    DataRepository* _repo = nullptr;
    gui::Image _bgImage;
    bool _bgLoaded = false;
};
