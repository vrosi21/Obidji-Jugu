#include "SidePanelView.h"
#include "DataRepository.h"
#include <gui/GridLayout.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <gui/GridComposer.h>

SidePanelView::SidePanelView()
    : // Section Headers
    lblAddSection(tr("Add New Point")),
    lblEditSection(tr("Edit Selected Point")),
    lblConnectionsSection(tr("Connections")),
    lblSolvingSection(tr("Solve")),
    // Add Point Section
    lblName(tr("City Name:")),
    lnEditName(),
    lblXCoord("X:"),
    txtEditXCoord(td::DataType::decimal1),
    lblYCoord("Y:"),
    txtEditYCoord(td::DataType::decimal1),
    btnAddPt(tr("Add Point")),
    // Edit Point Section
    lblChoosePoint(tr("Select:")),
    lblCurrentCityName(tr("Name:")),
    lblCurrentX("X:"),
    neCurrentX(td::DataType::decimal1),
    lblCurrentY("Y:"),
    neCurrentY(td::DataType::decimal1),
    lblCurrentWeight(tr("Weight:")),
    neCurrentWeight(td::DataType::decimal1),
    lblStatus(tr("Status:")),
    btnUpdatePoint(tr("Update")),
    btnDeletePoint(tr("Delete")),
    // Connections Section
    lblConnections(tr("Connected to:")),
    lblConnectionsValue(""),
    lblConnectTo(tr("Connect to:")),
    btnToggleConnection(tr("Toggle")),
    // Randomize Section
    lblRandomize(tr("Randomize")),
    hlRandomizeBtns(4),
    btnRandomizePosition(tr("Positions")),
    btnRandomizeWeight(tr("Weights")),
    btnRandomizeStatus(tr("Status")),
    btnRandomizeAll(tr("All")),
    // Solve Section
    lblAlgorithm(tr("Algorithm:")),
    btnStartPause(tr("Start/Pause")),
    btnStepFwd(tr("Step >")),
    btnStepBwd(tr("< Step")),
    btnResetSolution(tr("Reset")),
    // --- NN inputs
    lblImprovementCycles(tr("Improvement cycles:")),
    checkBoxEnable2opt(tr("2-opt optimization")),
    txtEditImprovementCycles(td::DataType::decimal1),
    // --- SA inputs ---
    lblInitialTemperature(tr("Initial temperature:")),
    lblCoolingRateAlpha(tr("Cooling rate (alpha):")),
    lblIterationsPerTemperatureLevel(tr("Iterations per temperature:")),
    txtEditInitialTemperature(td::DataType::decimal1),
    sliderCoolingRateAlpha(tr("Cooling rate - alpha")),
    txtEditIterationsPerTemperatureLevel(td::DataType::decimal1),
    cmbSAInitialSolution(),
    // --- GA inputs ---
    lblPopulationSize(tr("Population size:")),
    lblNumberOfGenerations(tr("Generations:")),
    lblMutationRate(tr("Mutation rate:")),
    lblCrossoverRate(tr("Crossover rate:")),
    lblElitismPercentage(tr("Elitism (%):")),
    txtEditPopulationSize(td::DataType::decimal1),
    txtEditNumberOfGenerations(td::DataType::decimal1),
    txtEditMutationRate(td::DataType::decimal1),
    txtEditCrossoverRate(td::DataType::decimal1),
    cmbSelectionMethod(),
    cmbMutationCrossoverOperator(),
    txtEditElitismPercentage(td::DataType::decimal1),
    btnRandomizeParameters(tr("Randomize parameters")),

    // Layout (24 rows, 4 columns)
    showSolution(tr("Show Solution")),
    lblAnimationSpeed(tr("Animation speed")),
    sliderAnimationSpeed(),
    gl(36, 4)
{
    gui::GridComposer gc(gl);

    // ========================================
    // SECTION 1: Add New Point
    // ========================================
    lblAddSection.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblAddSection, 4);
    
    gc.appendRow(lblName);
    gc.appendCol(lnEditName, 3);
    
    gc.appendRow(lblXCoord);
    gc.appendCol(txtEditXCoord);
    gc.appendCol(lblYCoord);
    gc.appendCol(txtEditYCoord);
    
    btnAddPt.setType(gui::Button::Type::Default);
    gc.appendRow(btnAddPt, 2);

    // ========================================
    // SECTION 2: Edit Selected Point
    // ========================================
    lblEditSection.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblEditSection, 4);
    
    gc.appendRow(lblChoosePoint);
    gc.appendCol(cmbPoints, 3);
    
    gc.appendRow(lblCurrentCityName);
    gc.appendCol(lnEditCurrentCityName, 3);
    
    gc.appendRow(lblCurrentX);
    gc.appendCol(neCurrentX);
    gc.appendCol(lblCurrentY);
    gc.appendCol(neCurrentY);
    
    gc.appendRow(lblCurrentWeight);
    gc.appendCol(neCurrentWeight, 3);
    
    gc.appendRow(lblStatus);
    gc.appendCol(cmbStatus, 3);
    
    btnUpdatePoint.setType(gui::Button::Type::Default);
    //btnUpdatePoint.hide(true, false);
    btnDeletePoint.setType(gui::Button::Type::Default);
    gc.appendRow(btnUpdatePoint, 2);
    gc.appendCol(btnDeletePoint, 2);

    // ========================================
    // SECTION 3: Connections
    // ========================================
    lblConnectionsSection.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblConnectionsSection, 4);
    
    gc.appendRow(lblConnections);
    gc.appendCol(lblConnectionsValue, 3);
    
    gc.appendRow(lblConnectTo);
    gc.appendCol(cmbConnectTo, 3);
    
    btnToggleConnection.setType(gui::Button::Type::Default);
    gc.appendRow(btnToggleConnection, 2);

    // ========================================
    // SECTION 4: Randomize
    // ========================================
    lblRandomize.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblRandomize, 1);
    gc.appendRow(btnRandomizePosition, 1, td::HAlignment::Left);
    gc.appendCol(btnRandomizeWeight, 1, td::HAlignment::Center);
    gc.appendCol(btnRandomizeStatus, 1, td::HAlignment::Center);
    gc.appendCol(btnRandomizeAll, 1, td::HAlignment::Center);


    // ========================================
    // SECTION 5: Solve
    // ========================================
    lblSolvingSection.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblSolvingSection, 4);
    
    gc.appendRow(lblAlgorithm);
    gc.appendCol(cmbSolvingAlgorithm, 3);
    

    //NN inputs (Shows only when NN selected)
    gc.appendRow(checkBoxEnable2opt, 4);
    gc.appendRow(lblImprovementCycles);
    gc.appendCol(txtEditImprovementCycles, 3);

    //SA inputs (Shows only when SA selected)
    gc.appendRow(lblInitialTemperature);
    gc.appendCol(txtEditInitialTemperature, 3);
    sliderCoolingRateAlpha.setRange(0.90, 0.99);
    gc.appendRow(lblCoolingRateAlpha);
    gc.appendCol(sliderCoolingRateAlpha, 3);
    gc.appendRow(lblIterationsPerTemperatureLevel);
    gc.appendCol(txtEditIterationsPerTemperatureLevel, 3);
    cmbSAInitialSolution.addItem("Random");
    cmbSAInitialSolution.addItem("Nearest Neighbor");
    gc.appendRow(cmbSAInitialSolution, 4);
    
    //GA inputs (Shows only when GA selected)
    gc.appendRow(lblPopulationSize);
    gc.appendCol(txtEditPopulationSize, 3);
    gc.appendRow(lblNumberOfGenerations);
    gc.appendCol(txtEditNumberOfGenerations, 3);
    gc.appendRow(lblMutationRate);
    gc.appendCol(txtEditMutationRate, 3);
    gc.appendRow(lblCrossoverRate);
    gc.appendCol(txtEditCrossoverRate, 3);
    gc.appendRow(cmbSelectionMethod, 4);
    gc.appendRow(cmbMutationCrossoverOperator, 4);
    gc.appendRow(lblElitismPercentage);
    gc.appendCol(txtEditElitismPercentage, 3);
    gc.appendRow(btnRandomizeParameters, 4);
    // Defaults / options
    cmbSelectionMethod.addItem("Roulette");
    cmbSelectionMethod.addItem("Tournament");
    cmbSelectionMethod.addItem("Rank");
    cmbMutationCrossoverOperator.addItem("Swap");
    cmbMutationCrossoverOperator.addItem("Inversion");
    cmbMutationCrossoverOperator.addItem("OX");


    btnStartPause.setType(gui::Button::Type::Default);
    btnStepBwd.setType(gui::Button::Type::Default);
    btnStepFwd.setType(gui::Button::Type::Default);
    btnResetSolution.setType(gui::Button::Type::Default);
    gc.appendRow(btnStartPause, 2);
    gc.appendCol(btnStepBwd);
    gc.appendCol(btnStepFwd);

    gc.appendRow(showSolution, 2);
    gc.appendCol(btnResetSolution, 2);
    gc.appendRow(lblAnimationSpeed);
    gc.appendCol(sliderAnimationSpeed,3);

    setLayout(&gl);

    // Initial button state: not started, only forward enabled
    btnStepBwd.enable(false);
    btnStepFwd.enable(true);
    btnResetSolution.enable(true);
    
    // Populate dropdowns
    populateStatusCombo();
    populateSolvingAlgorithms(_algorithmNames);
}

