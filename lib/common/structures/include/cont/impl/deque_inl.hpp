/******************************************************************************/
/*!
\file   deque_inl.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lock-free deque inline implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "deque.hpp"
#ifndef STRUCTURE_DEQUE_INLINE_IMPL_HPP
#define STRUCTURE_DEQUE_INLINE_IMPL_HPP

template <typename T, allocator<T> A>
inline std::optional<T> ebr::spmc_deque<T, A>::take() {
	std::size_t b{ m_bottom.load(std::memory_order_relaxed) - 1 };
	m_bottom.store(b, std::memory_order_relaxed);
	std::optional<T> val{std::nullopt};

	std::atomic_thread_fence(std::memory_order_seq_cst);
	std::size_t t{ m_top.load(std::memory_order_relaxed) };
	if (t <= b && b != ~0ull) {
		smr::ebr_guard g(m_ebr);
		raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_relaxed) };
		std::size_t sz{ a->m_buf_sz.load(std::memory_order_relaxed) };
		val = *(a->m_buf.load(std::memory_order_relaxed) + (b % sz));
		if (t == b) { //last element
			if (!m_top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
				val = std::nullopt; //lost
			}
			m_bottom.store(b + 1, std::memory_order_relaxed);
		}
	}
	else {
		m_bottom.store(b + 1, std::memory_order_relaxed);
	}
	return val;
}


template <typename T, allocator<T> A>
inline std::optional<T> ebr::spmc_deque<T, A>::steal() {
	std::size_t t{m_top.load(std::memory_order_acquire)};
	std::atomic_thread_fence(std::memory_order_seq_cst);

	std::size_t b{ m_bottom.load(std::memory_order_acquire) };
	std::optional<T> val{ std::nullopt };

	if (t < b && b != ~0ull) {
		smr::ebr_guard g(m_ebr);
		raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_consume) };
		std::size_t sz{ a->m_buf_sz.load(std::memory_order_relaxed) };
		val = *(a->m_buf.load(std::memory_order_relaxed) + (t % sz));

		if (!m_top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
			val = std::nullopt; //lost race
		}
	}
	return val;
}

template <typename T, allocator<T> A>
template <typename ... Args>
inline void ebr::spmc_deque<T, A>::push(Args&&... args) noexcept {
	std::size_t b{ m_bottom.load(std::memory_order_relaxed) };
	raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_relaxed) };
	std::size_t t{ m_top.load(std::memory_order_relaxed) };

	if (!a) {
		raii_atomic_buffer<T, A>* new_buff{ new raii_atomic_buffer<T, A>{1} };
		m_buf.store(new_buff, std::memory_order_relaxed);
		a = m_buf.load(std::memory_order_relaxed);
	}
	std::size_t sz{ a->m_buf_sz.load(std::memory_order_relaxed) };

	if (b - t > sz - 1) { //buff full
		resize();
		a = m_buf.load(std::memory_order_relaxed);
		sz = a->m_buf_sz.load(std::memory_order_relaxed);
	}
	new (a->m_buf.load(std::memory_order_relaxed) + (b % sz)) T{std::forward<Args>(args)...};
	std::atomic_thread_fence(std::memory_order_release);
	m_bottom.store(b + 1, std::memory_order_relaxed);
}

template <typename T, allocator<T> A>
inline void ebr::spmc_deque<T, A>::resize() {
	raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_relaxed) };
	std::size_t sz{ a->m_buf_sz.load(std::memory_order_relaxed) };
	std::size_t new_sz{ sz << 1 };
	raii_atomic_buffer<T, A>* new_buff{ new raii_atomic_buffer<T, A>{new_sz} };

	std::size_t b{ m_bottom.load(std::memory_order_relaxed) };
	std::size_t t{ m_top.load(std::memory_order_relaxed) };

	T* raw_buff{ new_buff->m_buf.load(std::memory_order_relaxed) };

	while (t <= b) {
		raw_buff[t % new_sz] = *(a->m_buf.load(std::memory_order_relaxed) + (t % sz));
		t++;
	}

	m_buf.store(new_buff, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_release);
	m_ebr.retire([a] {delete a; });
}

template <typename T, allocator<T> A>
inline void ebr::spmc_deque<T, A>::reserve(std::size_t new_sz) {
	raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_relaxed) };
	if (a && a->m_buf_sz.load(std::memory_order_relaxed) > new_sz) {
		return;
	}
	raii_atomic_buffer<T, A>* new_buff{ new raii_atomic_buffer<T, A>{new_sz} };
	std::size_t sz{ a ? a->m_buf_sz.load(std::memory_order_relaxed) : 0 };

	std::size_t b{ m_bottom.load(std::memory_order_relaxed) };
	std::size_t t{ m_top.load(std::memory_order_relaxed) };

	T* raw_buff{ new_buff->m_buf.load(std::memory_order_relaxed) };

	while (t <= b && a) {
		raw_buff[t % new_sz] = *(a->m_buf.load(std::memory_order_relaxed) + (t % sz));
		t++;
	}

	m_buf.store(new_buff, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_release);
	if (!a) {
		return;
	}
	m_ebr.retire([a] {delete a; });
}

template <typename T, allocator<T> A>
inline ebr::spmc_deque<T, A>::spmc_deque()
	: m_top{ 0 }, m_bottom{}, m_buf{ nullptr }, m_ebr{} {
}

template<typename T, allocator<T> A>
inline ebr::spmc_deque<T, A>::spmc_deque(spmc_deque&& other) noexcept
	: m_top{ other.m_top.exchange(0, std::memory_order_relaxed) }, m_bottom{ other.m_bottom.exchange(0, std::memory_order_relaxed) }, m_buf{ other.m_buf.exchange(nullptr, std::memory_order_relaxed) }, m_ebr{}
{
}

template <typename T, allocator<T> A>
inline ebr::spmc_deque<T, A>::~spmc_deque() {
	raii_atomic_buffer<T, A>* a{ m_buf.load(std::memory_order_relaxed) };
	if (a) {
		delete a;
	}
}

#endif 