#ifndef RP_RSC_CORE_HANDLE_HPP
#define RP_RSC_CORE_HANDLE_HPP

#include "rsc-core/guid.hpp"

namespace rp {
    struct BasicHandle {
        std::uint32_t m_index{};
        std::uint32_t m_generation{};
        explicit operator bool() const noexcept {
            return m_generation != 0;
        }
    };
    /*template <typename Type>
    struct TypedHandle : BasicHandle {
        using type = Type;
    };*/
}

#endif