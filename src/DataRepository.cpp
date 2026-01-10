#include "DataRepository.h"
#include "JsonService.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <limits>
#include <queue>
#include <utility>

DataRepository::DataRepository()
{
    load();
    recomputeMetricClosure();
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
    recomputeMetricClosure();
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

// === METRIC CLOSURE API ===

double DataRepository::getMetricDistance(int i, int j) const
{
    if (!isValidIndex(i) || !isValidIndex(j)) return std::numeric_limits<double>::infinity();
    if (i >= static_cast<int>(_metricDist.size()) || j >= static_cast<int>(_metricDist.size())) return std::numeric_limits<double>::infinity();
    return _metricDist[i][j];
}

bool DataRepository::getShortestRoadPath(int i, int j, std::vector<int>& outPath) const
{
    outPath.clear();
    if (!isValidIndex(i) || !isValidIndex(j)) return false;
    if (i >= static_cast<int>(_metricPrev.size()) || j >= static_cast<int>(_metricPrev.size())) return false;
    if (!std::isfinite(_metricDist[i][j])) return false;

    int cur = j;
    while (cur != -1 && cur != i) {
        outPath.push_back(cur);
        cur = _metricPrev[i][cur];
    }
    if (cur == i) {
        outPath.push_back(i);
        std::reverse(outPath.begin(), outPath.end());
        return true;
    }
    outPath.clear();
    return false;
}

std::vector<int> DataRepository::getLargestConnectedComponent() const
{
    int n = static_cast<int>(_cities.size());
    std::vector<int> compIds(n, -1);
    int comp = 0;

    for (int s = 0; s < n; ++s) {
        if (compIds[s] != -1) continue;
        std::vector<int> q;
        q.push_back(s);
        compIds[s] = comp;
        for (size_t qi = 0; qi < q.size(); ++qi) {
            int u = q[qi];
            for (int v : getConnections(u)) {
                if (compIds[v] == -1) { compIds[v] = comp; q.push_back(v); }
            }
        }
        comp++;
    }

    std::vector<int> sizes(comp, 0);
    for (int id : compIds) if (id >= 0) sizes[id]++;
    int bestComp = -1, bestSize = 0;
    for (int i = 0; i < comp; ++i) if (sizes[i] > bestSize) { bestSize = sizes[i]; bestComp = i; }

    std::vector<int> res;
    for (int i = 0; i < n; ++i) if (compIds[i] == bestComp) res.push_back(i);
    return res;
}

// === INTERNAL: METRIC CLOSURE COMPUTATION ===

void DataRepository::recomputeMetricClosure()
{
    int n = static_cast<int>(_cities.size());
    _metricDist.assign(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));
    _metricPrev.assign(n, std::vector<int>(n, -1));
    if (n == 0) return;

    // Build adjacency list with Euclidean edge weights for each road
    std::vector<std::vector<std::pair<int,double>>> adj(n);
    auto euclid = [&](int a, int b){
        double dx = _cities[a].x - _cities[b].x; double dy = _cities[a].y - _cities[b].y; return std::sqrt(dx*dx + dy*dy);
    };
    for (const auto& r : _roads) {
        if (!isValidIndex(r.fromId) || !isValidIndex(r.toId)) continue;
        double w = euclid(r.fromId, r.toId);
        adj[r.fromId].push_back({r.toId, w});
        adj[r.toId].push_back({r.fromId, w});
    }

    // Dijkstra from every source
    for (int s = 0; s < n; ++s) {
        std::vector<double> dist(n, std::numeric_limits<double>::infinity());
        std::vector<int> prev(n, -1);
        struct Node { double d; int v; };
        auto cmp = [](const Node& a, const Node& b){ return a.d > b.d; };
        std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);
        dist[s] = 0.0; pq.push({0.0, s});

        while (!pq.empty()) {
            Node top = pq.top(); pq.pop();
            double d = top.d; int u = top.v;
            if (d != dist[u]) continue;
            for (auto& e : adj[u]) {
                int v = e.first; double w = e.second;
                if (dist[u] + w < dist[v]) {
                    dist[v] = dist[u] + w;
                    prev[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }
        _metricDist[s] = std::move(dist);
        _metricPrev[s] = std::move(prev);
    }
}
