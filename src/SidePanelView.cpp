#include "SidePanelView.h"
#include <gui/GridLayout.h>


SidePanelView::SidePanelView() 
    : lblXCoord("X:"), 
    txtEditXCoord(td::DataType::decimal4), 
    lblYCoord("Y:"), 
    txtEditYCoord(td::DataType::decimal4), 
    btnAddPt(tr("Add point")), 
    lnEditName(), 
    lblName(tr("Name:")),
    gl(4, 2)
{
        txtEditXCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);
        txtEditYCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);
        lblName.setSizeLimitForNChars(1, gui::Control::Limit::UseAsMin);

        btnAddPt.setType(gui::Button::Type::Default);
        btnAddPt.setSizeLimitForNChars(7, gui::Control::Limit::None);

        // Grid layout: 2 columns
        // Row 0: Name label + name input
        gl.insert(0, 0, lblName, td::HAlignment::Left);
        gl.insert(0, 1, lnEditName, td::HAlignment::Left);
        // Row 1: X and Y labels
        gl.insert(1, 0, lblXCoord, td::HAlignment::Left);
        gl.insert(1, 1, lblYCoord, td::HAlignment::Left);
        // Row 2: X and Y inputs
        gl.insert(2, 0, txtEditXCoord, td::HAlignment::Left);
        gl.insert(2, 1, txtEditYCoord, td::HAlignment::Left);
        // Row 3: Button spanning all columns (span to end)
        gl.insert(3, 0, btnAddPt, -1);

        setLayout(&gl);
}

