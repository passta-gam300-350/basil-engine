#ifndef LIB_RES_UTILITY_H
#define LIB_RES_UTILITY_H

#include <string_view>
#include <array>
#include <string>
#include <vector>
#include <cstddef>
#include <Windows.h>

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

struct Blob {
    std::vector<std::byte> m_raw;
    std::byte* Raw() {
        return m_raw.data();
    }
    std::byte* End() {
        return m_raw.data() + m_raw.size();
    }
    std::size_t Size() {
        return m_raw.size();
    }
    void AllocateExact(std::size_t new_sz) {
        m_raw.resize(new_sz);
    }
};

std::wstring string_to_wstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
    return wstr;
}

std::string wstring_to_string(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], size_needed, nullptr, nullptr);
    return utf8;
}

#endif