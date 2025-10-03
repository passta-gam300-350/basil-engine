#ifndef STRUCTURE_CONSTRAINT
#define STRUCTURE_CONSTRAINT

#include <memory>
#include <type_traits>
#include <concepts>

namespace structures {
    template<typename A, typename T>
    concept allocator = std::same_as<typename std::allocator_traits<A>::value_type, T>&& requires(A a, const A ca, typename std::allocator_traits<A>::pointer p, typename std::allocator_traits<A>::size_type n) {
        //pointer and size_type
        typename std::allocator_traits<A>::pointer;
        typename std::allocator_traits<A>::size_type;

        //allocate and deallocate
        { a.allocate(n) } -> std::same_as<typename std::allocator_traits<A>::pointer>;
        a.deallocate(p, n);

        //max_size consistency
        { std::allocator_traits<A>::max_size(a) } -> std::same_as<typename std::allocator_traits<A>::size_type>;

        //equality
        { ca == ca } -> std::convertible_to<bool>;
        { ca != ca } -> std::convertible_to<bool>;

        //rebinding support
        typename std::allocator_traits<A>::template rebind_alloc<char>;
    };
}

#endif 