#include "../src/mods/medievil2_pp20.h"
#include <fstream>
#include <iterator>
#include <cstdio>
int main(int argc,char **argv) {
    if(argc!=3) return 2;
    try {
        std::ifstream in(argv[1],std::ios::binary);
        std::vector<unsigned char> source((std::istreambuf_iterator<char>(in)),{});
        auto decoded=medievil2::decode_pp20(source.data(),source.size());
        std::ofstream out(argv[2],std::ios::binary);
        out.write(reinterpret_cast<const char*>(&decoded.cursor),4);
        out.write(reinterpret_cast<const char*>(&decoded.remaining),4);
        out.write(reinterpret_cast<const char*>(&decoded.bits),4);
        out.write(reinterpret_cast<const char*>(decoded.bytes.data()),decoded.bytes.size());
        return out ? 0 : 3;
    } catch(const std::exception &e) { std::fprintf(stderr,"%s\n",e.what());return 1; }
}
