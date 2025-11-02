#pragma once
#include <gui/ToolBar.h>
#include <gui/Image.h>

// Simple toolbar with two language buttons (EN/BA) using images defined in res/main.xml
class ToolBar : public gui::ToolBar
{
protected:
    gui::Image _imgEN;
    gui::Image _imgBA;
public:
    ToolBar()
    : gui::ToolBar("tsToolBar", 1)
    , _imgEN(":flagEN")
    , _imgBA(":flagBA")
    {
        // Use menuID=255 like in examples; actionIDs 10 (EN), 20 (BA)
        addItem(tr("English"), &_imgEN, tr("English"), 255, 0, 0, 10);
        addItem(tr("Bosanski"), &_imgBA, tr("Bosanski"), 255, 0, 0, 20);
    }
};
