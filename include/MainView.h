#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include <gui/Timer.h>
#include <memory>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include "DataRepository.h"
#include "MapView.h"
#include "MapSplitterLayout.h"
#include "SidePanelScroller.h"
#include "SearchAlgorithm.h"
#include "BFSAlgorithm.h"
#include "DFSAlgorithm.h"
#include "NearestNeighborAlgorithm.h"
#include "SimulatedAnnealingAlgorithm.h"
#include "GeneticAlgorithmTSP.h"
#include "ExactTSPAlgorithm.h"

// Timer interval for auto-stepping (in seconds)
constexpr float SOLVER_STEP_INTERVAL = 0.001f;
constexpr float SOLUTION_ANIM_INTERVAL = 0.05f;


class MainView : public gui::View
{
private:
    struct SolveJob {
        DataRepository repo;
        std::unique_ptr<SearchAlgorithm> solver;
        std::atomic<bool> cancel{false}, done{false};
        bool failed = false;
        bool exact = false;
        bool benchmarkSelected = false;
        double benchmark = 0;
    };
    std::shared_ptr<SolveJob> _job;
    // Keeps the repository referenced by the completed solver alive.
    std::shared_ptr<SolveJob> _completedJob;
    MapSplitterLayout _splitter;
    DataRepository _repo;       // Single data source (owns the data)
    SplitterMapView _mapView;      // Right auxiliary map with bounded divider
    SidePanelScroller _sidePanelScroller; // Scrollable wrapper
    SidePanelView& _sidePanel = _sidePanelScroller.panel; // convenient alias
    gui::Timer _timer;          // Timer for auto-stepping
    std::unique_ptr<SearchAlgorithm> _solver;
    bool _running = false;
    int _currentAlgorithmIdx = 0;
    gui::Timer _solutionTimer;
    bool _solutionRunning = false;
    int _solveTickCounter = 0;
    int _solutionTickCounter = 0;
    bool _stepAnimRunning = false;   // step-tour animation in progress (SA/GA per-step replay)
    int _stepAnimTickCounter = 0;


public:
    ~MainView() { if (_job) _job->cancel = true; }
    MainView()
        : _mapView(_splitter)
        , _timer(this, SOLVER_STEP_INTERVAL, false)
        , _solutionTimer(this, SOLUTION_ANIM_INTERVAL, false)
    {
        setMargins(0, 0, 0, 0);

        // Resolve JSON data file path via framework resource system
        // (build-location-independent: -devResPath resolves ':' prefix)
        td::String jsonResPath = getResFileName(":exYu");
        _repo.init(jsonResPath.c_str());
        
        // Keep the application usable at small sizes. The splitter remains
        // freely draggable; MapView adapts narrow cells with top/bottom bands.
        setSizeLimits(700, gui::Control::Limit::UseAsMin,
                      minimumApplicationContentHeight(), gui::Control::Limit::UseAsMin);

        // Side panel scroller has a minimum width to keep controls usable
        _sidePanelScroller.setSizeLimits(300, gui::Control::Limit::UseAsMin,
                                        200, gui::Control::Limit::UseAsMin);
        // Absolute fallback only. The vector map itself always keeps its
        // 1000:866 aspect ratio inside the available cell.
        _mapView.setSizeLimits(700, gui::Control::Limit::UseAsMin);

        // Preserve the draggable divider. Narrow map cells switch to a compact
        // presentation rather than attempting to mutate layout limits while
        // natID is in the middle of a splitter operation.
        _splitter.setSpaceBetweenCells(4);
        _splitter.setContent(_sidePanelScroller, _mapView);
        setLayout(&_splitter);

        // Wire up: repository -> mapView & sidePanel
        _mapView.setRepository(&_repo);
        _sidePanel.setRepository(&_repo);
        _sidePanel.populatePointNames(_repo.getCityNames());
        _sidePanel.syncSelectionDetails();

        // Map interactions:
        // - left click city: select it in side panel
        // - right click another city: toggle connection from currently selected city to clicked city
        _mapView.setOnPrimaryCityClick([this](int cityIdx) {
            _sidePanel.selectPointIndex(cityIdx);
            _mapView.refresh();
        });
        _mapView.setOnSecondaryCityClick([this](int cityIdx) {
            int selectedIdx = _sidePanel.getSelectedPointIndex();

            // If nothing is selected yet, pick the clicked city as selection
            if (selectedIdx < 0) {
                _sidePanel.selectPointIndex(cityIdx);
                _mapView.refresh();
                return;
            }

            // Right click on non-selected city toggles connection selected <-> clicked
            if (selectedIdx == cityIdx) {
                return;
            }

            if (_repo.hasConnection(selectedIdx, cityIdx)) {
                _repo.removeConnection(selectedIdx, cityIdx);
            } else {
                _repo.addConnection(selectedIdx, cityIdx);
            }

            _sidePanel.syncSelectionDetails();
            _mapView.refresh();
        });

        // Wire up solver callback from side panel
        _sidePanel.setSolverCallback([this](int action, int algorithmIdx) {
            handleSolverAction(action, algorithmIdx);
        });

        // When data changes (city added/deleted/status changed), reset the solver
        _repo.setOnDataChanged([this]() {
            stopAllExecution();
            _mapView.clearExactMinimum();
            _sidePanel.clearResult();
            if (_solver) {
                _solver->reset(&_repo);
            }

            // Clear canvas slate until an explicit execute action (start/step/show).
            _mapView.setSolver(nullptr);
            _mapView.refreshData();
            refreshStepButtons();
        });

        // Initialize solver with default algorithm (BFS)
        selectAlgorithm(0);

        // Initial button state
        refreshStepButtons();
    }

protected:
    int getTicksPerAdvanceFromSpeed() const
    {
        // Speed slider is 1..10.
        // 1 => advance every 10 timer ticks (slowest)
        // 10 => advance every 1 timer tick (fastest)
        int speed = _sidePanel.getExecutionSpeedLevel();
        int ticks = 11 - speed;
        if (ticks < 1) ticks = 1;
        if (ticks > 10) ticks = 10;
        return ticks;
    }

