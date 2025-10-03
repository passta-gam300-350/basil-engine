#ifndef STRUCTURE_QUEUE_HPP
#define STRUCTURE_QUEUE_HPP

#include <atomic>
#include <optional>
#include "constraints.hpp"
#include "ring_buffer.hpp"
#include "smr/ebr.hpp"

namespace structures {
	//ebr smr implementation
	inline namespace ebr {
		template <typename T, allocator<T> A = std::allocator<T>>
		struct mpmc_queue {

		};

		template <typename T, allocator<T> A = std::allocator<T>>
		using spsc_queue = atomic_ring_buffer<T, A>;
	}
}

#endif 