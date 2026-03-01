#ifndef NAIVE_THREADPOOL_HPP
#define NAIVE_THREADPOOL_HPP

#include <array>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

#ifdef TP_DEBUG
#include <iostream>
#endif

//note: single producer mutiple consumer only
template <unsigned int threadCt>
struct NaiveThreadPool {
	using Task = std::function<void()>;

	std::array<std::jthread, threadCt> m_threads;
	std::queue<Task> m_taskqueue;
	std::condition_variable m_notifywork;
	std::mutex m_queuemtx;
	std::atomic_bool m_shouldjoin;

	void WorkerLoop() {
		while (!m_shouldjoin) {
			Task task;
			{
				std::unique_lock ul{ m_queuemtx };
				if (!m_taskqueue.empty()) {
					task = std::move(m_taskqueue.front());
					m_taskqueue.pop();
				}
				else {
					m_notifywork.wait(ul, [&] {return m_shouldjoin || !m_taskqueue.empty(); });
				}
			}
			if (!task) {
#ifdef TP_DEBUG
				std::cout << "thread exiting! should close: " << m_shouldjoin ? "true" : "false" << ", threadid: " << std::this_thread::get_id() << "\n";
#endif
				return;
			}
			task();
		}
	}
	template <typename Invokable, typename...Args>
	void Enqueue(Invokable&& ivk, Args&& ... args) {
		{
			std::lock_guard lg(m_queuemtx);
			m_taskqueue.push([fn = std::forward<Invokable>(ivk), ...cargs = std::forward<Args>(args)] {std::apply(fn, std::forward_as_tuple(cargs...)); });
		}
		m_notifywork.notify_one();
	}

	NaiveThreadPool() {
		for (int i{}; i < threadCt; i++) {
			m_threads[i] = std::jthread(&NaiveThreadPool::WorkerLoop, std::ref(*this));
		}
	}
	~NaiveThreadPool() {
		m_shouldjoin.store(true);
		m_notifywork.notify_all();
	}
};

#endif