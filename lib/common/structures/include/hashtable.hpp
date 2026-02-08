/******************************************************************************/
/*!
\file   hashtable.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Lock-free hash table implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_COMMON_STRUCTURE_HASHTABLE
#define LIB_COMMON_STRUCTURE_HASHTABLE

#include <atomic>
#include <cstdint>
#include <utility>
#include <memory>
#include <cassert>
#include <type_traits>
#include <concepts>

//stl compliant allocator concept for backwards compatibility std::allocator
template <typename A, typename T>
concept stl_allocator_t = std::same_as<typename A::value_type, T>&&
    requires(A a, T* p, std::size_t n) {
        { a.allocate(n) } -> std::same_as<T*>;
        { a.deallocate(p, n) };
};

//std::hash like functors
template <typename F, typename T>
concept stl_hash_t = requires(F h, T k) {
    { h(k) } -> std::same_as<std::uint64_t>;
};

namespace detail {
    static constexpr std::uint64_t _growth_rx{ 2 };
    template <typename K, typename T>
    struct node; //fwd

    template <typename K, typename T>
    struct markedptr_t {
        node<K, T>* _ptr{nullptr};
        bool _marked;
        uint32_t _tag;

        bool operator==(markedptr_t const& other) const noexcept {
            return _ptr == other._ptr && _marked == other._marked && _tag == other._tag;
        }

        bool operator!=(markedptr_t const& other) const noexcept {
            return !(*this==other);
        }
    };
    template <typename K, typename T>
    struct node {
        K _key;
        T _val;
        std::atomic<markedptr_t<K, T>> _next; //address + (lsb)delete mark
    };
    constexpr std::uint32_t NOT_MARKED{ 0ul };
    constexpr std::uint32_t MARKED{ 1ul };
    constexpr std::uint64_t DEFAULT_BUCKET_SIZE{ 32 };
}

//lock free hash table
//reference paper: https://docs.rs/crate/crossbeam/0.2.4/source/hash-and-skip.pdf
template <typename K, typename T, stl_hash_t<K> H = std::hash<K>, stl_allocator_t<detail::node<K, T>> A = std::allocator<detail::node<K, T>>>
struct hashtable {
    using key = K;
    using value_type = T;
    using key_value_pair = std::pair<key, value_type>;
    using value_reference = value_type&;
    using value_const_reference = value_type const&;
    using node = detail::node<key, value_type>;
    using markedptr = detail::markedptr_t<key, value_type>;

    struct iterator {};

    static_assert(std::is_trivially_copyable<markedptr>::value, "markedptr_t must be trivially copyable");
    static_assert(std::is_default_constructible<detail::node<K,T>>::value, "supplied key type and value type must be default constructible");

public:
    hashtable(std::uint64_t size = detail::DEFAULT_BUCKET_SIZE) : _allocator{}, _bucket_list{ new std::atomic<markedptr>[size] {} }, _bucket_size{size} {}
    ~hashtable() { delete[] _bucket_list; };

protected:
    //supplied allocator
    A _allocator;

    //bucket of linked lists
    std::atomic<markedptr>* _bucket_list;
    std::atomic<std::uint64_t> _bucket_size;

    //thread locals accessors
    std::atomic<markedptr>*& get_prev() const;
    std::atomic<markedptr>& get_curr() const;
    std::atomic<markedptr>& get_next() const;

    //ptr utility
    node* get_ptr(node* ptr) { return (node*)((std::uintptr_t)ptr & ~1ULL); }
    bool is_marked(node* ptr) { return (std::uintptr_t)ptr & 1ULL; }
    node* mark_ptr(node* ptr) { return (node*)((std::uintptr_t)ptr | 1ULL); }

    template <typename ... cargs>
    node* allocate_node(cargs&&... args) { auto ptr = _allocator.allocate(1); new (ptr) node{ std::forward<cargs>(args)... }; return ptr; }
    void deallocate_node(node* ptr) { _allocator.deallocate(ptr, 1); };
    
    bool list_find(std::atomic<markedptr>* head, key key_v);
    bool list_insert(std::atomic<markedptr>* head, node* nd);
    bool list_delete(std::atomic<markedptr>* head, key key_v);

    std::ptrdiff_t h(key) const;
    bool hash_insert(key);
    bool hash_delete(key);
    bool hash_search(key);

public:
    iterator find(key key_v);
    iterator begin();
    iterator end();
    bool exist(key key_v) {
        return hash_search(key_v);
    }

    void emplace(key key_v) {
        hash_insert(key_v);
    }
    
    void erase(key key_v) {
        assert(hash_delete(key_v)&&"key does not exist! trying to delete invalid key");
    }

    value_type& operator[](key);
    value_type const& operator[](key) const;
};

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
std::atomic<typename hashtable<K, T, H, A>::markedptr>*& hashtable<K, T, H, A>::get_prev() const {
    static thread_local std::atomic<markedptr>* _prev{};
    return _prev;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
std::atomic<typename hashtable<K, T, H, A>::markedptr>& hashtable<K, T, H, A>::get_curr() const {
    static thread_local std::atomic<markedptr> _curr{};
    return _curr;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
std::atomic<typename hashtable<K, T, H, A>::markedptr>& hashtable<K, T, H, A>::get_next() const {
    static thread_local std::atomic<markedptr> _next{};
    return _next;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
std::ptrdiff_t hashtable<K, T, H, A>::h(key key_v) const{
    return H{}(key_v)%_bucket_size;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::hash_insert(key key_v) {
    auto nd{ allocate_node() };
    nd->_key = key_v;
    //nd->_next = markedptr{nullptr, false, 0};
    if (list_insert(_bucket_list + h(key_v), nd)) {
        return true;
    }
    deallocate_node(nd);
    return false;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::hash_delete(key key_v) {
    return list_delete(_bucket_list + h(key_v), key_v);
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::hash_search(key key_v) {
    return list_find(_bucket_list + h(key_v), key_v);
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::list_find(std::atomic<markedptr>* head, key key_v) {
    auto& tlprev{ get_prev() };
    auto& tlcurr{ get_curr() };
    auto& tlnext{ get_next() };

    while (true) { //try
        tlprev = head;
        tlcurr.store(tlprev->load(std::memory_order_acquire));
        auto [curr, pmark, ptag] { tlprev->load(std::memory_order_acquire) };
        while (true) { //traversal
            if (curr == nullptr) {
                return false;
            }
            auto [next, cmark, ctag] { curr->_next.load(std::memory_order_acquire) };
            if (tlprev->load(std::memory_order_acquire) != markedptr{ curr, false, ptag }) {
                break;
            }
            if (!cmark) {
                if (curr->_key >= key_v) {
                    return curr->_key == key_v;
                }
                tlprev = &curr->_next;
            }
            else {
                markedptr mptr{ curr, false, ptag };
                if (tlprev->compare_exchange_strong(mptr, markedptr{ next, false, ptag + 1 }, std::memory_order_acq_rel))
                {
                    deallocate_node(curr);
                    ctag = ptag + 1;
                }
                else {
                    break;
                }
            }
            pmark = cmark;
            curr = next;
            ptag = ctag;
        }
    }
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::list_insert(std::atomic<markedptr>* head, node* nd) {
    auto& tlprev{ get_prev() };
    auto& tlcurr{ get_curr() };

    while (true) {
        if (list_find(head, nd->_key)) {
            return false;
        }
        [[maybe_unused]] auto [curr, pmark, ptag] {tlprev->load(std::memory_order_acquire)};
        auto [ndptr, nmark, ntag] {nd->_next.load(std::memory_order_acquire)};
        ndptr = curr;
        nmark = false;
        markedptr mptr{ curr, false, ptag };
        if (tlprev->compare_exchange_strong(mptr, markedptr{ nd, false, ptag + 1 }, std::memory_order_acq_rel))
        {
            return true;
        }
    }
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
bool hashtable<K, T, H, A>::list_delete(std::atomic<markedptr>* head, key key_v) {
    auto& tlprev{ get_prev() };
    auto& tlcurr{ get_curr() };
    auto& tlnext{ get_next() };

    [[maybe_unused]] auto [curr, pmark, ptag] {tlcurr.load(std::memory_order_acquire)};
    [[maybe_unused]] auto [next, cmark, ctag] {tlcurr.load(std::memory_order_acquire)};

    while (true) {
        if (!list_find(head, key_v)) {
            return false;
        }
        markedptr mptr{ curr, false, ctag };
        if (!curr->_next.compare_exchange_strong(mptr, markedptr{ next, true, ctag + 1 })) {
            continue;
        }
        mptr = markedptr{ curr, false, ptag };
        if (tlprev->compare_exchange_strong(mptr, markedptr{ next, false, ptag + 1 }, std::memory_order_acq_rel)) {
            deallocate_node(curr);
        }
        else {
            list_find(head, key_v);
        }
        return true;
    }
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
typename hashtable<K, T, H, A>::value_type& hashtable<K, T, H, A>::operator[](key key_val) {
    if (!hash_search(key_val)) {
        assert(hash_insert(key_val) && "tried to insert hash when hash exists");
    }
    [[maybe_unused]] auto [curr, pmark, ptag] {get_prev()->load(std::memory_order_acquire)};
    return curr->_val;
}

template <typename K, typename T, stl_hash_t<K> H, stl_allocator_t<detail::node<K, T>> A>
typename hashtable<K, T, H, A>::value_type const& hashtable<K, T, H, A>::operator[](key key_val) const {
    assert(hash_search(key_val) && "key value not found. consider exception instead of insert");
    [[maybe_unused]] auto [curr, pmark, ptag] {get_curr().load(std::memory_order_acquire)};
    return curr->_val;
}

#endif