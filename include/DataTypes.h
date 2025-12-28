#pragma once
#include <string>
#include <vector>

// Visitation status enumeration
enum class VisitationStatus : int {
    Blocked = 0,
    Open = 1,
    Goal = 2,
    Start = 3
};

// City data structure
struct CityPoint {
    int id = -1;
    double x = 0.0;
    double y = 0.0;
    double weight = 0.0;
    std::string name;
    VisitationStatus visitation_status = VisitationStatus::Open;
};

// Road connection (lightweight)
struct RoadEdge { 
    int fromId = -1; 
    int toId = -1; 
};

// Road with full info
struct RoadInfo {
    int fromId = -1;
    int toId = -1;
    double length = 0.0;
    double travelTimeH = 0.0;
    std::string type = "local";
    bool bidirectional = true;
};
