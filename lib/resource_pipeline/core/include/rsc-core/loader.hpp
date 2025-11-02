#ifndef RP_RSC_CORE_LOADER_HPP
#define RP_RSC_CORE_LOADER_HPP

#include <functional>
#include "rsc-core/guid.hpp"

namespace rp {
	template <std::uint64_t type_index>
	struct ResourceTypeLoader;

	template <typename Type>
	struct ResourceTypeLoaderData{
		using LoaderFn = std::function<Type(const std::byte*)>;
		using UnloaderFn = std::function<void(Type&&)>;

		ResourceTypeLoaderData(LoaderFn loader, UnloaderFn unloader) { 
			m_loader = loader; 
			m_unloader = unloader;
		}

		inline static LoaderFn m_loader{};
		inline static UnloaderFn m_unloader{};
	};
}

#endif