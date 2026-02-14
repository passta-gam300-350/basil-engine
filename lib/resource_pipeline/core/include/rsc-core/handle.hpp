/******************************************************************************/
/*!
\file   handle.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Resource handle definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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