void SidePanelView::populateStatusCombo()
{
    cmbStatus.clean();
    cmbStatus.addItem(tr("Blocked"));
    cmbStatus.addItem(tr("Open"));
    cmbStatus.addItem(tr("Goal"));
    cmbStatus.addItem(tr("Start"));
    cmbStatus.selectIndex(1); // Default to Open
}

void SidePanelView::populateSolvingAlgorithms(const std::vector<std::string>& names)
{
    _algorithmNames = names;
    cmbSolvingAlgorithm.clean();
    for (const auto& n : names)
    {
        cmbSolvingAlgorithm.addItem(n.c_str());
    }
}

void SidePanelView::populatePointNames(const std::vector<std::string>& names)
{
    _pointNames = names;
    cmbPoints.clean();
    for (const auto& n : names)
    {
        cmbPoints.addItem(n.c_str());
    }
    if (!names.empty())
    {
        selectIndexAndUpdate(0);
    }
    else
    {
        lnEditCurrentCityName.setText("");
        neCurrentX.setText("0.0");
        neCurrentY.setText("0.0");
        neCurrentWeight.setText("0.0");
    }
}

bool SidePanelView::onChangedSelection(gui::ComboBox* pCB)
{
    if (pCB == &cmbPoints)
    {
        int idx = pCB->getSelectedIndex();
        selectIndexAndUpdate(idx);
        return true;
    }
    if (pCB == &cmbStatus)
    {
        handleStatusChange();
        return true;
    }
    if (pCB == &cmbSolvingAlgorithm)
    {
        int algoIdx = pCB->getSelectedIndex();
        bool nearestNeighborSelected = (algoIdx == 0);
        bool simulatedAnnealingSelected = (algoIdx == 1);
        bool geneticAlgorithmSelected = (algoIdx == 2);

        // -------------------------
        // NN (label + inputs)
        // -------------------------
        checkBoxEnable2opt.hide(!nearestNeighborSelected, true);
        lblImprovementCycles.hide(!nearestNeighborSelected, true);
        txtEditImprovementCycles.hide(!nearestNeighborSelected, true);

        // -------------------------
        // SA (labels + inputs)
        // -------------------------
        lblInitialTemperature.hide(!simulatedAnnealingSelected, true);
        txtEditInitialTemperature.hide(!simulatedAnnealingSelected, true);

        lblCoolingRateAlpha.hide(!simulatedAnnealingSelected, true);
        sliderCoolingRateAlpha.hide(!simulatedAnnealingSelected, true);

        lblIterationsPerTemperatureLevel.hide(!simulatedAnnealingSelected, true);
        txtEditIterationsPerTemperatureLevel.hide(!simulatedAnnealingSelected, true);

        cmbSAInitialSolution.hide(!simulatedAnnealingSelected, true);

        // -------------------------
        // GA (labels + inputs)
        // -------------------------
        lblPopulationSize.hide(!geneticAlgorithmSelected, true);
        txtEditPopulationSize.hide(!geneticAlgorithmSelected, true);

        lblNumberOfGenerations.hide(!geneticAlgorithmSelected, true);
        txtEditNumberOfGenerations.hide(!geneticAlgorithmSelected, true);

        lblMutationRate.hide(!geneticAlgorithmSelected, true);
        txtEditMutationRate.hide(!geneticAlgorithmSelected, true);

        lblCrossoverRate.hide(!geneticAlgorithmSelected, true);
        txtEditCrossoverRate.hide(!geneticAlgorithmSelected, true);

        cmbSelectionMethod.hide(!geneticAlgorithmSelected, true);
        cmbMutationCrossoverOperator.hide(!geneticAlgorithmSelected, true);

        lblElitismPercentage.hide(!geneticAlgorithmSelected, true);
        txtEditElitismPercentage.hide(!geneticAlgorithmSelected, true);

        btnRandomizeParameters.hide(!geneticAlgorithmSelected, true);
        
        reDraw();
        // Notify that algorithm selection changed (action code 2)
        if (_solverCallback) {
            _solverCallback(2, pCB->getSelectedIndex());
        }
        return true;
    }
    return false;
}

