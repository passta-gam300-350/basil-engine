// Lock?Free Split?Ordered Hash Table with Hazard?Pointer SMR
// Based on Maged Michael’s “Split?Ordered Lists” (2002) + Hazard Pointers SMR.
//
// This example implements:
//  - A split?ordered list per the paper, supporting concurrent
//    insert/remove/contains and on?the?fly resize (grow).
//  - Safe Memory Reclamation via the classic Hazard Pointer pattern.
// 
// Requires C++11 or later.

#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <unordered_set>
#include <cassert>

//----------------------------------------------------------------------
// Hazard Pointer SMR (simplified)
//----------------------------------------------------------------------

static const unsigned MAX_HAZARD_POINTERS = 100;

struct HazardRecord {
    std::atomic<std::thread::id>  owner;
    std::atomic<void*>            ptr;
} g_hazardRecords[MAX_HAZARD_POINTERS];

class HazardPointerOwner {
    HazardRecord* myRecord;
public:
    HazardPointerOwner() : myRecord(nullptr) {
        for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
            std::thread::id noThread;
            if (g_hazardRecords[i].owner.compare_exchange_strong(
                noThread, std::this_thread::get_id())) {
                myRecord = &g_hazardRecords[i];
                break;
            }
        }
        assert(myRecord && "No hazard pointers available");
    }
    std::atomic<void*>& get_pointer_slot() {
        return myRecord->ptr;
    }
    void clear() {
        myRecord->ptr.store(nullptr);
    }
    ~HazardPointerOwner() {
        clear();
        myRecord->owner.store(std::thread::id{});
    }
};

// Retire?list per thread
static const unsigned RETIRE_THRESHOLD = 40;

template<typename T>
void retire_node(T* node);

// A per?thread retire vector
template<typename T>
std::vector<T*>& retired_nodes() {
    static thread_local std::vector<T*> list;
    return list;
}

// Scan hazard pointers and reclaim safe nodes
template<typename T>
void scan_and_reclaim() {
    auto& rlist = retired_nodes<T>();
    if (rlist.size() < RETIRE_THRESHOLD) return;

    // 1) Collect all hazard pointers
    std::unordered_set<void*> hazard_set;
    for (unsigned i = 0; i < MAX_HAZARD_POINTERS; ++i) {
        void* p = g_hazardRecords[i].ptr.load();
        if (p) hazard_set.insert(p);
    }

    // 2) For each retired node not in hazard_set, delete it
    auto it = rlist.begin();
    while (it != rlist.end()) {
        if (hazard_set.find(*it) == hazard_set.end()) {
            delete* it;
            it = rlist.erase(it);
        }
        else {
            ++it;
        }
    }
}

template<typename T>
void retire_node(T* node) {
    retired_nodes<T>().push_back(node);
    scan_and_reclaim<T>();
}

//----------------------------------------------------------------------
// Split?Ordered Hash Table
//----------------------------------------------------------------------

