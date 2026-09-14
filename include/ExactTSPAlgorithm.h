#pragma once
#include "TSPAlgorithm.h"

// Exhaustive branch-and-bound on the same weighted closure as the heuristics.
// Bounded work per step, O(n^2) storage, no exponential snapshot history.
// Completion certifies the optimum (within floating-point arithmetic).
class ExactTSPAlgorithm : public TSPAlgorithm {
    std::vector<std::vector<double>> distances;
    std::vector<std::vector<int>> order;
    std::vector<int> path, next;
    std::vector<double> costs;
    std::vector<bool> used;

    void pop() {
        used[path.back()] = false;
        path.pop_back(); next.pop_back(); costs.pop_back();
        if (path.empty()) _tspState.finished = true;
    }
    void accept(double cost) {
        if (cost >= _tspState.bestLength) return;
        _tspState.bestLength = _tspState.tourLength = cost;
        _tspState.bestTour.clear();
        for (int v : path) _tspState.bestTour.push_back(_requiredCities[v]);
        _tspState.tour = _tspState.bestTour;
    }
protected:
    void initializeTSP() override {
        const int n = _cityCount;
        distances.assign(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));
        order.assign(n, {});
        path.clear(); next.clear(); costs.clear(); used.assign(n, false);
        for (int a = 0; a < n; ++a) for (int b = 0; b < n; ++b) {
            if (a == b) continue;
            distances[a][b] = calculateTransitionCost(_requiredCities[a], _requiredCities[b]);
            if (b != 0) order[a].push_back(b);
        }
        for (int a = 0; a < n; ++a)
            std::sort(order[a].begin(), order[a].end(), [&](int b, int c) {
                return distances[a][b] < distances[a][c];
            });
        if (n < 2) { _tspState.finished = true; return; }
        // Seed an incumbent using a nearest-neighbor tour.
        path.push_back(0); used[0] = true;
        double total = 0;
        while (path.size() < static_cast<size_t>(n)) {
            int chosen = -1;
            for (int v : order[path.back()]) if (!used[v]) { chosen = v; break; }
            if (chosen < 0) break;
            total += distances[path.back()][chosen];
            path.push_back(chosen); used[chosen] = true;
        }
        if (path.size() == static_cast<size_t>(n)) accept(total + distances[path.back()][0]);
        path.assign(1, 0); next.assign(1, 0); costs.assign(1, 0);
        used.assign(n, false); used[0] = true;
    }
    bool performStep() override {
        if (_tspState.finished) return false;
        int last = path.back();
        if (path.size() == static_cast<size_t>(_cityCount)) {
            accept(costs.back() + distances[last][0]); pop();
            return !_tspState.finished;
        }
        if (next.back() == 0) {
            // Each remaining vertex and the current endpoint needs an outgoing
            // edge. Independent minima relax the tour constraints: a lower bound.
            double bound = costs.back();
            for (int a = 0; a < _cityCount; ++a) {
                if (used[a] && a != last) continue;
                double minimum = std::numeric_limits<double>::infinity();
                for (int b = 0; b < _cityCount; ++b)
                    if (a != b && (!used[b] || b == 0)) minimum = std::min(minimum, distances[a][b]);
                bound += minimum;
            }
            // Conservative margin avoids pruning a better tour due to roundoff.
            if (!std::isfinite(bound) || bound > _tspState.bestLength +
                    1e-10 * std::max(1.0, std::abs(_tspState.bestLength))) {
                pop(); return !_tspState.finished;
            }
        }
        while (next.back() < static_cast<int>(order[last].size())) {
            int v = order[last][next.back()++];
            if (used[v] || !std::isfinite(distances[last][v])) continue;
            double cost = costs.back() + distances[last][v];
            used[v] = true; path.push_back(v); next.push_back(0); costs.push_back(cost);
            return true;
        }
        pop(); return !_tspState.finished;
    }
public:
    const char* getName() const override { return "Exact TSP - weighted cost"; }
    bool step() override { return performStep(); }
    bool canStepBack() const override { return false; }
    bool stepBack() override { return false; }
};
