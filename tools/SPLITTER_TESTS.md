# Splitter regression test

`test_splitter_layout.cpp` is an optional GUI test, separate from the normal
application target. Build it with the same natID headers and libraries as the
application, linking all application `.cpp` files except `src/main.cpp`.
Use the normal Debug runtime (`/MDd` with `natGUID` and `mainUtilsD` on Windows).
Pass the project's absolute resource path as `-devResPath=<project directory>`.

The test opens its own window, runs 105 resize/drag combinations through the
real `SplitterLayout::setGeometry` and `onSplitterCellMove` entry points, then
closes automatically. It checks the left panel width, the right auxiliary map's
700-unit minimum, pane overlap, and the native rectangles of the rightmost form
controls for all three algorithm selections. The result is written alongside
the executable as `<executable name>.results.txt`; exit code 0 means success.

Define `SPLITTER_BASELINE_TEST` to replace the map's geometry hook with a regular
`MapView`. That configuration is expected to fail: it reproduces the native
splitter ignoring the primary pane's minimum.

The fix lives in `include/MapSplitterLayout.h`. Its left-pane minimum is the
larger of 420 logical units and the form's measured minimum plus 20 units for
the scrollbar. The map remains the right auxiliary pane. This uses natID 3.2.7's
child layout order (left panel, divider, right map); rerun this test when
upgrading the SDK. No SDK files need modification.