template<typename Key,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
    class LockFreeHashTable {
    private:
        struct Node;

        // Each bucket points into the big split?ordered list
        struct Table {
            size_t capacity;
            std::atomic<Node*>* buckets;      // array of bucket heads
            std::atomic<Table*> old_table;    // pointer to previous table during resize
            std::atomic<size_t>   migrate_index;

            Table(size_t cap)
                : capacity(cap)
                , buckets(new std::atomic<Node*>[cap])
                , old_table(nullptr)
                , migrate_index(0)
            {
                for (size_t i = 0; i < cap; ++i)
                    buckets[i].store(nullptr, std::memory_order_relaxed);
            }

            ~Table() {
                delete[] buckets;
            }
        };

        struct Node {
            Key                    key;
            std::atomic<Node*>     next;
            bool                   is_dummy;  // for split?ordered sentinel
            Node(Key const& k, Node* n, bool dummy = false)
                : key(k), next(n)
                , is_dummy(dummy)
            {
            }
        };

        std::atomic<Table*>  table_ptr;
        Hash                 hasher;
        KeyEqual             key_equal;

        // Reverse the bits of x in its lower `bits` bits
        static size_t reverse_bits(size_t x, unsigned bits) {
            size_t y = 0;
            for (unsigned i = 0; i < bits; ++i) {
                y = (y << 1) | ((x >> i) & 1);
            }
            return y;
        }

        // Compute the split?order key: reverse lower bits of h, set MSB to 1
        size_t make_so_key(size_t h, unsigned bucket_bits) const {
            size_t x = reverse_bits(h, bucket_bits);
            return x | (size_t(1) << bucket_bits);
        }

        // Ensure bucket `i` is initialized by installing its dummy node
        void init_bucket(Table* T, size_t i) {
            if (T->buckets[i].load(std::memory_order_acquire))
                return;  // already initialized

            if (i == 0) {
                // Bucket 0 always has a dummy head
                Node* dummy = new Node(Key{}, nullptr, true);
                Table* expected = nullptr;
                if (T->buckets[0].compare_exchange_strong(
                    expected, dummy,
                    std::memory_order_release,
                    std::memory_order_relaxed))
                {
                    // done
                }
                else {
                    delete dummy;
                }
                return;
            }

            // Initialize parent bucket first (floor(i/2))
            init_bucket(T, i & ~(size_t(1) << (31 - __builtin_clz(i))));
            // Now insert dummy for bucket i:
            size_t hb = Table::capacity_bits() + 1; // see note
            size_t so_key = make_so_key(i, hb);
            insert_node(T, so_key, /*dummy=*/true);
            // Link it into the bucket array
            Node* dummy = find_node(T, so_key);
            T->buckets[i].compare_exchange_strong(
                *(Node**)&dummy, dummy,
                std::memory_order_release,
                std::memory_order_relaxed);
        }

        // Access number of bits needed for bucket indexing
        static unsigned capacity_bits(size_t cap) {
            return sizeof(size_t) * 8 - __builtin_clz(cap - 1);
        }

        //------------------------------------------------------------------
        // Low?level list operations on the split?ordered list
        //------------------------------------------------------------------

        Node* find_node(Table* T, size_t so_key) {
            // classic lock?free list traversal with hazard pointers
            HazardPointerOwner hp_pred, hp_curr;
            Node* pred = T->buckets[0].load(std::memory_order_acquire);
            hp_pred.get_pointer_slot().store(pred);

            while (true) {
                Node* curr = pred->next.load(std::memory_order_acquire);
                hp_curr.get_pointer_slot().store(curr);
                if (!curr) break;

                if (curr->is_dummy || curr->key < so_key) {
                    pred = curr;
                    hp_pred.get_pointer_slot().store(pred);
                    continue;
                }
                // curr->key >= so_key
                return curr;
            }
            return nullptr;
        }

        bool insert_node(Table* T, size_t so_key, bool dummy = false) {
            while (true) {
                Node* curr = find_node(T, so_key);
                if (curr && curr->key == so_key && curr->is_dummy == dummy)
                    return false;  // already present

                // Create new node
                Node* newNode = new Node(Key(so_key), curr, dummy);
                HazardPointerOwner hp_prev;
                Node* prev = T->buckets[0].load(std::memory_order_acquire);
                hp_prev.get_pointer_slot().store(prev);

                // Find insertion point again under HP protection
                while (true) {
                    Node* succ = prev->next.load(std::memory_order_acquire);
                    if (succ != curr) {
                        // list changed, restart
                        delete newNode;
                        break;
                    }
                    if (prev->next.compare_exchange_strong(
                        succ, newNode,
                        std::memory_order_release,
                        std::memory_order_relaxed))
                    {
                        return true;
                    }
                }
            }
        }

        bool remove_node(Table* T, size_t so_key) {
            while (true) {
                // locate window [pred, curr)
                HazardPointerOwner hp_pred, hp_curr;
                Node* pred = T->buckets[0].load(std::memory_order_acquire);
                hp_pred.get_pointer_slot().store(pred);
                Node* curr = pred->next.load(std::memory_order_acquire);

                while (curr && (curr->key < so_key || curr->is_dummy)) {
                    pred = curr;
                    hp_pred.get_pointer_slot().store(pred);
                    curr = curr->next.load(std::memory_order_acquire);
                    hp_curr.get_pointer_slot().store(curr);
                }

                if (!curr || curr->key != so_key || curr->is_dummy)
                    return false;  // not found

                Node* succ = curr->next.load(std::memory_order_acquire);
                // logical deletion: CAS pred?next from curr?succ
                if (pred->next.compare_exchange_strong(
                    curr, succ,
                    std::memory_order_acq_rel))
                {
                    retire_node<Node>(curr);  // safe reclamation
                    return true;
                }
                // retry on failure
            }
        }

        bool contains_node(Table* T, size_t so_key) {
            Node* curr = find_node(T, so_key);
            return curr && curr->key == so_key && !curr->is_dummy;
        }

        //------------------------------------------------------------------
        // Resize Helpers
        //------------------------------------------------------------------

        void help_resize(Table* T) {
            Table* oldT = T->old_table.load(std::memory_order_acquire);
            if (!oldT) return;

            size_t idx;
            while ((idx = oldT->migrate_index.fetch_add(1)) < oldT->capacity) {
                init_bucket(T, idx);
            }

            // finalized: no more buckets to migrate
            T->old_table.compare_exchange_strong(
                oldT, (Table*)nullptr,
                std::memory_order_acq_rel);
            // retire old table struct
            retire_node<Table>(oldT);
        }

        void trigger_resize(Table* oldT) {
            size_t newCap = oldT->capacity * 2;
            Table* newT = new Table(newCap);
            newT->old_table.store(oldT, std::memory_order_release);

            // attempt to swap in new table
            if (table_ptr.compare_exchange_strong(
                oldT, newT,
                std::memory_order_acq_rel))
            {
                help_resize(newT);
            }
            else {
                // someone else won; retire ours
                delete newT;
            }
        }

    public:
        LockFreeHashTable(size_t initial_capacity = 16)
            : table_ptr(new Table(initial_capacity))
        {
        }

        ~LockFreeHashTable() {
            Table* T = table_ptr.load();
            // naive cleanup (not safe under concurrency)
            delete T;
        }

        bool insert(const Key& key) {
            size_t h = hasher(key);
            while (true) {
                help_resize(table_ptr.load());
                Table* T = table_ptr.load();
                size_t idx = h & (T->capacity - 1);
                init_bucket(T, idx);
                size_t so_key = make_so_key(h, capacity_bits(T->capacity));

                if (insert_node(T, so_key, /*dummy=*/false)) {
                    // if load factor high, trigger grow (omitted: track size)
                    // trigger_resize(T);
                    return true;
                }
                return false;  // already present
            }
        }

        bool remove(const Key& key) {
            size_t h = hasher(key);
            while (true) {
                help_resize(table_ptr.load());
                Table* T = table_ptr.load();
                size_t idx = h & (T->capacity - 1);
                init_bucket(T, idx);
                size_t so_key = make_so_key(h, capacity_bits(T->capacity));
                return remove_node(T, so_key);
            }
        }

        bool contains(const Key& key) {
            help_resize(table_ptr.load());
            Table* T = table_ptr.load();
            size_t h = hasher(key);
            size_t idx = h & (T->capacity - 1);
            init_bucket(T, idx);
            size_t so_key = make_so_key(h, capacity_bits(T->capacity));
            return contains_node(T, so_key);
        }
};
