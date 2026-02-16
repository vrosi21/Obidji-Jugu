#include "SidePanelView.h"
#include "DataRepository.h"
#include <gui/GridLayout.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cmath>
#include <gui/GridComposer.h>

namespace {
    struct CandidateEdge {
        int a = -1;
        int b = -1;
        double dist = 0.0;
    };

    static void dotToComma(std::string& s)
    {
        std::replace(s.begin(), s.end(), '.', ',');
    }

    static double orient(double ax, double ay, double bx, double by, double cx, double cy)
    {
        return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    }

    static double neToDouble(const gui::NumericEdit& ne)
    {
        td::Variant v = ne.getValue();          
        return v.toNumber<double>();            
    }

    static int neToIntRound(const gui::NumericEdit& ne)
    {
        return static_cast<int>(std::lround(neToDouble(ne)));
    }

    static bool onSeg(double ax, double ay, double bx, double by, double px, double py)
    {
        const double eps = 1e-9;
        return px <= std::max(ax, bx) + eps && px + eps >= std::min(ax, bx) &&
               py <= std::max(ay, by) + eps && py + eps >= std::min(ay, by);
    }

    static bool segmentsIntersect(
        double ax, double ay, double bx, double by,
        double cx, double cy, double dx, double dy)
    {
        double o1 = orient(ax, ay, bx, by, cx, cy);
        double o2 = orient(ax, ay, bx, by, dx, dy);
        double o3 = orient(cx, cy, dx, dy, ax, ay);
        double o4 = orient(cx, cy, dx, dy, bx, by);

        const double eps = 1e-9;
        if ((o1 > eps && o2 < -eps || o1 < -eps && o2 > eps) &&
            (o3 > eps && o4 < -eps || o3 < -eps && o4 > eps)) {
            return true;
        }

        if (std::abs(o1) <= eps && onSeg(ax, ay, bx, by, cx, cy)) return true;
        if (std::abs(o2) <= eps && onSeg(ax, ay, bx, by, dx, dy)) return true;
        if (std::abs(o3) <= eps && onSeg(cx, cy, dx, dy, ax, ay)) return true;
        if (std::abs(o4) <= eps && onSeg(cx, cy, dx, dy, bx, by)) return true;
        return false;
    }
}

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
    btnSetAllToVisit(tr("Set all to visit")),

    // Randomize Section
    lblRandomize(tr("Randomize")),
    hlRandomizeBtns(4),
    btnRandomizePosition(tr("Positions")),
    btnRandomizeConnections(tr("Connections")),
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
    gl(37, 5)
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

    btnSetAllToVisit.setType(gui::Button::Type::Default);
    gc.appendRow(btnSetAllToVisit, 4);

    // ========================================
    // SECTION 4: Randomize
    // ========================================
    lblRandomize.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblRandomize, 1);
    gc.appendRow(btnRandomizePosition, 1, td::HAlignment::Left);
    gc.appendCol(btnRandomizeConnections, 1, td::HAlignment::Center);
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
    cmbSAInitialSolution.addItem(tr("Random"));
    cmbSAInitialSolution.addItem(tr("Nearest Neighbor"));
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
    cmbSelectionMethod.addItem(tr("Roulette"));
    cmbSelectionMethod.addItem(tr("Tournament"));
    cmbSelectionMethod.addItem(tr("Rank"));
    cmbMutationCrossoverOperator.addItem(tr("Swap"));
    cmbMutationCrossoverOperator.addItem(tr("Inversion"));
    cmbMutationCrossoverOperator.addItem(tr("OX"));


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
    sliderAnimationSpeed.setRange(1.0, 10.0);
    gc.appendCol(sliderAnimationSpeed,3);

    setLayout(&gl);

    // Initial button state: not started, only forward enabled
    btnStepBwd.enable(false);
    btnStepFwd.enable(true);
    btnResetSolution.enable(true);
    
    // Populate dropdowns
    populateStatusCombo();
    populateSolvingAlgorithms(_algorithmNames);

    // NN defaults
    checkBoxEnable2opt.setChecked(false);
    txtEditImprovementCycles.setText("20");

    // SA defaults
    txtEditInitialTemperature.setText("10000");
    txtEditIterationsPerTemperatureLevel.setText("25");
    cmbSAInitialSolution.selectIndex(0); // Random

    // GA defaults
    txtEditPopulationSize.setText("50");
    txtEditNumberOfGenerations.setText("1000");
    txtEditMutationRate.setText("0,02");
    txtEditCrossoverRate.setText("0,80");
    txtEditElitismPercentage.setText("10");
    cmbSelectionMethod.selectIndex(0); // Roulette
    cmbMutationCrossoverOperator.selectIndex(0); // Swap

    // Shared speed default
    sliderAnimationSpeed.setValue(1.0);
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
        cmbSolvingAlgorithm.addItem(tr(n.c_str()));
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
        neCurrentX.setText("0,0");
        neCurrentY.setText("0,0");
        neCurrentWeight.setText("0,0");
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
        btnStepFwd.hide(!nearestNeighborSelected, true);
        btnStepBwd.hide(!nearestNeighborSelected, true);
        //btnStartPause.hide(nearestNeighborSelected, true);


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
        handleRandomizePositions();
        return true;
    }
    if (pBtn == &btnRandomizeWeight)
    {
        handleRandomizeWeights();
        return true;
    }
    if (pBtn == &btnRandomizeConnections)
    {
        handleRandomizeConnections();
        return true;
    }
    if (pBtn == &btnRandomizeStatus)
    {
        handleRandomizeStatus();
        return true;
    }
    if (pBtn == &btnRandomizeAll)
    {
        handleRandomizeAll();
        return true;
    }
    if (pBtn == &btnRandomizeParameters)
    {
        handleRandomizeGAParameters();
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
    if (pBtn == &btnSetAllToVisit)
    {
        handleSetAllToVisit();
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
            


            sx << std::fixed << cp.x;
            sy << std::fixed << cp.y;
            sw << std::fixed << cp.weight;
            
            xStr = sx.str();
            yStr = sy.str();
            wStr = sw.str();

            dotToComma(xStr);
            dotToComma(yStr);
            dotToComma(wStr);
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
    double x = txtEditXCoord.getValue().toNumber<double>();
    double y = txtEditYCoord.getValue().toNumber<double>();

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

    double x = neCurrentX.getValue().toNumber<double>();
    double y = neCurrentY.getValue().toNumber<double>();
    double weight = neCurrentWeight.getValue().toNumber<double>();

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

void SidePanelView::handleRandomizePositions()
{
    if (!_repo) return;
    const size_t n = _repo->cityCount();
    if (n == 0) return;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> xDist(20.0, 980.0);
    std::uniform_real_distribution<double> yDist(20.0, 846.0);

    for (size_t i = 0; i < n; ++i) {
        CityPoint cp;
        if (!_repo->getCity(static_cast<int>(i), cp)) continue;
        const double nx = xDist(rng);
        const double ny = yDist(rng);
        _repo->updateCity(static_cast<int>(i), cp.name, nx, ny, cp.weight);
    }

    syncSelectionDetails();
    reDraw();
}

void SidePanelView::handleRandomizeWeights()
{
    if (!_repo) return;
    const size_t n = _repo->cityCount();
    if (n == 0) return;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> wDist(0.0, 100.0);

    for (size_t i = 0; i < n; ++i) {
        CityPoint cp;
        if (!_repo->getCity(static_cast<int>(i), cp)) continue;
        const double nw = wDist(rng);
        _repo->updateCity(static_cast<int>(i), cp.name, cp.x, cp.y, nw);
    }

    syncSelectionDetails();
    reDraw();
}

void SidePanelView::handleRandomizeConnections()
{
    if (!_repo) return;
    const int n = static_cast<int>(_repo->cityCount());
    if (n < 2) return;

    const int minDegree = std::min(3, n - 1);
    const int maxDegree = std::min(5, n - 1);

    // 1) Clear all existing connections
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (_repo->hasConnection(i, j)) {
                _repo->removeConnection(i, j);
            }
        }
    }

    const auto& cities = _repo->cities();
    std::vector<CandidateEdge> edges;
    edges.reserve((n * (n - 1)) / 2);

    // 2) Build candidate edges sorted by distance (nearest-neighbor preference)
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double dx = cities[i].x - cities[j].x;
            double dy = cities[i].y - cities[j].y;
            edges.push_back({i, j, std::sqrt(dx * dx + dy * dy)});
        }
    }
    std::sort(edges.begin(), edges.end(), [](const CandidateEdge& l, const CandidateEdge& r) {
        return l.dist < r.dist;
    });

    std::vector<std::pair<int, int>> chosen;
    std::vector<int> degree(n, 0);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> targetDist(minDegree, maxDegree);
    std::vector<int> targetDegree(n, minDegree);
    for (int i = 0; i < n; ++i) {
        targetDegree[i] = targetDist(rng);
    }

    // DSU for connectivity tracking
    std::vector<int> parent(n), rank(n, 0);
    for (int i = 0; i < n; ++i) parent[i] = i;
    auto findSet = [&](int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    };
    auto unionSet = [&](int a, int b) {
        int ra = findSet(a), rb = findSet(b);
        if (ra == rb) return;
        if (rank[ra] < rank[rb]) std::swap(ra, rb);
        parent[rb] = ra;
        if (rank[ra] == rank[rb]) rank[ra]++;
    };
    auto isConnectedAll = [&]() {
        int root = findSet(0);
        for (int i = 1; i < n; ++i) {
            if (findSet(i) != root) return false;
        }
        return true;
    };

    auto intersectsExisting = [&](int a, int b) {
        const auto& A = cities[a];
        const auto& B = cities[b];
        for (const auto& e : chosen) {
            int c = e.first;
            int d = e.second;

            // sharing endpoints is allowed
            if (a == c || a == d || b == c || b == d) continue;

            const auto& C = cities[c];
            const auto& D = cities[d];
            if (segmentsIntersect(A.x, A.y, B.x, B.y, C.x, C.y, D.x, D.y)) {
                return true;
            }
        }
        return false;
    };

    auto tryAddEdge = [&](int a, int b, bool allowCrossing) {
        if (a == b) return false;
        if (_repo->hasConnection(a, b)) return false;
        if (degree[a] >= maxDegree || degree[b] >= maxDegree) return false;
        if (!allowCrossing && intersectsExisting(a, b)) return false;

        if (_repo->addConnection(a, b)) {
            chosen.push_back({a, b});
            degree[a]++;
            degree[b]++;
            unionSet(a, b);
            return true;
        }
        return false;
    };

    // 3) Build a connected backbone (prefer non-crossing + short edges)
    for (const auto& e : edges) {
        if (findSet(e.a) != findSet(e.b)) {
            tryAddEdge(e.a, e.b, false);
        }
        if (isConnectedAll()) break;
    }

    // Fallback: allow crossings if needed to guarantee one connected bundle
    if (!isConnectedAll()) {
        for (const auto& e : edges) {
            if (findSet(e.a) != findSet(e.b)) {
                tryAddEdge(e.a, e.b, true);
            }
            if (isConnectedAll()) break;
        }
    }

    // 4) Raise degree toward target [3..5] while still preferring minimal crossings
    auto hasBelowTarget = [&]() {
        for (int i = 0; i < n; ++i) {
            if (degree[i] < targetDegree[i]) return true;
        }
        return false;
    };

    // Pass A: non-crossing additions
    bool progress = true;
    while (hasBelowTarget() && progress) {
        progress = false;
        for (const auto& e : edges) {
            int a = e.a;
            int b = e.b;

            if (degree[a] >= targetDegree[a] && degree[b] >= targetDegree[b]) continue;
            if (tryAddEdge(a, b, false)) {
                progress = true;
            }
        }
    }

    // Pass B: allow crossings only if necessary to satisfy degree constraints
    progress = true;
    while (hasBelowTarget() && progress) {
        progress = false;
        for (const auto& e : edges) {
            int a = e.a;
            int b = e.b;

            if (degree[a] >= targetDegree[a] && degree[b] >= targetDegree[b]) continue;
            if (tryAddEdge(a, b, true)) {
                progress = true;
            }
        }
    }

    updateConnectionsLabel();
    syncSelectionDetails();
    reDraw();
}

