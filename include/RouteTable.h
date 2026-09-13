#pragma once
#include <gui/Canvas.h>
#include <gui/Context.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include <gui/ViewScroller.h>
#include <gui/GridLayout.h>
#include <gui/View.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <functional>
#include "DataRepository.h"

// Read-only route table: only its rows scroll, never the surrounding form.
// Data stays in memory; no database or new runtime dependency is required.
class RouteTable : public gui::View
{
    struct Row { std::string city; double distance; };
    class Rows : public gui::Canvas
    {
    public:
        Rows() : gui::Canvas({gui::InputDevice::Event::PrimaryClicks}) {}
        bool cityList = false;
        int selected = -1;
        std::function<void(int)> onSelect;
        std::vector<Row> rows;
        bool getModelSize(gui::Size& size) const override
        {
            size = gui::Size(100, std::max(28.0, rows.size() * 26.0));
            return true;
        }
        void changed() { handleModelSizeChanged(); reDraw(); }
        void redrawSelection() { reDraw(); }
    protected:
        void onPrimaryButtonPressed(const gui::InputDevice& input) override
        {
            if (!cityList || !input.isModelPointAvailable()) return;
            const double y = input.getModelPoint().y;
            if (y < 0 || y >= rows.size() * 26.0) return;
            selected = static_cast<int>(y / 26.0);
            reDraw();
            if (onSelect) onSelect(selected);
        }
        void onDraw(const gui::Rect& dirty) override
        {
            gui::Context context;
            gui::Size viewport;
            if (getScroller()) getScroller()->getPageSize(viewport);
            else getSize(viewport);
            const double width = std::max(100.0, viewport.width);
            gui::Shape::drawRect(dirty, td::ColorID::WhiteSmoke);
            const size_t first = static_cast<size_t>(std::max(0.0, std::floor(dirty.top / 26.0)));
            const size_t last = std::min(rows.size(), static_cast<size_t>(std::max(0.0, std::ceil(dirty.bottom / 26.0))));
            for (size_t i = first; i < last; ++i) {
                const double y = i * 26.0;
                if (i % 2 == 0) gui::Shape::drawRect(gui::Rect(0, y, width, y + 26), td::ColorID::White);
                if (cityList && selected == static_cast<int>(i))
                    gui::Shape::drawRect(gui::Rect(0, y, width, y + 26), td::ColorID::LightBlue);
                gui::DrawableString number(std::to_string(i + 1).c_str());
                gui::DrawableString city(rows[i].city.c_str());
                std::ostringstream value;
                value << std::fixed << std::setprecision(1) << rows[i].distance;
                gui::DrawableString distance(value.str().c_str());
                number.draw(gui::Rect(6, y + 4, 38, y + 24), gui::Font::ID::SystemNormal, td::ColorID::DarkSlateGray);
                city.draw(gui::Rect(42, y + 4, width - (cityList ? 8 : 118), y + 24), gui::Font::ID::SystemNormal, td::ColorID::DarkSlateGray);
                if (!cityList) distance.draw(gui::Rect(width - 114, y + 4, width - 8, y + 24), gui::Font::ID::SystemNormal,
                              td::ColorID::DarkSlateGray, td::TextAlignment::Right);
            }
        }
    } _rows;
    class Header : public gui::Canvas
    {
    public:
        bool cityList = false;
        Header() { setFixedHeight(28); }
    protected:
        void onDraw(const gui::Rect& dirty) override
        {
            gui::Context context;
            gui::Size size; getSize(size);
            gui::Shape::drawRect(dirty, td::ColorID::WhiteSmoke);
            gui::DrawableString("#").draw(gui::Point(6, 5), gui::Font::ID::SystemBold, td::ColorID::DarkSlateGray);
            gui::DrawableString(tr("City")).draw(gui::Point(42, 5), gui::Font::ID::SystemBold, td::ColorID::DarkSlateGray);
            if (!cityList) gui::DrawableString(tr("Total distance")).draw(gui::Rect(size.width - 132, 5, size.width - 22, 26),
                gui::Font::ID::SystemBold, td::ColorID::DarkSlateGray, td::TextAlignment::Right);
        }
    } _header;
    gui::ViewScroller _scroller;
    gui::GridLayout _layout;
protected:
    void measure(gui::CellInfo& cell) override
    { gui::View::measure(cell); cell.minVer = 100; cell.nResVer = 1; }
    void reMeasure(gui::CellInfo& cell) override
    { gui::View::reMeasure(cell); cell.minVer = 100; cell.nResVer = 1; }
public:
    explicit RouteTable(bool cityList = false) : gui::View(0, 0, 0, 0),
        _scroller(gui::ViewScroller::Type::NoScroll, gui::ViewScroller::Type::ScrollAndAutoHide), _layout(2, 1)
    {
        _rows.cityList = _header.cityList = cityList;
        _scroller.setContentView(&_rows);
        _scroller.setSizeLimits(100, gui::Control::Limit::UseAsMin, 64, gui::Control::Limit::UseAsMin);
        _layout.setMargins(0, 0);
        _layout.setSpaceBetweenCells(0, 0);
        _layout.insert(0, 0, _header);
        _layout.insert(1, 0, _scroller);
        setLayout(&_layout);
    }
    void clear() { _rows.rows.clear(); _rows.changed(); }
    void setCities(const std::vector<std::string>& names)
    {
        _rows.rows.clear();
        for (const auto& name : names) _rows.rows.push_back({name, 0});
        _rows.selected = -1;
        _rows.changed();
    }
    void onCitySelected(std::function<void(int)> callback) { _rows.onSelect = std::move(callback); }
    void selectCity(int index)
    {
        _rows.selected = index >= 0 && index < static_cast<int>(_rows.rows.size()) ? index : -1;
        _rows.redrawSelection();
    }
    void setRoute(const DataRepository& repo, const std::vector<int>& tour)
    {
        _rows.rows.clear();
        double total = 0;
        std::vector<int> expanded;
        for (size_t i = 0; i < tour.size(); ++i) {
            std::vector<int> leg;
            if (!repo.getShortestRoadPath(tour[i], tour[(i + 1) % tour.size()], leg)) { clear(); return; }
            if (expanded.empty()) expanded = leg;
            else if (!leg.empty()) expanded.insert(expanded.end(), leg.begin() + 1, leg.end());
        }
        for (size_t i = 0; i < expanded.size(); ++i) {
            CityPoint city;
            if (!repo.getCity(expanded[i], city)) { clear(); return; }
            if (i) total += repo.getMetricDistance(expanded[i - 1], expanded[i]);
            _rows.rows.push_back({city.name, total});
        }
        _rows.changed();
        _scroller.scrollVisibleOriginTo(gui::Point(0, 0));
    }
};
