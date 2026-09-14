#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>


struct MapCoordinate
{
    float x = 0.0f;
    float y = 0.0f;
};

struct MapRing
{
    bool isHole = false;
    std::vector<MapCoordinate> points;
};

struct MapPolygon
{
    std::vector<MapRing> rings;
};

struct CountryGeometry
{
    std::string countryCode;
    std::vector<MapPolygon> polygons;
};

struct MapCoastline
{
    bool closed = false;
    std::vector<MapCoordinate> points;
};

struct MapSettlement
{
    MapCoordinate position;
    unsigned int population = 0;
};

// Small, framework-independent loader for the generated offline vector map.
// Coordinates are already projected into the application's 1000 x 866 logical
// canvas, so drawing never depends on a network service or GIS library.
class MapGeometry
{
public:
    // Kept inline so MapView also links correctly when somebody opens an older
    // generated Visual Studio project that does not yet list MapGeometry.cpp.
    bool load(const std::filesystem::path& path)
    {
        _countries.clear();
        _coastlines.clear();
        _settlements.clear();

        std::ifstream input(path);
        if (!input) {
            std::fprintf(stderr, "[MapGeometry] Cannot open %s\n", path.string().c_str());
            return false;
        }

        std::string token;
        input >> token;
        const bool extendedFormat = token == "EXYU_MAP_V2";
        if (!extendedFormat && token != "EXYU_MAP_V1") {
            std::fprintf(stderr, "[MapGeometry] Unsupported map format in %s\n", path.string().c_str());
            return false;
        }

        CountryGeometry* country = nullptr;
        MapPolygon* polygon = nullptr;

        while (input >> token) {
            if (token == "COUNTRY") {
                std::string countryCode;
                if (!(input >> countryCode)) break;
                _countries.push_back({ countryCode, {} });
                country = &_countries.back();
                polygon = nullptr;
            }
            else if (token == "POLYGON") {
                if (!country) break;
                country->polygons.push_back({});
                polygon = &country->polygons.back();
            }
            else if (token == "RING") {
                int hole = 0;
                size_t pointCount = 0;
                if (!polygon || !(input >> hole >> pointCount) || pointCount < 3 || pointCount > 100000) break;

                MapRing ring;
                ring.isHole = hole != 0;
                ring.points.reserve(pointCount);
                for (size_t i = 0; i < pointCount; ++i) {
                    MapCoordinate point;
                    if (!(input >> point.x >> point.y)) {
                        _countries.clear();
                        _coastlines.clear();
                        _settlements.clear();
                        std::fprintf(stderr, "[MapGeometry] Truncated coordinate data in %s\n", path.string().c_str());
                        return false;
                    }
                    ring.points.push_back(point);
                }
                polygon->rings.push_back(std::move(ring));
            }
            else if (token == "ENDPOLYGON") {
                polygon = nullptr;
            }
            else if (token == "ENDCOUNTRY") {
                country = nullptr;
                polygon = nullptr;
            }
            else if (token == "COASTLINE") {
                int closed = 0;
                size_t pointCount = 0;
                if (!(input >> closed >> pointCount) || pointCount < 2 || pointCount > 250000) break;

                MapCoastline coastline;
                coastline.closed = closed != 0;
                coastline.points.reserve(pointCount);
                for (size_t i = 0; i < pointCount; ++i) {
                    MapCoordinate point;
                    if (!(input >> point.x >> point.y)) {
                        _countries.clear();
                        _coastlines.clear();
                        _settlements.clear();
                        std::fprintf(stderr, "[MapGeometry] Truncated coastline data in %s\n", path.string().c_str());
                        return false;
                    }
                    coastline.points.push_back(point);
                }
                _coastlines.push_back(std::move(coastline));
            }
            else if (token == "SETTLEMENT") {
                MapSettlement settlement;
                if (!(input >> settlement.position.x >> settlement.position.y >> settlement.population)) break;
                _settlements.push_back(settlement);
            }
            else if (token == "END") {
                break;
            }
            else {
                _countries.clear();
                _coastlines.clear();
                _settlements.clear();
                std::fprintf(stderr, "[MapGeometry] Unexpected token '%s' in %s\n", token.c_str(), path.string().c_str());
                return false;
            }
        }

        if (!input.eof() && input.fail()) {
            _countries.clear();
            _coastlines.clear();
            _settlements.clear();
            std::fprintf(stderr, "[MapGeometry] Invalid map data in %s\n", path.string().c_str());
            return false;
        }

        if (_countries.size() != 7) {
            _countries.clear();
            _coastlines.clear();
            _settlements.clear();
            std::fprintf(stderr, "[MapGeometry] Expected 7 country geometries in %s\n", path.string().c_str());
            return false;
        }

        if (extendedFormat && _coastlines.empty()) {
            _countries.clear();
            _coastlines.clear();
            _settlements.clear();
            std::fprintf(stderr, "[MapGeometry] Missing coastline layer in %s\n", path.string().c_str());
            return false;
        }

        return true;
    }

    const std::vector<CountryGeometry>& countries() const { return _countries; }
    const std::vector<MapCoastline>& coastlines() const { return _coastlines; }
    const std::vector<MapSettlement>& settlements() const { return _settlements; }
    bool empty() const { return _countries.empty(); }

private:
    std::vector<CountryGeometry> _countries;
    std::vector<MapCoastline> _coastlines;
    std::vector<MapSettlement> _settlements;
};
