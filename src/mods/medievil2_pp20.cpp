#include "medievil2_pp20.h"
#include <cstring>
#include <stdexcept>

namespace medievil2 {
namespace {
uint32_t reverse(unsigned char b) {
    uint32_t r=0;
    for (unsigned i=0;i<8;++i) { r=(r<<1)|(b&1); b>>=1; }
    return r;
}
struct Reader {
    const unsigned char *source;
    size_t cursor;
    uint32_t remaining=0, bits=0;
    uint32_t read(unsigned n) {
        if(n>31) throw std::runtime_error("PP20 bit width");
        uint32_t value=0;
        while(n) {
            if(n<remaining) {
                value=(value<<n)|(bits>>(32-n));
                bits<<=n; remaining-=n; n=0;
            } else {
                // The MIPS reader refills even when n == remaining.
                value=remaining==32 ? bits :
                    ((value<<remaining)|(remaining ? bits>>(32-remaining) : 0));
                n-=remaining;
                // A final word may include unused header bytes. The guest
                // reads four bytes at a time rather than stopping at byte 8.
                if(cursor<4) throw std::runtime_error("PP20 input underflow");
                bits=0;
                for(unsigned i=0;i<4;++i) bits=(bits<<8)|reverse(source[--cursor]);
                remaining=32;
            }
        }
        return value;
    }
};
}
PP20Result decode_pp20(const unsigned char *source,size_t size) {
    if(size<16 || size>0x200000 || std::memcmp(source,"PP20",4))
        throw std::runtime_error("PP20 header");
    for(unsigned i=4;i<8;++i) if(source[i]<1 || source[i]>24)
        throw std::runtime_error("PP20 offset width");
    size_t length=(size_t(source[size-4])<<16)|(size_t(source[size-3])<<8)|source[size-2];
    if(!length || length>0x200000 || source[size-1]>31)
        throw std::runtime_error("PP20 output size");
    Reader r{source,size-4}; r.read(source[size-1]);
    PP20Result out; out.bytes.resize(length);
    size_t cursor=length;
    auto count=[&](unsigned width,unsigned maximum,size_t initial) {
        unsigned x;
        do {
            x=r.read(width);
            if(initial>cursor || x>cursor-initial) throw std::runtime_error("PP20 run bound");
            initial+=x;
        } while(x==maximum);
        return initial;
    };
    while(cursor) {
        if(!r.read(1)) {
            size_t n=count(2,3,1);
            while(n--) out.bytes[--cursor]=static_cast<unsigned char>(r.read(8));
            if(!cursor) break;
        }
        unsigned kind=r.read(2), width=source[4+kind];
        size_t n=kind+2, offset;
        if(kind==3) {
            if(!r.read(1)) width=7;
            offset=r.read(width); n=count(3,7,n);
        } else offset=r.read(width);
        if(n>cursor || offset>=length-cursor) throw std::runtime_error("PP20 reference bound");
        while(n--) {
            unsigned char value=out.bytes[cursor+offset];
            out.bytes[--cursor]=value;
        }
    }
    out.cursor=uint32_t(r.cursor);out.remaining=r.remaining;out.bits=r.bits;
    return out;
}
}
