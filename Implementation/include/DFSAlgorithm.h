#pragma once
#include "SearchAlgorithm.h"
#include <vector>

// Depth-First Search algorithm implementation
// Uses LIFO stack to explore nodes depth-first
class DFSAlgorithm : public SearchAlgorithm {
public:
    DFSAlgorithm() = default;
    ~DFSAlgorithm() override = default;

    void reset(DataRepository* repo) override {
        _stack.clear();
        SearchAlgorithm::reset(repo);
    }

    const char* getName() const override {
        return "DFS";
    }

protected:
    void initializeFrontier(int startIdx) override {
        _stack.clear();
        _stack.push_back(startIdx);
        _state.frontier.clear();
        _state.frontier.push_back(startIdx);
        _state.parent[startIdx] = -1;  // Start has no parent
    }

    int getNextFromFrontier() override {
        if (_stack.empty()) return -1;
        
        int top = _stack.back();
        _stack.pop_back();
        
        // Update frontier representation for visualization
        updateFrontierFromStack();
        
        return top;
    }

    void expandFrontier(int currentIdx) override {
        auto neighbors = getNeighbors(currentIdx);
        
        // Add neighbors in reverse order so first neighbor is processed first
        for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
            int neighbor = *it;
            // Only add unvisited nodes
            if (_state.visited.count(neighbor) == 0 && !isInStack(neighbor)) {
                _stack.push_back(neighbor);
                _state.parent[neighbor] = currentIdx;
            }
        }
        
        updateFrontierFromStack();
    }

private:
    // Check if a node is already in the stack
    bool isInStack(int idx) const {
        for (int i : _stack) {
            if (i == idx) return true;
        }
        return false;
    }

    // Sync frontier vector with stack for visualization
    void updateFrontierFromStack() {
        _state.frontier.clear();
        for (int i : _stack) {
            _state.frontier.push_back(i);
        }
    }

    std::vector<int> _stack;
};
