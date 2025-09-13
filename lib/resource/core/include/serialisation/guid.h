#ifndef LIB_RESOURCE_SERIALISATION_GUID_H
#define LIB_RESOURCE_SERIALISATION_GUID_H

#include <chrono>
#include <random>
#include <string>
#include <format>
#include <cstdint>

namespace Resource {
	struct Guid {
		std::uint64_t m_high; //time
		std::uint64_t m_low; //rand

        static Guid generate() {
            static std::mt19937_64 get_rng64{ [] {
                std::random_device rd;
                std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
                return std::mt19937_64(seed);
                }() };
            auto get_time64{ []() {
                auto now = std::chrono::steady_clock::now().time_since_epoch();
                return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
                } };
            return Guid{get_time64(), get_rng64()};
        }
        
        std::string to_hex() const {
            return std::format("0x{:x}{:x}", m_high, m_low);
        }

        std::wstring to_hex_wstr() const {
            return std::format(L"0x{:x}{:x}", m_high, m_low);
        }

        std::string to_hex_no_delimiter() const {
            return std::format("{:x}{:x}", m_high, m_low);
        }

        std::wstring to_hex_no_delimiter_wstr() const {
            return std::format(L"{:x}{:x}", m_high, m_low);
        }

        bool operator!=(const Guid& other) const noexcept {
            return (m_high ^ other.m_high) | (m_low ^ other.m_low);
        }
        bool operator==(const Guid& other) const noexcept {
            return !(*this==other);
        }
        operator bool() const {
            return (m_high ^ ~0x0Ull) | (m_low ^ ~0x0ull);
        }
        bool operator<(const Guid& other) const
        {
            return std::tie(m_high, m_low) < std::tie(other.m_high, other.m_low);
        }
    };

    static constexpr Guid null_guid{ ~0x0ull, ~0x0ull };
}

#endif