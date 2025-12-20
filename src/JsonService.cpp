#include "JsonService.h"
#include "MapView.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <windows.h>

namespace {
    void dbg(const char* m) { OutputDebugStringA(m); }
}

bool JsonService::loadFromJson(
    const std::filesystem::path& jsonPath,
    std::vector<CityPoint>& outCities,
    std::vector<RoadEdge>& outRoads,
    std::vector<RoadInfo>& outRoadsFull
)
{
    namespace fs = std::filesystem;

    auto readFile = [](const fs::path& p)->std::string {
        try { 
            if (!fs::exists(p)) return {}; 
            std::ifstream in(p); 
            if (!in) return {}; 
            std::ostringstream ss; 
            ss << in.rdbuf(); 
            return ss.str(); 
        }
        catch (...) { return {}; }
    };

    std::string content = readFile(jsonPath);
    if (content.empty()) {
        dbg("[JsonService] Failed to read JSON file\n");
        return false;
    }

    // Parse cities array
    size_t citiesKey = content.find("\"cities\"");
    size_t roadsKey = content.find("\"roads\"");
    size_t arrayStart = (citiesKey == std::string::npos) ? content.find('[') : content.find('[', citiesKey);
    size_t arrayEnd = std::string::npos;
    if (roadsKey != std::string::npos && roadsKey > arrayStart) {
        arrayEnd = content.rfind(']', roadsKey);
    }
    if (arrayEnd == std::string::npos) {
        arrayEnd = content.rfind(']');
    }
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

        auto extractNumber = [&](const char* key)->double {
            size_t k = obj.find(key);
            if (k == std::string::npos) return 0.0;
            size_t colon = obj.find(':', k);
            size_t n1 = obj.find_first_of("0123456789-.", colon);
            if (n1 == std::string::npos) return 0.0;
            size_t n2 = obj.find_first_not_of("0123456789.-", n1);
            return std::atof(obj.substr(n1, n2 - n1).c_str());
        };
        auto extractInt = [&](const char* key)->int {
            return static_cast<int>(extractNumber(key));
        };
        auto extractString = [&](const char* key)->std::string {
            size_t k = obj.find(key);
            if (k == std::string::npos) return {};
            size_t colon = obj.find(':', k);
            size_t q1 = obj.find('"', colon);
            size_t q2 = obj.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) return {};
            return obj.substr(q1 + 1, q2 - q1 - 1);
        };

        CityPoint cp;
        int parsedId = extractInt("\"id\"");
        cp.id = parsedId >= 0 ? parsedId : static_cast<int>(outCities.size());
        cp.x = extractNumber("\"x\"");
        cp.y = extractNumber("\"y\"");
        cp.name = extractString("\"name\"");
        cp.weight = extractNumber("\"weight\"");
        int statusValue = extractInt("\"visitation_status\"");
        if (statusValue < 0 || statusValue > 2) statusValue = 1;
        cp.visitation_status = static_cast<VisitationStatus>(statusValue);
        
        if (!cp.name.empty()) {
            if (cp.id == static_cast<int>(outCities.size())) {
                outCities.push_back(cp);
            }
            else if (cp.id >= 0 && cp.id < 10000) {
                if (outCities.size() <= static_cast<size_t>(cp.id)) {
                    outCities.resize(static_cast<size_t>(cp.id) + 1);
                }
                outCities[static_cast<size_t>(cp.id)] = cp;
            }
            else {
                outCities.push_back(cp);
            }
        }

        pos = objEnd + 1;
    }

    char msg[128]; 
    sprintf_s(msg, "[JsonService] Loaded %zu cities\n", outCities.size()); 
    dbg(msg);

    // Parse roads array
    if (roadsKey != std::string::npos) {
        size_t rStart = content.find('[', roadsKey);
        size_t rEnd = content.find(']', rStart == std::string::npos ? 0 : rStart);
        if (rStart != std::string::npos && rEnd != std::string::npos) {
            size_t rpos = rStart + 1;
            while (rpos < rEnd) {
                size_t oStart = content.find('{', rpos);
                if (oStart == std::string::npos || oStart > rEnd) break;
                size_t oEnd = content.find('}', oStart);
                if (oEnd == std::string::npos || oEnd > rEnd) break;
                std::string obj = content.substr(oStart, oEnd - oStart + 1);

                auto extractInt = [&](const char* key)->int {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return -1;
                    size_t colon = obj.find(':', k);
                    size_t n1 = obj.find_first_of("0123456789-", colon);
                    if (n1 == std::string::npos) return -1;
                    size_t n2 = obj.find_first_not_of("0123456789-", n1);
                    return std::atoi(obj.substr(n1, n2 - n1).c_str());
                };
                auto extractDouble = [&](const char* key)->double {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return 0.0;
                    size_t colon = obj.find(':', k);
                    size_t n1 = obj.find_first_of("0123456789-.", colon);
                    if (n1 == std::string::npos) return 0.0;
                    size_t n2 = obj.find_first_not_of("0123456789.-", n1);
                    return std::atof(obj.substr(n1, n2 - n1).c_str());
                };
                auto extractString = [&](const char* key)->std::string {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return {};
                    size_t colon = obj.find(':', k);
                    size_t q1 = obj.find('"', colon);
                    size_t q2 = obj.find('"', q1 + 1);
                    if (q1 == std::string::npos || q2 == std::string::npos) return {};
                    return obj.substr(q1 + 1, q2 - q1 - 1);
                };
                auto extractBool = [&](const char* key)->bool {
                    size_t k = obj.find(key);
                    if (k == std::string::npos) return true;
                    size_t colon = obj.find(':', k);
                    size_t t = obj.find("true", colon);
                    size_t f = obj.find("false", colon);
                    if (t != std::string::npos && t < obj.size() && (t < f || f == std::string::npos)) return true;
                    if (f != std::string::npos && f < obj.size()) return false;
                    return true;
                };

                int from = extractInt("\"from\"");
                int to = extractInt("\"to\"");
                if (from >= 0 && to >= 0) {
                    outRoads.push_back({ from, to });
                    RoadInfo ri;
                    ri.fromId = from;
                    ri.toId = to;
                    ri.length = extractDouble("\"length\"");
                    ri.travelTimeH = extractDouble("\"travel_time_h\"");
                    ri.type = extractString("\"type\"");
                    ri.bidirectional = extractBool("\"bidirectional\"");
                    outRoadsFull.push_back(ri);
                }
                rpos = oEnd + 1;
            }
        }
    }

    return true;
}

