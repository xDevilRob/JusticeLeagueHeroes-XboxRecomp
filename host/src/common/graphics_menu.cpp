#include "app/app.h"
#include "controls.h"
#include "core/log.h"
#include "xdk/hle.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <string_view>

uint32_t g_jlh_language=0;

namespace {
struct Language { const char* name; uint32_t game_id; };
constexpr Language languages[]={{"English",9},{"Spanish",10},{"German",7},{"French",12},{"Italian",16},{"Portugu\xEAs (BR)",9}};
uint32_t language_choice=0;
uint32_t layout_choice=0;
// Keep persisted IDs stable; ID 2 was the removed Xbox 360 style.
constexpr const char* layouts[]={"Auto","Xbox Original","Xbox Modern","Xbox Modern","PlayStation","Keyboard & Mouse"};
constexpr uint32_t layout_ids[]={0,1,3,4,5};
struct Resolution { uint32_t w, h; const char* text; };
constexpr Resolution resolutions[] = {
    {1280,720,"1280x720"}, {1920,1080,"1920x1080"},
    {2560,1440,"2560x1440"}, {3840,2160,"3840x2160"}
};
struct Video { uint32_t w, h, percent; app::DisplayMode mode; };
bool active=false, pending=false;
Video previous{};
int last_seconds=-1;
std::chrono::steady_clock::time_point deadline;

Video Current() {
    auto& s=app::Get();
    return {s.width,s.height,s.render_percent,s.display_mode};
}
void Set(Video v) {
    auto& s=app::Get();
    s.width=v.w; s.height=v.h; s.render_percent=v.percent; s.display_mode=v.mode;
    app::ApplyDisplay();
    LOGR("Native video: %ux%u, render %u%%, mode %u",v.w,v.h,v.percent,unsigned(v.mode));
}
int ResolutionIndex() {
    auto output=app::Output();
    for(int i=0;i<4;++i)
        if(resolutions[i].w==output.width && resolutions[i].h==output.height)return i;
    return 0;
}

// All movie access happens through the game's APIs on its own execution thread.
// Strings live on a reserved guest stack area, like the original menu's setters.
struct Movie {
    X86Context& ctx;
    X86Context saved;
    uint32_t object, scratch, used=0;
    Movie(X86Context& c,uint32_t movie):ctx(c),saved(c),object(movie) {
        ctx.esp-=2048; scratch=ctx.esp;
    }
    ~Movie(){ctx=saved;}
    uint32_t String(const std::string& text) {
        const uint32_t address=scratch+used;
        std::memcpy(kernel::g_base+address,text.c_str(),text.size()+1);
        used+=uint32_t(text.size()+1);
        return address;
    }
    void Variable(const char* name,const char* value) {
        const uint32_t key=String(name),data=String(value);
        ctx.ecx=object;
        kernel::CallGuest(ctx,0x14AF8A,{key,data,0});
    }
};

void Publish(X86Context& ctx,uint32_t movie) {
    Movie ui(ctx,movie);
    const bool ptbr=g_jlh_language==5;
    ui.Variable("pcResolutionCaption",ptbr?"Resolu\xE7\xE3o":"Resolution");
    const auto size=app::Output();
    const std::string resolution=std::to_string(size.width)+"x"+std::to_string(size.height);
    ui.Variable("pcResolution",resolution.c_str());
    if(pending) {
        int seconds=std::max(1,int(std::chrono::duration_cast<std::chrono::seconds>(
            deadline-std::chrono::steady_clock::now()).count())+1);
        const std::string hint=std::string(ptbr?"A:Manter ":"A:Keep ")+std::to_string(seconds)+"s";
        ui.Variable("pcModeCaption",ptbr?"Manter ajustes?":"Keep changes?");
        ui.Variable("pcModeValue",hint.c_str());
        last_seconds=seconds;
    } else {
        ui.Variable("pcModeCaption",ptbr?"Modo de tela":"Display Mode");
        ui.Variable("pcModeValue",app::Get().display_mode==app::DisplayMode::Windowed?(ptbr?"Janela":"Windowed"):(ptbr?"Tela cheia":"Fullscreen"));
        last_seconds=-1;
    }
    ui.Variable("pcLanguageCaption",language_choice==g_jlh_language?(ptbr?"Idioma":"Language"):(ptbr?"Idioma (A:Reiniciar)":"Language (A:Restart)"));
    ui.Variable("pcLanguage",languages[language_choice].name);
    const auto buttons=JlhActiveLayout();
    ui.Variable("pcLayoutCaption",layout_choice==g_jlh_button_layout?"Button Layout":buttons==5?"Button Layout (Enter:Apply)":buttons==4?"Button Layout (Cross:Apply)":"Button Layout (A:Apply)");
    ui.Variable("pcLayout",layouts[layout_choice]);
    ui.Variable("pcPending",pending || language_choice!=g_jlh_language || layout_choice!=g_jlh_button_layout?"1":"0");
    ui.Variable("pcVideoActive",active?"1":"0");
}
void Revert() {
    if(!pending)return;
    Set(previous);pending=false;
    LOGR("Native video: changes reverted");
}
void BeginChange() {
    if(!pending)previous=Current();
    pending=true;deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
}
}

