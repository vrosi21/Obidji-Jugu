#pragma once

#include <algorithm>
#include <cmath>
#include <gui/SplitterLayout.h>
#include "MapView.h"
#include "ScreenLimits.h"

// natID 3.2.7's native drag handler caps the auxiliary pane at 90% of
// the view width, irrespective of the primary pane's measured minimum.
// Keep the native splitter, but reserve the space the controls actually need.
class MapSplitterLayout : public gui::SplitterLayout
{
    bool _correctingGeometry = false;

    gui::CoordType controlsMinimumWidth() const
    {
        // Include room for the vertical scrollbar next to the form.
        return std::max<gui::CoordType>(420.0, _cells[0].minHor + 20.0);
    }

    void reserveBothPanes(gui::CellInfo& cell) const
    {
        cell.minVer = std::max(cell.minVer, minimumApplicationContentHeight());
        const auto minimum = controlsMinimumWidth() + _cells[2].minHor
            + _spaceOfSplitterCell + 2.0 * getXMargin();
        cell.minHor = std::max(cell.minHor,
            static_cast<td::UINT2>(std::min(65535.0, std::ceil(minimum))));
    }

protected:
    void initialMeasure(gui::CellInfo& cell) override
    {
        gui::SplitterLayout::initialMeasure(cell);
        reserveBothPanes(cell);
    }

    void measure(gui::CellInfo& cell) override
    {
        gui::SplitterLayout::measure(cell);
        reserveBothPanes(cell);
    }

    void reMeasure(gui::CellInfo& cell) override
    {
        gui::SplitterLayout::reMeasure(cell);
        reserveBothPanes(cell);
    }

public:
    MapSplitterLayout()
        : gui::SplitterLayout(Orientation::Horizontal, AuxiliaryCell::Second)
    {
    }

    // Called by the right map before its geometry is applied. This hook also
    // runs on native divider drags, which bypass SplitterLayout::setGeometry.
    bool constrainMapWidth()
    {
        if (_correctingGeometry || _sizeOfAuxiliaryCell == 0
            || _splitterCell.getStatus() != gui::SplitterCell::Status::Normal)
            return false;

        const auto width = _lastGeometry.size.width;
        if (!std::isfinite(width) || width <= 0.0)
            return false;

        const auto maximum = std::max(0.0,
            width - 2.0 * getXMargin() - _spaceOfSplitterCell
                - controlsMinimumWidth());
        const auto boundedSize = static_cast<td::UINT4>(std::floor(maximum));
        if (_sizeOfAuxiliaryCell <= boundedSize)
            return false;

        _sizeOfAuxiliaryCell = boundedSize;
        _correctingGeometry = true;
        updateGeometry(_lastGeometry);
        _correctingGeometry = false;
        return true;
    }
};

class SplitterMapView final : public MapView
{
    MapSplitterLayout& _splitter;

public:
    explicit SplitterMapView(MapSplitterLayout& splitter)
        : _splitter(splitter)
    {
    }

    void setGeometry(const gui::Geometry& frame, const gui::Cell& cell) override
    {
        // The map is the LAST child laid out by natID's updateGeometry(). A
        // corrected pass has already placed all three children; do not apply
        // the old map rectangle after that pass. The guard bounds nesting to 1.
        if (_splitter.constrainMapWidth())
            return;
        MapView::setGeometry(frame, cell);
    }
};
