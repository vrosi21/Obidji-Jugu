# natGUI Framework — Comprehensive Quiz Prep Study Guide

> Covers: Layouts, Views, Windows, Canvas, Resources, and CMake build system basics.

---

## Table of Contents

1. [Class Hierarchy Overview](#1-class-hierarchy-overview)
2. [Layouts](#2-layouts)
   - [Layout (Base)](#layout-base-class)
   - [GridLayout & GridComposer](#gridlayout--gridcomposer)
   - [HorizontalLayout & VerticalLayout](#horizontallayout--verticallayout)
   - [SplitterLayout](#splitterlayout)
3. [Views](#3-views)
   - [BaseView](#baseview)
   - [View](#view)
   - [ViewScroller](#viewscroller)
   - [DrawableView](#drawableview)
4. [Windows](#4-windows)
   - [Frame (Base)](#frame-base-class)
   - [Window](#window)
   - [Dialog](#dialog)
5. [Canvas](#5-canvas)
   - [Canvas class](#canvas-class)
   - [Shape & Drawing Primitives](#shape--drawing-primitives)
   - [DrawableString](#drawablestring)
   - [Transformation](#transformation)
   - [Image (Drawing)](#image-drawing)
6. [Resources](#6-resources)
   - [DevRes.xml & main.xml](#devresxml--mainxml)
   - [Images & Icons](#images--icons)
   - [Sounds](#sounds)
   - [FileNames (Data Files)](#filenames-data-files)
   - [Translation / i18n (tr())](#translation--i18n-tr)
   - [IAppProperties (Persistent Settings)](#iappproperties-persistent-settings)
7. [CMake Build System](#7-cmake-build-system)
   - [Three-Layer Architecture](#three-layer-architecture)
   - [CMakeLists.txt Boilerplate](#cmakeliststxt-boilerplate)
   - [Project .cmake File Pattern](#project-cmake-file-pattern)
   - [Key CMake Functions](#key-cmake-functions)
   - [Windows Icon (.ico / .rc)](#windows-icon-ico--rc)
   - [macOS Icon (.icns) & Info.plist](#macos-icon-icns--infoplist)
   - [Executable Module & Linking](#executable-module--linking)
8. [Quick Reference Table](#8-quick-reference-table)

---

## 1. Class Hierarchy Overview

```
gui::NatObject             (tr(), getApplication(), getAppProperties())
  └── gui::Consumer        (onActionItem)
        └── gui::Frame     (setTitle, reDraw, setSize, key events, alerts)
              ├── gui::Control        (setSizeLimits, enable/disable, hide)
              │     ├── gui::Layout   (base for all layouts)
              │     │     ├── gui::GridLayout
              │     │     ├── gui::StackedLayout
              │     │     │     ├── gui::HorizontalLayout
              │     │     │     └── gui::VerticalLayout
              │     │     └── gui::SplitterLayout
              │     └── gui::BaseView (abstract base for all views)
              │           ├── gui::View          (form-based UI with layouts)
              │           ├── gui::ViewScroller   (scrollable wrapper)
              │           └── gui::DrawableView   (custom drawing)
              │                 └── gui::Canvas   (full 2D drawing surface)
              └── gui::Window         (top-level window)
                    └── gui::Dialog   (modal/non-modal dialog with buttons)
```

**Key insight**: Both Layouts and Views inherit from `gui::Control`. Layouts arrange controls; Views contain/display them. Windows are top-level containers that host a single central view.

---

## 2. Layouts

### Layout (Base Class)

All layout classes inherit from `gui::Layout → gui::Control`. You assign a layout to a view:

```cpp
view.setLayout(&myLayout);
```

A view can have **exactly one** layout. Layouts can contain other layouts (nesting).

---

### GridLayout & GridComposer

**GridLayout** — a 2D grid of rows × columns:

```cpp
gui::GridLayout _gl(8, 4);   // 8 rows, 4 columns
```

**Key methods:**
| Method | Purpose |
|--------|---------|
| `GridLayout(nRows, nCols)` | Constructor — dimensions |
| `insert(row, col, ctrl)` | Place control at position |
| `insert(row, col, ctrl, colSpan)` | Place with column span (`0` or `-1` = span to end) |
| `insert(row, col, ctrl, colSpan, hAlign, rowSpan, vAlign)` | Full control |
| `setMargins(marginX, marginY)` | Outer margins |
| `setSpaceBetweenCells(rowSpace, colSpace)` | Cell spacing |

**Direct insert example:**
```cpp
gui::GridLayout _gl(3, 2);
_gl.insert(0, 0, _lblName);
_gl.insert(0, 1, _editName);
_gl.insert(1, 0, _lblPassword);
_gl.insert(1, 1, _editPassword);
_gl.insert(2, 0, _btnSubmit, -1);  // spans all columns
setLayout(&_gl);
```

**GridComposer** — fluent builder that auto-tracks row/column position:

```cpp
gui::GridComposer gc(_gl);
gc.appendRow(ctrl);           // next row, column 0
gc.appendCol(ctrl);           // next column in current row
gc.appendCol(ctrl, colSpan);  // with column span
gc.appendRow(ctrl, colSpan, hAlignment);
```

**GridComposer example:**
```cpp
gui::GridLayout _gl(5, 4);
gui::GridComposer gc(_gl);

gc.appendRow(_lblName);                    // row 0, col 0
gc.appendCol(_editName, 3);               // row 0, col 1, spans 3 cols

gc.appendRow(_lblX);                       // row 1, col 0
gc.appendCol(_editX);                      // row 1, col 1
gc.appendCol(_lblY);                       // row 1, col 2
gc.appendCol(_editY);                      // row 1, col 3

gc.appendRow(_btnSubmit, 4);               // row 2, spans all 4 cols

setLayout(&_gl);
```

**`<<` operator shorthand:**
```cpp
gc.appendRow(_lbl) << _lineEdit << _lbl2 << _lineEdit2;
```

---

### HorizontalLayout & VerticalLayout

Both inherit from `gui::StackedLayout`. Stack controls in one direction.

**HorizontalLayout:**
```cpp
gui::HorizontalLayout _hl(4);   // reserve for 4 elements
_hl.append(_btn1);
_hl.append(_btn2);
_hl.appendSpacer();             // flexible space (pushes remaining to right)
_hl.append(_btn3);
// or: _hl << _btn1 << _btn2;
```

**VerticalLayout:**
```cpp
gui::VerticalLayout _vl(5);
_vl.append(_lbl, td::HAlignment::Left);
_vl.append(_lineEdit);
_vl.appendSpace(20);            // fixed 20px gap
_vl.append(_textEdit);
_vl.appendSpacer();             // flexible space
```

**Key methods (both):**
| Method | Purpose |
|--------|---------|
| `append(ctrl)` | Add control |
| `append(ctrl, hAlign, vAlign)` | Add with alignment |
| `appendSpace(px)` | Fixed-size gap |
| `appendSpacer(weight)` | Flexible space (default weight=1) |
| `appendLayout(layout)` | Nest another layout |
| `operator << (ctrl)` | Shorthand for append |
| `setSpaceBetweenCells(px)` | Uniform spacing |

---

### SplitterLayout

Two resizable panes with a draggable divider bar. One pane is the "auxiliary" cell (can be collapsed/minimized by the user).

```cpp
gui::SplitterLayout _splitter(
    gui::SplitterLayout::Orientation::Horizontal,    // side-by-side
    gui::SplitterLayout::AuxiliaryCell::Second        // right pane is collapsible
);
```

**Orientation enum:**
| Value | Meaning |
|-------|---------|
| `Horizontal` | Left and right panes |
| `Vertical` | Top and bottom panes |

**AuxiliaryCell enum:**
| Value | Meaning |
|-------|---------|
| `First` | Left/top pane is collapsible |
| `Second` | Right/bottom pane is collapsible |

**Key methods:**
```cpp
_splitter.setContent(ctrl1, ctrl2);                    // set both panes
_splitter.setView(0, leftCtrl);                        // set individual pane
_splitter.setToolTipForMinimizedAux("Show panel");     // tooltip when collapsed
_splitter.setMargins(mx, my);
_splitter.setSpaceBetweenCells(px);
```

**Example:**
```cpp
class MainView : public gui::View {
    gui::SplitterLayout _splitter{
        gui::SplitterLayout::Orientation::Horizontal,
        gui::SplitterLayout::AuxiliaryCell::Second
    };
    MapView _mapView;
    SidePanelScroller _sidePanel;
public:
    MainView() {
        setMargins(0, 0, 0, 0);
        _splitter.setContent(_mapView, _sidePanel);
        setLayout(&_splitter);
    }
};
```

---

## 3. Views

### BaseView

Abstract base for all views. Key virtuals:

| Virtual Method | Purpose |
|----------------|---------|
| `getModelSize(gui::Size&)` | Declare logical model size (for scrolling) |
| `onGeometryChange(const Geometry&)` | React to geometry changes |
| `shouldClose()` | Return `false` to prevent closing |
| `onClose()` | Cleanup on close |
| `scale(double)` / `scaleToPoint(...)` | Zoom support |

---

### View

The standard view for **form-based UI** (labels, buttons, text fields, etc.). Holds a layout.

```cpp
class MyView : public gui::View {
    gui::Label _lbl;
    gui::GridLayout _gl;
public:
    MyView() : _lbl("Hello"), _gl(1, 1) {
        _gl.insert(0, 0, _lbl);
        setLayout(&_gl);                // THE key call
    }
};
```

**Key methods:**
| Method | Purpose |
|--------|---------|
| `setLayout(Layout*)` | Assign the layout to this view |
| `setMargins(left, top, right, bottom)` | View padding |
| `adjustLayout(const Size&)` | Virtual — called on every resize |

**Pattern**: Declare member controls + layout → wire in constructor → call `setLayout(&layout)`.

---

### ViewScroller

Wraps any `BaseView` in a scrollable container. Shows scrollbars when content exceeds visible area.

```cpp
gui::ViewScroller(Type horizontal, Type vertical);
```

**Type enum:**
| Value | Meaning |
|-------|---------|
| `NoScroll` | No scrollbar on this axis |
| `ScrollerAlwaysVisible` | Scrollbar always shown |
| `ScrollAndAutoHide` | Scrollbar appears only when needed |

**Key methods:**
| Method | Purpose |
|--------|---------|
| `setContentView(BaseView*)` | Set the view to scroll |
| `setContentSize(gui::Size&)` | Explicitly set content dimensions |
| `handleModelSizeChanged()` | Call when model size changes at runtime |
| `scaleContent(double)` | Zoom the content |
| `makeVisible(const Rect&)` | Scroll to make a region visible |

**Example — Scrollable side panel:**
```cpp
class SidePanelScroller : public gui::ViewScroller {
public:
    SidePanelView panel;
    SidePanelScroller()
        : gui::ViewScroller(Type::NoScroll, Type::ScrollAndAutoHide)  // vertical only
    {
        setContentView(&panel);
    }
};
```

**Example — Scrollable canvas:**
```cpp
class MainView : public gui::ViewScroller {
    MyCanvas _canvas;
public:
    MainView()
    : gui::ViewScroller(Type::ScrollAndAutoHide, Type::ScrollAndAutoHide)
    {
        setContentView(&_canvas);
    }
};
```

When wrapping a `Canvas` in a `ViewScroller`, the canvas must override `getModelSize()` to declare its logical dimensions.

---

### DrawableView

Base class for custom 2D drawing. Override `onDraw()` to render content.

**Key virtual methods:**
| Method | Purpose |
|--------|---------|
| `onDraw(const gui::Rect& rect)` | Main drawing callback |
| `onResize(const gui::Size& newSize)` | Called on resize (if enabled) |
| `onPrimaryButtonPressed(InputDevice&)` | Left click |
| `onSecondaryButtonPressed(InputDevice&)` | Right click |
| `onPrimaryButtonDblClick(InputDevice&)` | Double click |
| `onZoom(InputDevice&)` | Pinch/scroll zoom |
| `onScroll(InputDevice&)` | Scroll wheel |
| `onCursorMoved(InputDevice&)` | Mouse move |
| `onCursorDragged(InputDevice&)` | Click+drag |
| `onCursorEntered/Exited(InputDevice&)` | Mouse enter/leave |

---

## 4. Windows

### Frame (Base Class)

Base for both `Control` and `Window`. Provides core infrastructure.

**Key methods:**
| Method | Purpose |
|--------|---------|
| `setTitle(const char*)` | Set window/frame title |
| `reDraw()` | Trigger full redraw |
| `reDraw(const Rect&)` | Redraw specific region |
| `setSize(const Size&)` | Set dimensions |
| `getSize(Size&)` | Get current dimensions |
| `setFocus(bool selectAll)` | Give keyboard focus |
| `setBackgroundColor(ColorID)` | Background color |
| `hide(bool)` | Show/hide |

**FixSizes enum** — controls window resize behavior:
| Value | Meaning |
|-------|---------|
| `No` | Fully resizable, no constraints |
| `FixMin` | Minimum size locked to content's measured size |
| `FixHorMax` | Horizontal max locked |
| `FixVerMax` | Vertical max locked |
| `FixMin_andHorMax` | Min locked + horizontal max locked |
| `FixMin_andVerMax` | Min locked + vertical max locked |
| `FixAll` | Both min and max locked (fixed-size window) |
| `FixAuto` | Framework decides (default) |

**Alert/Message methods:**
```cpp
showAlert("Title", "Message");
showInfo("Title", "Message");
showWarning("Title", "Message");
showError("Title", "Message");
showYesNoQuestionAsync(...);
```

---

### Window

Top-level application window. Contains toolbar + central view + status bar.

**Construction:**
```cpp
gui::Window(gui::Size(1500, 866));                // initial size
gui::Window(gui::Geometry(50, 50, 400, 300));     // position + size
```

**FrameSize enum** — how the window opens:
| Value | Meaning |
|-------|---------|
| `Minimized` | Start minimized |
| `UseSpecified` | Use the size from constructor |
| `AdjustToContent` | Fit to content (default) |
| `Maximized` | Start maximized |
| `FullScreen` | Full screen |

**Key methods:**
| Method | Purpose |
|--------|---------|
| `setCentralView(BaseView*, FixSizes)` | **THE** key method — sets the main content view |
| `setToolBar(ToolBar&)` | Attach toolbar |
| `setStatusBar(StatusBar&)` | Attach status bar |
| `open(FrameSize)` | Show the window |
| `openModal(FrameSize)` | Show as modal |
| `close()` | Close window |
| `setResizable(bool)` | Allow/prevent resizing |
| `shouldClose()` | Virtual — return `false` to prevent close |
| `onClose()` | Virtual — cleanup on close |
| `onInitialAppearance()` | Virtual — called once when first shown |
| `onActionItem(ActionItemDescriptor&)` | Handle menu/toolbar actions |

**Standard window pattern:**
```cpp
class MainWindow : public gui::Window {
    ToolBar _toolBar;
    MainView _mainView;
public:
    MainWindow() : gui::Window(gui::Size(1500, 866)) {
        setTitle("My Application");
        setToolBar(_toolBar);
        setCentralView(&_mainView, Frame::FixSizes::FixMin);
    }
    bool shouldClose() override { return true; }
    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override {
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        // handle toolbar/menu actions...
        return false;
    }
};
```

---

### Dialog

A window with built-in buttons (OK, Cancel, etc.) and a central view. Inherits from `Window`.

**Button IDs:** `Cancel`, `OK`, `Apply`, `Delete`, `Close`, `User0`..`User9`

**Construction:**
```cpp
gui::Dialog(parentFrame,
    { {Dialog::Button::ID::OK, tr("Ok"), gui::Button::Type::Default},
      {Dialog::Button::ID::Cancel, tr("Cancel")} },
    gui::Size(300, 200),
    dialogID);
```

**Key methods:**
| Method | Purpose |
|--------|---------|
| `onClick(Button::ID, Button*)` | Virtual — handle button clicks; return `true` to close |
| `openModal(callback)` | Show modal with completion callback |
| `openNonModal(...)` | Show non-modal |
| `getClickedButtonID()` | Which button was clicked |
| `setCentralView(BaseView*)` | Set dialog content |

**Predefined button sets:** `Dialog::OkCancel`

**Example:**
```cpp
class MyDialog : public gui::Dialog {
    MyDialogView _view;
public:
    MyDialog(gui::Frame* parent)
    : gui::Dialog(parent, Dialog::OkCancel, gui::Size(400, 300))
    {
        setTitle("Settings");
        setCentralView(&_view);
    }
protected:
    bool onClick(Dialog::Button::ID btnID, gui::Button*) override {
        if (btnID == Dialog::Button::ID::OK) {
            // validate and save...
            return true;  // close
        }
        return true;
    }
};
```

---

## 5. Canvas

### Canvas Class

Full-featured 2D drawing surface. Inherits: `Canvas → DrawableView → BaseView → Control`.

**Construction with input events:**
```cpp
gui::Canvas({
    gui::InputDevice::Event::PrimaryClicks,
    gui::InputDevice::Event::SecondaryClicks,
    gui::InputDevice::Event::Zoom,
    gui::InputDevice::Event::Keyboard
});
```

**InputDevice::Event values:**
| Event | Purpose |
|-------|---------|
| `PrimaryClicks` | Left mouse button |
| `SecondaryClicks` | Right mouse button |
| `PrimaryDblClick` | Double click |
| `CursorMove` | Mouse movement |
| `CursorDrag` | Click + drag |
| `CursorEnterLeave` | Enter/exit tracking |
| `Zoom` | Pinch/scroll zoom |
| `Keyboard` | Key events on canvas |

**Critical: `enableResizeEvent(true)`** — you MUST call this in the constructor to receive `onResize()` callbacks. Without it, `onResize()` is never called.

**Key methods:**
| Method | Purpose |
|--------|---------|
| `enableResizeEvent(true)` | **Required** for `onResize` to fire |
| `onDraw(const Rect&)` | Override to draw content |
| `onResize(const Size&)` | Override to react to size changes |
| `reDraw()` | Trigger redraw |
| `getModelSize(Size&)` | Override for scrollable canvas |
| `scale(double)` / `scaleToPoint(...)` | Zoom |
| `getScale()` | Current zoom level |
| `setCursor(Cursor::Type)` | Change mouse cursor |
| `startAnimation()` / `stopAnimation()` | Built-in animation loop |

**InputDevice — accessing mouse/touch data:**
```cpp
void onPrimaryButtonPressed(const gui::InputDevice& inputDevice) override {
    const gui::Point& framePoint = inputDevice.getFramePoint();   // pixel coords
    const gui::Point& modelPoint = inputDevice.getModelPoint();   // model coords (with zoom)
    double zoomScale = inputDevice.getScale();                    // for onZoom
    const gui::Point& scrollDelta = inputDevice.getScrollDelta(); // for onScroll
}
```

**Minimal canvas example:**
```cpp
class MyCanvas : public gui::Canvas {
    gui::Size _currentSize;
public:
    MyCanvas()
        : gui::Canvas({gui::InputDevice::Event::PrimaryClicks})
    {
        enableResizeEvent(true);
    }
    void onResize(const gui::Size& newSize) override {
        _currentSize = newSize;
    }
    void onDraw(const gui::Rect& rect) override {
        gui::Shape::drawRect(rect, td::ColorID::White);    // background
        gui::Shape circle;
        circle.createCircle(gui::Circle(200, 200, 50), 2);
        circle.drawFillAndWire(td::ColorID::Blue, td::ColorID::Black);
    }
    void onPrimaryButtonPressed(const gui::InputDevice& dev) override {
        auto pt = dev.getFramePoint();
        // handle click at pt.x, pt.y
    }
};
```

---

### Shape & Drawing Primitives

`gui::Shape` — create geometry, then draw it. Can be reused across frames.

**Creation methods:**
| Method | Creates |
|--------|---------|
| `createRect(Rect, lineWidth, pattern)` | Rectangle |
| `createRoundedRect(Rect, radius, lw)` | Rounded rectangle |
| `createCircle(Circle, lw)` | Circle |
| `createOval(Rect, lw)` | Ellipse |
| `createArc(Circle, fromAngle, toAngle, lw)` | Arc |
| `createPie(Circle, from, to, lw)` | Pie/wedge |
| `createPolyLine(pts, n, lw)` | Open polyline |
| `createPolygon(pts, n, lw)` | Closed polygon |
| `createLines(pts, n, lw)` | Individual line segments |
| `createBezier(lw, pattern)` | Bezier path (returns `Shape::Bezier`) |

**Drawing methods:**
| Method | Renders |
|--------|---------|
| `drawWire(lineColor)` | Outline only |
| `drawFill(fillColor)` | Fill only |
| `drawFillAndWire(fill, line)` | Both fill and outline |
| `drawFillAndWire(fill, line, lw)` | Both with explicit line width |

**Static one-shot drawing (no pre-creation):**
```cpp
Shape::drawRect(rect, td::ColorID::White);                         // filled rect
Shape::drawRect(rect, td::ColorID::Blue, td::ColorID::Black, 1);  // fill + outline
Shape::drawLine(p1, p2, td::ColorID::Red, 2.0f);                  // line
```

**Bezier paths:**
```cpp
gui::Shape shape;
auto bezier = shape.createBezier(2.0f, td::LinePattern::Solid);
bezier.moveTo({x1, y1});
bezier.lineTo({x2, y2});
bezier.quadraticTo(point, controlPoint);
bezier.cubicTo(point, ctrl1, ctrl2);
bezier.end();
shape.drawWire(td::ColorID::Black);
```

---

### DrawableString

Text rendering inside `onDraw()`:

```cpp
gui::DrawableString msg("Hello World");
gui::Point pos(50.0f, 100.0f);
msg.draw(pos, gui::Font::ID::SystemNormal, td::ColorID::Black);
```

**Draw in rectangle with alignment:**
```cpp
gui::Rect textRect(50, 500, 600, 540);
msg.draw(textRect, gui::Font::ID::SystemLargerBold, td::ColorID::Red,
         td::TextAlignment::Center, td::VAlignment::Center);
```

**Measure text size:**
```cpp
gui::Size textSize;
msg.measure(gui::Font::ID::SystemNormal, textSize);
// textSize.width, textSize.height now contain the text dimensions
```

**Static one-shot draw:**
```cpp
gui::DrawableString::draw(td::String("Quick text"), point, fontID, colorID);
```

**Available Font::ID values:** `SystemSmall`, `SystemNormal`, `SystemBold`, `SystemLarger`, `SystemLargerBold`, etc.

---

### Transformation

Affine transforms applied to the drawing context. Affect all subsequent drawing operations.

```cpp
gui::Transformation tr;
tr.translate(dx, dy);        // move origin
tr.scale(factor);            // uniform scale
tr.scale(sx, sy);            // non-uniform scale
tr.rotateDeg(angleDegrees);  // rotate
tr.rotateRad(angleRadians);
tr.identity();               // reset to identity
tr.appendToContext();         // multiply into current CTM
tr.setToContext();            // replace current CTM
```

**Save/Restore pattern** (push/pop transform stack):
```cpp
gui::Transformation::saveContext();     // save current state

gui::Transformation tr;
tr.translate(100, 200);
tr.rotateDeg(45);
tr.appendToContext();
// ... all drawing here is translated + rotated ...

gui::Transformation::restoreContext();  // restore previous state
```

**Clipping:**
```cpp
gui::Transformation::setClip(gui::Rect(0, 0, 500, 500));
// Only content within this rect is drawn
```

---

### Image (Drawing)

Load and render images:

```cpp
gui::Image img(":resID");       // load from resource system
gui::Image img("file.png");     // load from file path
gui::Image blank(gui::Size(200, 150));  // blank image
```

**Key methods:**
| Method | Purpose |
|--------|---------|
| `isOK()` | Check if loaded successfully |
| `draw(Rect, AspectRatio, hAlign, vAlign)` | Draw into rectangle |
| `load(path)` | Load at runtime |
| `getSize(Size&)` | Get image dimensions |
| `getHWRatio()` | Height/width ratio |
| `saveToFile(path, Type)` | Export PNG/JPG/BMP |

**AspectRatio enum:**
| Value | Meaning |
|-------|---------|
| `No` | Stretch to fill rectangle |
| `Keep` | Maintain proportions (default) |

**Drawing example:**
```cpp
gui::Rect imgRect(offsetX, offsetY, offsetX + width, offsetY + height);
_bgImage.draw(imgRect, gui::Image::AspectRatio::No);     // stretch
_bgImage.draw(imgRect, gui::Image::AspectRatio::Keep);   // maintain ratio
```

---

## 6. Resources

### DevRes.xml & main.xml

The **resource system** uses two XML config files:

**`res/DevRes.xml`** — master configuration (one per project):
```xml
<?xml version='1.0'?>
<DevRes regionals="Work/Common/Regionals">
    <Langs>
        <Lang id="EN" name="English"/>
        <Lang id="BA" name="Bosanski"/>
    </Langs>
    <Config path="Work/Common/ResConfigs/natGUI.xml"/>
    <ArtWork>
        <Config path=":main.xml"/>
    </ArtWork>
    <Translation>
        <Config path="Work/Common/ResConfigs/tr/%s/natGUI.xml"/>
        <Config path=":tr/%s/main.xml"/>
    </Translation>
</DevRes>
```

| Section | Purpose |
|---------|---------|
| `<Langs>` | Declare supported languages |
| `regionals` attribute | Path to shared regional data |
| `<Config>` (root level) | Core framework configuration |
| `<ArtWork>` | Image/resource declarations |
| `<Translation>` | Translation file locations (`%s` = language ID) |

**`res/main.xml`** — application-specific resource declarations:
```xml
<?xml version='1.0'?>
<DevRes>
    <Images>
        <Res id="flagEN" path=":flag-us.png"/>
        <Res id="bgMap" path=":assets/yugoslavia.png"/>
        <Res id="zoomIn" path="Work/Common/Icons/zoomIn64.png"/>
    </Images>
    <FileNames>
        <Res id="exYu" path=":exYu.json"/>
    </FileNames>
</DevRes>
```

**Path conventions:**
- `:` prefix = relative to project's `res/` folder (e.g., `:flag-us.png` → `res/flag-us.png`)
- Without `:` = relative to the development `Work/` root
- Resolved at runtime via the `-devResPath` argument passed by CMake

---

### Images & Icons

Declared in `<Images>` section of `main.xml`. Loaded in code with the resource ID:

```cpp
gui::Image _bgImage{":bgMap"};       // loads "bgMap" from <Images>
gui::Image _flagEN(":flagEN");        // loads "flagEN" from <Images>
```

For toolbar icons, images are loaded and passed to `addItem()`:
```cpp
gui::Image _imgEN(":flagEN");
addItem(tr("English"), &_imgEN, tr("English"), 255, 0, 0, 10);
```

---

### Sounds

Declared in `<Sounds>` section (if used). Loaded like images:

```cpp
gui::Sound snd(":mySoundID");
snd.play();           // play once
snd.play(true);       // loop
snd.stop();

// System sounds:
gui::Sound::play(gui::Sound::Type::Beep);
```

---

### FileNames (Data Files)

Declared in `<FileNames>` section of `main.xml`. Resolved via `getResFileName()`:

```cpp
td::String jsonPath = getResFileName(":exYu");    // resolves to actual filesystem path
// Use the path to load data:
std::ifstream file(std::string(jsonPath.c_str()));
```

---

### Translation / i18n (tr())

Translation files live in `res/tr/<LANG_ID>/main.xml`:

```xml
<!-- res/tr/EN/main.xml -->
<DevRes>
    <Translations>
        <Res id="Algorithm:" tr="Algorithm:"/>
        <Res id="Start Tour" tr="Start Tour"/>
    </Translations>
</DevRes>

<!-- res/tr/BA/main.xml -->
<DevRes>
    <Translations>
        <Res id="Algorithm:" tr="Algoritam:"/>
        <Res id="Start Tour" tr="Pokreni turu"/>
    </Translations>
</DevRes>
```

**Usage** — the `tr()` function is available on all `gui::NatObject` subclasses:
```cpp
gui::Label _lbl(tr("Algorithm:"));    // "Algorithm:" or "Algoritam:" depending on language
gui::Button _btn(tr("Start Tour"));   // translated automatically
```

**Language initialization (main.cpp):**
```cpp
Application app(argc, argv);
auto appProperties = app.getProperties();
td::String trLang = appProperties->getValue("translation", "EN");
app.init(trLang);    // loads the corresponding translation file
return app.run();
```

**Language switching at runtime:**
```cpp
auto appProperties = getAppProperties();
appProperties->setValue("translation", "BA");
gui::getApplication()->restart();   // restarts app with new language
```

---

### IAppProperties (Persistent Settings)

OS-native persistent storage (Windows registry / macOS plist / Linux settings):

```cpp
// Available from any gui::NatObject:
auto props = getAppProperties();
// Or from main():
auto props = app.getProperties();

// Read:
td::String lang = props->getValue("translation", "EN");   // default "EN"
bool visible = props->getValue("TBVisible", true);         // default true

// Write (auto-saved, no explicit save needed):
props->setValue("translation", "BA");
props->setValue("TBVisible", false);
```

---

## 7. CMake Build System

### Three-Layer Architecture

| Layer | File | Purpose |
|-------|------|---------|
| **Common framework** | `DevEnv/Common.cmake` | C++20 standard, output paths, platform detection, utility functions |
| **Library modules** | `DevEnv/natGUI.cmake` | Set library paths for debug/release per platform |
| **Per-project** | `CMakeLists.txt` → `ProjectName.cmake` | `add_executable`, link, apply framework functions |

---

### CMakeLists.txt Boilerplate

Every project uses this exact pattern:

```cmake
cmake_minimum_required(VERSION 3.18)
set(SOLUTION_NAME The_Travelling_Salesman)

project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(HOME_ROOT $ENV{HOME})
if (WIN32)
    string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()
set(WORK_ROOT ${HOME_ROOT}/Work)
include(${WORK_ROOT}/DevEnv/Common.cmake)     # 1. Always first
include(${WORK_ROOT}/DevEnv/natGUI.cmake)     # 2. Library module(s)
include(TTS.cmake)                            # 3. Project-specific
```

**Key variables set by Common.cmake:**
| Variable | Value |
|----------|-------|
| `MY_BIN` | `$HOME/other_bin` (runtime binaries) |
| `MY_LIB` | `$HOME/other_bin/myLib` (libraries) |
| `MY_INC` | `$HOME/Work/Common/Include` (headers) |

---

### Project .cmake File Pattern

```cmake
set(PROJECT_NAME The_Travelling_Salesman)

# Gather source files
file(GLOB PROJECT_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB PROJECT_INCS     ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
set(PROJECT_PLIST           ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)
file(GLOB PROJECT_INC_TD   ${MY_INC}/td/*.h)
file(GLOB PROJECT_INC_GUI  ${MY_INC}/gui/*.h)

# Create executable
add_executable(${PROJECT_NAME}
    ${PROJECT_INCS} ${PROJECT_SOURCES}
    ${PROJECT_INC_TD} ${PROJECT_INC_GUI})

# Include path for project headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include)

# IDE source grouping (Visual Studio filters)
source_group("inc"         FILES ${PROJECT_INCS})
source_group("inc\\td"     FILES ${PROJECT_INC_TD})
source_group("inc\\gui"    FILES ${PROJECT_INC_GUI})
source_group("src"         FILES ${PROJECT_SOURCES})

# Link framework libraries (debug + release)
target_link_libraries(${PROJECT_NAME}
    debug     ${MU_LIB_DEBUG}     debug     ${NATGUI_LIB_DEBUG}
    optimized ${MU_LIB_RELEASE}   optimized ${NATGUI_LIB_RELEASE})

# Apply platform-specific properties
setTargetPropertiesForGUIApp(${PROJECT_NAME} ${PROJECT_PLIST})
setIDEPropertiesForGUIExecutable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${PROJECT_NAME})
```

---

### Key CMake Functions

#### `setTargetPropertiesForGUIApp(target, plistFile)`

| Platform | Effect |
|----------|--------|
| **Windows** | `WIN32_EXECUTABLE TRUE` — creates GUI app (WinMain, no console) |
| **macOS** | `MACOSX_BUNDLE TRUE` + sets `MACOSX_BUNDLE_INFO_PLIST` — creates `.app` bundle |
| **Linux** | No-op |

#### `setIDEPropertiesForGUIExecutable(target, devResPath)`

| Platform | Effect |
|----------|--------|
| **Windows** | Sets `VS_DEBUGGER_COMMAND_ARGUMENTS "-devResPath=<path>"` (Visual Studio) |
| **macOS** | Configures Xcode scheme: `-devResPath`, `DYLD_LIBRARY_PATH`, `-fobjc-arc` |
| **All** | Updates `DevEnv/DevResPaths.txt` with target→path mapping |

The `-devResPath` argument tells the natGUI runtime where to find `res/DevRes.xml` and resolve `:` prefixed resource paths.

#### `setPlatformDLLPath(target)`

| Platform | Effect |
|----------|--------|
| **Windows** | Sets `VS_DEBUGGER_ENVIRONMENT` `PATH` to include `~/other_bin` and GTK bin dirs |
| **Other** | No-op |

#### `setAppIcon(target, projectPath)` *(optional — for custom app icons)*

| Platform | Effect |
|----------|--------|
| **macOS** | Embeds `res/appIcon/macApp.icns` into the `.app` bundle |
| **Windows** | Handled via `.rc` file (see below) |

---

### Windows Icon (.ico / .rc)

Windows executables embed icons via a **resource script** (`.rc` file):

1. Create `res/appIcon/winApp.ico` (the icon file)
2. Create `res/appIcon/winAppIcon.rc`:
   ```
   IDI_ICON1 ICON "winApp.ico"
   ```
3. Create `res/appIcon/winAppIcon.cpp` (empty stub for non-Windows):
   ```cpp
   #include <td/Types.h>
   ```
4. In the project `.cmake`:
   ```cmake
   if(WIN32)
       set(WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
   else()
       set(WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
   endif()
   add_executable(${NAME} ... ${WINAPP_ICON})
   ```

---

### macOS Icon (.icns) & Info.plist

**Info.plist** — macOS bundle configuration. Located at `src/Info.plist`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "...">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>     <string>en</string>
    <key>CFBundleExecutable</key>            <string>$(EXECUTABLE_NAME)</string>
    <key>CFBundleIconFile</key>              <string></string>
    <key>CFBundleIdentifier</key>            <string>$(PRODUCT_BUNDLE_IDENTIFIER)</string>
    <key>CFBundleName</key>                  <string>$(PRODUCT_NAME)</string>
    <key>CFBundlePackageType</key>           <string>APPL</string>
    <key>CFBundleVersion</key>               <string>1</string>
    <key>NSHighResolutionCapable</key>       <true/>
</dict>
</plist>
```

Variables like `$(EXECUTABLE_NAME)` are substituted by CMake/Xcode at build time.

**Icon setup:**
1. Place `macApp.icns` in `res/appIcon/`
2. Set `CFBundleIconFile` to `macApp.icns` in the plist
3. Call `setAppIcon(target, projectPath)` in CMake — this embeds the `.icns` into the bundle's `Resources/` folder

---

### Executable Module & Linking

The project links against two libraries:

| Library | Variable | Contains |
|---------|----------|----------|
| **mainUtils** (mu) | `MU_LIB_DEBUG/RELEASE` | Core utilities, application base, properties, types |
| **natGUI** | `NATGUI_LIB_DEBUG/RELEASE` | GUI framework (views, controls, drawing, resources) |

Library naming per platform:
| Platform | Debug | Release |
|----------|-------|---------|
| Windows | `nameD.lib` | `name.lib` |
| macOS | `nameD.dylib` | `name.dylib` |
| Linux | `nameD.so` | `name.so` |

Additional library modules can be added (e.g., `natGL.cmake` for OpenGL, `natPlot.cmake` for plotting).

---

## 8. Quick Reference Table

| Task | Code |
|------|------|
| Set layout on view | `setLayout(&_gl);` |
| Set central view on window | `setCentralView(&_view, Frame::FixSizes::FixMin);` |
| Set toolbar on window | `setToolBar(_toolBar);` |
| Trigger canvas redraw | `reDraw();` |
| Enable resize events on canvas | `enableResizeEvent(true);` |
| Create canvas with click events | `Canvas({InputDevice::Event::PrimaryClicks})` |
| Get resource file path | `getResFileName(":resID")` |
| Load image from resources | `gui::Image img(":resID");` |
| Draw image | `img.draw(rect, Image::AspectRatio::Keep);` |
| Draw rectangle | `Shape::drawRect(rect, fillColor);` |
| Draw line | `Shape::drawLine(p1, p2, color, lineWidth);` |
| Draw text | `DrawableString("text").draw(point, fontID, color);` |
| Translate string | `tr("stringID")` |
| Get app properties | `getAppProperties()` or `app.getProperties()` |
| Read property | `props->getValue("key", "default")` |
| Write property | `props->setValue("key", value)` |
| Switch language | `props->setValue("translation", "BA"); getApplication()->restart();` |
| Set min size on control | `ctrl.setSizeLimits(w, Limit::UseAsMin, h, Limit::UseAsMin);` |
| Hide/show control | `ctrl.hide(bHide, recalcLayout);` |
| Enable/disable control | `ctrl.enable(true);` / `ctrl.disable();` |
| Open dialog modal | `pDlg->openModal(callback);` |
| Handle toolbar action | `auto [menuID, sub1, sub2, actionID] = aiDesc.getIDs();` |
| ComboBox add + select | `cmb.addItem("text"); cmb.selectIndex(0);` |
