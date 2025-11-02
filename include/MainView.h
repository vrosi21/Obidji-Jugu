#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include "MapView.h"
#include "SidePanelView.h"

class MainView : public gui::View
{
private:
    gui::SplitterLayout _splitter;
    MapView _mapView;
    SidePanelView _sidePanel;

public:
    MainView()
        : _splitter(gui::SplitterLayout::Orientation::Horizontal,
                    gui::SplitterLayout::AuxiliaryCell::Second)
    {
        setMargins(0, 0, 0, 0);
        _splitter.setContent(_mapView, _sidePanel);
        setLayout(&_splitter);

        // For the Travelling Salesman project the left canvas (MapView)
        // and the right side panel (SidePanelView) are intentionally
        // empty for now. Wiring of algorithm buttons and solver
        // callbacks is deferred until MapView/SidePanelView are fleshed out.
    }
};
