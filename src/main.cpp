// Main entry point for Obiđi Jugu application
#include "Application.h"
#include <td/StringConverter.h>
#include <fstream>
#include <gui/WinMain.h>
#include <windows.h>


// Note: Update ResPaths.txt if needed

int main(int argc, const char * argv[])
{
    Application app(argc, argv);
    // Read preferred language from config (defaults to EN). If the file is missing, create it.
    const char* cfgFile = "lang.cfg";
    std::string lang = "EN";
    bool haveCfg = false;
    {
        std::ifstream fin(cfgFile);
        if (fin)
        {
            std::string tmp;
            if (std::getline(fin, tmp))
            {
                if (tmp == "EN" || tmp == "BA") {
                    lang = tmp;
                    haveCfg = true;
                }
            }
        }
        if (!haveCfg)
        {
            std::ofstream fout(cfgFile, std::ios::trunc);
            if (fout) { fout << lang; fout.flush(); fout.close(); }
        }
    }
    app.init(lang.c_str());

    return app.run();
}
