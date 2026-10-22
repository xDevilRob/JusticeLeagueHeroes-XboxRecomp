#include "xdk/hle.h"
#include "core/log.h"

extern uint32_t g_jlh_language;
X86_EXTERN_FUNC(__imp__sub_0007062E);
X86_EXTERN_FUNC(__imp__sub_00044695);
X86_EXTERN_FUNC(__imp__sub_000446E0);
X86_EXTERN_FUNC(__imp__sub_00044D79);

namespace {
constexpr uint32_t ids[]={9,10,7,12,16};
constexpr uint32_t names[]={0x1B85AC,0x1B85A4,0x1B8594,0x1B858C,0x1B859C};
thread_local uint32_t text_column=0;
thread_local bool loading_table=false;
// PT-BR (preference 5) uses translated copies of the English text columns.
uint32_t Choice(){return g_jlh_language<5?g_jlh_language:0;}
struct TextScope {
    uint32_t previous;
    explicit TextScope(uint32_t column):previous(text_column){text_column=column;}
    ~TextScope(){text_column=previous;}
};
}

// The original table loader discards other language columns. Keep the selected
// text column as well as English, while the global game/media language stays 9.
X86_FUNC(sub_00044D79) {
    const bool previous=loading_table;
    loading_table=true;
    __imp__sub_00044D79(ctx,base);
    loading_table=previous;
}
X86_FUNC(sub_000446E0) {
    const uint32_t result=kernel::R32(ctx.esp+8);
    __imp__sub_000446E0(ctx,base);
    if(loading_table && result && (ctx.eax&255) && kernel::R32(result)==ids[Choice()])
        ctx.eax&=0xFFFFFF00u; // Treat this column as ordinary data: retain it.
}

// Only the text reader gets an alternate locale. No shared guest-language
// mutation: music, movie track selection and save routines always see English.
X86_FUNC(sub_00044695) {
    if(text_column) {ctx.eax=text_column;ctx.esp+=8;return;}
    __imp__sub_00044695(ctx,base);
}
// Leave Flash's resource-existence column (0x1E8FF8) in English too.
// The Unicode reader below localizes the displayed string independently.
X86_FUNC(sub_0007062E) {
    if(text_column){__imp__sub_0007062E(ctx,base);return;}
    const uint32_t object=ctx.ecx;
    const uint32_t key=kernel::R32(ctx.esp+4);
    const uint32_t out=kernel::R32(ctx.esp+8);
    const uint32_t capacity=kernel::R32(ctx.esp+12);
    const uint32_t name_space=kernel::R32(ctx.esp+16);
    TextScope locale(names[Choice()]);
    __imp__sub_0007062E(ctx,base);
    if(!out || capacity<4 || Choice()==0)return;
    if(kernel::R16(out)!='N' || kernel::R16(out+2)!='F' || kernel::R16(out+4)!=':')return;
    const auto returned=ctx;
    ctx.ecx=object;
    TextScope english(names[0]);
    kernel::CallGuest(ctx,0x7062E,{key,out,capacity,name_space});
    ctx=returned;
    LOG_FIRST(12,"Language: English text fallback for %s",key?reinterpret_cast<const char*>(base+key):"<null>");
}
