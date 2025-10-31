#ifndef RP_RSC_CORE_POOLS_H
#define RP_RSC_CORE_POOLS_H

#include <optional>
#include <atomic>
#include "rsc-core/loader.hpp"

namespace rp {
	template <typename Type>
	struct TypeStorage {
		alignas(Type) std::byte m_data[sizeof(Type)];
	};

	enum class SlotStatus : std::uint8_t {
		UNLOADED,
		LOADING,
		READY,
		UNLOADING
	};

	template <typename Type>
	struct PoolSlot {
		using type = Type;

		std::atomic<std::uint32_t> m_ref_ct{};
		std::atomic<std::uint32_t> m_generation{};
		std::atomic<std::uint32_t> m_handle_id{};
		SlotStatus m_status;
		TypeStorage<Type> m_storage;
	};

	template <typename Type, std::uint64_t PageSize>
	struct PagedArray {
		std::vector<std::unique_ptr<Type[]>> m_pg_table;
	};

	struct PoolHandleEntry {
		std::uint32_t m_idx : 31 {~0x0ul};
		std::uint32_t m_free : 1 {};
	};

	template <typename Type, std::uint64_t PageSize = 16ull>
	struct Pool {
		//PagedArray<std::uint32_t, PageSize> m_sparse; //maybe dont need, unlikely to have that many resource loaded at once
		static constexpr std::uint32_t null_handle{lo_bitmask32_v<31>};
		std::vector<PoolHandleEntry> m_sparse;
		std::vector<PoolSlot<Type>> m_packed;
		std::uint32_t m_freelist{ null_handle };

		std::uint32_t allocate_slot() {
			if (m_freelist == null_handle) {
				std::uint32_t internal_id{ static_cast<std::uint32_t>(m_packed.size()) };
				std::uint32_t hdl{ static_cast<std::uint32_t>(m_sparse.size()) };
				m_packed.emplace_back();
				m_sparse.emplace_back(internal_id);
				return hdl;
			}
			else {
				std::uint32_t hdl{ m_freelist };
				std::uint32_t internal_id{ static_cast<std::uint32_t>(m_packed.size()) };
				m_freelist = m_sparse[m_freelist].m_idx;
				m_packed.emplace_back();
				m_sparse[hdl].m_idx = internal_id;
				m_sparse[hdl].m_free = 0;
				return hdl;
			}
		}


		//schedules asset release and marks slot for deallocation
		void release_slot(std::uint32_t id) {
		}

		//deallocs the underlying storage //internal only, UB if used externally
		void deallocate_slot(std::uint32_t id) {
			if (m_packed.size() == 1) {
				m_packed.clear();
			}
			else {
				m_sparse[m_packed.back().m_handle_id].m_idx = m_sparse[id].m_idx;
				std::swap(m_packed[m_sparse[id].m_idx], m_packed.back());
				m_packed.pop_back();
			}
			m_sparse[id].m_idx = m_freelist;
			m_sparse[id].m_free = 1;
			m_freelist = id;
		}
	};
}

#endif