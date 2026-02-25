// Main application window for Obiđi Jugu
#pragma once
#include <gui/Window.h>
#include <fstream>
#include <gui/Application.h>
#include "MainView.h"
#include "ToolBar.h"

class MainWindow : public gui::Window
{
protected:
    
    ToolBar _toolBar;
    MainView _mainView;

protected:
    
    bool shouldClose() override
    {
        return true;
    }
    
public:
    MainWindow()
    : gui::Window(gui::Size(1500, 866))
    {
        setTitle("Obiđi Jugu");
            setToolBar(_toolBar);
        setCentralView(&_mainView, Frame::FixSizes::FixMin);
    }
    
    ~MainWindow()
    {
    }
    
        bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
        {
            auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();

            // Handle language toolbar actions (menuID 255 per examples)
            if (menuID == 255)
            {
                if (actionID == 10)
                {
                    // Persist language and restart application
                    std::ofstream fout("lang.cfg", std::ios::trunc);
                    if (fout) { fout << "EN"; fout.flush(); fout.close(); }
                    gui::getApplication()->restart();
                    return true;
                }
                else if (actionID == 20)
                {
                    std::ofstream fout("lang.cfg", std::ios::trunc);
                    if (fout) { fout << "BA"; fout.flush(); fout.close(); }
                    gui::getApplication()->restart();
                    return true;
                }
            }

            return false;
        }
    
};
