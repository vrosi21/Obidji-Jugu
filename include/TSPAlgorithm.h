#pragma once
#include "SearchAlgorithm.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>
#include <any>

// State snapshot for TSP step-back functionality
struct TSPState {
    std::vector<int> tour;
    std::vector<int> bestTour;
    double tourLength = std::numeric_limits<double>::max();
    double bestLength = std::numeric_limits<double>::max();
    int currentStep = 0;
    bool finished = false;
};

// Combined snapshot: base TSP state + opaque subclass-specific state
struct TSPSnapshot {
    TSPState baseState;
    std::any subclassState;  // subclass-specific fields (NN, SA, GA)
};

// Abstract base class for TSP optimization algorithms
// These algorithms optimize a tour over required cities:
// - Goal cities MUST be visited
// - Start city is included as anchor if present
// - Open cities are optional transit nodes via shortest-road paths
class TSPAlgorithm : public SearchAlgorithm {
public:
    TSPAlgorithm() : _rng(std::random_device{}()) {}
    ~TSPAlgorithm() override = default;

    // Override reset to initialize required city set for TSP
    void reset(DataRepository* repo) override {
        _repo = repo;
        _tspState = TSPState();
        _tspHistory.clear();
        _path.clear();
        _cityCount = 0;
        _requiredCities.clear();
        _startCityIdx = -1;
        _goalIndices.clear();

        if (!_repo) return;

        // Build required set:
        // - collect all non-blocked Goal cities
        // - include non-blocked Start city as anchor
        const auto& cities = _repo->cities();
        for (size_t i = 0; i < cities.size(); ++i) {
            if (cities[i].visitation_status == VisitationStatus::Blocked)
                continue;

            if (cities[i].visitation_status == VisitationStatus::Start)
                _startCityIdx = static_cast<int>(i);
            if (cities[i].visitation_status == VisitationStatus::Goal)
                _goalIndices.push_back(static_cast<int>(i));
        }

        // Start (if exists) is anchor city in the required tour
        if (_startCityIdx >= 0) {
            _requiredCities.push_back(_startCityIdx);
        }

        // Add all goals, avoiding duplicate if Start is also Goal
        for (int g : _goalIndices) {
            if (g != _startCityIdx) {
                _requiredCities.push_back(g);
            }
        }

        _cityCount = static_cast<int>(_requiredCities.size());
        
        // Initialize TSP-specific state
        initializeTSP();
    }

    // TSP uses its own step logic
    bool step() override {
        if (!_repo || _tspState.finished || _cityCount < 2) {
            _tspState.finished = true;
            return false;
        }

        // Save current state (base + subclass) for step-back
        TSPSnapshot snap;
        snap.baseState = _tspState;
        snap.subclassState = saveSubclassState();
        _tspHistory.push_back(std::move(snap));

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

        const auto& snap = _tspHistory.back();
        _tspState = snap.baseState;
        restoreSubclassState(snap.subclassState);
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

    // Save/restore subclass-specific state for step-back.
    // Default implementation stores nothing (suitable if subclass has no extra state).
    virtual std::any saveSubclassState() const { return std::any{}; }
    virtual void restoreSubclassState(const std::any& /*state*/) {}

    // Calculate Euclidean distance between two cities
    double calculateDistance(int i, int j) const {
        if (!_repo) return std::numeric_limits<double>::infinity();
        return _repo->getMetricDistance(i, j);
    }

    // Weighted transition cost:
    // Cost(A -> B) = Distance(A, B) * Weight(B)
    double calculateTransitionCost(int a, int b) const {
        if (!_repo) return std::numeric_limits<double>::infinity();
        const auto& cities = _repo->cities();
        if (b < 0 || b >= static_cast<int>(cities.size())) {
            return std::numeric_limits<double>::infinity();
        }

        const double d = calculateDistance(a, b);
        if (!std::isfinite(d)) return std::numeric_limits<double>::infinity();

        return d * cities[b].weight;
    }

    // Calculate total tour length (including return to start)
    double calculateTourLength(const std::vector<int>& tour) const {
        if (tour.size() < 2) return 0.0;
        double length = 0.0;
        for (size_t i = 0; i < tour.size(); ++i) {
            int a = tour[i];
            int b = tour[(i + 1) % tour.size()];
            double c = calculateTransitionCost(a, b);
            if (!std::isfinite(c)) return std::numeric_limits<double>::infinity();
            length += c;
        }
        return length;
    }

    // Generate a random tour of required cities only
    // Start city is pinned at position 0 if present
    std::vector<int> generateRandomTour() {
        // Start with required cities
        std::vector<int> tour = _requiredCities;
        if (tour.empty()) return tour;

        std::shuffle(tour.begin(), tour.end(), _rng);

        // If there's a Start city, move it to the front
        if (_startCityIdx >= 0) {
            auto it = std::find(tour.begin(), tour.end(), _startCityIdx);
            if (it != tour.end() && it != tour.begin()) {
                std::iter_swap(tour.begin(), it);
            }
        }

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
    std::vector<TSPSnapshot> _tspHistory;
    int _cityCount = 0;
    std::vector<int> _requiredCities;  // Start + Goal indices that must be in the tour
    int _startCityIdx = -1;            // index of Start city (-1 if none)
    std::vector<int> _goalIndices;     // indices of Goal cities (must visit)
    std::mt19937 _rng;
};
