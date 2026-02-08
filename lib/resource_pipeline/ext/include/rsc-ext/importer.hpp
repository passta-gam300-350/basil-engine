/******************************************************************************/
/*!
\file   importer.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Resource importer interface

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RP_RSC_EXT_IMPORTER_HPP
#define RP_RSC_EXT_IMPORTER_HPP

#include <functional>
#include <rsc-core/guid.hpp>

namespace rp {
	template <std::uint64_t type_index>
	struct ResourceTypeImporter;
}

#endif