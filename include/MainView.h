#pragma once
#include <gui/View.h>
#include <gui/HorizontalLayout.h>
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

class MainView : public gui::View
{
private:
    gui::HorizontalLayout _hlayout;
    DataRepository _repo;       // Single data source (owns the data)
    MapView _mapView;           // Rendering only
    SidePanelView _sidePanel;   // UI controls
    gui::Timer _timer;          // Timer for auto-stepping
    std::unique_ptr<SearchAlgorithm> _solver;
    bool _running = false;
    int _currentAlgorithmIdx = 0;

public:
    MainView()
        : _hlayout(2)
        , _timer(this, SOLVER_STEP_INTERVAL, false)
    {
        setMargins(0, 0, 0, 0);
        
        // Size limits
        _sidePanel.setSizeLimits(500, gui::Control::Limit::UseAsMin,
                                 866, gui::Control::Limit::UseAsMin);
        _mapView.setSizeLimits(1000, gui::Control::Limit::UseAsMin,
                               866, gui::Control::Limit::UseAsMin);

        _hlayout.append(_mapView, td::HAlignment::Left, td::VAlignment::Top);
        _hlayout.append(_sidePanel, td::HAlignment::Left, td::VAlignment::Top);
        setLayout(&_hlayout);

        // Wire up: repository -> mapView & sidePanel
        _mapView.setRepository(&_repo);
        _sidePanel.setRepository(&_repo);
        _sidePanel.populatePointNames(_repo.getCityNames());
        _sidePanel.syncSelectionDetails();

        // Wire up solver callback from side panel
        _sidePanel.setSolverCallback([this](int action, int algorithmIdx) {
            handleSolverAction(action, algorithmIdx);
        });

        // Initialize solver with default algorithm (BFS)
        selectAlgorithm(0);
    }

protected:
    // Timer callback for auto-stepping
    bool onTimer(gui::Timer* pTimer) override
    {
        if (pTimer == &_timer && _running && _solver) {
            if (!_solver->step()) {
                // Algorithm finished
                stopSolver();
            }
            _mapView.refresh();

            // Restart timer for next step if still running
            if (_running) {
                _timer.start();
            }
        }
        return true;
    }

private:
    // Handle solver control actions from SidePanelView
    // action: 0=start/pause, 1=step forward, -1=step back, 2=algorithm changed
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
            case 0:
                _solver = std::make_unique<BFSAlgorithm>();
                break;
            case 1:
                _solver = std::make_unique<DFSAlgorithm>();
                break;
            case 2:
                _solver = std::make_unique<NearestNeighborAlgorithm>();
                break;
            case 3:
                _solver = std::make_unique<SimulatedAnnealingAlgorithm>();
                break;
            case 4:
                _solver = std::make_unique<GeneticAlgorithmTSP>();
                break;
            default:
                _solver = std::make_unique<BFSAlgorithm>();
                break;
        }

        // Initialize solver with current data
        _solver->reset(&_repo);
        
        // Connect solver to MapView for visualization
        _mapView.setSolver(_solver.get());
        _mapView.refresh();
    }

    void startSolver()
    {
        if (!_solver) return;

        // Reset solver if it was completed
        if (_solver->isComplete()) {
            _solver->reset(&_repo);
        }

        _running = true;
        _timer.start();
    }

    void stopSolver()
    {
        _running = false;
        _timer.stop();
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
    }
};