bool JsonService::saveToJson(
    const std::filesystem::path& jsonPath,
    const std::vector<CityPoint>& cities,
    const std::vector<RoadInfo>& roadsFull
)
{
    std::ofstream out(jsonPath, std::ios::trunc);
    if (!out) {
        dbg("[JsonService] Failed to open JSON file for writing\n");
        return false;
    }

    out << "{\n";
    out << "  \"cities\": [\n";
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
    out << "  ],\n";
    out << "  \"roads\": [\n";
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
    out << "  ]\n";
    out << "}\n";
    return true;
}

std::filesystem::path JsonService::findJsonFile()
{
    namespace fs = std::filesystem;

    static const char* candidates[] = {
        "../res/exYu.json"
    };

    auto readFile = [](const fs::path& p)->std::string {
        try { 
            if (!fs::exists(p)) return {}; 
            std::ifstream in(p); 
            if (!in) return {}; 
            std::ostringstream ss; 
            ss << in.rdbuf(); 
            return ss.str(); 
        }
        catch (...) { return {}; }
    };

    fs::path cwd = fs::current_path();
    fs::path foundPath;

    // Try from current working directory
    char exeBuf[MAX_PATH] = { 0 };
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    fs::path exeDir(exeBuf);
    exeDir = exeDir.parent_path();

    std::string content;
    for (auto c : candidates) {
        fs::path p = cwd / c;
        content = readFile(p);
        if (!content.empty()) { foundPath = p; break; }
    }
    if (content.empty()) {
        for (auto c : candidates) {
            fs::path p = exeDir / c;
            content = readFile(p);
            if (!content.empty()) { foundPath = p; break; }
        }
    }
    if (foundPath.empty()) {
        foundPath = cwd / candidates[0];
    }
    
    return foundPath;
}
