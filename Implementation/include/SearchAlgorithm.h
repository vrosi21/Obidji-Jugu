#pragma once
#include "DataRepository.h"
#include <vector>
#include <set>
#include <map>

// State snapshot for step-back functionality
struct AlgorithmState {
    std::set<int> visited;
    std::vector<int> frontier;
    std::map<int, int> parent;  // child -> parent for path reconstruction
    int currentIdx = -1;
    bool finished = false;
    bool foundGoal = false;
};

// Abstract base class for graph search algorithms
class SearchAlgorithm {
public:
    SearchAlgorithm() = default;
    virtual ~SearchAlgorithm() = default;

    // Initialize/reset the algorithm with the data repository
    virtual void reset(DataRepository* repo) {
        _repo = repo;
        _state = AlgorithmState();
        _history.clear();
        _path.clear();
        _startIdx = -1;
        _goalIdx = -1;

        if (!_repo) return;

        // Find start and goal nodes
        const auto& cities = _repo->cities();
        for (size_t i = 0; i < cities.size(); ++i) {
            if (cities[i].visitation_status == VisitationStatus::Start) {
                _startIdx = static_cast<int>(i);
            }
            if (cities[i].visitation_status == VisitationStatus::Goal) {
                _goalIdx = static_cast<int>(i);
            }
        }

        // Initialize with start node if found
        if (_startIdx >= 0) {
            initializeFrontier(_startIdx);
        }
    }

    // Execute one step of the algorithm
    // Returns true if step was executed, false if algorithm is complete
    virtual bool step() {
        if (!_repo || _state.finished || _state.frontier.empty()) {
            _state.finished = true;
            return false;
        }

        // Save current state for step-back
        _history.push_back(_state);

        // Get next node from frontier (subclass-specific)
        int current = getNextFromFrontier();
        _state.currentIdx = current;

        // Check if we reached the goal
        if (current == _goalIdx) {
            _state.foundGoal = true;
            _state.finished = true;
            reconstructPath();
            return true;
        }

        // Skip if already visited
        if (_state.visited.count(current)) {
            return true;
        }

        // Mark as visited
        _state.visited.insert(current);

        // Expand frontier with neighbors
        expandFrontier(current);

        // Check if frontier is empty (no path found)
        if (_state.frontier.empty() && !_state.foundGoal) {
            _state.finished = true;
        }

        return true;
    }

    // Undo the last step
    virtual bool stepBack() {
        if (_history.empty()) {
            return false;
        }

        _state = _history.back();
        _history.pop_back();
        _path.clear();  // Clear path since we're going back

        return true;
    }

    // Check if we can step back (have history)
    virtual bool canStepBack() const {
        return !_history.empty();
    }

    // Check if algorithm has finished
    virtual bool isComplete() const {
        return _state.finished;
    }

    // Check if goal was found
    virtual bool foundGoal() const {
        return _state.foundGoal;
    }

    // Get visited node indices
    virtual std::vector<int> getVisited() const {
        return std::vector<int>(_state.visited.begin(), _state.visited.end());
    }

    // Get frontier node indices
    virtual const std::vector<int>& getFrontier() const {
        return _state.frontier;
    }

    // Get current path (only valid after goal is found)
    virtual const std::vector<int>& getPath() const {
        return _path;
    }

    // Get current node being processed
    virtual int getCurrentIdx() const {
        return _state.currentIdx;
    }

    // Get algorithm name for display
    virtual const char* getName() const = 0;

protected:
    // Initialize frontier with starting node - implemented by subclass
    virtual void initializeFrontier(int startIdx) = 0;

    // Get next node from frontier - implemented by subclass
    virtual int getNextFromFrontier() = 0;

    // Expand frontier with neighbors of current node - implemented by subclass
    virtual void expandFrontier(int currentIdx) = 0;

    // Get neighbors of a node (connected and not blocked)
    std::vector<int> getNeighbors(int idx) const {
        std::vector<int> neighbors;
        if (!_repo) return neighbors;

        const auto& cities = _repo->cities();
        auto connections = _repo->getConnections(idx);

        for (int connIdx : connections) {
            // Skip blocked nodes
            if (connIdx >= 0 && connIdx < static_cast<int>(cities.size())) {
                if (cities[connIdx].visitation_status != VisitationStatus::Blocked) {
                    neighbors.push_back(connIdx);
                }
            }
        }

        return neighbors;
    }

    // Reconstruct path from start to goal using parent pointers
    void reconstructPath() {
        _path.clear();
        if (!_state.foundGoal || _goalIdx < 0) return;

        int current = _goalIdx;
        while (current != -1) {
            _path.push_back(current);
            auto it = _state.parent.find(current);
            if (it != _state.parent.end()) {
                current = it->second;
            } else {
                break;
            }
        }

        // Reverse to get path from start to goal
        std::reverse(_path.begin(), _path.end());
    }

    DataRepository* _repo = nullptr;
    AlgorithmState _state;
    std::vector<AlgorithmState> _history;
    std::vector<int> _path;
    int _startIdx = -1;
    int _goalIdx = -1;
};