    // How many expanded-path edges to advance per animation tick.
    // Higher speed => more edges per tick => faster path draw.
    size_t getStepAnimEdgesPerTick() const
    {
        int speed = _sidePanel.getExecutionSpeedLevel();
        // speed 1 => 1 edge/tick, speed 10 => 10 edges/tick
        return static_cast<size_t>(speed < 1 ? 1 : speed);
    }

    // Start step-tour animation for the current solver state (SA or GA).
    // Returns true if animation was started.
    bool beginStepTourAnimationForCurrentAlgo()
    {
        if (!_solver) return false;

        auto* ga = dynamic_cast<GeneticAlgorithmTSP*>(_solver.get());
        if (ga) {
            const auto& pop = ga->getPopulation();
            if (!pop.empty()) {
                _mapView.queueStepTourFrames(pop);
                if (_mapView.isStepTourAnimating()) {
                    _stepAnimRunning = true;
                    _stepAnimTickCounter = 0;
                    return true;
                }
            }
            return false;
        }

        auto* sa = dynamic_cast<SimulatedAnnealingAlgorithm*>(_solver.get());
        if (sa) {
            const auto& tour = sa->getTour();
            if (tour.size() >= 2) {
                _mapView.startStepTourAnimation(tour);
                if (_mapView.isStepTourAnimating()) {
                    _stepAnimRunning = true;
                    _stepAnimTickCounter = 0;
                    return true;
                }
            }
            return false;
        }

        return false;
    }