bool SidePanelView::onClick(gui::Button* pBtn)
{
    if (pBtn == &btnAddPt)
    {
        handleAddPoint();
        return true;
    }
    if (pBtn == &btnUpdatePoint)
    {
        handleUpdatePoint();
        return true;
    }
    if (pBtn == &btnDeletePoint)
    {
        handleDeletePoint();
        return true;
    }
    if (pBtn == &btnToggleConnection)
    {
        handleToggleConnection();
        return true;
    }
    if (pBtn == &btnRandomizePosition)
    {
        // TO DO: IMPLEMENT LOGIC FOR RANDOMIZING POSITIONS
        return true;
    }
    if (pBtn == &btnRandomizeWeight)
    {
        // TO DO: IMPLEMENT LOGIC FOR RANDOMIZING WEIGHTS
        return true;
    }
    if (pBtn == &btnRandomizeStatus)
    {
        // TO DO: IMPLEMENT LOGIC FOR RANDOMIZING STATUS
        return true;
    }
    if (pBtn == &btnRandomizeAll)
    {
        // TO DO: IMPLEMENT LOGIC FOR RANDOMIZING ALL ATTRIBUTES
        return true;
    }
    if (pBtn == &btnStartPause) {
        if (_solverCallback) {
            _solverCallback(0, cmbSolvingAlgorithm.getSelectedIndex());
        }
        return true;
    }
    if (pBtn == &btnStepFwd) {
        if (_solverCallback) {
            _solverCallback(1, cmbSolvingAlgorithm.getSelectedIndex());
        }
        return true;
    }
    if (pBtn == &btnStepBwd) {
        if (_solverCallback) {
            _solverCallback(-1, cmbSolvingAlgorithm.getSelectedIndex());
        }
        return true;
    }
    if (pBtn == &showSolution) {
        if (_solverCallback) {
            _solverCallback(3, cmbSolvingAlgorithm.getSelectedIndex()); // 3 = show solution
        }
        return true;
    }
    if (pBtn == &btnResetSolution) {
        if (_solverCallback) {
            _solverCallback(4, cmbSolvingAlgorithm.getSelectedIndex()); // 4 = reset solver state/visualization
        }
        return true;
    }


    return false;
}

