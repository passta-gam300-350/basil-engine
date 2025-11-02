#ifndef RP_RSC_EXT_DESCRIPTOR_HPP
#define RP_RSC_EXT_DESCRIPTOR_HPP

#include <rsc-core/guid.hpp>

namespace rp {
	//composite this. do not inherit it
	struct descriptor_base {
		Guid m_guid;
		std::string m_importer;
		std::string m_name;
		std::uint64_t m_importer_type;
	};
}

#endif