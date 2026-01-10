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
    // Solve Section
    lblAlgorithm(tr("Algorithm:")),
    btnStartPause(tr("Start/Pause")),
    btnStepFwd(tr("Step >")),
    btnStepBwd(tr("< Step")),
    // Layout (24 rows, 4 columns)
    gl(24, 4)
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
    // SECTION 4: Solve
    // ========================================
    lblSolvingSection.setFont(gui::Font::ID::SystemLargerBold);
    gc.appendRow(lblSolvingSection, 4);
    
    gc.appendRow(lblAlgorithm);
    gc.appendCol(cmbSolvingAlgorithm, 3);
    
    btnStartPause.setType(gui::Button::Type::Default);
    btnStepBwd.setType(gui::Button::Type::Default);
    btnStepFwd.setType(gui::Button::Type::Default);
    gc.appendRow(btnStartPause, 2);
    gc.appendCol(btnStepBwd);
    gc.appendCol(btnStepFwd);

    setLayout(&gl);
    
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
