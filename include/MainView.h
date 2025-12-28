#pragma once
#include <gui/View.h>
#include <gui/HorizontalLayout.h>
#include "DataRepository.h"
#include "MapView.h"
#include "SidePanelView.h"

class MainView : public gui::View
{
private:
    gui::HorizontalLayout _hlayout;
    DataRepository _repo;       // Single data source (owns the data)
    MapView _mapView;           // Rendering only
    SidePanelView _sidePanel;   // UI controls

public:
    MainView()
        : _hlayout(2)
    {
        setMargins(0, 0, 0, 0);
        
        // Size limits
        _sidePanel.setSizeLimits(500, gui::Control::Limit::UseAsMin,
                                 866, gui::Control::Limit::UseAsMin);
        _mapView.setSizeLimits(1000, gui::Control::Limit::UseAsMin,
                               866, gui::Control::Limit::UseAsMin);

        _hlayout.append(_mapView, td::HAlignment::Left, td::VAlignment::Top);
        _hlayout.append(_sidePanel, td::HAlignment::Left, td::VAlignment::Top);
        setLayout(&_hlayout);

        // Wire up: repository -> mapView & sidePanel
        _mapView.setRepository(&_repo);
        _sidePanel.setRepository(&_repo);
        _sidePanel.populatePointNames(_repo.getCityNames());
        _sidePanel.syncSelectionDetails();
    }
};
