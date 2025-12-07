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
    gl(11, 2)
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

        // Row 10: Update point button spanning all columns
        btnUpdatePoint.setType(gui::Button::Type::Default);
        btnUpdatePoint.setSizeLimitForNChars(12, gui::Control::Limit::None);
        gl.insert(10, 0, btnUpdatePoint, -1);

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
    }
}

void SidePanelView::selectIndexAndUpdate(int idx)
{
    if (idx < 0 || idx >= (int)_pointNames.size()) return;
    cmbPoints.selectIndex(idx);
    updateCurrentNameFromSelection();
    updateCurrentCoordsFromSelection();
}
