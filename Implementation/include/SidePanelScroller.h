#pragma once
#include <gui/ViewScroller.h>
#include "SidePanelView.h"

// The tabs and forms stay visible; only the result table scrolls internally.
// Keep this wrapper's public interface unchanged for MainView/the splitter.
class SidePanelScroller : public gui::View
{
    gui::GridLayout _layout;
public:
    SidePanelView panel;

    SidePanelScroller()
        : gui::View(0, 0, 0, 0), _layout(1, 1)
    {
        _layout.insert(0, 0, panel);
        setLayout(&_layout);
    }
};
