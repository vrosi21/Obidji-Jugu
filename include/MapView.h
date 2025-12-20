#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/Image.h>
#include <gui/DrawableString.h>
#include "JsonService.h"
#include <vector>
#include <string>
#include <filesystem>

// Visitation status enumeration
enum class VisitationStatus : int {
    Blocked = 0,
    Open = 1,
    Goal = 2
};

// Simple data holder for a city loaded from JSON
struct CityPoint {
    int id = -1;
    double x = 0.0;
    double y = 0.0;
    double weight = 0.0;
    std::string name;      // Optional label
    VisitationStatus visitation_status = VisitationStatus::Open;
};

class MapView : public gui::Canvas
{
public:
    MapView();
    ~MapView() = default;

    // Returns the loaded city names (empty if not loaded or none)
    std::vector<std::string> getCityNames() const;

    // Returns false if index invalid; fills out with the city data
    bool getCity(int index, CityPoint& out) const;

    // Adds a city to current session and persists to JSON on success
    bool addCity(const std::string& name, double x, double y);

    // Updates an existing city and persists to JSON on success
    bool updateCity(int index, const std::string& name, double x, double y);

    // Updates an existing city's visitation status and persists to JSON on success
    bool updateCityStatus(int index, VisitationStatus status);

    // Deletes a city (and connected roads) and persists on success
    bool deleteCity(int index);

    // Returns connected city indices for a given city
    std::vector<int> getConnections(int index) const;
    std::vector<std::string> getConnectionNames(int index) const;

    // Adds/removes a bidirectional connection and persists on success
    bool addConnection(int fromIndex, int toIndex);
    bool removeConnection(int fromIndex, int toIndex);
    bool hasConnection(int fromIndex, int toIndex) const;

protected:
    void onDraw(const gui::Rect& rect) override;

private:
    void loadCities();
    void loadBackground();
    bool saveJson() const;
    int nextCityId() const;
    std::vector<CityPoint> _cities;
    std::vector<RoadEdge> _roads;
    std::vector<RoadInfo> _roadsFull;
    std::filesystem::path _jsonPath;
    bool _loaded = false;
    gui::Image _bgImage;
    bool _bgLoaded = false;

    // Returns the center of a city (10x10 rectangle): (x+5, y+5)
    std::pair<gui::CoordType, gui::CoordType> getPointCenter(const CityPoint& p) const;
};
