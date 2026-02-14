#pragma once
#include <gui/View.h>
#include <gui/HorizontalLayout.h>
#include <gui/SplitterLayout.h>
#include <gui/Timer.h>
#include <memory>
#include "DataRepository.h"
#include "MapView.h"
#include "SidePanelView.h"
#include "SearchAlgorithm.h"
#include "BFSAlgorithm.h"
#include "DFSAlgorithm.h"
#include "NearestNeighborAlgorithm.h"
#include "SimulatedAnnealingAlgorithm.h"
#include "GeneticAlgorithmTSP.h"

// Timer interval for auto-stepping (in seconds)
constexpr float SOLVER_STEP_INTERVAL = 0.5f;
constexpr float SOLUTION_ANIM_INTERVAL = 0.05f;


class MainView : public gui::View
{
private:
    gui::SplitterLayout _splitter{gui::SplitterLayout::Orientation::Horizontal, gui::SplitterLayout::AuxiliaryCell::Second};
    DataRepository _repo;       // Single data source (owns the data)
    MapView _mapView;           // Rendering only
    SidePanelView _sidePanel;   // UI controls
    gui::Timer _timer;          // Timer for auto-stepping
    std::unique_ptr<SearchAlgorithm> _solver;
    bool _running = false;
    int _currentAlgorithmIdx = 0;
    gui::Timer _solutionTimer;
    bool _solutionRunning = false;


public:
    MainView()
        : _splitter(gui::SplitterLayout::Orientation::Horizontal, gui::SplitterLayout::AuxiliaryCell::Second)
        , _timer(this, SOLVER_STEP_INTERVAL, false)
        , _solutionTimer(this, SOLUTION_ANIM_INTERVAL, false)
    {
        setMargins(0, 0, 0, 0);
        
        // Size limits - allow resizing with reasonable minimums
        // Side panel has a minimum width to keep controls usable
        _sidePanel.setSizeLimits(300, gui::Control::Limit::UseAsMin,
                                 200, gui::Control::Limit::UseAsMin);
        // Map view can scale down but has minimum to stay readable
        _mapView.setSizeLimits(400, gui::Control::Limit::UseAsMin,
                               350, gui::Control::Limit::UseAsMin);

        _splitter.setContent(_mapView, _sidePanel);
        setLayout(&_splitter);

        // Wire up: repository -> mapView & sidePanel
        _mapView.setRepository(&_repo);
        _sidePanel.setRepository(&_repo);
        _sidePanel.populatePointNames(_repo.getCityNames());
        _sidePanel.syncSelectionDetails();

        // Wire up solver callback from side panel
        _sidePanel.setSolverCallback([this](int action, int algorithmIdx) {
            handleSolverAction(action, algorithmIdx);
        });

        // When data changes (city added/deleted/status changed), reset the solver
        _repo.setOnDataChanged([this]() {
            if (_running) stopSolver();

            _solutionRunning = false;
            _solutionTimer.stop();
            _mapView.stopSolutionAnimation();

            if (_solver) {
                _solver->reset(&_repo);
                _mapView.refresh();
                refreshStepButtons();
            }
        });

        // Initialize solver with default algorithm (BFS)
        selectAlgorithm(0);

        // Initial button state
        refreshStepButtons();
    }

protected:
    // Timer callback for auto-stepping
    bool onTimer(gui::Timer* pTimer) override
    {
        if (pTimer == &_timer && _running && _solver) {
            if (!_solver->step()) stopSolver();
            _mapView.refresh();
            if (_running) _timer.start();
            refreshStepButtons();
            return true;
        }

        if (pTimer == &_solutionTimer && _solutionRunning) {
            if (!_mapView.advanceSolutionAnimation(1)) {
                _solutionRunning = false;
                _solutionTimer.stop();
            }
            else {
                _solutionTimer.start(); // one-shot restart
            }
            _mapView.refresh();
            return true;
        }

        return true;
    }


private:
    // Stop both solve execution and final-solution animation
    void stopAllExecution()
    {
        _running = false;
        _timer.stop();
        _solutionRunning = false;
        _solutionTimer.stop();
        _mapView.stopSolutionAnimation();
    }

