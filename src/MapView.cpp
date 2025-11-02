#include "MapView.h"

MapView::MapView()
{
    // Empty canvas for now - render nothing but a plain background
}

void MapView::onDraw(const gui::Rect& rect)
{
    // Clear the area with a plain background (white)
    gui::Shape bg;
    bg.createRect(rect);
    bg.drawFillAndWire(td::ColorID::White, td::ColorID::Black, 0.0f);
}
