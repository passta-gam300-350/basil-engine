#ifndef LIB_JOBSYSTEM_HPP
#define LIB_JOBSYSTEM_HPP

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <vector>
#include <chrono>
#include <semaphore>
#include <stdexcept>

#include <cont/queue.hpp>
#include <cont/deque.hpp>

// MAJOR TODO: fix std::vector<Worker> (temp solution to prealloc)(not threadsafe in submit access and destructor), fix JobPool (make it resizable, potential point of failure or inefficiency if workload is too high or too low) 
// use ebr
namespace JobSys {
    static constexpr uint32_t npos = ~0x0u;
    static constexpr uint64_t hi_dword_bitmask = static_cast<uint64_t>(~0x0u) << 32;
    static constexpr uint64_t lo_dword_bitmask = static_cast<uint64_t>(~0x0u);
}

using namespace structures;

struct JobID {
    uint64_t m_value = 0;

    static constexpr uint32_t npos = JobSys::npos;

    static JobID make(uint32_t index, uint32_t generation);
    uint32_t index() const;
    uint32_t generation() const;
    explicit operator bool() const;
};

struct JobNode; //forward
struct JobSystem;

struct DepList {
    struct Node {
        std::atomic<Node*> m_next{ nullptr };
        JobID m_dep; //dependent job to notify
        Node(JobID d) : m_dep(d) {}
    };

    struct NodePool {
        std::atomic<Node*> m_head{ nullptr };

        Node* acquire(JobID d);
        void release(Node* n);
        ~NodePool();
    };

    std::atomic<Node*> m_head{ nullptr };
    NodePool m_pool;

    void push(JobID d);
    //release nodes and return head to node list
    Node* detach_all();
    void recycle_list(Node* list);
};

struct JobTask {
    struct promise_type {
        JobSystem* m_sys = nullptr;
        JobID m_id;

        JobTask get_return_object() noexcept;
        std::suspend_always initial_suspend() noexcept;

        struct FinalAwaiter {
            bool await_ready() const noexcept;
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept;
            void await_resume() const noexcept;
        };
        FinalAwaiter final_suspend() noexcept;

        void unhandled_exception();
        void return_void() noexcept {}
    };

    std::coroutine_handle<promise_type> m_handle;

    explicit JobTask(std::coroutine_handle<promise_type> h);
    JobTask(JobTask&& o) noexcept;
    JobTask& operator=(JobTask&& o) noexcept;
    ~JobTask();
};

//for non coroutine jobs
template <typename F, typename... Args>
struct CoroPackagedJob {
    CoroPackagedJob(F&& f, Args... args)
        : fn{ std::forward<F>(f) }, m_args{ std::forward<Args>(args)... }{
    }
    JobTask operator()() {
        std::apply(fn, m_args);
        co_return;
    };
    F fn;
    std::tuple<Args...> m_args; //not perfect forwarding i think
};

namespace JobSys{
    template <typename F, typename... Args>
    auto make_packaged_job(F&& f, Args&&... args) {
        return CoroPackagedJob<std::decay_t<F>, std::decay_t<Args>...>(std::forward<F>(f), std::forward<Args>(args)...);
    }
}

struct ScheduleOn {
    JobSystem* m_sys;
    int m_worker_index;

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept;
    void await_resume() const noexcept {}
};

struct Yield {
    JobSystem* m_sys;

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept;
    void await_resume() const noexcept {}
};

struct DependsOn {
    JobSystem* m_sys;
    JobID m_dep;

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept;
    void await_resume() const noexcept {}
};

struct JobNode {
    std::atomic<int> m_preferred_worker{ -1 };
    std::atomic<int> m_remaining{ 0 };
    std::atomic<std::coroutine_handle<JobTask::promise_type>> m_handle{};
    std::atomic<bool> m_done{ false };

    DepList m_dependents;
};

