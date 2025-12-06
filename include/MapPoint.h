#pragma once

#include <string>
#include <vector>
#include <cmath>

struct MapPoint {
    int id;                  // unique id for easy indexing
    float x;
    float y;
    std::string name;
    float weight;
    std::string color;       // hex or named color for UI use

    MapPoint()
        : id(-1), x(0.0f), y(0.0f), name(""), weight(1.0f), color("") {}

    MapPoint(int pid, float px, float py, const std::string& pname, float pweight, const std::string& pcolor)
        : id(pid), x(px), y(py), name(pname), weight(pweight), color(pcolor) {}

    // Convenience overloads to avoid ambiguous conversions
    MapPoint(int pid, float px, float py, const char* pname, float pweight, const char* pcolor)
        : id(pid), x(px), y(py), name(pname ? pname : ""), weight(pweight), color(pcolor ? pcolor : "") {}

    MapPoint(int pid, float px, float py, const std::string& pname, float pweight)
        : id(pid), x(px), y(py), name(pname), weight(pweight), color("") {}

    MapPoint(int pid, float px, float py, const char* pname, float pweight)
        : id(pid), x(px), y(py), name(pname ? pname : ""), weight(pweight), color("") {}
};

struct Road {
    int from_id;             // map point id
    int to_id;               // map point id
    float length;            // Euclidean distance in same coordinate units as MapPoint
    float travel_time_h;     // travel time in hours (length / assumed_speed)
    std::string type;        // "local", "regional", "intercity"
    bool bidirectional;

    Road(int f, int t, float len, float travel_h, std::string rtype, bool bidi = true)
        : from_id(f), to_id(t), length(len), travel_time_h(travel_h), type(std::move(rtype)), bidirectional(bidi) {}
};

struct MapGraph {
    std::vector<MapPoint> points;
    std::vector<Road> roads;

    // Utility: compute Euclidean distance between two points (by id)
    static float distance(const MapPoint &a, const MapPoint &b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx*dx + dy*dy);
    }

    // (Optional) Add other helpers: findNeighbors, buildKNNGraph, export, etc.
};
