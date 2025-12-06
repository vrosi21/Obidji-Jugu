#pragma once

#include <string>

struct MapPoint {
    float x;
    float y;
    std::string name;
    float weight;
    std::string color; // hex or named color for UI use

    MapPoint() : x(0.0f), y(0.0f), name(""), weight(1.0f), color("") {}
    MapPoint(float px, float py, std::string pname, float pweight)
        : x(px), y(py), name(std::move(pname)), weight(pweight), color("") {}
    MapPoint(float px, float py, std::string pname, float pweight, std::string pcolor)
        : x(px), y(py), name(std::move(pname)), weight(pweight), color(std::move(pcolor)) {}
};
