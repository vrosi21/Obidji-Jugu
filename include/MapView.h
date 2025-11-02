#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>

class MapView : public gui::Canvas
{
public:
    MapView();
    ~MapView() = default;

protected:
    void onDraw(const gui::Rect& rect) override;
};
