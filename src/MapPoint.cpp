#include "MapPoint.h"

// === MapPointStyle Implementation ===

void MapPointStyle::getColors(VisitationStatus status, td::ColorID& borderColor, td::ColorID& centerColor)
{
    borderColor = td::ColorID::Yellow;
    centerColor = td::ColorID::Yellow;
    
    switch (status) {
        case VisitationStatus::Blocked:
            borderColor = td::ColorID::Red;
            centerColor = td::ColorID::Yellow;
            break;
        case VisitationStatus::Goal:
            borderColor = td::ColorID::Yellow;
            centerColor = td::ColorID::Violet;
            break;
        case VisitationStatus::Start:
            borderColor = td::ColorID::Yellow;
            centerColor = td::ColorID::Cyan;
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

std::pair<double, double> MapPointStyle::getScaledCenter(double x, double y, float scale, float offsetX, float offsetY)
{
    // Scale the position first, then add half of the scaled size, and apply offsets
    float scaledSize = Size * scale; // uniform scale for size
    double sx = offsetX + x * scale + scaledSize / 2.0;
    double sy = offsetY + y * scale + scaledSize / 2.0;
    return { sx, sy };
}

// === MapPointRenderer Implementation ===

void MapPointRenderer::draw(const CityPoint& mapPoint)
{
    td::ColorID borderColor, centerColor;
    MapPointStyle::getColors(mapPoint.visitation_status, borderColor, centerColor);
    
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

void MapPointRenderer::drawScaled(const CityPoint& mapPoint, float scale, float offsetX, float offsetY)
{
    td::ColorID borderColor, centerColor;
    MapPointStyle::getColors(mapPoint.visitation_status, borderColor, centerColor);

    // Scale coordinates and apply offsets
    float sx = offsetX + static_cast<float>(mapPoint.x) * scale;
    float sy = offsetY + static_cast<float>(mapPoint.y) * scale;

    // Scale sizes using uniform scale
    float scaledSize = MapPointStyle::Size * scale;
    float scaledBorderOffset = MapPointStyle::BorderOffset * scale;
    float scaledLineWidth = 1.0f * scale;
    if (scaledLineWidth < 0.5f) scaledLineWidth = 0.5f;

    // Border rectangle (larger, behind)
    gui::Rect borderRect(
        sx - scaledBorderOffset,
        sy - scaledBorderOffset,
        sx + scaledSize + scaledBorderOffset,
        sy + scaledSize + scaledBorderOffset
    );
    gui::Shape borderShape;
    borderShape.createRect(borderRect);
    borderShape.drawFillAndWire(borderColor, td::ColorID::Black, scaledLineWidth);

    // Center rectangle (inner)
    gui::Rect centerRect(sx, sy, sx + scaledSize, sy + scaledSize);
    gui::Shape centerShape;
    centerShape.createRect(centerRect);
    centerShape.drawFillAndWire(centerColor, td::ColorID::Black, scaledLineWidth);

    // MapPoint name label
    td::String pointName = mapPoint.name;
    gui::DrawableString label(pointName);
    label.draw(gui::Point(sx, sy + scaledSize), 
               gui::Font::ID::SystemNormal, td::ColorID::Black);
}
