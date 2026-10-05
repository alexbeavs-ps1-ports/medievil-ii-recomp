#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace medievil2 {
struct PP20Result {
    std::vector<unsigned char> bytes;
    // Exact final state of the original decoder's GP+86C/870/874 bit reader.
    uint32_t cursor, remaining, bits;
};
PP20Result decode_pp20(const unsigned char *source, size_t size);
}
