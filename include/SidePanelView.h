#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TextEdit.h>
#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/LineEdit.h>
#include <gui/NumericEdit.h>
#include <gui/ComboBox.h>

#include <vector>
#include <string>

class MapView;


// Minimal side panel view for the Travelling Salesman project.
class SidePanelView : public gui::View
{
public:
    gui::Label lblXCoord;
    gui::NumericEdit txtEditXCoord;
    gui::Label lblYCoord;
    gui::NumericEdit txtEditYCoord;
    gui::Button btnAddPt;
    gui::Label lblName;
    gui::LineEdit lnEditName;
    gui::Label lblSeparator;
    gui::Label lblChoosePoint;
    gui::ComboBox cmbPoints;
    gui::Label lblCurrentCityName;
    gui::LineEdit lnEditCurrentCityName;
    gui::Label lblCurrentX;
    gui::NumericEdit neCurrentX;
    gui::Label lblCurrentY;
    gui::NumericEdit neCurrentY;
    gui::Button btnUpdatePoint;
    gui::Button btnDeletePoint;
    gui::Label lblConnections;
    gui::Label lblConnectionsValue;
    gui::Label lblConnectTo;
    gui::ComboBox cmbConnectTo;
    gui::Button btnAddConnection;
    gui::Button btnRemoveConnection;
    gui::GridLayout gl;

    SidePanelView();

    ~SidePanelView() = default;

    // Populate dropdown with provided point names
    void populatePointNames(const std::vector<std::string>& names);

    // Connects to the map view so we can query/update points
    void setMapView(MapView* mapView);

    // Sync current selection fields (name + coords) with selected dropdown item
    void syncSelectionDetails();

protected:
    bool onChangedSelection(gui::ComboBox* pCB) override;
    bool onClick(gui::Button* pBtn) override;

private:
    std::vector<std::string> _pointNames;
    MapView* _mapView = nullptr;

    void handleAddPoint();
    void handleUpdatePoint();
    void handleDeletePoint();
    void handleAddConnection();
    void handleRemoveConnection();
    void selectIndexAndUpdate(int idx);
    void updateCurrentNameFromSelection();
    void updateCurrentCoordsFromSelection();
    void updateConnectionsLabel();
    void populateConnectToCombo();
};
