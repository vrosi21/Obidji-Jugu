#include "SidePanelView.h"


SidePanelView::SidePanelView() 
    :hlCoordsX(2), 
    lblXCoord("X:"), 
    txtEditXCoord(td::DataType::decimal4), 
    hlCoordsY(2), 
    lblYCoord("Y:"), 
    txtEditYCoord(td::DataType::decimal4), 
    hlAddBtn(1), 
    btnAddPt(tr("Add point")), 
    vl(8), 
    hlName(2), 
    lnEditName(), 
    lblName(tr("Name:"))
{
    // Intentionally empty for now; will host controls later.
        txtEditXCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);
        txtEditYCoord.setSizeLimits(0, gui::Control::Limit::None, 20, gui::Control::Limit::None);

        lblName.setSizeLimitForNChars(1, gui::Control::Limit::UseAsMin);

        hlCoordsX.append(txtEditXCoord);
        hlCoordsX.setSizeLimits(0, gui::Control::Limit::Fixed, 40, gui::Control::Limit::Fixed);

        hlCoordsY.append(txtEditYCoord);

        btnAddPt.setType(gui::Button::Type::Default);
        btnAddPt.setSizeLimitForNChars(7, gui::Control::Limit::None);
        hlAddBtn.append(btnAddPt);

        hlName.append(lnEditName);

        vl.append(lblXCoord, td::HAlignment::Left);
        vl.append(hlCoordsX);
        vl.append(lblYCoord, td::HAlignment::Left);
        vl.append(hlCoordsY);
        vl.append(lblName);
        vl.append(hlName);
        vl.append(hlAddBtn);
        vl.appendSpacer(10);
        setLayout(&vl);
}

