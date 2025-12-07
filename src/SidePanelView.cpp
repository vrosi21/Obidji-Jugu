#include "SidePanelView.h"
#include "MapView.h"
#include <gui/GridLayout.h>
#include <cstdlib>
#include <sstream>
#include <iomanip>

SidePanelView::SidePanelView() 
    : lblXCoord("X:"), 
    txtEditXCoord(td::DataType::decimal4), 
    lblYCoord("Y:"), 
    txtEditYCoord(td::DataType::decimal4), 
    btnAddPt(tr("Add point")), 
    lnEditName(), 
    lblName(tr("Name:")),
    lblChoosePoint(tr("Select point:")),
    lblCurrentCityName(tr("Current name:")),
    lblCurrentX(tr("Current X:")),
    neCurrentX(td::DataType::decimal4),
    lblCurrentY(tr("Current Y:")),
    neCurrentY(td::DataType::decimal4),
    btnUpdatePoint(tr("Update point")),
    btnDeletePoint(tr("Delete point")),
    lblConnections(tr("Connections:")),
    lblConnectionsValue(""),
    lblConnectTo(tr("Connect to:")),
    btnAddConnection(tr("Add connection")),
    btnRemoveConnection(tr("Remove connection")),
    gl(14, 2)
{
        txtEditXCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);
        txtEditYCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);
        lblName.setSizeLimitForNChars(1, gui::Control::Limit::UseAsMin);

        btnAddPt.setType(gui::Button::Type::Default);
        btnAddPt.setSizeLimitForNChars(7, gui::Control::Limit::None);

        // Grid layout: 2 columns
        // Row 0: Name label + name input
        gl.insert(0, 0, lblName, td::HAlignment::Left);
        gl.insert(0, 1, lnEditName, td::HAlignment::Left);
        // Row 1: X and Y labels
        gl.insert(1, 0, lblXCoord, td::HAlignment::Left);
        gl.insert(1, 1, lblYCoord, td::HAlignment::Left);
        // Row 2: X and Y inputs
        gl.insert(2, 0, txtEditXCoord, td::HAlignment::Left);
        gl.insert(2, 1, txtEditYCoord, td::HAlignment::Left);
        // Row 3: Button spanning all columns (span to end)
        gl.insert(3, 0, btnAddPt, -1);

        // Row 4: Dropdown label spanning all columns
        gl.insert(4, 0, lblChoosePoint, -1);
        // Row 5: Dropdown spanning all columns
        gl.insert(5, 0, cmbPoints, -1);

        // Row 6: Current city name label
        gl.insert(6, 0, lblCurrentCityName, -1);
        // Row 7: Current city name input spanning all columns
        gl.insert(7, 0, lnEditCurrentCityName, -1);

        // Row 8: Current X and Y labels
        gl.insert(8, 0, lblCurrentX, td::HAlignment::Left);
        gl.insert(8, 1, lblCurrentY, td::HAlignment::Left);
        // Row 9: Current X and Y inputs
        neCurrentX.setText("0");
        neCurrentY.setText("0");
        gl.insert(9, 0, neCurrentX, td::HAlignment::Left);
        gl.insert(9, 1, neCurrentY, td::HAlignment::Left);

        // Row 10: Update + Delete buttons
        btnUpdatePoint.setType(gui::Button::Type::Default);
        btnUpdatePoint.setSizeLimitForNChars(12, gui::Control::Limit::None);
        gl.insert(10, 0, btnUpdatePoint, td::HAlignment::Left);
        btnDeletePoint.setType(gui::Button::Type::Default);
        btnDeletePoint.setSizeLimitForNChars(12, gui::Control::Limit::None);
        gl.insert(10, 1, btnDeletePoint, td::HAlignment::Left);

        // Row 11: connections label/value
        gl.insert(11, 0, lblConnections, td::HAlignment::Left);
        gl.insert(11, 1, lblConnectionsValue, td::HAlignment::Left);

        // Row 12: connect-to dropdown
        gl.insert(12, 0, lblConnectTo, td::HAlignment::Left);
        gl.insert(12, 1, cmbConnectTo, td::HAlignment::Left);

        // Row 13: add/remove connection buttons
        btnAddConnection.setType(gui::Button::Type::Default);
        gl.insert(13, 0, btnAddConnection, td::HAlignment::Left);
        gl.insert(13, 1, btnRemoveConnection, td::HAlignment::Left);

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
        neCurrentX.setText("0");
        neCurrentY.setText("0");
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
    if (pBtn == &btnAddConnection)
    {
        handleAddConnection();
        return true;
    }
    if (pBtn == &btnRemoveConnection)
    {
        handleRemoveConnection();
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
    if (_mapView && idx >= 0)
    {
        CityPoint cp;
        if (_mapView->getCity(idx, cp))
        {
            std::ostringstream sx;
            std::ostringstream sy;
            sx << std::fixed << std::setprecision(2) << cp.x;
            sy << std::fixed << std::setprecision(2) << cp.y;
            xStr = sx.str();
            yStr = sy.str();
        }
    }
    neCurrentX.setText(xStr.c_str());
    neCurrentY.setText(yStr.c_str());
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

    if (_mapView->updateCity(idx, name, x, y))
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

void SidePanelView::handleAddConnection()
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

    if (_mapView->addConnection(fromIdx, actualTo))
    {
        updateConnectionsLabel();
    }
}

void SidePanelView::handleRemoveConnection()
{
    if (!_mapView) return;
    int fromIdx = cmbPoints.getSelectedIndex();
    int toIdxLocal = cmbConnectTo.getSelectedIndex();
    if (fromIdx < 0 || toIdxLocal < 0) return;

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

    if (_mapView->removeConnection(fromIdx, actualTo))
    {
        updateConnectionsLabel();
    }
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
