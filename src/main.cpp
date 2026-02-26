// Main entry point for Obiđi Jugu application
#include "Application.h"
#include <td/StringConverter.h>
#include <gui/WinMain.h>


int main(int argc, const char * argv[])
{
    Application app(argc, argv);
    // Read preferred language from OS-backed properties (registry / plist / settings scheme).
    // Defaults to EN if no preference has been saved yet.
    auto appProperties = app.getProperties();
    td::String trLang = appProperties->getValue("translation", "EN");
    app.init(trLang);

    return app.run();
}