void SidePanelView::handleRandomizeStatus()
{
    if (!_repo) return;
    const size_t n = _repo->cityCount();
    if (n == 0) return;

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> statusDist(0, 2); // Blocked, Open, Goal
    std::uniform_int_distribution<int> idxDist(0, static_cast<int>(n) - 1);

    // First assign random statuses excluding Start
    for (size_t i = 0; i < n; ++i) {
        VisitationStatus s = static_cast<VisitationStatus>(statusDist(rng));
        _repo->updateCityStatus(static_cast<int>(i), s);
    }

    // Ensure exactly one Start (side-panel invariant)
    int startIdx = idxDist(rng);
    _repo->updateCityStatus(startIdx, VisitationStatus::Start);

    // Ensure at least one Goal (when possible, different from Start)
    bool hasGoal = false;
    for (size_t i = 0; i < n; ++i) {
        CityPoint cp;
        if (_repo->getCity(static_cast<int>(i), cp) && cp.visitation_status == VisitationStatus::Goal) {
            hasGoal = true;
            break;
        }
    }
    if (!hasGoal && n > 1) {
        int goalIdx = startIdx;
        while (goalIdx == startIdx) goalIdx = idxDist(rng);
        _repo->updateCityStatus(goalIdx, VisitationStatus::Goal);
    }

    syncSelectionDetails();
    reDraw();
}

