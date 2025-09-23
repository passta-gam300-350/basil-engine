#ifndef LIB_HAZARD_PTR_HPP
#define LIB_HAZARD_PTR_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <unordered_set>
#include <type_traits>

namespace hp {

    //
    // Hazard Pointer Record Node
    //
    struct HazardRecord {
        std::atomic<void*> ptr;
        HazardRecord* next;
        HazardRecord() : ptr(nullptr), next(nullptr) {}
    };

    //
    // Global head of the lock-free list of all hazard records
    //
    static std::atomic<HazardRecord*> head{ nullptr };

    //
    // Per-thread list of acquired hazard records
    //
    inline HazardRecord* acquire_hazard_record() {
        thread_local std::vector<HazardRecord*> myRecords;

        // Try to find an existing free record in this thread’s pool
        for (auto* rec : myRecords) {
            if (rec->ptr.load(std::memory_order_relaxed) == nullptr) {
                return rec;
            }
        }

        // Otherwise allocate a new record and push it onto the global list
        auto* rec = new HazardRecord();
        HazardRecord* oldHead = head.load(std::memory_order_acquire);
        do {
            rec->next = oldHead;
        } while (!head.compare_exchange_weak(
            oldHead, rec,
            std::memory_order_release,
            std::memory_order_acquire));

        myRecords.push_back(rec);
        return rec;
    }

    //
    // Acquire a hazard pointer, read & protect the atomic pointer 'p'
    //
    template<typename T>
    T* protect(std::atomic<T*>& p) {
        auto* rec = acquire_hazard_record();
        T* observed = nullptr;

        do {
            observed = p.load(std::memory_order_acquire);
            rec->ptr.store(observed, std::memory_order_release);
        } while (observed != p.load(std::memory_order_acquire));

        return observed;
    }

    //
    // Release a single hazard pointer record (clear it)
    //
    inline void clear_hazard(HazardRecord* rec) {
        rec->ptr.store(nullptr, std::memory_order_release);
    }

    //
    // Per-thread retired list and default reclamation threshold
    //
    static const unsigned DEFAULT_THRESHOLD = 100;
    thread_local std::vector<void*> retiredList;

    //
    // Scan all hazard pointers in the system and reclaim unprotected nodes
    //
    inline void scan_and_reclaim() {
        // Collect all actively protected addresses
        std::unordered_set<void*> protectedAddrs;
        HazardRecord* cur = head.load(std::memory_order_acquire);
        while (cur) {
            if (void* p = cur->ptr.load(std::memory_order_acquire)) {
                protectedAddrs.insert(p);
            }
            cur = cur->next;
        }

        // Walk our retired list
        auto it = retiredList.begin();
        while (it != retiredList.end()) {
            void* node = *it;
            if (protectedAddrs.find(node) == protectedAddrs.end()) {
                // Safe to reclaim
                using NodeT = typename std::remove_pointer<decltype(node)>::type;
                delete static_cast<NodeT*>(node);
                it = retiredList.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    //
    // Retire a node for later reclamation
    //
    template<typename T>
    void retire(T* node, unsigned threshold = DEFAULT_THRESHOLD) {
        retiredList.push_back(node);
        if (retiredList.size() >= threshold) {
            scan_and_reclaim();
        }
    }

} // namespace hp


struct hazard_entry {
    std::atomic<void*> ptr;
    hazard_entry* next;
    hazard_entry() : ptr(nullptr), next(nullptr) {}
};

struct hazard_ptr {
    hazard_ptr() noexcept;
    hazard_ptr(hazard_ptr&&) noexcept;
    hazard_ptr& operator=(hazard_ptr&&) noexcept;
    ~hazard_ptr();

    bool empty() const noexcept;
    template<class T> T* protect(const atomic<T*>& src) noexcept;
    template<class T> bool try_protect(T*& ptr, const atomic<T*>& src) noexcept;
    template<class T> void reset_protection(const T* ptr) noexcept;
    void reset_protection(nullptr_t = nullptr) noexcept;
    void swap(hazard_ptr&) noexcept;
};

#endif // LIB_HAZARD_PTR_HPP
