#ifndef LIB_JOB_SYSTEM_HPP
#define LIB_JOB_SYSTEM_HPP

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
// Job graph node definitions
//------------------------------------------------------------------------------

struct JobNodeBase {
    struct DepNode {
        std::weak_ptr<JobNodeBase> child;
        DepNode* next;
    };

    std::function<void()>                    run;
    std::atomic<DepNode*>                    out{ nullptr };
    std::atomic<int>                         pending{ 0 };
    std::atomic<bool>                        enqueued{ false };

    // We allocate one JobCell per job at schedule time
    struct JobCell* cell = nullptr;

    virtual ~JobNodeBase() = default;
};

template<typename R>
struct JobNode : JobNodeBase {
    std::promise<R> prom;
};

struct JobHandle {
    std::weak_ptr<JobNodeBase> node;
};

struct JobCell {
    std::shared_ptr<JobNodeBase> node;
    explicit JobCell(std::shared_ptr<JobNodeBase> n) : node(std::move(n)) {}
};

//------------------------------------------------------------------------------
// Thread-safe queue with mutex + condition_variable
//------------------------------------------------------------------------------

class JobQueue {
public:
    // Push a new cell into the queue
    void push(JobCell* cell) {
        {
            std::lock_guard<std::mutex> lk(_mtx);
            _q.push_back(cell);
        }
        _cv.notify_one();
    }

    // Pop a cell; blocks until a cell is available or shutdown is signaled
    JobCell* pop() {
        std::unique_lock<std::mutex> lk(_mtx);
        _cv.wait(lk, [&] { return !_q.empty() || _shutdown; });
        if (_q.empty()) return nullptr;
        JobCell* cell = _q.front();
        _q.pop_front();
        return cell;
    }

    // Wake all waiting threads (on shutdown)
    void wakeAll() {
        {
            std::lock_guard<std::mutex> lk(_mtx);
            _shutdown = true;
        }
        _cv.notify_all();
    }

    // Drain and process all remaining cells
    void drain(std::function<void(JobCell*)> fn) {
        std::lock_guard<std::mutex> lk(_mtx);
        while (!_q.empty()) {
            fn(_q.front());
            _q.pop_front();
        }
    }

private:
    std::deque<JobCell*>      _q;
    std::mutex                _mtx;
    std::condition_variable   _cv;
    bool                      _shutdown{ false };
};

//------------------------------------------------------------------------------
// JobSystem: threads + queue + dependencies
//------------------------------------------------------------------------------

class JobSystem {
public:
    explicit JobSystem(size_t numThreads = std::thread::hardware_concurrency()) {
        if (numThreads == 0) numThreads = 1;
        for (size_t i = 0; i < numThreads; ++i) {
            _threads.emplace_back([this] { workerLoop(); });
        }
    }

    ~JobSystem() {
        shutdown();
    }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    // Schedule a job with no dependencies
    template<typename F, typename... Args>
    auto schedule(F&& f, Args&&... args)
        -> std::pair<JobHandle, std::future<std::invoke_result_t<F, Args...>>>
    {
        return scheduleWithDeps({}, std::forward<F>(f), std::forward<Args>(args)...);
    }

    // Schedule a job with a list of dependencies
    template<typename F, typename... Args>
    auto scheduleWithDeps(const std::vector<JobHandle>& deps,
        F&& f, Args&&... args)
        -> std::pair<JobHandle, std::future<std::invoke_result_t<F, Args...>>>
    {
        using R = std::invoke_result_t<F, Args...>;

        // 1) Create node + shared_ptr
        auto node_sp = std::make_shared<JobNode<R>>();
        JobNodeBase* raw = node_sp.get();

        // 2) Allocate the one JobCell now, so the promise can't vanish
        raw->cell = new JobCell(node_sp);

        // 3) Build the run() lambda capturing only raw* and argument tuple
        auto fn = std::forward<F>(f);
        auto tup = std::make_tuple(std::forward<Args>(args)...);
        raw->run = [raw, fn = std::move(fn), tup = std::move(tup)]() mutable {
            try {
                if constexpr (std::is_void_v<R>) {
                    std::apply(fn, tup);
                    static_cast<JobNode<R>*>(raw)->prom.set_value();
                }
                else {
                    R res = std::apply(fn, tup);
                    static_cast<JobNode<R>*>(raw)->prom.set_value(std::move(res));
                }
            }
            catch (...) {
                static_cast<JobNode<R>*>(raw)->prom.set_exception(std::current_exception());
            }
            };

        // 4) Hook up dependencies
        int cnt = 0;
        for (auto& h : deps) {
            if (auto sp = h.node.lock()) {
                ++cnt;
                JobNodeBase::DepNode* prev_node{ sp->out.load(std::memory_order_acquire) };
                JobNodeBase::DepNode* dep_node{ new JobNodeBase::DepNode{ node_sp, prev_node } };
                sp->out.store(dep_node, std::memory_order_relaxed);
            }
        }
        raw->pending.store(cnt, std::memory_order_relaxed);

        // 5) Prepare handle + future
        JobHandle handle{ node_sp };
        auto future = static_cast<JobNode<R>*>(raw)->prom.get_future();

        // 6) If no dependencies, enqueue immediately
        if (cnt == 0) enqueue(raw);

        return { handle, std::move(future) };
    }

    // Shutdown: wake threads, join, drain remaining jobs
    void shutdown() {
        _queue.wakeAll();
        for (auto& t : _threads) {
            if (t.joinable()) t.join();
        }

        // Drain + run leftover jobs so all promises are satisfied
        _queue.drain([&](JobCell* cell) {
            runCell(cell);
            });
    }

    std::size_t getWorkerCount() const {
        return _threads.size();
    }

private:
    std::vector<std::thread> _threads;
    JobQueue                _queue;

    // Worker thread body
    void workerLoop() {
        while (true) {
            JobCell* cell = _queue.pop();
            if (!cell) break;   // shutdown signaled
            runCell(cell);
        }
    }

    // Enqueue a job’s preallocated cell exactly once
    void enqueue(JobNodeBase* raw) {
        bool exp = false;
        if (!raw->enqueued.compare_exchange_strong(exp, true, std::memory_order_acq_rel))
            return;
        _queue.push(raw->cell);
    }

    // Execute one cell: delete it, run the job, and notify dependents
    void runCell(JobCell* cell) {
        auto node_sp = cell->node;  // keep node alive
        delete cell;                // destroys the only shared_ptr

        // Run the task and set the promise
        node_sp->run();

        // Trigger dependent jobs
        while (node_sp->out) {
            auto dep_node = node_sp->out.load(std::memory_order_relaxed);
            if (auto dep = dep_node->child.lock()) {
                int prev = dep->pending.fetch_sub(1, std::memory_order_acq_rel);
                if (prev == 1) {
                    enqueue(dep.get());
                }
            }
            std::atomic_thread_fence(std::memory_order_release);
            node_sp->out.store(dep_node->next, std::memory_order_relaxed);
            delete dep_node;
        }
    }
};


#endif