void SidePanelView::handleRandomizeAll()
{
    if (!_repo) return;
    const size_t n = _repo->cityCount();
    if (n == 0) return;

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> xDist(20.0, 980.0);
    std::uniform_real_distribution<double> yDist(20.0, 846.0);
    std::uniform_int_distribution<int> statusDist(0, 2); // Blocked, Open, Goal
    std::uniform_int_distribution<int> idxDist(0, static_cast<int>(n) - 1);

    for (size_t i = 0; i < n; ++i) {
        CityPoint cp;
        if (!_repo->getCity(static_cast<int>(i), cp)) continue;

        const double nx = xDist(rng);
        const double ny = yDist(rng);
        _repo->updateCity(static_cast<int>(i), cp.name, nx, ny, cp.weight);
    }

    // After positions are randomized, regenerate nearest-neighbor style connections
    handleRandomizeConnections();

    // Only then randomize weights
    handleRandomizeWeights();

    // Finally randomize statuses
    for (size_t i = 0; i < n; ++i) {
        VisitationStatus s = static_cast<VisitationStatus>(statusDist(rng));
        _repo->updateCityStatus(static_cast<int>(i), s);
    }

    // Ensure exactly one Start
    int startIdx = idxDist(rng);
    _repo->updateCityStatus(startIdx, VisitationStatus::Start);

    // Ensure at least one Goal if possible
    bool hasGoal = false;
    for (size_t i = 0; i < n; ++i) {
        CityPoint cp;
        if (_repo->getCity(static_cast<int>(i), cp) && cp.visitation_status == VisitationStatus::Goal) {
            hasGoal = true;
            break;
        }
    }
    if (!hasGoal && n > 1) {
        int goalIdx = startIdx;
        while (goalIdx == startIdx) goalIdx = idxDist(rng);
        _repo->updateCityStatus(goalIdx, VisitationStatus::Goal);
    }

    syncSelectionDetails();
    reDraw();
}