    // Handle solver control actions from SidePanelView
    // action: 0=start/pause, 1=step forward, -1=step back, 2=algorithm changed, 3=show final solution, 4=reset
    void handleSolverAction(int action, int algorithmIdx)
    {
        switch (action) {
            case 0:  // Start/Pause
                if (_running) {
                    stopSolver();
                } else {
                    startSolver();
                }
                break;
            case 1:  // Step forward
                stepForward();
                break;
            case -1: // Step back
                stepBackward();
                break;
            case 2:  // Algorithm changed
                selectAlgorithm(algorithmIdx);
                break;
            case 3: // Show Solution (animated)
                if (_running) stopSolver();

                // Build final solution first (run remaining iterations immediately)
                if (_solver) {
                    while (!_solver->isComplete()) {
                        if (!_solver->step()) break;
                    }
                }

                _solutionRunning = false;
                _solutionTimer.stop();

                _mapView.startSolutionAnimation();  
                if (_mapView.isSolutionAnimating()) {
                    _solutionRunning = true;
                    _solutionTimer.start();
                }
                _mapView.refresh();
                refreshStepButtons();
                break;
            case 4: // Reset solver state + clear drawn algorithm paths/animation
                stopAllExecution();
                if (_solver) {
                    _solver->reset(&_repo);
                }
                _mapView.refresh();
                refreshStepButtons();
                break;

        }
    }

    void selectAlgorithm(int algorithmIdx)
    {
        // Stop any running solver first
        if (_running) {
            stopSolver();
        }

        _currentAlgorithmIdx = algorithmIdx;

        // Create new solver based on selection
        switch (algorithmIdx) {
            case 0: _solver = std::make_unique<NearestNeighborAlgorithm>(); break;
            case 1: _solver = std::make_unique<SimulatedAnnealingAlgorithm>(); break;
            case 2: _solver = std::make_unique<GeneticAlgorithmTSP>(); break;
            default: _solver = std::make_unique<NearestNeighborAlgorithm>(); break;
        }

        // Initialize solver with current data
        _solver->reset(&_repo);
        
        // Connect solver to MapView for visualization
        _mapView.setSolver(_solver.get());

        _solutionRunning = false;
        _solutionTimer.stop();
        _mapView.stopSolutionAnimation();

        refreshStepButtons();
        _mapView.refresh();
    }

    // Update step button enable/disable state based on current solver state
    void refreshStepButtons()
    {
        bool canFwd = _solver && !_solver->isComplete();
        bool canBwd = _solver && _solver->canStepBack();
        _sidePanel.updateStepButtons(_running, canFwd, canBwd);
        _sidePanel.updateExecutionState(_running);
    }

    void startSolver()
    {
        if (!_solver) return;

        // Reset solver if it was completed
        if (_solver->isComplete()) {
            _solver->reset(&_repo);
        }


        _solutionRunning = false;
        _solutionTimer.stop();
        _mapView.stopSolutionAnimation();

        
        _running = true;
        _timer.start();
        refreshStepButtons();
    }

    void stopSolver()
    {
        _running = false;
        _timer.stop();
        refreshStepButtons();
    }

    void stepForward()
    {
        if (!_solver) return;

        // Stop auto-run if active
        if (_running) {
            stopSolver();
        }

        // Reset if complete
        if (_solver->isComplete()) {
            _solver->reset(&_repo);
        }

        _solver->step();
        _mapView.refresh();
        refreshStepButtons();
    }

    void stepBackward()
    {
        if (!_solver) return;

        // Stop auto-run if active
        if (_running) {
            stopSolver();
        }

        _solver->stepBack();
        _mapView.refresh();
        refreshStepButtons();
    }
};
