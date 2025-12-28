#include "DataRepository.h"
#include "JsonService.h"
#include <algorithm>
#include <cmath>
#include <set>

DataRepository::DataRepository()
{
    load();
}

// === PRIVATE HELPERS ===

void DataRepository::load()
{
    _jsonPath = JsonService::findJsonFile();
    JsonService::loadFromJson(_jsonPath, _cities, _roads, _roadsFull);
}

bool DataRepository::save()
{
    auto path = _jsonPath.empty() ? JsonService::findJsonFile() : _jsonPath;
    return JsonService::saveToJson(path, _cities, _roadsFull);
}

int DataRepository::nextCityId() const
{
    int maxId = -1;
    for (const auto& c : _cities) {
        maxId = std::max(maxId, c.id);
    }
    return (maxId < 0) ? static_cast<int>(_cities.size()) : maxId + 1;
}

bool DataRepository::isValidIndex(int index) const
{
    return index >= 0 && index < static_cast<int>(_cities.size());
}

void DataRepository::notifyChange()
{
    if (_onDataChanged) _onDataChanged();
}

// === CITY CRUD ===

bool DataRepository::addCity(const std::string& name, double x, double y)
{
    if (name.empty()) return false;
    
    CityPoint city;
    city.id = nextCityId();
    city.name = name;
    city.x = x;
    city.y = y;
    city.weight = 0.0;
    city.visitation_status = VisitationStatus::Open;

    _cities.push_back(city);
    
    if (!save()) {
        _cities.pop_back();
        return false;
    }
    
    notifyChange();
    return true;
}

bool DataRepository::updateCity(int index, const std::string& name, double x, double y, double weight)
{
    if (!isValidIndex(index) || name.empty()) return false;

    CityPoint backup = _cities[index];
    _cities[index].name = name;
    _cities[index].x = x;
    _cities[index].y = y;
    _cities[index].weight = weight;

    if (!save()) {
        _cities[index] = backup;
        return false;
    }
    
    notifyChange();
    return true;
}

bool DataRepository::updateCityStatus(int index, VisitationStatus status)
{
    if (!isValidIndex(index)) return false;

    CityPoint backup = _cities[index];
    _cities[index].visitation_status = status;

    if (!save()) {
        _cities[index] = backup;
        return false;
    }
    
    notifyChange();
    return true;
}

bool DataRepository::deleteCity(int index)
{
    if (!isValidIndex(index)) return false;
    
    // Backup for rollback
    auto backupCities = _cities;
    auto backupRoads = _roads;
    auto backupRoadsFull = _roadsFull;

    // Remove city and reindex
    _cities.erase(_cities.begin() + index);
    for (size_t i = 0; i < _cities.size(); ++i) {
        _cities[i].id = static_cast<int>(i);
    }

    // Adjust road indices
    auto adjustId = [index](int id) { return (id > index) ? id - 1 : id; };
    
    std::vector<RoadEdge> newRoads;
    std::vector<RoadInfo> newRoadsFull;
    
    for (const auto& r : _roads) {
        if (r.fromId == index || r.toId == index) continue;
        newRoads.push_back({ adjustId(r.fromId), adjustId(r.toId) });
    }
    
    for (const auto& r : _roadsFull) {
        if (r.fromId == index || r.toId == index) continue;
        RoadInfo nr = r;
        nr.fromId = adjustId(r.fromId);
        nr.toId = adjustId(r.toId);
        newRoadsFull.push_back(nr);
    }
    
    _roads = std::move(newRoads);
    _roadsFull = std::move(newRoadsFull);

    if (!save()) {
        _cities = std::move(backupCities);
        _roads = std::move(backupRoads);
        _roadsFull = std::move(backupRoadsFull);
        return false;
    }
    
    notifyChange();
    return true;
}

// === CITY QUERIES ===

bool DataRepository::getCity(int index, CityPoint& out) const
{
    if (!isValidIndex(index)) return false;
    out = _cities[index];
    return !out.name.empty();
}

std::vector<std::string> DataRepository::getCityNames() const
{
    std::vector<std::string> names;
    names.reserve(_cities.size());
    for (const auto& c : _cities) {
        names.push_back(c.name);
    }
    return names;
}

// === ROAD CRUD ===

bool DataRepository::addConnection(int fromIndex, int toIndex)
{
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex)) return false;
    if (fromIndex == toIndex) return false;
    if (hasConnection(fromIndex, toIndex)) return true; // Already exists

    const auto& from = _cities[fromIndex];
    const auto& to = _cities[toIndex];
    
    double dx = from.x - to.x;
    double dy = from.y - to.y;
    double length = std::sqrt(dx * dx + dy * dy);

    _roads.push_back({ fromIndex, toIndex });
    
    RoadInfo ri;
    ri.fromId = fromIndex;
    ri.toId = toIndex;
    ri.length = length;
    ri.travelTimeH = length / 60.0;
    ri.type = "local";
    ri.bidirectional = true;
    _roadsFull.push_back(ri);

    if (!save()) {
        _roads.pop_back();
        _roadsFull.pop_back();
        return false;
    }
    
    notifyChange();
    return true;
}

bool DataRepository::removeConnection(int fromIndex, int toIndex)
{
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex)) return false;
    if (fromIndex == toIndex) return false;

    auto backupRoads = _roads;
    auto backupRoadsFull = _roadsFull;

    auto matchesPair = [=](int a, int b) {
        return (a == fromIndex && b == toIndex) || (a == toIndex && b == fromIndex);
    };

    _roads.erase(
        std::remove_if(_roads.begin(), _roads.end(), 
            [&](const RoadEdge& r) { return matchesPair(r.fromId, r.toId); }),
        _roads.end());

    _roadsFull.erase(
        std::remove_if(_roadsFull.begin(), _roadsFull.end(),
            [&](const RoadInfo& r) { return matchesPair(r.fromId, r.toId); }),
        _roadsFull.end());

    if (!save()) {
        _roads = std::move(backupRoads);
        _roadsFull = std::move(backupRoadsFull);
        return false;
    }
    
    notifyChange();
    return true;
}

// === ROAD QUERIES ===

bool DataRepository::hasConnection(int fromIndex, int toIndex) const
{
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex)) return false;
    if (fromIndex == toIndex) return false;

    for (const auto& r : _roads) {
        if ((r.fromId == fromIndex && r.toId == toIndex) || 
            (r.fromId == toIndex && r.toId == fromIndex)) {
            return true;
        }
    }
    return false;
}

std::vector<int> DataRepository::getConnections(int index) const
{
    std::set<int> ids;
    if (!isValidIndex(index)) return {};
    
    for (const auto& r : _roads) {
        if (r.fromId == index && isValidIndex(r.toId)) ids.insert(r.toId);
        if (r.toId == index && isValidIndex(r.fromId)) ids.insert(r.fromId);
    }
    return std::vector<int>(ids.begin(), ids.end());
}

std::vector<std::string> DataRepository::getConnectionNames(int index) const
{
    std::vector<std::string> names;
    for (int i : getConnections(index)) {
        names.push_back(_cities[i].name);
    }
    return names;
}
