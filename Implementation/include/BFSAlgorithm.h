#pragma once
#include "SearchAlgorithm.h"
#include <deque>

// Breadth-First Search algorithm implementation
// Uses FIFO queue to explore nodes level by level
class BFSAlgorithm : public SearchAlgorithm {
public:
    BFSAlgorithm() = default;
    ~BFSAlgorithm() override = default;

    void reset(DataRepository* repo) override {
        _queue.clear();
        SearchAlgorithm::reset(repo);
    }

    const char* getName() const override {
        return "BFS";
    }

protected:
    void initializeFrontier(int startIdx) override {
        _queue.clear();
        _queue.push_back(startIdx);
        _state.frontier.clear();
        _state.frontier.push_back(startIdx);
        _state.parent[startIdx] = -1;  // Start has no parent
    }

    int getNextFromFrontier() override {
        if (_queue.empty()) return -1;
        
        int front = _queue.front();
        _queue.pop_front();
        
        // Update frontier representation for visualization
        updateFrontierFromQueue();
        
        return front;
    }

    void expandFrontier(int currentIdx) override {
        auto neighbors = getNeighbors(currentIdx);
        
        for (int neighbor : neighbors) {
            // Only add unvisited nodes
            if (_state.visited.count(neighbor) == 0 && !isInQueue(neighbor)) {
                _queue.push_back(neighbor);
                _state.parent[neighbor] = currentIdx;
            }
        }
        
        updateFrontierFromQueue();
    }

private:
    // Check if a node is already in the queue
    bool isInQueue(int idx) const {
        for (int i : _queue) {
            if (i == idx) return true;
        }
        return false;
    }

    // Sync frontier vector with queue for visualization
    void updateFrontierFromQueue() {
        _state.frontier.clear();
        for (int i : _queue) {
            _state.frontier.push_back(i);
        }
    }

    std::deque<int> _queue;
};
