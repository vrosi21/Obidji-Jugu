#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TextEdit.h>
#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/LineEdit.h>
#include <gui/NumericEdit.h>


// Minimal side panel view for the Travelling Salesman project.
class SidePanelView : public gui::View
{
public:
    gui::Label lblXCoord;
    gui::NumericEdit txtEditXCoord;
    gui::Label lblYCoord;
    gui::NumericEdit txtEditYCoord;
    gui::Button btnAddPt;
    gui::Label lblName;
    gui::LineEdit lnEditName;
    gui::GridLayout gl;

    SidePanelView();

    ~SidePanelView() = default;
};
