#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "../src/mods/medievil2_seamless_store.h"
namespace {
std::filesystem::path input,cache;
std::string plan="stock-test-plan";
bool changed=false;
unsigned reads=0;
}
namespace PSXRecompV4 {
const std::string &mod_runtime_fingerprint() { return plan; }
}
extern "C" int psx_mod_read_disc_file(const char *path,void *buffer,uint32_t capacity,uint32_t *size) {
    ++reads;std::ifstream in(input/path,std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),{});
    if(bytes.empty() || capacity<bytes.size()) return 0;
    *size=uint32_t(bytes.size());memcpy(buffer,bytes.data(),bytes.size());
    if(changed && !strcmp(path,"PROJFILE.MWD")) static_cast<unsigned char*>(buffer)[2050]^=1;
    return 1;
}
int main(int argc,char **argv) {
    assert(argc==3);input=argv[1];cache=argv[2];
    assert(medievil2_seamless_prepare() && reads==25);
    auto source=medievil2_seamless_source(53382,34441216);
    assert(source && !medievil2_seamless_source(53381,2048));
    assert(!medievil2_seamless_source(53382,34441217));
    assert(!medievil2_seamless_source(0xFFFFFFFF,2048));
    assert(!medievil2_seamless_source(53382,0));
    // All compressed parents must be available, including exact decoder state.
    std::ifstream engine(input/"MED2.EXE",std::ios::binary);
    std::vector<unsigned char> main((std::istreambuf_iterator<char>(engine)),{});
    unsigned count=0;
    auto word=[&](size_t p) { uint32_t n;memcpy(&n,main.data()+p,4);return n; };
    for(unsigned i=0;i<0x51A;i++) {
        size_t p=0x800+0xBF6A4-0x10000+36*i;
        if(!(word(p+4)&16)) continue;
        uint32_t n,prefix,cursor,remaining,bits,meta=word(p+28),size=word(p+24);
        auto block=medievil2_seamless_decoded(source+2048*word(p+12),size,&n,&prefix,&cursor,&remaining,&bits);
        assert(block && n==(meta&0xFFFFFF) && prefix==4*(meta>>24) && cursor<size && remaining<=32);
        ++count;
    }
    assert(count==66);
    assert(medievil2_seamless_prepare() && reads==50); // warm still verifies effective disc
    // Truncation, payload corruption, extra tail, and another mod plan rebuild.
    auto pack=std::filesystem::directory_iterator(cache)->path();
    std::filesystem::resize_file(pack,9);assert(medievil2_seamless_prepare());
    { std::fstream file(pack,std::ios::in|std::ios::out|std::ios::binary);
      file.seekg(20);int value=file.get();file.seekp(20);file.put(char(value^0x80)); }
    assert(medievil2_seamless_prepare());
    { std::ofstream file(pack,std::ios::app|std::ios::binary);file.put('x'); }
    assert(medievil2_seamless_prepare());
    plan="changed-test-plan";assert(medievil2_seamless_prepare());
    assert(std::distance(std::filesystem::directory_iterator(cache),std::filesystem::directory_iterator{})==2);
    // Sector-patched effective assets must never receive stock cached bytes.
    changed=true;assert(!medievil2_seamless_prepare());
    assert(!medievil2_seamless_source(53382,2048));
    changed=false;assert(medievil2_seamless_prepare());
    return 0;
}
