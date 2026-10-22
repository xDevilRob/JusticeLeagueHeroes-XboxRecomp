#include "controls.h"
#include "app/app.h"
#include "core/log.h"
#include "gpu/texture.h"
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace {
struct Sample {uint32_t x,y,color;};
struct Patch {uint32_t x,y,w,h;std::vector<uint8_t> pixels;};
struct Group {uint32_t w,h;std::vector<Sample> samples;std::vector<Patch> variants[3];};
std::vector<Group> Load() {
    auto dir=app::ExeDir();std::ifstream file;
    for(int i=0;i<6;++i,dir=dir.parent_path()) {
        file.open(dir/"ui"/"control-layouts.bin",std::ios::binary);
        if(file)break;file.clear();
    }
    if(!file){LOGE("Controls: control-layouts.bin missing");return {};}
    auto read=[&](void* p,size_t n){file.read(static_cast<char*>(p),n);};
    auto u32=[&](){uint32_t v=0;read(&v,4);return v;};
    char magic[4];read(magic,4);if(std::memcmp(magic,"JLC2",4))return {};
    const auto count=u32();if(count>16)return {};
    std::vector<Group> groups(count);
    for(auto& g:groups) {
        g.w=u32();g.h=u32();const auto n=u32();if(n>4096)return {};
        for(unsigned i=0;i<n;++i)g.samples.push_back({u32(),u32(),u32()});
        for(auto& variant:g.variants) {
            const auto patches=u32();if(patches>32)return {};
            for(unsigned i=0;i<patches;++i) {
                Patch p{u32(),u32(),u32(),u32(),{}};
                if(p.w>256 || p.h>512)return {};
                p.pixels.resize(size_t(p.w)*p.h*4);read(p.pixels.data(),p.pixels.size());variant.push_back(std::move(p));
            }
        }
    }
    if(!file)return {};
    LOGR("Controls: loaded %zu UI texture fingerprints",groups.size());return groups;
}
bool Eligible(const gpu::TextureDesc& d) {
    return d.format==0x0b && d.depth==1 && !d.cube && ((d.width==32 && d.height==32) || (d.width==128 && d.height==64) || (d.width==256 && d.height>=128 && d.height<=512));
}
}
uint64_t JlhControlTextureEpoch(const gpu::TextureDesc& d) {
    return Eligible(d)?uint64_t(JlhActiveLayout())*0x9e3779b97f4a7c15ull:0;
}
void JlhControlTexture(const gpu::TextureDesc& d,const gpu::FormatInfo& fi,uint32_t face,uint32_t level,std::vector<uint8_t>& pixels,uint32_t pitch) {
    const auto layout=JlhActiveLayout();
    if(layout<3 || layout>5 || face || level || fi.host!=gpu::Host::BGRA8 || !Eligible(d))return;
    static const auto groups=Load();
    for(size_t index=0;index<groups.size();++index) {
        const auto& g=groups[index];if(d.width!=g.w || (g.h && d.height!=g.h))continue;
        bool same=true;unsigned differences=0;
        for(auto s:g.samples) {
            const size_t offset=size_t(s.y)*pitch+s.x*4;uint32_t actual=0;
            if(offset+4>pixels.size()){same=false;break;}
            std::memcpy(&actual,pixels.data()+offset,4);
            if(g.w==32) {
                auto white=[](uint32_t c){const unsigned b=c&255,g=(c>>8)&255,r=(c>>16)&255,a=c>>24;return std::min({b,g,r})>=170 && std::max({b,g,r})-std::min({b,g,r})<=40 && a>=220;};
                // Palette color correction preserves alpha and the white letter silhouettes.
                if((actual>>24)!=(s.color>>24) || (white(actual)!=white(s.color) && ++differences>2)){same=false;break;}
            } else if(actual!=s.color){same=false;break;}
        }
        if(!same)continue;
        for(const auto& patch:g.variants[layout-3]) {
            if(patch.x+patch.w>d.width || patch.y+patch.h>d.height)continue;
            for(unsigned y=0;y<patch.h;++y)std::memcpy(pixels.data()+size_t(patch.y+y)*pitch+patch.x*4,patch.pixels.data()+size_t(y)*patch.w*4,patch.w*4);
        }
        if(std::getenv("JLH_TRACE_CONTROLS"))LOGR("Controls texture: %X matched %zu layout %u",d.data,index,layout);
        return;
    }
}
