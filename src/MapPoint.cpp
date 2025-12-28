#include "MapPoint.h"

// === MapPointStyle Implementation ===

void MapPointStyle::getColors(VisitationStatus status, bool isStart, td::ColorID& borderColor, td::ColorID& centerColor)
{
    borderColor = td::ColorID::Yellow;
    centerColor = td::ColorID::Yellow;
    
    // Start point has cyan center
    if (isStart) {
        centerColor = td::ColorID::Cyan;
    }
    
    switch (status) {
        case VisitationStatus::Blocked:
            borderColor = td::ColorID::Red;
            if (!isStart) centerColor = td::ColorID::Yellow;
            break;
        case VisitationStatus::Goal:
            borderColor = td::ColorID::Yellow;
            if (!isStart) centerColor = td::ColorID::Violet;
            break;
        case VisitationStatus::Open:
        default:
            break;
    }
}

std::pair<double, double> MapPointStyle::getCenter(double x, double y)
{
    return { x + Size / 2.0, y + Size / 2.0 };
}

// === MapPointRenderer Implementation ===

void MapPointRenderer::draw(const CityPoint& mapPoint, int startPointId)
{
    bool isStart = (startPointId >= 0 && mapPoint.id == startPointId);
    td::ColorID borderColor, centerColor;
    MapPointStyle::getColors(mapPoint.visitation_status, isStart, borderColor, centerColor);
    
    int ix = static_cast<int>(mapPoint.x);
    int iy = static_cast<int>(mapPoint.y);
    
    // Border rectangle (larger, behind)
    gui::Rect borderRect(
        ix - MapPointStyle::BorderOffset,
        iy - MapPointStyle::BorderOffset,
        ix + MapPointStyle::Size + MapPointStyle::BorderOffset,
        iy + MapPointStyle::Size + MapPointStyle::BorderOffset
    );
    gui::Shape borderShape;
    borderShape.createRect(borderRect);
    borderShape.drawFillAndWire(borderColor, td::ColorID::Black, 1.0f);
    
    // Center rectangle (inner)
    gui::Rect centerRect(ix, iy, ix + MapPointStyle::Size, iy + MapPointStyle::Size);
    gui::Shape centerShape;
    centerShape.createRect(centerRect);
    centerShape.drawFillAndWire(centerColor, td::ColorID::Black, 1.0f);
    
    // MapPoint name label
    td::String pointName = mapPoint.name;
    gui::DrawableString label(pointName);
    label.draw(gui::Point(mapPoint.x, mapPoint.y + MapPointStyle::Size), 
               gui::Font::ID::SystemNormal, td::ColorID::Black);
}
