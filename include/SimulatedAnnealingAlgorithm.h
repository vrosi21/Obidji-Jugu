#pragma once
#include "TSPAlgorithm.h"
#include <set>

// Simulated Annealing algorithm for TSP
// Metaheuristic that uses temperature-based acceptance to escape local minima
class SimulatedAnnealingAlgorithm : public TSPAlgorithm {
public:
    enum class InitialSolutionMode {
        Random = 0,
        NearestNeighbor = 1
    };

    SimulatedAnnealingAlgorithm(
        double initialTemp = 10000.0,
        double minTemp = 0.001,
        double coolingRate = 0.995,
        int iterationsPerTemperature = 25,
        InitialSolutionMode initialMode = InitialSolutionMode::Random
    ) : _initialTemp(initialTemp),
        _minTemp(minTemp),
        _coolingRate(coolingRate),
        _iterationsPerTemperature(iterationsPerTemperature > 0 ? iterationsPerTemperature : 25),
        _initialMode(initialMode) {}

    ~SimulatedAnnealingAlgorithm() override = default;

    const char* getName() const override {
        return "Simulated Annealing";
    }

    // Get current temperature for visualization
    double getTemperature() const { return _temperature; }

protected:
    void initializeTSP() override {
        _temperature = _initialTemp;
        _iterAtCurrentTemp = 0;

        // Generate initial tour (random or NN)
        if (_initialMode == InitialSolutionMode::NearestNeighbor) {
            _tspState.tour = buildNearestNeighborInitialTour();
        } else {
            _tspState.tour = generateRandomTour();
        }
        _tspState.tourLength = calculateTourLength(_tspState.tour);
        _tspState.bestTour = _tspState.tour;
        _tspState.bestLength = _tspState.tourLength;

        // For random number generation
        _realDist = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    bool performStep() override {
        if (_temperature <= _minTemp || _cityCount < 2) {
            _tspState.finished = true;
            return false;
        }

        // Generate neighbor solution using 2-opt swap
        std::vector<int> newTour = _tspState.tour;
        int tourSize = static_cast<int>(newTour.size());
        if (tourSize < 4) {
            // With fewer than 4 required nodes, 2-opt is not meaningful.
            // Keep current valid tour as final.
            _tspState.finished = true;
            return false;
        }
        
        // Select two random positions to swap (preserve Start at index 0)
        int lo = (_startCityIdx >= 0) ? 1 : 0; // don't move the Start city
        std::uniform_int_distribution<int> posDist(lo, tourSize - 1);
        int i = posDist(_rng);
        int j = posDist(_rng);
        
        // Ensure i < j and they're not adjacent
        if (i > j) std::swap(i, j);
        if (j - i < 2) {
            j = std::min(i + 2, tourSize - 1);
            if (j - i < 2) {
                // Tour segment too small to swap
                _temperature *= _coolingRate;
                return _temperature > _minTemp;
            }
        }

        // Perform 2-opt swap
        twoOptSwap(newTour, i, j);

        // Calculate new tour length
        double newLength = calculateTourLength(newTour);
        if (!std::isfinite(newLength)) {
            // Reject infeasible tour edges (disconnected); treat as not accepted
            _temperature *= _coolingRate;
            return _temperature > _minTemp;
        }
        double delta = newLength - _tspState.tourLength;

        // Accept or reject the new solution
        bool accept = false;
        if (delta < 0) {
            // Always accept improvements
            accept = true;
        } else {
            // Accept worse solutions with probability exp(-delta/T)
            double probability = std::exp(-delta / _temperature);
            accept = _realDist(_rng) < probability;
        }

        if (accept) {
            _tspState.tour = newTour;
            _tspState.tourLength = newLength;
        }

        // Cool down after configured number of iterations on same temperature
        _iterAtCurrentTemp++;
        if (_iterAtCurrentTemp >= _iterationsPerTemperature) {
            _temperature *= _coolingRate;
            _iterAtCurrentTemp = 0;
        }

        return _temperature > _minTemp;
    }

    std::vector<int> buildNearestNeighborInitialTour()
    {
        std::vector<int> tour;
        if (_requiredCities.empty()) return tour;

        std::set<int> unvisited(_requiredCities.begin(), _requiredCities.end());
        int current = (_startCityIdx >= 0) ? _startCityIdx : *unvisited.begin();

        tour.push_back(current);
        unvisited.erase(current);

        while (!unvisited.empty()) {
            int bestCity = -1;
            double bestCost = std::numeric_limits<double>::max();

            for (int c : unvisited) {
                double cost = calculateTransitionCost(current, c);
                if (std::isfinite(cost) && cost < bestCost) {
                    bestCost = cost;
                    bestCity = c;
                }
            }

            if (bestCity < 0) {
                // fallback: append remaining deterministically
                for (int c : unvisited) tour.push_back(c);
                break;
            }

            tour.push_back(bestCity);
            unvisited.erase(bestCity);
            current = bestCity;
        }

        return tour;
    }

private:
    double _initialTemp;
    double _minTemp;
    double _coolingRate;
    int _iterationsPerTemperature = 25;
    int _iterAtCurrentTemp = 0;
    InitialSolutionMode _initialMode = InitialSolutionMode::Random;
    double _temperature = 0.0;
    std::uniform_real_distribution<double> _realDist;
};
