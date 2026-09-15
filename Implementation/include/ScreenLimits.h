#pragma once
#include <gui/Display.h>
#include <algorithm>
#include <cmath>

inline gui::Size applicationScreenSize()
{
    gui::Size screen;
    gui::Display::getDefaultLogicalSize(screen);
    if (!std::isfinite(screen.height) || screen.height <= 0) screen.height = 768;
    if (!std::isfinite(screen.width) || screen.width <= 0) screen.width = 1366;
    return screen;
}

inline td::UINT2 minimumApplicationContentHeight()
{
    // Logical pixels work with Retina/Windows scaling. Reserve space for the
    // title, toolbar, menu bar and dock/taskbar instead of using physical pixels.
    return static_cast<td::UINT2>(std::max(1.0,
        std::min(640.0, applicationScreenSize().height - 140.0)));
}

inline gui::Size initialApplicationSize()
{
    const auto screen = applicationScreenSize();
    return gui::Size(std::max(1.0, std::min(1500.0, screen.width - 60.0)),
                     std::max(1.0, std::min(866.0, screen.height - 80.0)));
}
