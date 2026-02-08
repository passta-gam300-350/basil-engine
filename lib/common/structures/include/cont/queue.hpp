/******************************************************************************/
/*!
\file   queue.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lock-free queue implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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