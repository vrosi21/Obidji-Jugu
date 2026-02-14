#pragma once
#include "TSPAlgorithm.h"
#include <set>

// Nearest Neighbor algorithm for TSP
// Greedy constructive heuristic that builds tour by always visiting nearest unvisited city
class NearestNeighborAlgorithm : public TSPAlgorithm {
public:
    NearestNeighborAlgorithm(bool enable2Opt = false, int improvementCycles = 20)
        : _enable2Opt(enable2Opt),
          _configuredImprovementCycles(improvementCycles < 0 ? 0 : improvementCycles) {}
    ~NearestNeighborAlgorithm() override = default;

    const char* getName() const override {
        return "Nearest Neighbor";
    }

protected:
    void initializeTSP() override {
        _unvisited.clear();
        
        // Initialize unvisited set with required cities only (Goals + Start anchor)
        for (int idx : _requiredCities) {
            _unvisited.insert(idx);
        }

        // Start from Start city if set, otherwise first required city
        int startCity = (_startCityIdx >= 0) ? _startCityIdx
                      : (_requiredCities.empty() ? 0 : _requiredCities.front());

        if (!_requiredCities.empty()) {
            _tspState.tour.clear();
            _tspState.tour.push_back(startCity);
            _unvisited.erase(startCity);
            _currentCity = startCity;
        }

        _tspState.tourLength = 0.0;
        _tspState.bestTour.clear();
        _tspState.bestLength = std::numeric_limits<double>::max();
        _remaining2OptCycles = _configuredImprovementCycles;
    }

    bool performStep() override {
        if (_unvisited.empty()) {
            // Construction complete. Optionally perform 2-opt improvement cycles.
            if (_enable2Opt && _remaining2OptCycles > 0 && _tspState.tour.size() >= 4) {
                bool improved = applyBestTwoOptMove();
                _remaining2OptCycles--;
                _tspState.tourLength = calculateTourLength(_tspState.tour);

                if (improved && _remaining2OptCycles > 0) {
                    return true; // continue iterative 2-opt improvement
                }
            }

            // Finalize tour
            _tspState.tourLength = calculateTourLength(_tspState.tour);
            _tspState.bestTour = _tspState.tour;
            _tspState.bestLength = _tspState.tourLength;
            _tspState.finished = true;
            return false;
        }

        // Find nearest unvisited city
        int nearestCity = -1;
        double bestCost = std::numeric_limits<double>::max();

        for (int city : _unvisited) {
            double moveCost = calculateTransitionCost(_currentCity, city);
            if (!std::isfinite(moveCost)) continue; // skip unreachable
            if (moveCost < bestCost) {
                bestCost = moveCost;
                nearestCity = city;
            }
        }

        if (nearestCity >= 0) {
            // Add nearest city to tour
            _tspState.tour.push_back(nearestCity);
            _unvisited.erase(nearestCity);
            _currentCity = nearestCity;

            // If this was the last remaining city, finalize now so Show Solution
            // has a completed tour immediately.
            if (_unvisited.empty()) {
                _tspState.tourLength = calculateTourLength(_tspState.tour);
                _tspState.bestTour = _tspState.tour;
                _tspState.bestLength = _tspState.tourLength;
                if (_enable2Opt && _remaining2OptCycles > 0 && _tspState.tour.size() >= 4) {
                    return true; // enter 2-opt phase on next step
                }
                _tspState.finished = true;
                return false;
            }

            // Update partial tour length
            _tspState.tourLength = calculateTourLength(_tspState.tour);
            return true;
        } else {
            // No reachable city remains; terminate as infeasible to complete
            _tspState.finished = true;
            return false;
        }
    }

private:
    bool applyBestTwoOptMove()
    {
        const int n = static_cast<int>(_tspState.tour.size());
        if (n < 4) return false;

        int lo = (_startCityIdx >= 0) ? 1 : 0; // keep start pinned at position 0
        if (lo >= n - 2) return false;

        double bestLength = _tspState.tourLength;
        if (!std::isfinite(bestLength)) {
            bestLength = calculateTourLength(_tspState.tour);
        }

        int bestI = -1;
        int bestJ = -1;

        for (int i = lo; i < n - 1; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (j - i < 2) continue;

                std::vector<int> candidate = _tspState.tour;
                twoOptSwap(candidate, i, j);
                double candLen = calculateTourLength(candidate);
                if (std::isfinite(candLen) && candLen + 1e-9 < bestLength) {
                    bestLength = candLen;
                    bestI = i;
                    bestJ = j;
                }
            }
        }

        if (bestI >= 0) {
            twoOptSwap(_tspState.tour, bestI, bestJ);
            _tspState.tourLength = bestLength;
            return true;
        }
        return false;
    }

    std::set<int> _unvisited;
    int _currentCity = 0;
    bool _enable2Opt = false;
    int _configuredImprovementCycles = 20;
    int _remaining2OptCycles = 0;
};
