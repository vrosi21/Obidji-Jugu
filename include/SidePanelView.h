#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TextEdit.h>
#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/LineEdit.h>
#include <gui/NumericEdit.h>
#include <gui/ComboBox.h>
#include <gui/HorizontalLayout.h>
#include <gui/CheckBox.h>
#include <gui/VerticalLayout.h>
#include <gui/Slider.h>
#include <gui/StandardTabView.h>
#include <gui/ViewScroller.h>


#include <vector>
#include <string>
#include <functional>

class DataRepository;
enum class VisitationStatus : int;

// Solver action codes: 0=start/pause, 1=step forward, -1=step back, 2=algorithm changed, 3=show solution, 4=reset solver view/state
using SolverCallback = std::function<void(int action, int algorithmIdx)>;

// Side panel for city/road CRUD operations and algorithm control
class SidePanelView : public gui::View
{
    // StandardTabView's GTK backend cannot recover a form directly through
    // a scroller's GtkViewport. A regular View host keeps native getView and
    // measurement valid while the inner form still scrolls independently.
    class ScrollPage : public gui::View
    {
        gui::GridLayout _layout;
        gui::ViewScroller _scroller;
    public:
        ScrollPage()
            : gui::View(0, 0, 0, 0), _layout(1, 1),
              _scroller(gui::ViewScroller::Type::NoScroll,
                        gui::ViewScroller::Type::ScrollAndAutoHide)
        {
            _layout.insert(0, 0, _scroller);
            setLayout(&_layout);
        }
        void setContentView(gui::View* view) { _scroller.setContentView(view); }
    };

    // Controls keep their existing owner/callbacks even though their native
    // parent is now a tab page. In particular, algorithm selection must still
    // update parameter visibility and notify MainView.
    class Page : public gui::View
    {
        SidePanelView& _owner;
    public:
        explicit Page(SidePanelView& owner) : _owner(owner) {}
    protected:
        bool onClick(gui::Button* button) override { return _owner.onClick(button); }
        bool onChangedSelection(gui::ComboBox* combo) override
        { return _owner.onChangedSelection(combo); }
    };

public:
    gui::StandardTabView tabs;
    // --- Section Headers ---
    gui::Label lblAddSection;
    gui::Label lblEditSection;
    gui::Label lblConnectionsSection;
    gui::Label lblSolvingSection;
    
    // --- Add Point Section ---
    gui::Label lblName;
    gui::LineEdit lnEditName;
    gui::Label lblXCoord;
    gui::NumericEdit txtEditXCoord;
    gui::Label lblYCoord;
    gui::NumericEdit txtEditYCoord;
    gui::Button btnAddPt;
    
    // --- Edit Point Section ---
    gui::Label lblChoosePoint;
    gui::ComboBox cmbPoints;
    gui::Label lblCurrentCityName;
    gui::LineEdit lnEditCurrentCityName;
    gui::Label lblCurrentX;
    gui::NumericEdit neCurrentX;
    gui::Label lblCurrentY;
    gui::NumericEdit neCurrentY;
    gui::Label lblCurrentWeight;
    gui::NumericEdit neCurrentWeight;
    gui::Label lblStatus;
    gui::ComboBox cmbStatus;
    gui::Button btnUpdatePoint;
    gui::Button btnDeletePoint;
    
    // --- Connections Section ---
    gui::Label lblConnections;
    gui::Label lblConnectionsValue;
    gui::Label lblConnectTo;
    gui::ComboBox cmbConnectTo;
    gui::Button btnToggleConnection;


    gui::Button btnSetAllToVisit;


    // --- Randomize Section ---
    gui::Label lblRandomize;
    gui::HorizontalLayout hlRandomizeBtns;
    gui::Button btnRandomizePosition;
    gui::Button btnRandomizeConnections;
    gui::Button btnRandomizeWeight;
    gui::Button btnRandomizeStatus;
    gui::Button btnRandomizeAll;

    
    // --- Solve Section ---
    gui::Label lblAlgorithm;
    gui::ComboBox cmbSolvingAlgorithm;
    
    gui::Button btnStartPause;
    gui::Button btnStepFwd;
    gui::Button btnStepBwd;
    gui::Button btnResetSolution;
    
    gui::GridLayout gl;

private:
    gui::GridLayout _setupLayout;
    gui::GridLayout _solveLayout;
    gui::HorizontalLayout _solveActions;
    gui::HorizontalLayout _solutionActions;
    Page _setupPage;
    Page _solvePage;
    ScrollPage _setupScroller;
    ScrollPage _solveScroller;

public:

    

