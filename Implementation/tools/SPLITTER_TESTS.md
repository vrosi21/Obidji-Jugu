# Splitter regression test

`test_splitter_layout.cpp` is an optional GUI test, separate from the normal
application target. Build it with the same natID headers and libraries as the
application, linking all application `.cpp` files except `src/main.cpp`.
Use the normal Debug runtime (`/MDd` with `natGUID` and `mainUtilsD` on Windows).
Pass the project's absolute resource path as `-devResPath=<project directory>`.

The test opens its own window, runs 420 resize/drag combinations through the
real `SplitterLayout::setGeometry` and `onSplitterCellMove` entry points, then
closes automatically. It checks the left panel width, the right auxiliary map's
700-unit minimum, pane overlap, and the native rectangles of the rightmost form
controls for both tabs, all three algorithm selections, and two viewport heights.
It also verifies retained form values, algorithm-specific field visibility,
setup CRUD/randomization, and solver button message routing through the tab
pages. CRUD checks use only a disposable in-memory repository. On Windows,
native assertions are redirected to a separate diagnostic file instead of
blocking the test with a runtime dialog. The result is written alongside
the executable as `<executable name>.results.txt`; exit code 0 means success.

Define `SPLITTER_BASELINE_TEST` to replace the map's geometry hook with a regular
`MapView`. That configuration is expected to fail: it reproduces the native
splitter ignoring the primary pane's minimum.

The fix lives in `include/MapSplitterLayout.h`. Its left-pane minimum is the
larger of 420 logical units and the form's measured minimum plus 20 units for
the scrollbar. The map remains the right auxiliary pane. This uses natID 3.2.7's
child layout order (left panel, divider, right map); rerun this test when
upgrading the SDK. No SDK files need modification.

The tab layout lives in `SidePanelView`: Simulation Setup contains city setup
and randomization; Solve contains algorithm parameters and animation controls.
The forms are top-aligned in plain View hosts. Solve includes a bounded,
internally scrollable route table below the inputs. The surrounding UI does
not scroll. Solve/Resolve replaces Start/Pause and Show Solution.

For v23 only the application was compiled, per the requested single-pass scope;
this optional regression suite was not run again.
