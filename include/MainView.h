#pragma once
#include <gui/View.h>
#include <gui/HorizontalLayout.h>
#include "MapView.h"
#include "SidePanelView.h"

class MainView : public gui::View
{
private:
    gui::HorizontalLayout _hlayout;
    MapView _mapView;
    SidePanelView _sidePanel;

public:
    MainView()
        : _hlayout(2)
    {
        setMargins(0, 0, 0, 0);
        // Size allocation: side panel fixed/min width, both views min height 1000
        // Adjust 500 to desired side panel width
        _sidePanel.setSizeLimits(500, gui::Control::Limit::UseAsMin,
                     866, gui::Control::Limit::UseAsMin);
        // Map canvas should take the remaining space; set min width 0, min height 1000
        _mapView.setSizeLimits(1000, gui::Control::Limit::UseAsMin,
                       866, gui::Control::Limit::UseAsMin);

        _hlayout.append(_mapView, td::HAlignment::Left, td::VAlignment::Top);
        _hlayout.append(_sidePanel, td::HAlignment::Left, td::VAlignment::Top);
        setLayout(&_hlayout);

        // Left: MapView (canvas), Right: SidePanelView (controls)
        // Wiring for interactions will be added as views evolve.

        // Populate side panel dropdown with city names loaded by MapView
        _sidePanel.setMapView(&_mapView);
        _sidePanel.populatePointNames(_mapView.getCityNames());
        _sidePanel.syncSelectionDetails();
    }
};
