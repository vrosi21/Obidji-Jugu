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
    btnUpdatePoint(tr("Update point")),
    gl(8, 6)
    
{
        

        btnAddPt.setType(gui::Button::Type::Default);
        btnAddPt.setSizeLimitForNChars(11, gui::Control::Limit::UseAsMin);

        gui::GridComposer gc(gl);

        btnUpdatePoint.setSizeLimitForNChars(11, gui::Control::Limit::UseAsMin);
        btnUpdatePoint.setType(gui::Button::Type::Default);


        // Row 0: Name label and Name input
        gc.appendRow(lblName); gc.appendCol(lnEditName,5);
        // Row 1: X and Y labels and X and Y inputs
        gc.appendRow(lblXCoord); gc.appendCol(txtEditXCoord); gc.appendCol(lblYCoord); gc.appendCol(txtEditYCoord);
        // Row 2: Add point button
        gc.appendRow(btnAddPt, 4, td::HAlignment::Left);
        // Row 3: Dropdown label spanning all columns
        gc.appendRow(lblChoosePoint, 4); 
        // Row 4: Dropdown spanning all columns
        gc.appendRow(cmbPoints, 4);
        // Row 5: Current city name label and currenc city name input
        gc.appendRow(lblCurrentCityName); gc.appendCol(lnEditCurrentCityName, 5);
        // Row 6: Current X and Y labels and current X and Y inputs
        gc.appendRow(lblCurrentX); gc.appendCol(neCurrentX); gc.appendCol(lblCurrentY); gc.appendCol(neCurrentY);
        // Row 7: Update point button spanning all columns
        gc.appendRow(btnUpdatePoint);


        
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
