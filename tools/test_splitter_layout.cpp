// GUI regression test against the real natID layout library. Run with the
// project's normal -devResPath argument. Writes <executable>.results.txt in
// the executable directory and stops the test application automatically.
#include "MapSplitterLayout.h"
#include "SidePanelScroller.h"
#include <gui/Application.h>
#include <gui/Window.h>
#include <gui/Timer.h>
#include <gui/WinMain.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
std::filesystem::path resultPath;
int testExitCode = 1;

#ifdef SPLITTER_BASELINE_TEST
// Reproduce the native bug using an ordinary map without the geometry hook.
class TestMap : public MapView
{
public:
    explicit TestMap(MapSplitterLayout&) {}
};
#else
using TestMap = SplitterMapView;
#endif

class TestSplitter : public MapSplitterLayout
{
public:
    void resizeAndDrag(double width, td::UINT4 mapWidth, const gui::Cell& cell)
    {
        // setGeometry is the resize entry; onSplitterCellMove is the native
        // entry called by SplitterCell after dragging changes the stored size.
        setGeometry(gui::Geometry(0, 0, width, 780), cell);
        _sizeOfAuxiliaryCell = mapWidth;
        onSplitterCellMove();
    }

    double formWidth() const { return _cells[0].minHor; }
    double dividerWidth() const { return _spaceOfSplitterCell; }
    bool mapIsAuxiliary() const { return _posOfAuxiliaryCell == AuxiliaryCell::Second; }
};

class TestView : public gui::View
{
    TestSplitter _splitter;
    SidePanelScroller _controls;
    TestMap _map;

public:
    TestView() : _map(_splitter)
    {
        setMargins(0, 0, 0, 0);
        _controls.setSizeLimits(300, gui::Control::Limit::UseAsMin,
                                200, gui::Control::Limit::UseAsMin);
        _map.setSizeLimits(700, gui::Control::Limit::UseAsMin);
        _splitter.setSpaceBetweenCells(4);
        _splitter.setContent(_controls, _map);
        setLayout(&_splitter);
    }

    void run(std::ostream& out)
    {
        unsigned count = 0;
        for (int algorithm = 0; algorithm < 3; ++algorithm) {
            _controls.panel.cmbSolvingAlgorithm.selectIndex(algorithm);
            // Change widths in both directions, including after a capped drag.
            for (double width : {1500.0, 1920.0, 1280.0, 2560.0, 1124.0, 1200.0, 1500.0}) {
                for (double fraction : {0.5, 0.75, 0.89, 0.6, 0.89}) {
                    const auto requested = static_cast<td::UINT4>(
                        std::max(700.0, std::floor(width * fraction)));
                    _splitter.resizeAndDrag(width, requested, getCell());
                    gui::Geometry left, right;
                    _controls.getGeometry(left);
                    _map.getGeometry(right);
                    out << "algorithm=" << algorithm << " width=" << width
                        << " requestedMap=" << requested
                        << " formMin=" << _splitter.formWidth()
                        << " left=" << left.size.width
                        << " right=" << right.size.width
                        << " rightX=" << right.point.x << std::endl;
                    if (!_splitter.mapIsAuxiliary()
                        || left.size.width + 1.0 < 420.0
                        || left.size.width + 1.0 < _splitter.formWidth()
                        || right.size.width + 1.0 < 700.0
                        || right.point.x < left.point.x + left.size.width
                        || right.point.x + right.size.width > width + 1.0)
                        throw std::runtime_error("Pane geometry violated its limits");

                    // These occupy the rightmost columns of the original form.
                    // Verify the actual native widget frames, not only the pane.
                    unsigned controlIndex = 0;
                    for (gui::Control* control : {
                            static_cast<gui::Control*>(&_controls.panel.txtEditYCoord),
                            static_cast<gui::Control*>(&_controls.panel.neCurrentY),
                            static_cast<gui::Control*>(&_controls.panel.btnDeletePoint),
                            static_cast<gui::Control*>(&_controls.panel.btnRandomizeAll),
                            static_cast<gui::Control*>(&_controls.panel.btnStepFwd),
                            static_cast<gui::Control*>(&_controls.panel.cmbSolvingAlgorithm),
                            static_cast<gui::Control*>(&_controls.panel.sliderAnimationSpeed)}) {
                        if (control->isHidden()) {
                            ++controlIndex;
                            continue;
                        }
                        gui::Geometry widget;
                        control->getGeometry(widget);
                        if (widget.point.x < -1.0
                            || widget.point.x + widget.size.width > left.size.width + 1.0) {
                            out << "controlIndex=" << controlIndex
                                << " widgetX=" << widget.point.x
                                << " widgetWidth=" << widget.size.width << std::endl;
                            throw std::runtime_error("A form control extends outside its viewport");
                        }
                        ++controlIndex;
                    }
                    ++count;
                }
            }
        }
        out << "PASS: " << count << " native resize/drag cases" << std::endl;
    }
};

class TestWindow : public gui::Window
{
    TestView _view;
    gui::Timer _timer;
protected:
    void onInitialAppearance() override { _timer.start(); }
    bool onTimer(gui::Timer*) override
    {
        _timer.stop();
        std::ofstream out(resultPath);
        try {
            _view.run(out);
            testExitCode = 0;
        } catch (const std::exception& error) {
            out << "FAIL: " << error.what() << std::endl;
        }
        close();
        return true;
    }
public:
    TestWindow() : gui::Window(gui::Size(1500, 866)), _timer(this, 0.1f, false)
    {
        setTitle("Splitter constraint regression");
        setCentralView(&_view, Frame::FixSizes::FixMin);
    }
};

class TestApplication : public gui::Application
{
    gui::Window* createInitialWindow() override { return new TestWindow(); }
public:
    using gui::Application::Application;
};
}

int main(int argc, const char** argv)
{
    resultPath = std::filesystem::absolute(argv[0]);
    resultPath.replace_extension(".results.txt");
    const std::string defaultResources = "-devResPath="
        + std::filesystem::path(__FILE__).parent_path().parent_path().string();
    const char* defaults[] = {argv[0], defaultResources.c_str()};
    TestApplication app(argc == 1 ? 2 : argc, argc == 1 ? defaults : argv);
    app.init("EN");
    app.run();
    return testExitCode;
}
