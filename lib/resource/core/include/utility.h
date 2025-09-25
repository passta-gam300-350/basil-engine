#ifndef LIB_RES_UTILITY_H
#define LIB_RES_UTILITY_H

#include <string_view>
#include <array>

// 8bytes file magic (signature) to binary
consteval std::uint64_t iso8859ToBinary(std::string_view input) {
    constexpr std::uint64_t maxSize = 8;
    std::uint64_t size = 0;
    /*constexpr std::uint64_t strSize = input.size();
    static_assert(strSize <= maxSize);*/
    std::uint64_t out{};
    
    for (char ch : input) {
        if (maxSize == size)
            break;
        out <<= 8;
        out |= ch;
        size++;
    }

    return out;
}


#endif