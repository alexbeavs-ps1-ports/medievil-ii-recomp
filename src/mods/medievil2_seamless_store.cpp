#include "medievil2_seamless_store.h"
#include "medievil2_pp20.h"
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "psx_sha256.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {
using Bytes=std::vector<unsigned char>;
struct FileEntry { const char *path; uint32_t lba,size; const char *hash; };
struct CompressedEntry {
    uint32_t index,flags,sector,size,descriptor;
    const char *hash,*decoded_hash;
    uint32_t cursor,remaining,bits;
};
#include "medievil2_seamless_catalog.inc"
std::vector<Bytes> resident,decoded;
std::string digest(const unsigned char *data,size_t size) {
    unsigned char hash[32];char hex[65];
    psx_sha256_compute(data,size,hash);
    for(unsigned i=0;i<32;++i) std::snprintf(hex+2*i,3,"%02x",hash[i]);
    return hex;
}
std::filesystem::path cache_path() {
    const auto &plan=PSXRecompV4::mod_runtime_fingerprint();
    const std::string name="scus94564-pp20-v1-"+
        digest(reinterpret_cast<const unsigned char*>(plan.data()),plan.size())+".pack";
    if(const char *p=std::getenv("MEDIEVIL2_SEAMLESS_CACHE")) if(*p)
        return std::filesystem::path(p)/name;
    std::filesystem::path root;
#ifdef _WIN32
    if(const char *p=std::getenv("LOCALAPPDATA")) root=p;
#else
    if(const char *p=std::getenv("XDG_CACHE_HOME")) root=p;
    else if(const char *p=std::getenv("HOME")) root=std::filesystem::path(p)/".cache";
#endif
    if(root.empty()) throw std::runtime_error("no cache directory");
    return root/"MediEvilIIRecomp"/"seamless"/name;
}
bool load_cache(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);char magic[8];
    if(!in.read(magic,8) || std::memcmp(magic,"M2PP2001",8)) return false;
    std::vector<Bytes> result;
    for(const auto &entry:compressed) {
        Bytes bytes(entry.descriptor&0xffffffu);
        if(!in.read(reinterpret_cast<char*>(bytes.data()),bytes.size()) ||
           digest(bytes.data(),bytes.size())!=entry.decoded_hash) return false;
        result.push_back(std::move(bytes));
    }
    if(in.peek()!=std::char_traits<char>::eof()) return false;
    decoded=std::move(result);return true;
}
void decode_all() {
    decoded.clear();
    const auto &archive=resident.front();
    for(const auto &entry:compressed) {
        size_t offset=size_t(entry.sector)*2048;
        if(offset>archive.size() || entry.size>archive.size()-offset ||
           digest(archive.data()+offset,entry.size)!=entry.hash)
            throw std::runtime_error("compressed resource mismatch");
        auto out=medievil2::decode_pp20(archive.data()+offset,entry.size);
        if(out.bytes.size()!=(entry.descriptor&0xffffffu) ||
           digest(out.bytes.data(),out.bytes.size())!=entry.decoded_hash ||
           out.cursor!=entry.cursor || out.remaining!=entry.remaining || out.bits!=entry.bits)
            throw std::runtime_error("decoded resource mismatch");
        decoded.push_back(std::move(out.bytes));
    }
}
void write_cache(const std::filesystem::path &path) {
    std::filesystem::create_directories(path.parent_path());auto tmp=path;
#ifdef _WIN32
    tmp+="."+std::to_string(GetCurrentProcessId())+".tmp";
#else
    tmp+="."+std::to_string(getpid())+".tmp";
#endif
    std::ofstream out(tmp,std::ios::binary|std::ios::trunc);out.write("M2PP2001",8);
    for(const auto &bytes:decoded)
        out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    out.close();if(!out) throw std::runtime_error("cannot write resource cache");
#ifdef _WIN32
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("cannot publish resource cache");
#else
    std::filesystem::rename(tmp,path);
#endif
}
}
extern "C" int medievil2_seamless_prepare(void) {
    resident.clear();decoded.clear();
    try {
        // Always verify effective mounted bytes, even with a warm cache. Sector
        // mods cannot be hidden by old stock data. A changed file falls back.
        for(const auto &entry:files) {
            Bytes bytes((entry.size+2047u)&~2047u,0);uint32_t size=0;
            if(!psx_mod_read_disc_file(entry.path,bytes.data(),uint32_t(bytes.size()),&size) ||
               size!=entry.size || digest(bytes.data(),bytes.size())!=entry.hash)
                throw std::runtime_error(std::string("disc resource changed: ")+entry.path);
            resident.push_back(std::move(bytes));
        }
        bool warm=false;
        try { warm=load_cache(cache_path()); } catch(const std::exception &) {}
        if(!warm) {
            decode_all();
            try { write_cache(cache_path()); }
            catch(const std::exception &e) {
                std::fprintf(stderr,"medievil2 seamless: memory cache only: %s\n",e.what());
            }
        }
        size_t bytes=0;for(const auto &b:resident) bytes+=b.size();
        for(const auto &b:decoded) bytes+=b.size();
        std::fprintf(stdout,"medievil2 seamless: 25 files, 66 decoded PP20 containers, %zu resident bytes; %s cache\n",
            bytes,warm?"warm":"prepared");std::fflush(stdout);
        return 1;
    } catch(const std::exception &e) {
        resident.clear();decoded.clear();
        std::fprintf(stderr,"medievil2 seamless: original loader: %s\n",e.what());return 0;
    }
}
extern "C" const unsigned char *medievil2_seamless_source(uint32_t lba,uint32_t bytes) {
    if(!bytes || resident.size()!=std::size(files)) return nullptr;
    for(size_t i=0;i<std::size(files);++i) {
        if(lba<files[i].lba) continue;
        uint64_t offset=uint64_t(lba-files[i].lba)*2048;
        if(offset<=resident[i].size() && bytes<=resident[i].size()-offset)
            return resident[i].data()+size_t(offset);
    }
    return nullptr;
}
extern "C" const unsigned char *medievil2_seamless_decoded(const unsigned char *source,uint32_t size,
    uint32_t *length,uint32_t *prefix,uint32_t *cursor,uint32_t *remaining,uint32_t *bits) {
    if(resident.empty() || decoded.size()!=std::size(compressed)) return nullptr;
    for(size_t i=0;i<std::size(compressed);++i) {
        const auto &entry=compressed[i];
        if(size!=entry.size || std::memcmp(source,resident.front().data()+size_t(entry.sector)*2048,size)) continue;
        *length=uint32_t(decoded[i].size());*prefix=4*(entry.descriptor>>24);
        *cursor=entry.cursor;*remaining=entry.remaining;*bits=entry.bits;
        return decoded[i].data();
    }
    return nullptr;
}