void SidePanelView::handleRandomizeGAParameters()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> popDist(20, 200);
    std::uniform_int_distribution<int> genDist(100, 3000);
    std::uniform_real_distribution<double> mutDist(0.005, 0.20);
    std::uniform_real_distribution<double> crossDist(0.50, 0.95);
    std::uniform_real_distribution<double> elitDist(2.0, 30.0);
    std::uniform_int_distribution<int> selDist(0, 2);
    std::uniform_int_distribution<int> opDist(0, 2);

    txtEditPopulationSize.setText(std::to_string(popDist(rng)).c_str());
    txtEditNumberOfGenerations.setText(std::to_string(genDist(rng)).c_str());

    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3) << mutDist(rng);
        std::string s = os.str();
        dotToComma(s);
        txtEditMutationRate.setText(s.c_str());
    }
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(3) << crossDist(rng);
        std::string s = os.str();
        dotToComma(s);
        txtEditCrossoverRate.setText(s.c_str());
    }
    {
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << elitDist(rng);
        std::string s = os.str();
        dotToComma(s);
        txtEditElitismPercentage.setText(s.c_str());
    }

    cmbSelectionMethod.selectIndex(selDist(rng));
    cmbMutationCrossoverOperator.selectIndex(opDist(rng));
    reDraw();
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

bool SidePanelView::isNN2OptEnabled() const
{
    return checkBoxEnable2opt.isChecked();
}

