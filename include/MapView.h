#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <vector>
#include <string>

// Simple data holder for a city loaded from JSON
struct CityPoint {
    int x = 0;
    int y = 0;
    std::string name;   // Not drawn yet (could be used for labels later)
    std::string colorHex; // Original hex; mapped to td::ColorID when drawing
};

class MapView : public gui::Canvas
{
public:
    MapView();
    ~MapView() = default;

protected:
    void onDraw(const gui::Rect& rect) override;

private:
    void loadCities();
    td::ColorID mapHexToColor(const std::string& hex) const;
    std::vector<CityPoint> _cities;
    bool _loaded = false;
};
