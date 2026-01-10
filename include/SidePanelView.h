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

#include <vector>
#include <string>
#include <functional>

class DataRepository;
enum class VisitationStatus : int;

// Solver action codes: 0=start/pause, 1=step forward, -1=step back, 2=algorithm changed
using SolverCallback = std::function<void(int action, int algorithmIdx)>;

// Side panel for city/road CRUD operations and algorithm control
class SidePanelView : public gui::View
{
public:
    // --- UI Controls ---
    gui::Label lblXCoord;
    gui::NumericEdit txtEditXCoord;
    gui::Label lblYCoord;
    gui::NumericEdit txtEditYCoord;
    gui::Button btnAddPt;
    gui::Label lblName;
    gui::LineEdit lnEditName;
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
    gui::Button btnUpdatePoint;
    gui::Button btnDeletePoint;
    gui::Label lblConnections;
    gui::Label lblConnectionsValue;
    gui::Label lblConnectTo;
    gui::ComboBox cmbConnectTo;
    gui::Button btnToggleConnection;
    gui::Label lblStatus;
    gui::ComboBox cmbStatus;
    
    gui::Label lblSolvingSection;
    gui::ComboBox cmbSolvingAlgorithm;
    
    gui::Button btnStartPause;
    gui::Button btnStepFwd;
    gui::Button btnStepBwd;
    
    gui::GridLayout gl;

    SidePanelView();
    ~SidePanelView() = default;

    // Populate dropdown with provided point names
    void populatePointNames(const std::vector<std::string>& names);
    void populateSolvingAlgorithms(const std::vector<std::string>& names);

    // Connect to data repository for CRUD operations
    void setRepository(DataRepository* repo);
    
    // Set callback for solver control actions
    void setSolverCallback(SolverCallback callback);
    
    // Get currently selected algorithm index
    int getSelectedAlgorithmIndex() const;
    
    // Sync current selection fields with selected dropdown item
    void syncSelectionDetails();

protected:
    bool onChangedSelection(gui::ComboBox* pCB) override;
    bool onClick(gui::Button* pBtn) override;

private:
    std::vector<std::string> _pointNames;
    std::vector<std::string> _algorithmNames = {"BFS", "DFS"};
    DataRepository* _repo = nullptr;
    SolverCallback _solverCallback;

    void handleAddPoint();
    void handleUpdatePoint();
    void handleDeletePoint();
    void handleToggleConnection();
    void selectIndexAndUpdate(int idx);
    void updateCurrentNameFromSelection();
    void updateCurrentCoordsFromSelection();
    void updateConnectionsLabel();
    void populateConnectToCombo();
    void populateStatusCombo();
    void updateStatusFromSelection();
    void handleStatusChange();
};