int SidePanelView::getNNImprovementCycles() const
{
    int cycles = static_cast<int>(std::lround(
        txtEditImprovementCycles.getValue().toNumber<double>()
    ));

    if (cycles < 0) cycles = 0;
    return cycles;
}


double SidePanelView::getSAInitialTemperature() const
{
    double t = txtEditInitialTemperature.getValue().toNumber<double>();
    return (t > 0.0) ? t : 10000.0;
}

double SidePanelView::getSACoolingRateAlpha() const
{
    double a = sliderCoolingRateAlpha.getValue();
    if (a < 0.90) a = 0.90;
    if (a > 0.99) a = 0.99;
    return a;
}

int SidePanelView::getSAIterationsPerTemperature() const
{
    int it = static_cast<int>(std::lround(
        txtEditIterationsPerTemperatureLevel.getValue().toNumber<double>()
    ));

    return (it > 0) ? it : 25;
}


bool SidePanelView::useNearestNeighborAsSAInitialSolution() const
{
    return cmbSAInitialSolution.getSelectedIndex() == 1;
}

int SidePanelView::getGAPopulationSize() const
{
    int v = static_cast<int>(std::lround(
        txtEditPopulationSize.getValue().toNumber<double>()
    ));
    return (v >= 2) ? v : 50;
}


int SidePanelView::getGANumberOfGenerations() const
{
    int v = neToIntRound(txtEditNumberOfGenerations);
    return (v >= 1) ? v : 1000;
}

double SidePanelView::getGAMutationRate() const
{
    double x = txtEditMutationRate.getValue().toNumber<double>();

    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    return x;
}

double SidePanelView::getGACrossoverRate() const
{
    double x = txtEditCrossoverRate.getValue().toNumber<double>();

    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    return x;
}


int SidePanelView::getGASelectionMethodIndex() const
{
    int idx = cmbSelectionMethod.getSelectedIndex();
    return (idx >= 0) ? idx : 0;
}

int SidePanelView::getGAMutationOperatorIndex() const
{
    int idx = cmbMutationCrossoverOperator.getSelectedIndex();
    return (idx >= 0) ? idx : 0;
}

double SidePanelView::getGAElitismPercentage() const
{
    double x = txtEditElitismPercentage.getValue().toNumber<double>();

    if (x < 0.0) x = 0.0;
    if (x > 100.0) x = 100.0;
    return x;
}

int SidePanelView::getExecutionSpeedLevel() const
{
    int speed = static_cast<int>(sliderAnimationSpeed.getValue() + 0.5);
    if (speed < 1) speed = 1;
    if (speed > 10) speed = 10;
    return speed;
}

int SidePanelView::getSelectedPointIndex() const
{
    return cmbPoints.getSelectedIndex();
}

void SidePanelView::selectPointIndex(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(_pointNames.size())) return;
    selectIndexAndUpdate(idx);
    reDraw();
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

    // Lock NN controls while running
    checkBoxEnable2opt.enable(!running);
    txtEditImprovementCycles.enable(!running);

    // Lock SA controls while running
    txtEditInitialTemperature.enable(!running);
    sliderCoolingRateAlpha.enable(!running);
    txtEditIterationsPerTemperatureLevel.enable(!running);
    cmbSAInitialSolution.enable(!running);

    // Lock GA controls while running
    txtEditPopulationSize.enable(!running);
    txtEditNumberOfGenerations.enable(!running);
    txtEditMutationRate.enable(!running);
    txtEditCrossoverRate.enable(!running);
    cmbSelectionMethod.enable(!running);
    cmbMutationCrossoverOperator.enable(!running);
    txtEditElitismPercentage.enable(!running);
    btnRandomizeParameters.enable(!running);
}


void SidePanelView::handleSetAllToVisit()
{
    if (!_repo) return;

    const auto& cities = _repo->cities();
    if (cities.empty()) return;

    int startIdx = -1;
    for (size_t i = 0; i < cities.size(); ++i) {
        if (cities[i].visitation_status == VisitationStatus::Start) {
            startIdx = static_cast<int>(i);
            break;
        }
    }

    for (size_t i = 0; i < cities.size(); ++i) {
        int idx = static_cast<int>(i);
        if (idx == startIdx) continue;
        _repo->updateCityStatus(idx, VisitationStatus::Goal);
    }



    syncSelectionDetails();
    reDraw();
}
