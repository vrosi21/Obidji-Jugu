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

    DataRepository() = default;
    ~DataRepository() = default;

    // Initialize with resolved JSON file path
    void init(const std::filesystem::path& jsonPath);

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

    // --- Metric closure (shortest paths on road graph) ---
    // Returns shortest road distance between cities i and j; infinity if unreachable
    double getMetricDistance(int i, int j) const;
    // Reconstructs the shortest road path from i to j as a sequence of city indices; returns false if unreachable
    bool getShortestRoadPath(int i, int j, std::vector<int>& outPath) const;
    // Returns indices of the largest connected component (by road connectivity)
    std::vector<int> getLargestConnectedComponent() const;
    
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

    // Metric-closure data: all-pairs shortest-path distances and predecessors
    std::vector<std::vector<double>> _metricDist;  // dist[src][dst]
    std::vector<std::vector<int>> _metricPrev;     // prev[src][dst]
    
    // --- Internal helpers ---
    void load();
    bool save();
    int nextCityId() const;
    bool isValidIndex(int index) const;
    void notifyChange();
    void recomputeMetricClosure();
};