struct JobPool {
    //linked list
    struct Slot {
        std::atomic<uint32_t> m_next{ JobSys::npos };
        std::atomic<uint32_t> m_generation{ 0 };

        JobNode* m_node;
        alignas(alignof(std::max_align_t)) unsigned char m_storage[sizeof(JobNode)]{};
        Slot() : m_node(reinterpret_cast<JobNode*>(m_storage)) {}
    };

    explicit JobPool(uint32_t capacity);
    ~JobPool();

    //JobSys::npos if alloc fails
    JobID alloc();
    void release(JobID id);

    JobNode* get(uint32_t idx) const;
    //current generation of node
    uint32_t live_generation(uint32_t idx) const;

private:
    struct TaggedHead {
        uint32_t index;
        uint32_t tag;
    };

    static uint64_t pack(TaggedHead h);
    static TaggedHead unpack(uint64_t v);

    uint32_t m_capacity;
    std::unique_ptr<Slot[]> m_slots;
    std::atomic<uint64_t> m_head{ 0 };
};

struct Worker {
    int m_index = -1; //worker id, not thread id
    spmc_deque<uint64_t> m_deque; //producer, put job into worker job pool
    spsc_queue<uint64_t> m_inbox; //consumer, consumes job from job queue
    std::thread m_thread;
    std::atomic<bool> m_running{ true };
};

struct JobSystem {
    JobSystem(int thread_count, uint32_t pool_capacity = 0x1000); //potential point of failure, make pool resizable in the future
    ~JobSystem();

    template <typename CoroFunc, typename... Args>
    JobID submit(int preferred_worker, std::vector<JobID> deps, CoroFunc&& f, Args&&... args);

    ScheduleOn schedule_on(int worker_index);
    Yield yield();
    DependsOn depends_on(JobID dep);

    void wait_for(JobID id);

    void schedule_ready(JobID id, int worker_index);
    void notify_complete(JobID id) noexcept;
    bool is_done(JobID id) const noexcept;
    uint64_t get_thread_ct() const noexcept;

    //should consider encapsulating this instead of friendship
    friend struct ScheduleOn;
    friend struct Yield;
    friend struct DependsOn;

private:
    void worker_loop(int index);

    JobPool m_pool;
    //std::counting_semaphore<> m_semaphore_idle; //worker idles if this is 0
    std::vector<std::unique_ptr<Worker>> m_workers;
};

inline JobID JobID::make(uint32_t index, uint32_t generation) {
    return JobID{ (static_cast<uint64_t>(generation) << 32) | index };
}

inline uint32_t JobID::index() const {
    return static_cast<uint32_t>(m_value & JobSys::lo_dword_bitmask);
}

inline uint32_t JobID::generation() const {
    return static_cast<uint32_t>(m_value >> 32);
}

inline JobID::operator bool() const {
    return index() != npos;
}

