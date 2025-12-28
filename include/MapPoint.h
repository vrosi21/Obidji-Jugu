#pragma once

#include <string>
#include <vector>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include "DataTypes.h"

// === MapPoint Rendering Configuration ===
struct MapPointStyle {
    static constexpr int Size = 10;           // Inner rectangle size
    static constexpr int BorderOffset = 3;    // Border extends this much beyond inner rect
    
    // Get colors based on visitation status
    static void getColors(VisitationStatus status, td::ColorID& borderColor, td::ColorID& centerColor);
    
    // Get center point of a map point (for road connections)
    static std::pair<double, double> getCenter(double x, double y);
};

// === MapPoint Renderer ===
struct MapPointRenderer {
    // Draw a map point at given position with border and center based on status
    static void draw(const CityPoint& mapPoint);
};
