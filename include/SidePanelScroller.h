#pragma once
#include <gui/ViewScroller.h>
#include "SidePanelView.h"

// Wraps SidePanelView in a scrollable container so the side panel
// can be scrolled vertically when the window is too small to show it all.
class SidePanelScroller : public gui::ViewScroller
{
public:
    SidePanelView panel;

    SidePanelScroller()
        : gui::ViewScroller(gui::ViewScroller::Type::NoScroll,
                            gui::ViewScroller::Type::ScrollAndAutoHide)
    {
        setContentView(&panel);
    }
};
