#ifndef RP_RSC_CORE_GUID_HPP
#define RP_RSC_CORE_GUID_HPP

#include <chrono>
#include <random>
#include <format>
#include "rsc-core/utility.hpp"

namespace rp {
    //RFC9562 (uuid v7)
    inline namespace v1 {
        namespace Detail {
            constexpr std::uint64_t UUID7_VERSION{ 0x7 };
            constexpr std::uint64_t UUID7_VERSION_UUID_BITFIELD{ UUID7_VERSION << 12 };
            constexpr std::uint64_t UUID_VARIANT{ 0x2 };
            constexpr std::uint64_t UUID_VARIANT_UUID_BITFIELD{ UUID_VARIANT << 62 };

            //impl from boost docs: https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
            template <typename KeyType>
            constexpr void hash_combine(std::size_t& seed, KeyType const& key) {
                seed ^= std::hash<KeyType>{}(key)+0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
        }

        struct Guid {
            union {
                struct impl_hi {
                    std::uint64_t m_UnixTsMs : 48;
                    std::uint64_t m_Ver : 4;
                    std::uint64_t m_Rand12 : 12;
                };
                std::uint64_t m_high; //timestamp (ms) + ver + rand
            };
            union {
                struct impl_lo {
                    std::uint64_t m_Var : 2;;
                    std::uint64_t m_Rand62 : 62;
                };
                std::uint64_t m_low; //rand + var
            };

            static Guid generate() {
                thread_local std::mt19937_64 get_rng64{ [] {
                    std::random_device rd;
                    std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
                    return std::mt19937_64(seed);
                    }() };
                auto get_time64_48hi{ []() {
                    auto now = std::chrono::steady_clock::now().time_since_epoch();//unix epoch
                    return ((static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) & lo_bitmask64_v<48>) << 16);
                    } };
                std::uint64_t high{ get_time64_48hi() | Detail::UUID7_VERSION_UUID_BITFIELD | (get_rng64() & lo_bitmask64_v<12>) };
                std::uint64_t low{ Detail::UUID_VARIANT_UUID_BITFIELD | (get_rng64() & lo_bitmask64_v<62>) };
                return Guid{ high, low };
            }

            static Guid to_guid(std::string const& str) {
                Guid tmp;
                if (str.size() > 2 && std::tolower(str[1]) == 'x') {
                    return to_guid(str.substr(2));
                }

                if (str.size() > 16) {
                    std::uint64_t rchar{ str.size() - 16 };
                    std::from_chars(str.data(), str.data() + rchar, tmp.m_high, 16);
                    std::from_chars(str.data() + rchar, str.data() + str.size(), tmp.m_low, 16);
                }
                else {
                    std::from_chars(str.data(), str.data() + str.size(), tmp.m_low, 16);
                    tmp.m_high = 0;
                }
                return tmp;
            }

            std::string to_hex_prefix() const {
                return std::format("0x{:x}{:x}", m_high, m_low);
            }

            std::wstring to_hex_prefix_wstr() const {
                return std::format(L"0x{:x}{:x}", m_high, m_low);
            }
            /*
            std::string to_formatted_string() const {
                return std::format("{:x}{:x}", m_high, m_low);
            }

            std::string to_formatted_wstring() const {
                return std::format("L{:x}{:x}", m_high, m_low);
            }
            */
            std::string to_hex() const {
                return std::format("{:x}{:x}", m_high, m_low);
            }

            std::wstring to_hex_wstr() const {
                return std::format(L"{:x}{:x}", m_high, m_low);
            }

            bool operator!=(const Guid& other) const noexcept {
                return (m_high ^ other.m_high) | (m_low ^ other.m_low);
            }
            bool operator==(const Guid& other) const noexcept {
                return !(*this != other);
            }
            operator bool() const {
                return (m_high | m_low);
            }
            bool operator<(const Guid& other) const
            {
                return std::tie(m_high, m_low) < std::tie(other.m_high, other.m_low);
            }
        };

        static constexpr Guid null_guid{ 0x0ull, 0x0ull };

        struct BasicIndexedGuid {
            Guid m_guid;
            std::uint64_t m_typeindex;
        };

        template <utility::static_string ss>
        struct TypeNameGuid;

        template <typename Type>
        struct TypedGuid;
    }
}

template <>
class std::hash<rp::Guid> {
public:
    std::size_t operator() (rp::Guid const& key) const noexcept {
        std::size_t hash_val{ std::hash<std::uint64_t>{}(key.m_high) };
        rp::Detail::hash_combine(hash_val, key.m_low);
        return hash_val;
    }
};

#endif