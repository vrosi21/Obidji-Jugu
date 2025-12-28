#pragma once
#include <vector>
#include <string>
#include <filesystem>

// Forward declarations for data structures
struct CityPoint;

struct RoadInfo {
    int fromId = -1;
    int toId = -1;
    double length = 0.0;
    double travelTimeH = 0.0;
    std::string type;
    bool bidirectional = true;
};

struct RoadEdge { 
    int fromId; 
    int toId; 
};

// Service class for JSON file operations
class JsonService
{
public:
    // Load cities and roads from JSON file
    static bool loadFromJson(
        const std::filesystem::path& jsonPath,
        std::vector<CityPoint>& outCities,
        std::vector<RoadEdge>& outRoads,
        std::vector<RoadInfo>& outRoadsFull
    );

    // Save cities and roads to JSON file
    static bool saveToJson(
        const std::filesystem::path& jsonPath,
        const std::vector<CityPoint>& cities,
        const std::vector<RoadInfo>& roadsFull
    );

    // Find JSON file in search paths
    static std::filesystem::path findJsonFile();

private:
    JsonService() = delete; // Static-only class
};