void SidePanelView::setRepository(DataRepository* repo)
{
    _repo = repo;
    syncSelectionDetails();
}

void SidePanelView::syncSelectionDetails()
{
    int idx = cmbPoints.getSelectedIndex();
    if (idx < 0 && !_pointNames.empty())
    {
        idx = 0;
    }
    if (idx >= 0)
    {
        selectIndexAndUpdate(idx);
    }
}

void SidePanelView::updateCurrentNameFromSelection()
{
    int idx = cmbPoints.getSelectedIndex();
    if (idx >= 0 && idx < (int)_pointNames.size())
    {
        lnEditCurrentCityName.setText(_pointNames[idx].c_str());
        reDraw();
    }
}

void SidePanelView::updateCurrentCoordsFromSelection()
{
    int idx = cmbPoints.getSelectedIndex();
    std::string xStr = "0";
    std::string yStr = "0";
    std::string wStr = "0";
    if (_repo && idx >= 0)
    {
        CityPoint cp;
        if (_repo->getCity(idx, cp))
        {
            std::ostringstream sx;
            std::ostringstream sy;
            std::ostringstream sw;
            sx << std::fixed << std::setprecision(2) << cp.x;
            sy << std::fixed << std::setprecision(2) << cp.y;
            sw << std::fixed << std::setprecision(2) << cp.weight;
            xStr = sx.str();
            yStr = sy.str();
            wStr = sw.str();
        }
    }
    neCurrentX.setText(xStr.c_str());
    neCurrentY.setText(yStr.c_str());
    neCurrentWeight.setText(wStr.c_str());
    reDraw();
}