    // --- Algorithm Specific Buttons ---
    // -- Nearest Neighbor --
    gui::CheckBox checkBoxEnable2opt;

    gui::Label lblImprovementCycles;
    gui::NumericEdit txtEditImprovementCycles;


    // -- Simulated Annealing --
    gui::Label lblInitialTemperature;
    gui::NumericEdit txtEditInitialTemperature;

    gui::Label lblCoolingRateAlpha;
    gui::Slider sliderCoolingRateAlpha;

    gui::Label lblIterationsPerTemperatureLevel;
    gui::NumericEdit txtEditIterationsPerTemperatureLevel;

    gui::ComboBox cmbSAInitialSolution;


    // -- Genetic Algorithm --
    gui::Label lblPopulationSize;
    gui::NumericEdit txtEditPopulationSize;

    gui::Label lblNumberOfGenerations;
    gui::NumericEdit txtEditNumberOfGenerations;

    gui::Label lblMutationRate;
    gui::NumericEdit txtEditMutationRate;

    gui::Label lblCrossoverRate;
    gui::NumericEdit txtEditCrossoverRate;

    gui::ComboBox cmbSelectionMethod;

    gui::ComboBox cmbMutationCrossoverOperator;

    gui::Label lblElitismPercentage;
    gui::NumericEdit txtEditElitismPercentage;

    gui::Button btnRandomizeParameters;




    gui::Button showSolution;
    gui::Label lblAnimationSpeed;
    gui::Slider sliderAnimationSpeed;







    SidePanelView();
    ~SidePanelView() = default;

    // Populate dropdown with provided point names
    void populatePointNames(const std::vector<std::string>& names);
    void populateSolvingAlgorithms(const std::vector<std::string>& names);

    // Connect to data repository for CRUD operations
    void setRepository(DataRepository* repo);
    
    // Set callback for solver control actions
    void setSolverCallback(SolverCallback callback);
    void handleSetAllToVisit();

    // Get currently selected algorithm index
    int getSelectedAlgorithmIndex() const;

    // NN settings
    bool isNN2OptEnabled() const;
    int getNNImprovementCycles() const;

    // SA settings
    double getSAInitialTemperature() const;
    double getSACoolingRateAlpha() const;
    int getSAIterationsPerTemperature() const;
    bool useNearestNeighborAsSAInitialSolution() const;

    // GA settings
    int getGAPopulationSize() const;
    int getGANumberOfGenerations() const;
    double getGAMutationRate() const;
    double getGACrossoverRate() const;
    int getGASelectionMethodIndex() const;
    int getGAMutationOperatorIndex() const;
    double getGAElitismPercentage() const;

    // Shared execution speed for auto-step and solution animation
    int getExecutionSpeedLevel() const;

    // Get / set currently selected city index
    int getSelectedPointIndex() const;
    void selectPointIndex(int idx);

    // Update step button enabled state
    // running: solver is auto-stepping
    // canFwd: solver can step forward (not complete)
    // canBwd: solver can step backward (has history)
    void updateStepButtons(bool running, bool canFwd, bool canBwd);

    // Lock/unlock controls based on execution state
    void updateExecutionState(bool running);
    
    // Sync current selection fields with selected dropdown item
    void syncSelectionDetails();

protected:
    bool onChangedSelection(gui::ComboBox* pCB) override;
    bool onClick(gui::Button* pBtn) override;

private:
    std::vector<std::string> _pointNames;
    std::vector<std::string> _algorithmNames = {"Nearest Neighbor", "Simulated Annealing", "Genetic Algorithm"}; // "BFS" and "DFS" exist but are not rendered in the combobox
    DataRepository* _repo = nullptr;
    SolverCallback _solverCallback;

    void handleAddPoint();
    void handleUpdatePoint();
    void handleDeletePoint();
    void handleToggleConnection();
    void handleRandomizePositions();
    void handleRandomizeConnections();
    void handleRandomizeWeights();
    void handleRandomizeStatus();
    void handleRandomizeAll();
    void handleRandomizeGAParameters();
    void selectIndexAndUpdate(int idx);
    void updateCurrentNameFromSelection();
    void updateCurrentCoordsFromSelection();
    void updateConnectionsLabel();
    void populateConnectToCombo();
    void populateStatusCombo();
    void updateAlgorithmFieldsVisibility();
    void updateStatusFromSelection();
    void handleStatusChange();
};
