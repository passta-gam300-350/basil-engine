/******************************************************************************/
/*!
\file   ebr.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Epoch-based reclamation for safe memory management

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SMR_EPOCH_BASED_RECLAIMATION_HPP
#define SMR_EPOCH_BASED_RECLAIMATION_HPP

#include <mutex>
#include <atomic>
#include <vector>
#include <limits>
#include <cassert>
#include <functional>
#include <unordered_map>

namespace structures {
	inline namespace smr {
		struct ebr_record {
			std::function<void()> fn_release{nullptr};
			std::size_t m_record_epoch;
		};

		struct ebr_node_record {
			ebr_record m_record;
			ebr_node_record* m_next;
		};

		struct ebr_trinode_record {
			ebr_record m_record[3];
			ebr_trinode_record* m_next;
		};

		//please ensure that control (owner) doesnt exit before running threads
		struct ebr_thread_record {
			std::atomic<bool> m_active{ false };
			std::atomic<uint64_t> m_local_epoch{ 0 };
			std::atomic<ebr_node_record*> m_retired_head{ nullptr };
			std::atomic<size_t> m_retired_count{ 0 }; //doubles as freelist_next when active is false 
			~ebr_thread_record();
		};

		//might do a cas linked list impl in the future
		inline namespace vector_impl {
			//reference material: https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf
			//note: most index based function are deprecated. TODO: clean up interface
			struct ebr_control {
				using deleter = std::function<void()>; //deleter is typed erased and should be packaged into a void(void)

				std::atomic_size_t m_current_epoch;
				std::mutex m_record_slots_mtx;
				std::vector<std::unique_ptr<ebr_thread_record>> m_record_slots;
				//std::atomic_size_t m_head; //head to free list //deprecated changed to swap and pop. deleting threads is not O(1) but keeps scan packed. deleting thread is unlikely
				const std::size_t m_instance_id;

				void enter(std::size_t);
				void enter(ebr_thread_record&);

				bool try_advance_epoch(); //advance according to epoch consensus
				void retire(deleter);
				void collect();
				std::size_t min_epoch();

				void collect_thread(ebr_thread_record&);
				void collect_thread(std::size_t);
				void exit(std::size_t);
				void exit(ebr_thread_record&);

				std::size_t register_thread_record_idx(); //ensure that this does not get called multiple times //deprecated //this is unsafe and has issues with swap and pop policy
				ebr_thread_record* register_thread_record();
				void release_thread_record(std::size_t); //policy is swap and pop
				void release_thread_record(ebr_thread_record* ptr);
				ebr_thread_record& get_thread_record(std::size_t);
				ebr_thread_record& get_thread_record();

				std::atomic_size_t& get_instance_id();

				ebr_control();
				ebr_control(ebr_control const&) = delete;
				~ebr_control();
			};
		}

		//impl only. do not use
		struct ebr_tls_thread_record_registry {
		private:
			using ebr_thread_record_pair = std::pair<ebr_thread_record*, ebr_control*>;
			std::unordered_map<std::size_t, ebr_thread_record_pair> m_record_registry; //consider if ebr control exits before thread. same issue highlighted in ebr_thread_record. this use case is unsupported since it is unlikely for the owner thread to exit before working threads.
			ebr_tls_thread_record_registry() = default; //tls singleton

		public:
			~ebr_tls_thread_record_registry();
			static ebr_tls_thread_record_registry& tls_reg();
			static void tls_erase(ebr_control*);
			static ebr_thread_record& tls_get(ebr_control*); //tls get record per domain. non owning record
			static std::size_t tls_get_idx(ebr_control*);
		};

		//raii guard
		struct ebr_guard {
			ebr_guard() = delete;
			ebr_guard(ebr_guard const&) = delete;
			explicit ebr_guard(ebr_control&);
			~ebr_guard();

		private:
			ebr_control& m_ebrcontrol;
			ebr_thread_record& m_record;
		};

#include "ebr_inl.hpp"
	}
}

#endif