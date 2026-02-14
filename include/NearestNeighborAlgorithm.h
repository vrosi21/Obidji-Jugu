#pragma once
#include "TSPAlgorithm.h"
#include <set>

// Nearest Neighbor algorithm for TSP
// Greedy constructive heuristic that builds tour by always visiting nearest unvisited city
class NearestNeighborAlgorithm : public TSPAlgorithm {
public:
    NearestNeighborAlgorithm() = default;
    ~NearestNeighborAlgorithm() override = default;

    const char* getName() const override {
        return "Nearest Neighbor";
    }

protected:
    void initializeTSP() override {
        _unvisited.clear();
        
        // Initialize unvisited set with only eligible (non-Blocked) cities
        for (int idx : _eligibleCities) {
            _unvisited.insert(idx);
        }

        // Start from Start city if set, otherwise first eligible city
        int startCity = (_startCityIdx >= 0) ? _startCityIdx
                      : (_eligibleCities.empty() ? 0 : _eligibleCities.front());

        if (!_eligibleCities.empty()) {
            _tspState.tour.clear();
            _tspState.tour.push_back(startCity);
            _unvisited.erase(startCity);
            _currentCity = startCity;
        }

        _tspState.tourLength = 0.0;
        _tspState.bestTour.clear();
        _tspState.bestLength = std::numeric_limits<double>::max();
    }

    bool performStep() override {
        if (_unvisited.empty()) {
            // Tour complete - calculate final length including return to start
            _tspState.tourLength = calculateTourLength(_tspState.tour);
            _tspState.finished = true;
            return false;
        }

        // Find nearest unvisited city
        int nearestCity = -1;
        double nearestDist = std::numeric_limits<double>::max();

        for (int city : _unvisited) {
            double dist = calculateDistance(_currentCity, city);
            if (!std::isfinite(dist)) continue; // skip unreachable
            if (dist < nearestDist) {
                nearestDist = dist;
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
    std::set<int> _unvisited;
    int _currentCity = 0;
};
