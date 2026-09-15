#include "JsonService.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>

namespace {
    void dbg(const char* m) { fprintf(stderr, "%s", m); }
}

// === PRIVATE HELPERS ===

std::string JsonService::readFileContent(const std::filesystem::path& path)
{
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(path)) return {};
        std::ifstream in(path);
        if (!in) return {};
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }
    catch (...) { return {}; }
}

double JsonService::extractNumber(const std::string& obj, const char* key)
{
    size_t k = obj.find(key);
    if (k == std::string::npos) return 0.0;
    size_t colon = obj.find(':', k);
    size_t n1 = obj.find_first_of("0123456789-.", colon);
    if (n1 == std::string::npos) return 0.0;
    size_t n2 = obj.find_first_not_of("0123456789.-", n1);
    return std::atof(obj.substr(n1, n2 - n1).c_str());
}

int JsonService::extractInt(const std::string& obj, const char* key)
{
    return static_cast<int>(extractNumber(obj, key));
}

std::string JsonService::extractString(const std::string& obj, const char* key)
{
    size_t k = obj.find(key);
    if (k == std::string::npos) return {};
    size_t colon = obj.find(':', k);
    size_t q1 = obj.find('"', colon);
    size_t q2 = obj.find('"', q1 + 1);
    if (q1 == std::string::npos || q2 == std::string::npos) return {};
    return obj.substr(q1 + 1, q2 - q1 - 1);
}

bool JsonService::extractBool(const std::string& obj, const char* key)
{
    size_t k = obj.find(key);
    if (k == std::string::npos) return true;
    size_t colon = obj.find(':', k);
    size_t t = obj.find("true", colon);
    size_t f = obj.find("false", colon);
    if (t != std::string::npos && t < obj.size() && (t < f || f == std::string::npos)) return true;
    if (f != std::string::npos && f < obj.size()) return false;
    return true;
}

// === PUBLIC API ===

bool JsonService::loadFromJson(
    const std::filesystem::path& jsonPath,
    std::vector<CityPoint>& outCities,
    std::vector<RoadEdge>& outRoads,
    std::vector<RoadInfo>& outRoadsFull)
{
    std::string content = readFileContent(jsonPath);
    if (content.empty()) {
        dbg("[JsonService] Failed to read JSON file\n");
        return false;
    }

    // Find array boundaries
    size_t citiesKey = content.find("\"cities\"");
    size_t roadsKey = content.find("\"roads\"");
    size_t arrayStart = (citiesKey == std::string::npos) ? content.find('[') : content.find('[', citiesKey);
    size_t arrayEnd = (roadsKey != std::string::npos && roadsKey > arrayStart) 
        ? content.rfind(']', roadsKey) 
        : content.rfind(']');

    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayStart >= arrayEnd) {
        dbg("[JsonService] Invalid JSON array format\n");
        return false;
    }

    // Parse cities
    size_t pos = arrayStart + 1;
    while (pos < arrayEnd) {
        size_t objStart = content.find('{', pos);
        if (objStart == std::string::npos || objStart > arrayEnd) break;
        size_t objEnd = content.find('}', objStart);
        if (objEnd == std::string::npos || objEnd > arrayEnd) break;
        
        std::string obj = content.substr(objStart, objEnd - objStart + 1);

        CityPoint city;
        int parsedId = extractInt(obj, "\"id\"");
        city.id = (parsedId >= 0) ? parsedId : static_cast<int>(outCities.size());
        city.x = extractNumber(obj, "\"x\"");
        city.y = extractNumber(obj, "\"y\"");
        city.name = extractString(obj, "\"name\"");
        city.weight = extractNumber(obj, "\"weight\"");
        
        int statusValue = extractInt(obj, "\"visitation_status\"");
        statusValue = (statusValue < 0 || statusValue > 3) ? 1 : statusValue;
        city.visitation_status = static_cast<VisitationStatus>(statusValue);
        
        if (!city.name.empty()) {
            if (city.id == static_cast<int>(outCities.size())) {
                outCities.push_back(city);
            }
            else if (city.id >= 0 && city.id < 10000) {
                if (outCities.size() <= static_cast<size_t>(city.id)) {
                    outCities.resize(city.id + 1);
                }
                outCities[city.id] = city;
            }
            else {
                outCities.push_back(city);
            }
        }
        pos = objEnd + 1;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "[JsonService] Loaded %zu cities\n", outCities.size());
    dbg(msg);

    // Parse roads
    if (roadsKey != std::string::npos) {
        size_t rStart = content.find('[', roadsKey);
        size_t rEnd = content.find(']', rStart != std::string::npos ? rStart : 0);
        
        if (rStart != std::string::npos && rEnd != std::string::npos) {
            pos = rStart + 1;
            while (pos < rEnd) {
                size_t objStart = content.find('{', pos);
                if (objStart == std::string::npos || objStart > rEnd) break;
                size_t objEnd = content.find('}', objStart);
                if (objEnd == std::string::npos || objEnd > rEnd) break;
                
                std::string obj = content.substr(objStart, objEnd - objStart + 1);

                int from = extractInt(obj, "\"from\"");
                int to = extractInt(obj, "\"to\"");
                
                if (from >= 0 && to >= 0) {
                    outRoads.push_back({ from, to });
                    
                    RoadInfo ri;
                    ri.fromId = from;
                    ri.toId = to;
                    ri.length = extractNumber(obj, "\"length\"");
                    ri.travelTimeH = extractNumber(obj, "\"travel_time_h\"");
                    ri.type = extractString(obj, "\"type\"");
                    ri.bidirectional = extractBool(obj, "\"bidirectional\"");
                    outRoadsFull.push_back(ri);
                }
                pos = objEnd + 1;
            }
        }
    }

    return true;
}

bool JsonService::saveToJson(
    const std::filesystem::path& jsonPath,
    const std::vector<CityPoint>& cities,
    const std::vector<RoadInfo>& roadsFull)
{
    std::ofstream out(jsonPath, std::ios::trunc);
    if (!out) {
        dbg("[JsonService] Failed to open JSON file for writing\n");
        return false;
    }

    out << "{\n  \"cities\": [\n";
    for (size_t i = 0; i < cities.size(); ++i) {
        const auto& c = cities[i];
        out << "    {\"id\":" << c.id
            << ",\"x\":" << std::fixed << std::setprecision(2) << c.x
            << ",\"y\":" << std::fixed << std::setprecision(2) << c.y
            << ",\"name\":\"" << c.name << "\""
            << ",\"weight\":" << std::fixed << std::setprecision(1) << c.weight
            << ",\"visitation_status\":" << static_cast<int>(c.visitation_status) << "}";
        if (i + 1 < cities.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n  \"roads\": [\n";
    for (size_t i = 0; i < roadsFull.size(); ++i) {
        const auto& r = roadsFull[i];
        out << "    {\"from\":" << r.fromId
            << ",\"to\":" << r.toId
            << ",\"length\":" << std::fixed << std::setprecision(2) << r.length
            << ",\"travel_time_h\":" << std::fixed << std::setprecision(3) << r.travelTimeH
            << ",\"type\":\"" << r.type << "\""
            << ",\"bidirectional\":" << (r.bidirectional ? "true" : "false") << "}";
        if (i + 1 < roadsFull.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
    
    return true;
}
