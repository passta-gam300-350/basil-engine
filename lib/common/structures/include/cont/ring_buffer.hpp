/******************************************************************************/
/*!
\file   ring_buffer.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lock-free ring buffer implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef STRUCTURE_RING_BUFFER_HPP
#define STRUCTURE_RING_BUFFER_HPP

#include <atomic>
#include <optional>
#include "constraints.hpp"
#include "smr/ebr.hpp"

namespace structures {
	template <typename T, allocator<T> A = std::allocator<T>>
	struct raii_atomic_buffer {
		std::atomic_size_t m_buf_sz;
		std::atomic<T*> m_buf;

		static_assert(std::is_copy_assignable_v<T> && std::is_trivially_constructible_v<T>);

		raii_atomic_buffer(std::size_t slot_ct = 0);
		raii_atomic_buffer(raii_atomic_buffer const&) = delete;
		raii_atomic_buffer(raii_atomic_buffer&&) noexcept;
		~raii_atomic_buffer();
	};

	inline namespace ebr {
		//implemented as a spsc ring buffer
		template <typename T, allocator<T> A = std::allocator<T>>
		struct atomic_ring_buffer : public raii_atomic_buffer<T, A> {
			using buffer = raii_atomic_buffer<T, A>;

			std::atomic_size_t m_head;
			std::atomic_size_t m_tail;
			ebr_control m_ebr;

			void push(T const&);
			void push(T&&);
			void pop();

			atomic_ring_buffer(std::size_t slot_ct = 0);
			atomic_ring_buffer(atomic_ring_buffer const&) = delete;
			atomic_ring_buffer(atomic_ring_buffer&&) noexcept = default;
			~atomic_ring_buffer() {};

			template <typename... Args>
			T& emplace(Args&&...);

			//throw exception if empty
			T front();
			//throw exception if empty
			T back();
			
			bool empty();
			std::size_t size();
			void reserve(std::size_t);

		private:
			void resize();

			static_assert(std::is_copy_assignable_v<T>&& std::is_trivially_constructible_v<T>);
		};

		template<typename T, allocator<T> A>
		inline void atomic_ring_buffer<T, A>::push(T const& v)
		{
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };

			if (t - h >= bs) {
				resize();
				bs = buffer::m_buf_sz.load(std::memory_order_relaxed);
			}
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			buf[t % bs] = v;
			std::atomic_thread_fence(std::memory_order_release);
			m_tail.store(t + 1, std::memory_order_relaxed);
		}

		template<typename T, allocator<T> A>
		inline void atomic_ring_buffer<T, A>::push(T&& v)
		{
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			
			if (t - h >= bs) {
				resize();
				bs = buffer::m_buf_sz.load(std::memory_order_relaxed);
			}
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			buf[t % bs] = std::move(v);
			std::atomic_thread_fence(std::memory_order_release);
			m_tail.store(t + 1, std::memory_order_relaxed);
		}
		template<typename T, allocator<T> A>
		inline void atomic_ring_buffer<T, A>::pop()
		{
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			if (t == h) {
				throw std::out_of_range("attempted to dequeue an empty queue");
			}
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			std::destroy_at(buf + (h % bs));
			m_head.store(h + 1, std::memory_order_relaxed);
		}
		template<typename T, allocator<T> A>
		inline atomic_ring_buffer<T, A>::atomic_ring_buffer(std::size_t slot_ct)
			: raii_atomic_buffer<T, A>(slot_ct), m_head{}, m_tail{}, m_ebr{}
		{}
		template<typename T, allocator<T> A>
		template<typename ...Args>
		inline T& atomic_ring_buffer<T, A>::emplace(Args && ... args)
		{
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };

			if (t - h >= bs) {
				resize();
				bs = buffer::m_buf_sz.load(std::memory_order_relaxed);
			}
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			new (buf + (t % bs)) T{std::forward<Args>(args)...};
			std::atomic_thread_fence(std::memory_order_release);
			m_tail.store(t + 1, std::memory_order_relaxed);
			return buf[t % bs];
		}
		template<typename T, allocator<T> A>
		inline T atomic_ring_buffer<T, A>::front() {
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			if (t == h) {
				throw std::out_of_range("attempted to read an empty queue");
			}
			ebr_guard g(m_ebr);
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			return buf[h % bs];
		}
		template<typename T, allocator<T> A>
		inline T atomic_ring_buffer<T, A>::back() {
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			if (t == h) {
				throw std::out_of_range("attempted to read an empty queue");
			}
			ebr_guard g(m_ebr);
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			T* buf{ buffer::m_buf.load(std::memory_order_relaxed) };
			return buf[(t-1) % bs];
		}
		template<typename T, allocator<T> A>
		inline bool atomic_ring_buffer<T, A>::empty() {
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			return t == h;
		}
		template<typename T, allocator<T> A>
		inline std::size_t atomic_ring_buffer<T, A>::size() {
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			return t - h;
		}
		template<typename T, allocator<T> A>
		inline void atomic_ring_buffer<T, A>::reserve(std::size_t new_sz) {
			T* raw{ buffer::m_buf.load(std::memory_order_relaxed) };
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			if (bs > new_sz) {
				return;
			}
			T* new_buff{ A().allocate(new_sz) };
			
			std::size_t h{ m_head.load(std::memory_order_relaxed) };
			std::size_t t{ m_tail.load(std::memory_order_relaxed) };;

			while (h <= t && raw) {
				new_buff[h % new_sz] = raw[h % bs];
				h++;
			}

			buffer::m_buf.store(new_buff, std::memory_order_relaxed);
			buffer::m_buf_sz.store(new_sz, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_release);
			if (!raw) {
				return;
			}
			m_ebr.retire([raw, bs] {A().deallocate(raw, bs); });
		}
		template<typename T, allocator<T> A>
		inline void atomic_ring_buffer<T, A>::resize() {
			std::size_t bs{ buffer::m_buf_sz.load(std::memory_order_relaxed) };
			bs ? reserve(bs << 1) : reserve(1);
		}
	}


	template <typename T, allocator<T> A>
	inline raii_atomic_buffer<T, A>::raii_atomic_buffer(std::size_t slot_ct) : m_buf_sz{ slot_ct }, m_buf{ nullptr } {
		if (slot_ct) {
			m_buf.store(A().allocate(slot_ct), std::memory_order_relaxed);
		}
	}

	template <typename T, allocator<T> A>
	inline raii_atomic_buffer<T, A>::raii_atomic_buffer(raii_atomic_buffer&& other) noexcept 
		: m_buf_sz{ other.m_buf_sz.load(std::memory_order_relaxed) }, m_buf{ other.m_buf.load(std::memory_order_relaxed) }
	{	
		other.m_buf_sz.store(0, std::memory_order_release);
		other.m_buf.store(nullptr, std::memory_order_release);
	}

	template <typename T, allocator<T> A>
	inline raii_atomic_buffer<T, A>::~raii_atomic_buffer() {
		std::size_t slot_ct{ m_buf_sz.load(std::memory_order_relaxed) };
		if (slot_ct) {
			m_buf_sz.compare_exchange_strong(slot_ct, 0, std::memory_order_seq_cst, std::memory_order_relaxed);
			T* buff{ m_buf.load(std::memory_order_relaxed) };
			m_buf.compare_exchange_strong(buff, nullptr, std::memory_order_seq_cst, std::memory_order_relaxed);
			A().deallocate(buff, slot_ct);
		}
	}


}

#endif