void SidePanelView::updateConnectionsLabel()
{
    std::string text;
    int idx = cmbPoints.getSelectedIndex();
    if (_repo && idx >= 0)
    {
        auto names = _repo->getConnectionNames(idx);
        for (size_t i = 0; i < names.size(); ++i)
        {
            text += names[i];
            if (i + 1 < names.size()) text += ", ";
        }
    }
    if (text.empty()) text = tr("None").c_str();
    lblConnectionsValue.setTitle(text.c_str());
}

void SidePanelView::populateConnectToCombo()
{
    cmbConnectTo.clean();
    int idx = cmbPoints.getSelectedIndex();
    for (size_t i = 0; i < _pointNames.size(); ++i)
    {
        if (static_cast<int>(i) == idx) continue;
        cmbConnectTo.addItem(_pointNames[i].c_str());
    }
    if (cmbConnectTo.getNoOfItems() > 0)
    {
        cmbConnectTo.selectIndex(0);
    }
}

void SidePanelView::updateStatusFromSelection()
{
    int idx = cmbPoints.getSelectedIndex();
    if (_repo && idx >= 0)
    {
        CityPoint cp;
        if (_repo->getCity(idx, cp))
        {
            cmbStatus.selectIndex(static_cast<int>(cp.visitation_status));
        }
    }
}

void SidePanelView::handleStatusChange()
{
    if (!_repo) return;
    int cityIdx = cmbPoints.getSelectedIndex();
    int statusIdx = cmbStatus.getSelectedIndex();
    if (cityIdx < 0 || statusIdx < 0) return;
    
    VisitationStatus status = static_cast<VisitationStatus>(statusIdx);
    
    // If setting to Start, clear any existing Start status
    if (status == VisitationStatus::Start) {
        for (size_t i = 0; i < _repo->cityCount(); ++i) {
            if (static_cast<int>(i) == cityIdx) continue;
            CityPoint cp;
            if (_repo->getCity(static_cast<int>(i), cp) && cp.visitation_status == VisitationStatus::Start) {
                _repo->updateCityStatus(static_cast<int>(i), VisitationStatus::Open);
            }
        }
    }
    
    _repo->updateCityStatus(cityIdx, status);
}

void SidePanelView::handleAddPoint()
{
    if (!_repo) return;
    std::string name = lnEditName.getText().c_str();
    double x = std::atof(txtEditXCoord.getText().c_str());
    double y = std::atof(txtEditYCoord.getText().c_str());

    if (_repo->addCity(name, x, y))
    {
        _pointNames = _repo->getCityNames();
        populatePointNames(_pointNames);
        cmbPoints.selectIndex(static_cast<int>(_pointNames.size()) - 1);
        updateCurrentNameFromSelection();
        updateCurrentCoordsFromSelection();
        updateConnectionsLabel();
        populateConnectToCombo();
        lnEditName.setText("");
        txtEditXCoord.setText("0");
        txtEditYCoord.setText("0");
    }
}

