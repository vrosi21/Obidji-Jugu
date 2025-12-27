#include "SidePanelView.h"
#include "MapView.h"
#include <gui/GridLayout.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <gui/GridComposer.h>

SidePanelView::SidePanelView()
    : lblXCoord("X:"),
    txtEditXCoord(td::DataType::decimal1),
    lblYCoord("Y:"),
    txtEditYCoord(td::DataType::decimal1),
    btnAddPt(tr("Add point")),
    lnEditName(),
    lblName(tr("City name:")),
    lblChoosePoint(tr("Select point:")),
    lblCurrentCityName(tr("Current city name:")),
    lblCurrentX(tr("Current X:")),
    neCurrentX(td::DataType::decimal1),
    lblCurrentY(tr("Current Y:")),
    neCurrentY(td::DataType::decimal1),
    lblCurrentWeight(tr("Weight:")),
    neCurrentWeight(td::DataType::decimal1),
    btnUpdatePoint(tr("Update point")),
    btnDeletePoint(tr("Delete point")),
    lblConnections(tr("Connections:")),
    lblConnectionsValue(""),
    lblConnectTo(tr("Connect to:")),
    btnToggleConnection(tr("Toggle connection")),
    gl(14, 6),
    mustVisit(false),
    toggleMustVisit("Toggle Must Visit"),
    lblSolvingSection("Solve:")
{
        gui::GridComposer gc(gl);

        // Row 0: Name label and Name input
        gc.appendRow(lblName); gc.appendCol(lnEditName, 5);
        // Row 1: X and Y labels and X and Y inputs
        gc.appendRow(lblXCoord); gc.appendCol(txtEditXCoord); gc.appendSpace(1,0); gc.appendCol(lblYCoord); gc.appendCol(txtEditYCoord);
        // Row 2: Add point button
        btnAddPt.setType(gui::Button::Type::Default);
        btnAddPt.setSizeLimitForNChars(11, gui::Control::Limit::UseAsMin);
        gc.appendRow(btnAddPt, -1, td::HAlignment::Left);
        // Row 3: Select point label and dropdown on the same row
        gc.appendRow(lblChoosePoint); gc.appendCol(cmbPoints, 5);
        // Row 5: Current city name label and currenc city name input
        gc.appendRow(lblCurrentCityName); gc.appendCol(lnEditCurrentCityName, 5);
        // Row 6: Current X and Y labels and current X and Y inputs
        gc.appendRow(lblCurrentX); gc.appendCol(neCurrentX); gc.appendCol(lblCurrentY); gc.appendCol(neCurrentY); gc.appendCol(lblCurrentWeight); gc.appendCol(neCurrentWeight);
        gc.appendRow(toggleMustVisit);
        // Row 7: Update and Delete buttons
        btnUpdatePoint.setType(gui::Button::Type::Default);
        btnUpdatePoint.setSizeLimitForNChars(11, gui::Control::Limit::UseAsMin);
        btnDeletePoint.setType(gui::Button::Type::Default);
        btnDeletePoint.setSizeLimitForNChars(11, gui::Control::Limit::UseAsMin);
        gc.appendRow(btnUpdatePoint); gc.appendCol(btnDeletePoint, -1, td::HAlignment::Right);
        // Row 8: Connections label
        gc.appendRow(lblConnections, -1, td::HAlignment::Left);
        // Row 9: Connections value label
        gc.appendRow(lblConnectionsValue, -1, td::HAlignment::Left);
        // Row 10: Connect to label and dropdown
        gc.appendRow(lblConnectTo); 
        // Row 11: Toggle connection button
        btnToggleConnection.setType(gui::Button::Type::Default);
        btnToggleConnection.setSizeLimitForNChars(15, gui::Control::Limit::UseAsMin);
        gc.appendRow(cmbConnectTo); gc.appendCol(btnToggleConnection, 3);

        // Row 13
        lblSolvingSection.setFont(gui::Font::ID::SystemLargerBold);
        gc.appendRow(lblSolvingSection);
        gc.appendRow(cmbSolvingAlgorithm);

        
        // Populate solving algorithms combobox
        populateSolvingAlgorithms(_algorithmNames);


        setLayout(&gl);
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
    }
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

bool SidePanelView::onChangedSelection(gui::ComboBox* pCB)
{
    if (pCB == &cmbPoints)
    {
        int idx = pCB->getSelectedIndex();
        selectIndexAndUpdate(idx);
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
    if (pBtn == &toggleMustVisit)
    {
        handleToggleMustVisit();
        return true;
    }
    return false;
}

void SidePanelView::setMapView(MapView* mapView)
{
    _mapView = mapView;
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
    if (_mapView && idx >= 0)
    {
        CityPoint cp;
        if (_mapView->getCity(idx, cp))
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

            mustVisit = cp.mustVisit;
            if (mustVisit) {
                toggleMustVisit.setTitle("Must Visit: YES");
                // Opcionalno promijeni boju teksta ako framework dopušta
            }
            else {
                toggleMustVisit.setTitle("Must Visit: NO");
            }
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
    if (_mapView && idx >= 0)
    {
        auto names = _mapView->getConnectionNames(idx);
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

void SidePanelView::handleToggleMustVisit() {
    if (mustVisit) {
        mustVisit = false;
        toggleMustVisit.setTitle("Must Visit: NO");
    }
    else {
        mustVisit = true;
        toggleMustVisit.setTitle("Must Visit: YES");
    }
}

void SidePanelView::handleAddPoint()
{
    if (!_mapView) return;
    std::string name = lnEditName.getText().c_str();
    double x = std::atof(txtEditXCoord.getText().c_str());
    double y = std::atof(txtEditYCoord.getText().c_str());

    if (_mapView->addCity(name, x, y))
    {
        _pointNames = _mapView->getCityNames();
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
    if (!_mapView) return;
    int idx = cmbPoints.getSelectedIndex();
    if (idx < 0) return;

    std::string name = lnEditCurrentCityName.getText().c_str();
    double x = std::atof(neCurrentX.getText().c_str());
    double y = std::atof(neCurrentY.getText().c_str());
    double weight = std::atof(neCurrentWeight.getText().c_str());

    if (_mapView->updateCity(idx, name, x, y, weight, mustVisit))
    {
        _pointNames = _mapView->getCityNames();
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
    if (!_mapView) return;
    int idx = cmbPoints.getSelectedIndex();
    if (idx < 0) return;
    if (_mapView->deleteCity(idx))
    {
        _pointNames = _mapView->getCityNames();
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
    if (!_mapView) return;
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
    if (_mapView->hasConnection(fromIdx, actualTo))
    {
        _mapView->removeConnection(fromIdx, actualTo);
    }
    else
    {
        _mapView->addConnection(fromIdx, actualTo);
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
}
