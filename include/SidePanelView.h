#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TextEdit.h>
#include <gui/Button.h>
#include <gui/VerticalLayout.h>
#include <gui/HorizontalLayout.h>
#include <gui/LineEdit.h>
#include <gui/NumericEdit.h>


// Minimal side panel view for the Travelling Salesman project.
class SidePanelView : public gui::View
{
public:
    gui::HorizontalLayout hlCoordsX;
    gui::Label lblXCoord;
    gui::NumericEdit txtEditXCoord;
    
    gui::HorizontalLayout hlCoordsY;
    gui::Label lblYCoord;
    gui::NumericEdit txtEditYCoord;

    gui::HorizontalLayout hlAddBtn;
    gui::Button btnAddPt;

    gui::HorizontalLayout hlName;
    gui::Label lblName;
    gui::LineEdit lnEditName;

    gui::VerticalLayout vl;

    SidePanelView();

    ~SidePanelView() = default;
};
