#pragma once
#include "SearchAlgorithm.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>

// State snapshot for TSP step-back functionality
struct TSPState {
    std::vector<int> tour;
    std::vector<int> bestTour;
    double tourLength = std::numeric_limits<double>::max();
    double bestLength = std::numeric_limits<double>::max();
    int currentStep = 0;
    bool finished = false;
};

// Abstract base class for TSP optimization algorithms
// These algorithms find a tour visiting ALL cities (not just start-to-goal path)
class TSPAlgorithm : public SearchAlgorithm {
public:
    TSPAlgorithm() : _rng(std::random_device{}()) {}
    ~TSPAlgorithm() override = default;

    // Override reset to initialize for TSP (all cities, not start/goal)
    void reset(DataRepository* repo) override {
        _repo = repo;
        _tspState = TSPState();
        _tspHistory.clear();
        _path.clear();
        _cityCount = 0;

        if (!_repo) return;

        _cityCount = static_cast<int>(_repo->cities().size());
        
        // Initialize TSP-specific state
        initializeTSP();
    }

    // TSP uses its own step logic
    bool step() override {
        if (!_repo || _tspState.finished || _cityCount < 2) {
            _tspState.finished = true;
            return false;
        }

        // Save current state for step-back
        _tspHistory.push_back(_tspState);

        // Perform one step of the TSP algorithm (subclass-specific)
        bool continued = performStep();

        // Update best tour if current is better
        if (_tspState.tourLength < _tspState.bestLength) {
            _tspState.bestLength = _tspState.tourLength;
            _tspState.bestTour = _tspState.tour;
        }

        _tspState.currentStep++;
        return continued;
    }

    // Override stepBack for TSP
    bool stepBack() override {
        if (_tspHistory.empty()) {
            return false;
        }

        _tspState = _tspHistory.back();
        _tspHistory.pop_back();
        return true;
    }

    // TSP-specific getters
    const std::vector<int>& getTour() const { return _tspState.tour; }
    const std::vector<int>& getBestTour() const { return _tspState.bestTour; }
    double getTourLength() const { return _tspState.tourLength; }
    double getBestLength() const { return _tspState.bestLength; }
    int getCurrentStep() const { return _tspState.currentStep; }

    // Override getPath to return best tour for visualization
    const std::vector<int>& getPath() const override {
        return _tspState.bestTour;
    }

    // Override canStepBack for TSP history
    bool canStepBack() const override {
        return !_tspHistory.empty();
    }

    // Override isComplete
    bool isComplete() const override {
        return _tspState.finished;
    }

    // For TSP, visited means "in the current tour"
    std::vector<int> getVisited() const override {
        return _tspState.tour;
    }

    // TSP doesn't use frontier in the same way
    const std::vector<int>& getFrontier() const override {
        static std::vector<int> empty;
        return empty;
    }

    // Check if this is a TSP algorithm (for visualization differentiation)
    virtual bool isTSPAlgorithm() const { return true; }

protected:
    // Initialize TSP-specific state - implemented by subclass
    virtual void initializeTSP() = 0;

    // Perform one step of the algorithm - implemented by subclass
    // Returns true if algorithm should continue, false if finished
    virtual bool performStep() = 0;

    // Calculate Euclidean distance between two cities
    double calculateDistance(int i, int j) const {
        if (!_repo) return std::numeric_limits<double>::infinity();
        return _repo->getMetricDistance(i, j);
    }

    // Calculate total tour length (including return to start)
    double calculateTourLength(const std::vector<int>& tour) const {
        if (tour.size() < 2) return 0.0;
        double length = 0.0;
        for (size_t i = 0; i < tour.size(); ++i) {
            int a = tour[i];
            int b = tour[(i + 1) % tour.size()];
            double d = calculateDistance(a, b);
            if (!std::isfinite(d)) return std::numeric_limits<double>::infinity();
            length += d;
        }
        return length;
    }

    // Generate a random tour (permutation of all city indices)
    std::vector<int> generateRandomTour() {
        std::vector<int> component = _repo ? _repo->getLargestConnectedComponent() : std::vector<int>();
        if (!component.empty()) {
            std::shuffle(component.begin(), component.end(), _rng);
            return component;
        }
        std::vector<int> tour;
        tour.reserve(_cityCount);
        for (int i = 0; i < _cityCount; ++i) {
            tour.push_back(i);
        }
        std::shuffle(tour.begin(), tour.end(), _rng);
        return tour;
    }

    // 2-opt swap: reverse segment between i and j
    void twoOptSwap(std::vector<int>& tour, int i, int j) {
        std::reverse(tour.begin() + i, tour.begin() + j + 1);
    }

    // These are not used by TSP algorithms but required by base class
    void initializeFrontier(int) override {}
    int getNextFromFrontier() override { return -1; }
    void expandFrontier(int) override {}

    TSPState _tspState;
    std::vector<TSPState> _tspHistory;
    int _cityCount = 0;
    std::mt19937 _rng;
};