void JlhLanguageLoaded() {
    JlhControlsLoaded();layout_choice=g_jlh_button_layout;
    if(g_jlh_language>=std::size(languages))g_jlh_language=0;
    language_choice=g_jlh_language;
    // Keep save/media resource selection compatible with this English disc.
    // Text localization is isolated in language_fallback.cpp.
    kernel::W32(0x1DD4CC,9);
    LOGR("Language: text %s (id %u), media/save language English",languages[g_jlh_language].name,languages[g_jlh_language].game_id);
}

std::filesystem::path JlhUiFile(const std::vector<std::string>& parts) {
    if(parts.size()!=2)return {};
    auto lower=[](std::string s){for(char& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;};
    if(lower(parts[0])!="res")return {};
    const auto name=lower(parts[1]);
    std::filesystem::path relative;
    if(name=="frontend.lmp" || name=="pause.lmp")relative=name;
    else if(name=="font.lmp")relative=name;
    else if(g_jlh_language==5 && (name=="gametext.lmp" || name=="chartext.lmp" || name=="font.lmp" || name=="stats.lmp"))relative=std::filesystem::path("pt-BR")/name;
    else return {};
    auto dir=app::ExeDir();std::error_code ec;
    for(int i=0;i<6;++i,dir=dir.parent_path()) {
        auto file=dir/"ui"/relative;
        if(std::filesystem::is_regular_file(file,ec))return file;
        if(dir==dir.parent_path())break;
    }
    return {};
}

X86_EXTERN_FUNC(__imp__sub_0014F231);
X86_FUNC(sub_0014F231) {
    const uint32_t p=kernel::R32(ctx.esp+4);
    std::string_view cmd=p?reinterpret_cast<const char*>(base+p):"";
    if(cmd.starts_with("CMD_"))cmd.remove_prefix(4);
    if(!cmd.starts_with("PC_")) {__imp__sub_0014F231(ctx,base);return;}
    const uint32_t movie=ctx.ecx;
    if(cmd=="PC_EXIT_INIT") {
        constexpr const char* captions[]={"Exit to Desktop","Salir al escritorio","Zur\xFC""ck zum Desktop","Retour au bureau","Esci al desktop","Sair para o desktop"};
        Movie ui(ctx,movie);ui.Variable("pcExitCaption",captions[g_jlh_language<6?g_jlh_language:0]);
        LOGR("Native exit: label ready, language %u",g_jlh_language);
    } else if(cmd=="PC_EXIT_DESKTOP") {
        LOGR("Native exit: desktop requested, language %u",g_jlh_language);
        app::Quit();
    } else if(cmd=="PC_VIDEO_INIT" || cmd=="PC_VIDEO_ACTIVE") {
        if(cmd=="PC_VIDEO_INIT"){Revert();language_choice=g_jlh_language;layout_choice=g_jlh_button_layout;g_jlh_layout_preview.store(layout_choice);}
        active=true;Publish(ctx,movie);
        LOGR("Native video: Options ready");
    } else if(cmd=="PC_VIDEO_CLOSE") {
        Revert();active=false;
        language_choice=g_jlh_language;
        layout_choice=g_jlh_button_layout;g_jlh_layout_preview.store(layout_choice);
        Movie ui(ctx,movie);ui.Variable("pcVideoActive","0");
    } else if(cmd=="PC_VIDEO_TICK") {
        if(active && pending) {
            LOG_FIRST(2,"Native video: confirmation timer tick");
            const auto now=std::chrono::steady_clock::now();
            if(now>=deadline){Revert();Publish(ctx,movie);}
            else {
                const int seconds=int(std::chrono::duration_cast<std::chrono::seconds>(deadline-now).count())+1;
                if(seconds!=last_seconds)Publish(ctx,movie);
            }
        }
    } else if(active) {
        if(cmd=="PC_RES_LEFT" || cmd=="PC_RES_RIGHT") {
            const int i=(ResolutionIndex()+(cmd=="PC_RES_LEFT"?3:1))%4;
            BeginChange();auto value=Current();
            value.w=resolutions[i].w;value.h=resolutions[i].h;value.percent=100;
            Set(value);Publish(ctx,movie);
        } else if(cmd=="PC_MODE") {
            // While pending, this row is the native Keep Changes prompt.
            if(!pending) {
                BeginChange();auto value=Current();
                value.mode=value.mode==app::DisplayMode::Windowed?app::DisplayMode::Borderless:app::DisplayMode::Windowed;
                Set(value);Publish(ctx,movie);
            }
        } else if(cmd=="PC_VIDEO_KEEP") {
            if(pending){app::Save();pending=false;Publish(ctx,movie);LOGR("Native video: changes saved");}
        } else if(cmd=="PC_VIDEO_REVERT") {
            Revert();language_choice=g_jlh_language;layout_choice=g_jlh_button_layout;g_jlh_layout_preview.store(layout_choice);Publish(ctx,movie);
        } else if(cmd=="PC_LAYOUT_LEFT" || cmd=="PC_LAYOUT_RIGHT") {
            if(!pending) {
                const auto current=std::find(std::begin(layout_ids),std::end(layout_ids),layout_choice);
                const auto index=current==std::end(layout_ids)?0:current-std::begin(layout_ids);
                layout_choice=layout_ids[(index+(cmd=="PC_LAYOUT_LEFT"?4:1))%5];
                g_jlh_layout_preview.store(layout_choice);Publish(ctx,movie);
            }
        } else if(cmd=="PC_LAYOUT_KEEP") {
            if(!pending) {
                g_jlh_button_layout=layout_choice;app::Save();Publish(ctx,movie);
                LOGR("Controls: saved layout %s",layouts[layout_choice]);
            }
        } else if(cmd=="PC_LANG_LEFT" || cmd=="PC_LANG_RIGHT") {
            if(!pending) {
                language_choice=(language_choice+(cmd=="PC_LANG_LEFT"?std::size(languages)-1:1))%std::size(languages);
                Publish(ctx,movie);
            }
        } else if(cmd=="PC_LANG_KEEP") {
            if(!pending && language_choice!=g_jlh_language) {
                g_jlh_language=language_choice;
                app::Save();
                LOGR("Language: confirmed %s; restarting current profile",languages[g_jlh_language].name);
                kernel::Relaunch("JusticeLeagueHeroes");
            }
        }
    }
    ctx.esp+=8;
}
