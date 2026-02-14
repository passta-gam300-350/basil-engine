/******************************************************************************/
/*!
\file   descriptor.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Resource descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RP_RSC_EXT_DESCRIPTOR_HPP
#define RP_RSC_EXT_DESCRIPTOR_HPP

#include <rsc-core/guid.hpp>

namespace rp {
	//composite this. do not inherit it
	struct descriptor_base {
		Guid m_guid;
		std::string m_importer;
		std::string m_source;
		std::string m_name;
		std::uint64_t m_importer_type;
	};
}

#endif