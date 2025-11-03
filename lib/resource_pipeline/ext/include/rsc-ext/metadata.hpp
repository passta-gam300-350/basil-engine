#ifndef RP_RSC_EXT_METADATA_HPP
#define RP_RSC_EXT_METADATA_HPP

#include <rsc-core/guid.hpp>

namespace rp {
	//meta data does not care about type it simply indexes
	struct MetaData {
		Guid m_file_guid; //generated at indexed
		std::string m_file_relative_path;
		std::uint64_t m_file_hash;
	};
}

#endif