inline DepList::Node* DepList::NodePool::acquire(JobID d) {
    Node* n = m_head.load(std::memory_order_acquire);
    while (n) {
        Node* next = n->m_next.load(std::memory_order_relaxed);
        if (m_head.compare_exchange_weak(n, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
            n->m_next.store(nullptr, std::memory_order_relaxed);
            n->m_dep = d;
            return n;
        }
    }
    return new Node(d);
}

inline void DepList::NodePool::release(Node* n) {
    Node* h = m_head.load(std::memory_order_acquire);
    do {
        n->m_next.store(h, std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(h, n, std::memory_order_acq_rel, std::memory_order_acquire));
}

inline DepList::NodePool::~NodePool() {
    Node* n = m_head.load();
    while (n) {
        Node* next = n->m_next.load();
        delete n;
        n = next;
    }
}

inline void DepList::push(JobID d) {
    Node* n = m_pool.acquire(d);
    Node* h = m_head.load(std::memory_order_acquire);
    do {
        n->m_next.store(h, std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(h, n, std::memory_order_acq_rel, std::memory_order_acquire));
}

inline DepList::Node* DepList::detach_all() {
    return m_head.exchange(nullptr, std::memory_order_acq_rel);
}

inline void DepList::recycle_list(Node* list) {
    while (list) {
        Node* next = list->m_next.load(std::memory_order_relaxed);
        list->m_next.store(nullptr, std::memory_order_relaxed);
        m_pool.release(list);
        list = next;
    }
}

inline JobTask JobTask::promise_type::get_return_object() noexcept {
    return JobTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
}

inline std::suspend_always JobTask::promise_type::initial_suspend() noexcept {
    return {};
}

inline bool JobTask::promise_type::FinalAwaiter::await_ready() const noexcept {
    return false;
}

inline void JobTask::promise_type::FinalAwaiter::await_suspend(std::coroutine_handle<promise_type> h) noexcept {
    h.promise().m_sys->notify_complete(h.promise().m_id);
}

inline void JobTask::promise_type::FinalAwaiter::await_resume() const noexcept {
}

inline JobTask::promise_type::FinalAwaiter JobTask::promise_type::final_suspend() noexcept {
    return {};
}

inline void JobTask::promise_type::unhandled_exception() {
    std::terminate();
}

inline JobTask::JobTask(std::coroutine_handle<promise_type> h)
    : m_handle(h) {
}

inline JobTask::JobTask(JobTask&& o) noexcept
    : m_handle(o.m_handle) {
    o.m_handle = {};
}

inline JobTask& JobTask::operator=(JobTask&& o) noexcept {
    if (this != &o) {
        if (m_handle) {
            m_handle.destroy();
        }
        m_handle = o.m_handle;
        o.m_handle = {};
    }
    return *this;
}

inline JobTask::~JobTask() {
    if (m_handle) {
        m_handle.destroy();
    }
}

inline bool ScheduleOn::await_ready() const noexcept {
    return false;
}

inline void ScheduleOn::await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept {
    JobID id = h.promise().m_id;
    JobNode* node = m_sys->m_pool.get(id.index());
    node->m_handle.store(h, std::memory_order_release);
    m_sys->schedule_ready(id, m_worker_index);
}

inline bool Yield::await_ready() const noexcept {
    return false;
}

inline void Yield::await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept {
    JobID id = h.promise().m_id;
    JobNode* node = m_sys->m_pool.get(id.index());
    int w = node->m_preferred_worker.load(std::memory_order_relaxed);
    node->m_handle.store(h, std::memory_order_release);
    m_sys->schedule_ready(id, (w >= 0 ? w : 0));
}

inline bool DependsOn::await_ready() const noexcept {
    return m_sys->is_done(m_dep);
}

inline void DependsOn::await_suspend(std::coroutine_handle<JobTask::promise_type> h) noexcept {
    if (m_sys->is_done(m_dep)) {
        int w = m_sys->m_pool.get(h.promise().m_id.index())->m_preferred_worker.load(std::memory_order_relaxed);
        m_sys->schedule_ready(h.promise().m_id, (w >= 0 ? w : 0));
        return;
    }
    JobNode* depnode = m_sys->m_pool.get(m_dep.index());
    depnode->m_dependents.push(h.promise().m_id);
    JobNode* me = m_sys->m_pool.get(h.promise().m_id.index());
    me->m_handle.store(h, std::memory_order_release);
}

inline JobPool::JobPool(uint32_t capacity)
    : m_capacity(capacity), m_slots(std::make_unique<Slot[]>(capacity)) {
    for (uint32_t i = 0; i < m_capacity; ++i) {
        m_slots.get()[i].m_next.store(i + 1 < m_capacity ? i + 1 : JobSys::npos, std::memory_order_relaxed);
    }
    m_head.store(pack({ 0u, 1u }), std::memory_order_release);
}

inline JobPool::~JobPool() {
}

inline JobID JobPool::alloc() {
    while (true) {
        uint64_t raw = m_head.load(std::memory_order_acquire);
        TaggedHead h = unpack(raw);
        if (h.index == JobSys::npos) {
            return JobID{ 0 }; //invalid
        }
        uint32_t next = m_slots[h.index].m_next.load(std::memory_order_relaxed);
        TaggedHead nh{ next, h.tag + 1 };
        if (m_head.compare_exchange_weak(raw, pack(nh), std::memory_order_acq_rel, std::memory_order_acquire)) {
            //bump generation and construct node
            uint32_t gen = m_slots[h.index].m_generation.fetch_add(1, std::memory_order_acq_rel) + 1;
            new (m_slots[h.index].m_node) JobNode();
            return JobID::make(h.index, gen);
        }
    }
}

inline void JobPool::release(JobID id) {
    uint32_t idx = id.index();
    m_slots[idx].m_node->~JobNode();

    while (true) {
        uint64_t raw = m_head.load(std::memory_order_acquire);
        TaggedHead h = unpack(raw);
        m_slots[idx].m_next.store(h.index, std::memory_order_relaxed);
        TaggedHead nh{ idx, h.tag + 1 };
        if (m_head.compare_exchange_weak(raw, pack(nh), std::memory_order_acq_rel, std::memory_order_acquire)) {
            return; //cas until succeed
        }
    }
}

inline JobNode* JobPool::get(uint32_t idx) const {
    return m_slots[idx].m_node;
}

inline uint32_t JobPool::live_generation(uint32_t idx) const {
    return m_slots[idx].m_generation.load(std::memory_order_acquire);
}

inline uint64_t JobPool::pack(TaggedHead h) {
    return (static_cast<uint64_t>(h.tag) << 32) | h.index;
}

inline JobPool::TaggedHead JobPool::unpack(uint64_t v) {
    return TaggedHead{ static_cast<uint32_t>(v & JobSys::lo_dword_bitmask), static_cast<uint32_t>(v >> 32) };
}

inline JobSystem::JobSystem(int thread_count, uint32_t pool_capacity)
    : m_pool(pool_capacity)/*, m_semaphore_idle{0}*/ {
    m_workers.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        auto w = std::make_unique<Worker>();
        w->m_index = i;
        w->m_inbox.reserve(1024);
        w->m_deque.reserve(1024);
        w->m_thread = std::thread([this, i] { worker_loop(i); });
        m_workers.push_back(std::move(w));
    }
}

inline JobSystem::~JobSystem() {
    for (auto& w : m_workers) {
        w->m_running.store(false, std::memory_order_release);
    }
    //m_semaphore_idle.release(static_cast<ptrdiff_t>(m_workers.size())); //wake all for cooperative shutdown
    for (auto& w : m_workers) {
        if (w->m_thread.joinable()) w->m_thread.join();
    }
}

template <typename CoroFunc, typename... Args>
inline JobID JobSystem::submit(int preferred_worker, std::vector<JobID> deps, CoroFunc&& f, Args&&... args) {
    JobID id = m_pool.alloc();
    if (!id) throw std::runtime_error("JobPool exhausted");

    JobNode* node = m_pool.get(id.index());
    node->m_preferred_worker.store(preferred_worker, std::memory_order_release);
    node->m_remaining.store(static_cast<int>(deps.size()), std::memory_order_release);
    node->m_done.store(false, std::memory_order_release);
    node->m_handle.store({}, std::memory_order_release);

    // Register dependents (append-only, safe)
    for (JobID dep : deps) {
        if (m_pool.live_generation(dep.index()) == dep.generation()) {
            JobNode* d = m_pool.get(dep.index());
            if (d->m_done.load(std::memory_order_acquire)) {
                // Dependency already done: reduce remaining immediately
                node->m_remaining.fetch_sub(1, std::memory_order_acq_rel);
            }
            else {
                d->m_dependents.push(id);
            }
        }
        else {
            node->m_remaining.fetch_sub(1, std::memory_order_acq_rel);
        }
    }

    // Create coroutine
    JobTask task = f(std::forward<Args>(args)...);
    task.m_handle.promise().m_sys = this;
    task.m_handle.promise().m_id = id;
    node->m_handle.store(task.m_handle, std::memory_order_release);

    // If ready, schedule
    if (node->m_remaining.load(std::memory_order_acquire) == 0) {
        schedule_ready(id, preferred_worker >= 0 ? preferred_worker : (id.index() % m_workers.size()));
    }

    task.m_handle = {}; // ownership transferred
    return id;
}

inline ScheduleOn JobSystem::schedule_on(int worker_index) {
    return ScheduleOn{ this, worker_index };
}

inline Yield JobSystem::yield() {
    return Yield{ this };
}

inline DependsOn JobSystem::depends_on(JobID dep) {
    return DependsOn{ this, dep };
}

inline void JobSystem::wait_for(JobID id) {
    while (!is_done(id)); //busy waits
}

inline void JobSystem::schedule_ready(JobID id, int worker_index) {
    m_workers[worker_index]->m_inbox.push(id.m_value);

}

inline void JobSystem::notify_complete(JobID id) noexcept {
    JobNode* node = m_pool.get(id.index());
    node->m_done.store(true, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_release);

    // Detach dependents list and traverse safely
    DepList::Node* flist = node->m_dependents.detach_all();
    DepList::Node* list = flist;
    while (list) {
        JobID depid = list->m_dep;
        JobNode* d = m_pool.get(depid.index());
        // Only act if the generation matches the current live generation
        if (m_pool.live_generation(depid.index()) == depid.generation()) {
            int prev = d->m_remaining.fetch_sub(1, std::memory_order_acq_rel);
            if (prev == 1) {
                int w = d->m_preferred_worker.load(std::memory_order_relaxed);
                schedule_ready(depid, (w >= 0 ? w : (depid.index() % m_workers.size())));
            }
        }
        list = list->m_next.load(std::memory_order_relaxed);
    }
    node->m_dependents.recycle_list(flist); //recycles old list

    // Destroy coroutine and reclaim slot
    auto h = node->m_handle.load(std::memory_order_acquire);
    if (h) { h.destroy(); node->m_handle.store({}, std::memory_order_release); }
    m_pool.release(id);
}

inline bool JobSystem::is_done(JobID id) const noexcept {
    JobNode* node = m_pool.get(id.index());
    return m_pool.live_generation(id.index()) != id.generation() || node->m_done.load(std::memory_order_acquire); //check gen if not same, job is expired. otherwise check status
}

inline void JobSystem::worker_loop(int index) {
    Worker& self = *m_workers[index];

    while (self.m_running.load(std::memory_order_acquire)) {
        //push job queue to job pool
        while (!self.m_inbox.empty()) {
            uint64_t v = self.m_inbox.front(); self.m_inbox.pop();
            self.m_deque.push(v);
        }

        //get work from self job pool or steal others
        std::optional<uint64_t> o = self.m_deque.take();
        if (!o.has_value()) {
            bool stolen = false;
            for (size_t i = 0; i < m_workers.size(); ++i) {
                if (static_cast<int>(i) == index) {
                    continue;
                }
                std::optional<uint64_t> s = m_workers[i]->m_deque.steal();
                if (s.has_value()) {
                    o = s;
                    stolen = true;
                    break;
                }
            }
            if (!stolen) {
                continue;
            }
        }

        JobID id{ *o };
        uint32_t idx = id.index();
        uint32_t gen = id.generation();

        if (m_pool.live_generation(idx) != gen) {
            //stale job reference; skip
            continue;
        }

        JobNode* node = m_pool.get(idx);
        auto h = node->m_handle.load(std::memory_order_acquire);
        if (h && !node->m_done.load(std::memory_order_acquire)) {
            node->m_preferred_worker.store(index, std::memory_order_release);
            h.resume();
        }
    }
}

inline uint64_t JobSystem::get_thread_ct() const noexcept {
    return m_workers.size();
}

#endif