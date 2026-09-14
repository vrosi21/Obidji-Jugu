// Main application window for Obiđi Jugu
#pragma once
#include <gui/Window.h>
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
    : gui::Window(initialApplicationSize())
    {
        setTitle("Obidji Jugu");
            setToolBar(_toolBar);
        setCentralView(&_mainView, Frame::FixSizes::FixMin);
    }
    
    ~MainWindow()
    {
    }
    
        bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
        {
            auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();

            // Handle language toolbar actions (menuID 255 per framework convention)
            if (menuID == 255)
            {
                const char* lang = nullptr;
                if (actionID == 10)
                    lang = "EN";
                else if (actionID == 20)
                    lang = "BA";

                if (lang)
                {
                    auto appProperties = getAppProperties();
                    appProperties->setValue("translation", lang);
                    gui::getApplication()->restart();
                    return true;
                }
            }

            return false;
        }
    
};