    // Timer callback for auto-stepping
    bool onTimer(gui::Timer* pTimer) override
    {
        if (pTimer == &_timer && _running && _job) {
            if (!_job->done.load()) {
                _timer.start();
                return true;
            }
            if (_job->failed) {
                stopAllExecution();
                _sidePanel.clearResult();
                refreshStepButtons();
                return true;
            }
            _solver = std::move(_job->solver);
            _completedJob = _job;
            _mapView.setBenchmark(_job->benchmark, _job->exact);
            _job.reset();
            stopSolver();
            ensureMapSolverAttached();
            if (auto* tsp = dynamic_cast<TSPAlgorithm*>(_solver.get())) {
                const auto& tour = tsp->getBestTour().empty() ? tsp->getTour() : tsp->getBestTour();
                _sidePanel.showResult(tour);
            }
            _mapView.startSolutionAnimation();
            _solutionRunning = _mapView.isSolutionAnimating();
            if (_solutionRunning) _solutionTimer.start();
            _mapView.refresh();
            refreshStepButtons();
            return true;
        }

        if (pTimer == &_solutionTimer && _solutionRunning) {
            int ticksPerAdvance = getTicksPerAdvanceFromSpeed();
            _solutionTickCounter++;

            if (_solutionTickCounter < ticksPerAdvance) {
                _solutionTimer.start();
                return true;
            }

            _solutionTickCounter = 0;
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

        // Manual step forward/back playback for SA/GA:
        // reuse solution timer to advance one-step path visualization
        if (pTimer == &_solutionTimer && !_running && _stepAnimRunning && _mapView.isStepTourAnimating()) {
            _stepAnimTickCounter++;
            int ticksNeeded = getTicksPerAdvanceFromSpeed();

            if (_stepAnimTickCounter < ticksNeeded) {
                _solutionTimer.start();
                return true;
            }

            _stepAnimTickCounter = 0;
            size_t edges = getStepAnimEdgesPerTick();

            if (!_mapView.advanceStepTourAnimation(edges)) {
                _stepAnimRunning = false;
                _mapView.stopStepTourAnimation();
                _solutionTimer.stop();
            } else {
                _solutionTimer.start();
            }

            _mapView.refresh();
            refreshStepButtons();
            return true;
        }

        return true;
    }


private:
    bool canExecuteAlgorithms() const
    {
        const auto& cities = _repo.cities();
        int startIdx = -1;
        std::vector<int> goals;

        for (int i = 0; i < static_cast<int>(cities.size()); ++i) {
            if (cities[i].visitation_status == VisitationStatus::Start) {
                startIdx = i;
            }
            if (cities[i].visitation_status == VisitationStatus::Goal) {
                goals.push_back(i);
            }
        }

        if (startIdx < 0 || goals.empty()) return false;

        for (int g : goals) {
            double d = _repo.getMetricDistance(startIdx, g);
            if (std::isfinite(d)) {
                return true;
            }
        }
        return false;
    }

    // Stop both solve execution and final-solution animation
    void stopAllExecution()
    {
        if (_job) { _job->cancel = true; _job.reset(); }
        _running = false;
        _timer.stop();
        _solveTickCounter = 0;
        _solutionRunning = false;
        _solutionTimer.stop();
        _solutionTickCounter = 0;
        _stepAnimRunning = false;
        _stepAnimTickCounter = 0;
        _mapView.stopSolutionAnimation();
        _mapView.stopStepTourAnimation();
    }

    void ensureMapSolverAttached()
    {
        if (_solver) {
            _mapView.setSolver(_solver.get());
        }
    }

    // Handle solver control actions from SidePanelView
    // action: 0=start/pause, 1=step forward, -1=step back, 2=algorithm changed, 3=show final solution, 4=reset
    void handleSolverAction(int action, int algorithmIdx)
    {
        switch (action) {
            case 0:  // Solve / Resolve: always use the latest parameters
                startSolver();
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
            case 3: // Legacy action: same unified solve flow
                startSolver();
                break;
            case 5: // Replay the completed route without restarting the solver.
                // SidePanelView only dispatches Replay for a valid result.
                if (!_running && _solver && _solver->isComplete()) {
                    stopAllExecution();
                    ensureMapSolverAttached();
                    _mapView.startSolutionAnimation();
                    _solutionTickCounter = 0;
                    _solutionRunning = _mapView.isSolutionAnimating();
                    if (_solutionRunning) _solutionTimer.start();
                    _mapView.refresh();
                    refreshStepButtons();
                }
                break;
            case 4: // Reset solver state + clear drawn algorithm paths/animation
                stopAllExecution();

                // Re-create the solver with latest side-panel parameters
                selectAlgorithm(_currentAlgorithmIdx);

                // Keep map clear until user explicitly executes (start/step/show).
                _mapView.setSolver(nullptr);
                _mapView.refresh();
                refreshStepButtons();
                break;

        }
    }

    void selectAlgorithm(int algorithmIdx)
    {
        // Side-panel change: stop all execution and clear any active animation.
        stopAllExecution();
        _sidePanel.clearResult();

        _currentAlgorithmIdx = algorithmIdx;

        // Create new solver based on selection
        switch (algorithmIdx) {
            case 0:
                _solver = std::make_unique<NearestNeighborAlgorithm>(
                    _sidePanel.isNN2OptEnabled(),
                    _sidePanel.getNNImprovementCycles());
                break;
            case 1:
                _solver = std::make_unique<SimulatedAnnealingAlgorithm>(
                    _sidePanel.getSAInitialTemperature(),
                    0.001,
                    _sidePanel.getSACoolingRateAlpha(),
                    _sidePanel.getSAIterationsPerTemperature(),
                    _sidePanel.useNearestNeighborAsSAInitialSolution()
                        ? SimulatedAnnealingAlgorithm::InitialSolutionMode::NearestNeighbor
                        : SimulatedAnnealingAlgorithm::InitialSolutionMode::Random);
                break;
            case 2:
                _solver = std::make_unique<GeneticAlgorithmTSP>(
                    _sidePanel.getGAPopulationSize(),
                    _sidePanel.getGAMutationRate(),
                    _sidePanel.getGANumberOfGenerations(),
                    _sidePanel.getGACrossoverRate(),
                    static_cast<GeneticAlgorithmTSP::SelectionMethod>(_sidePanel.getGASelectionMethodIndex()),
                    static_cast<GeneticAlgorithmTSP::MutationOperator>(_sidePanel.getGAMutationOperatorIndex()),
                    _sidePanel.getGAElitismPercentage());
                break;
            case 3: _solver = std::make_unique<ExactTSPAlgorithm>(); break;
            default: _solver = std::make_unique<NearestNeighborAlgorithm>(); break;
        }

        // Initialize solver with current data
        _solver->reset(&_repo);

        // Keep canvas slate clean until explicit execute action.
        _mapView.setSolver(nullptr);

        _solutionRunning = false;
        _solutionTimer.stop();
        _solutionTickCounter = 0;
        _mapView.stopSolutionAnimation();

        refreshStepButtons();
        _mapView.refresh();
    }

    // Update step button enable/disable state based on current solver state
    void refreshStepButtons()
    {
        bool canRun = canExecuteAlgorithms();
        bool canFwd = _solver && !_solver->isComplete() && canRun;
        bool canBwd = _solver && _solver->canStepBack() && canRun;
        _sidePanel.updateStepButtons(_running, canFwd, canBwd);
        _sidePanel.updateExecutionState(_running);
    }

    void startSolver()
    {
        if (!canExecuteAlgorithms()) {
            stopAllExecution();
            _sidePanel.clearResult();
            _mapView.setSolver(nullptr);
            _mapView.refresh();
            refreshStepButtons();
            return;
        }
        // Resolve is a fresh run, not a replay of the previous best tour.
        selectAlgorithm(_currentAlgorithmIdx);
        _mapView.setSolver(nullptr);
        _mapView.refresh();
        _sidePanel.btnSolve.setTitle(tr("Resolve"));
        _mapView.clearExactMinimum();
        auto job = std::make_shared<SolveJob>();
        job->repo = _repo;
        job->repo.setOnDataChanged({});
        size_t required = 0;
        for (const auto& city : job->repo.cities())
            if (city.visitation_status == VisitationStatus::Start ||
                city.visitation_status == VisitationStatus::Goal) ++required;
        job->exact = required <= 10;
        job->benchmarkSelected = _currentAlgorithmIdx == 3;
        job->solver = std::move(_solver);
        // The explicit benchmark selection follows the same size policy.
        if (_currentAlgorithmIdx == 3 && !job->exact)
            job->solver = std::make_unique<NearestNeighborAlgorithm>(true, 100);
        _job = job;
        std::thread([job]() {
            // No GUI access and no reference to MainView: cancelling/closing
            // discards this job safely without blocking on a thread join.
            try {
                job->solver->reset(&job->repo);
                while (!job->cancel && !job->solver->isComplete()) job->solver->step();
                if (job->cancel) return;
                if (job->benchmarkSelected) {
                    job->benchmark = static_cast<TSPAlgorithm*>(job->solver.get())->getBestLength();
                    job->done = true;
                    return;
                }
                std::unique_ptr<TSPAlgorithm> benchmark;
                if (job->exact) benchmark = std::make_unique<ExactTSPAlgorithm>();
                else benchmark = std::make_unique<NearestNeighborAlgorithm>(true, 100);
                benchmark->reset(&job->repo);
                while (!job->cancel && !benchmark->isComplete()) benchmark->step();
                if (job->cancel) return;
                job->benchmark = benchmark->getBestLength();
            } catch (...) { job->failed = true; }
            job->done = true;
        }).detach();
        _running = true;
        _solveTickCounter = 0;
        _timer.start();
        refreshStepButtons();
    }


    void stopSolver()
    {
        _running = false;
        _timer.stop();
        _solveTickCounter = 0;
        refreshStepButtons();
    }

    void stepForward()
    {
        if (!canExecuteAlgorithms()) {
            stopAllExecution();
            _mapView.refresh();
            refreshStepButtons();
            return;
        }

        // Stop auto-run if active
        if (_running) {
            stopSolver();
        }

        if (_solutionRunning) {
            _solutionRunning = false;
            _solutionTimer.stop();
            _mapView.stopSolutionAnimation();
        }

        if (_stepAnimRunning) {
            _stepAnimRunning = false;
            _stepAnimTickCounter = 0;
            _solutionTimer.stop();
            _mapView.stopStepTourAnimation();
        }

        // If solver hasn't started yet (no history), re-create it with
        // the latest side-panel parameters so manual tweaks are applied.
        if (_solver && !_solver->canStepBack() && !_solver->isComplete()) {
            selectAlgorithm(_currentAlgorithmIdx);
        }

        if (!_solver) return;

        // If algorithm is already complete, don't step further
        if (_solver->isComplete()) {
            ensureMapSolverAttached();
            _mapView.refresh();
            refreshStepButtons();
            return;
        }

        ensureMapSolverAttached();

        _sidePanel.clearResult();
        _solver->step();

        // Match Start/Pause one-step behavior for SA/GA
        if (beginStepTourAnimationForCurrentAlgo()) {
            _stepAnimTickCounter = 0;
            _solutionTimer.start();
        }

        _mapView.refresh();
        refreshStepButtons();
    }

    void stepBackward()
    {
        if (!_solver) return;
        if (!_solver->canStepBack()) {
            refreshStepButtons();
            return;
        }

        // Stop auto-run if active
        if (_running) {
            stopSolver();
        }

        // Stop any ongoing animation/solution playback
        if (_solutionRunning) {
            _solutionRunning = false;
            _solutionTimer.stop();
            _solutionTickCounter = 0;
        }
        _mapView.stopSolutionAnimation();

        if (_stepAnimRunning) {
            _stepAnimRunning = false;
            _stepAnimTickCounter = 0;
            _solutionTimer.stop();
        }
        _mapView.stopStepTourAnimation();

        ensureMapSolverAttached();

        _sidePanel.clearResult();
        _solver->stepBack();

        // Match Start/Pause one-step behavior for SA/GA on step-back snapshots too
        if (beginStepTourAnimationForCurrentAlgo()) {
            _stepAnimTickCounter = 0;
            _solutionTimer.start();
        }

        _mapView.refresh();
        refreshStepButtons();
    }
};