void SidePanelView::handleUpdatePoint()
{
    if (!_repo) return;
    int idx = cmbPoints.getSelectedIndex();
    if (idx < 0) return;

    std::string name = lnEditCurrentCityName.getText().c_str();
    double x = std::atof(neCurrentX.getText().c_str());
    double y = std::atof(neCurrentY.getText().c_str());
    double weight = std::atof(neCurrentWeight.getText().c_str());

    if (_repo->updateCity(idx, name, x, y, weight))
    {
        _pointNames = _repo->getCityNames();
        populatePointNames(_pointNames);
        if (idx >= (int)_pointNames.size())
        {
            idx = static_cast<int>(_pointNames.size()) - 1;
        }
        selectIndexAndUpdate(idx);
        updateConnectionsLabel();
        populateConnectToCombo();
    }
}

void SidePanelView::handleDeletePoint()
{
    if (!_repo) return;
    int idx = cmbPoints.getSelectedIndex();
    if (idx < 0) return;
    if (_repo->deleteCity(idx))
    {
        _pointNames = _repo->getCityNames();
        populatePointNames(_pointNames);
        int newIdx = idx;
        if (newIdx >= (int)_pointNames.size()) newIdx = static_cast<int>(_pointNames.size()) - 1;
        selectIndexAndUpdate(newIdx);
        updateConnectionsLabel();
        populateConnectToCombo();
    }
}

void SidePanelView::handleToggleConnection()
{
    if (!_repo) return;
    int fromIdx = cmbPoints.getSelectedIndex();
    int toIdxLocal = cmbConnectTo.getSelectedIndex();
    if (fromIdx < 0 || toIdxLocal < 0) return;

    // map combo index back to actual point index skipping selected
    int actualTo = -1;
    int counter = 0;
    for (size_t i = 0; i < _pointNames.size(); ++i)
    {
        if (static_cast<int>(i) == fromIdx) continue;
        if (counter == toIdxLocal)
        {
            actualTo = static_cast<int>(i);
            break;
        }
        ++counter;
    }
    if (actualTo < 0) return;

    // Check if connection exists and toggle it
    if (_repo->hasConnection(fromIdx, actualTo))
    {
        _repo->removeConnection(fromIdx, actualTo);
    }
    else
    {
        _repo->addConnection(fromIdx, actualTo);
    }
    updateConnectionsLabel();
}

void SidePanelView::selectIndexAndUpdate(int idx)
{
    if (idx < 0 || idx >= (int)_pointNames.size()) return;
    cmbPoints.selectIndex(idx);
    updateCurrentNameFromSelection();
    updateCurrentCoordsFromSelection();
    updateConnectionsLabel();
    populateConnectToCombo();
    updateStatusFromSelection();
}

void SidePanelView::setSolverCallback(SolverCallback callback)
{
    _solverCallback = callback;
}

int SidePanelView::getSelectedAlgorithmIndex() const
{
    return cmbSolvingAlgorithm.getSelectedIndex();
}

void SidePanelView::updateStepButtons(bool running, bool canFwd, bool canBwd)
{
    if (running) {
        // While auto-running, disable both step buttons
        btnStepFwd.enable(false);
        btnStepBwd.enable(false);
    } else {
        // Paused or not started: enable based on state
        btnStepFwd.enable(canFwd);
        btnStepBwd.enable(canBwd);
    }
}

void SidePanelView::updateExecutionState(bool running)
{
    // Lock algorithm selection while executing
    cmbSolvingAlgorithm.enable(!running);

    // Keep solution/reset disabled while running to avoid conflicting states
    showSolution.enable(!running);
    btnResetSolution.enable(!running);
}
