#include "app/app.h"
#include "controls.h"
void JlhCaptureFrames(void*);
void JlhDiagnosticPad(int, input::Pad&);
extern uint32_t g_jlh_language;
void JlhLanguageLoaded();
std::filesystem::path JlhUiFile(const std::vector<std::string>&);

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Justice League Heroes",               // window and splash
    .short_title = L"JusticeLeagueHeroes",               // message boxes
    .name = L"JusticeLeagueHeroes",                      // window classes, crash dumps, handover event
    .log = "justice_league_heroes",                        // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "JusticeLeagueHeroes"}},  // each recompiled xbe and its exe, for title launches
    .options = {{"language",nullptr,&g_jlh_language},{"button_layout",nullptr,&g_jlh_button_layout}},
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = nullptr,                      // after the settings load, before the window and boot
    .loaded = JlhLanguageLoaded,
    .window = JlhControlsWindow,
    .message = JlhControlsMessage,
    .ui = nullptr,
    .pad = JlhControlsPad,
    .rumble = nullptr,                    // changes a rumble before it is set
    .file_override = JlhUiFile,
};



