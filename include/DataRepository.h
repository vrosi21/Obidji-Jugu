#pragma once
#include "DataTypes.h"
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

// Repository pattern: Manages city/road data with JSON persistence
class DataRepository
{
public:
    using ChangeCallback = std::function<void()>;

    DataRepository();
    ~DataRepository() = default;

    // --- City CRUD Operations ---
    bool addCity(const std::string& name, double x, double y);
    bool updateCity(int index, const std::string& name, double x, double y, double weight);
    bool updateCityStatus(int index, VisitationStatus status);
    bool deleteCity(int index);
    
    // --- City Queries ---
    bool getCity(int index, CityPoint& out) const;
    std::vector<std::string> getCityNames() const;
    size_t cityCount() const { return _cities.size(); }
    
    // --- Road CRUD Operations ---
    bool addConnection(int fromIndex, int toIndex);
    bool removeConnection(int fromIndex, int toIndex);
    
    // --- Road Queries ---
    bool hasConnection(int fromIndex, int toIndex) const;
    std::vector<int> getConnections(int index) const;
    std::vector<std::string> getConnectionNames(int index) const;
    
    // --- Start Point ---
    int getStartPointId() const { return _startPointId; }
    bool setStartPoint(int index);
    
    // --- Direct access for rendering (read-only) ---
    const std::vector<CityPoint>& cities() const { return _cities; }
    const std::vector<RoadEdge>& roads() const { return _roads; }
    
    // --- Change notifications ---
    void setOnDataChanged(ChangeCallback callback) { _onDataChanged = callback; }

private:
    std::vector<CityPoint> _cities;
    std::vector<RoadEdge> _roads;
    std::vector<RoadInfo> _roadsFull;
    std::filesystem::path _jsonPath;
    ChangeCallback _onDataChanged;
    int _startPointId = -1;  // ID of the start point (-1 = none)
    
    // --- Internal helpers ---
    void load();
    bool save();
    int nextCityId() const;
    bool isValidIndex(int index) const;
    void notifyChange();
};
