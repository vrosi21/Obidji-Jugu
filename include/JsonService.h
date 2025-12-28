#pragma once
#include "DataTypes.h"
#include <vector>
#include <filesystem>

// Pure I/O service for JSON file operations (no state)
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
    
    // JSON parsing helpers
    static std::string readFileContent(const std::filesystem::path& path);
    static double extractNumber(const std::string& obj, const char* key);
    static int extractInt(const std::string& obj, const char* key);
    static std::string extractString(const std::string& obj, const char* key);
    static bool extractBool(const std::string& obj, const char* key);
};
