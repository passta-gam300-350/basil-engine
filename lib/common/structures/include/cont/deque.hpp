/******************************************************************************/
/*!
\file   deque.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lock-free deque implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef STRUCTURE_DEQUE_HPP
#define STRUCTURE_DEQUE_HPP

#include <atomic>
#include <optional>
#include "constraints.hpp"
#include "ring_buffer.hpp"
#include "smr/ebr.hpp"

namespace structures {

	//potential memory issues or inefficiencies when thread stalls
	inline namespace ebr { 
		//chase-lev spmc workstealing deque implementation
		//ref material: https://fzn.fr/readings/ppopp13.pdf
		template <typename T, allocator<T> A = std::allocator<T>>
		struct spmc_deque {
			std::atomic_size_t m_top;
			std::atomic_size_t m_bottom;
			std::atomic<raii_atomic_buffer<T, A>*> m_buf;
			smr::ebr_control m_ebr;

			//spmc_deque(spmc_deque const&) = default;
			spmc_deque();
			spmc_deque(spmc_deque const&) = delete;
			spmc_deque(spmc_deque&&) noexcept;
			~spmc_deque();

			//nullopt if empty
			std::optional<T> take();
			//nullopt if lost
			std::optional<T> steal();
			template <typename ... Args>
			void push(Args&&...) noexcept;
			void resize();
			void reserve(std::size_t);
		};
	}

#include "impl/deque_inl.hpp"

}

#endif // !STRUCTURE_DEQUE_HPP
