/******************************************************************************/
/*!
\file   loader.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Resource loader interface

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RP_RSC_CORE_LOADER_HPP
#define RP_RSC_CORE_LOADER_HPP

#include <functional>
#include "rsc-core/guid.hpp"

namespace rp {
	template <std::uint64_t type_index>
	struct ResourceTypeLoader;
}

#endif