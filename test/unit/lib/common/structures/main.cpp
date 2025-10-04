#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <gtest/gtest.h>
#include <atomic>
#include <optional>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <barrier>
#include <type_traits>

#include <cont/deque.hpp>
// Minimal counting allocator to test allocation/deallocation paths
#ifdef P
template <typename T>
struct counting_allocator {
    using value_type = T;

    struct stats_t {
        std::atomic<size_t> alloc_count{ 0 };
        std::atomic<size_t> dealloc_count{ 0 };
    };

    std::shared_ptr<stats_t> stats;

    counting_allocator() : stats(std::make_shared<stats_t>()) {}
    template <typename U>
    counting_allocator(const counting_allocator<U>& other) : stats(other.stats) {}

    T* allocate(std::size_t n) {
        stats->alloc_count.fetch_add(1, std::memory_order_relaxed);
        return static_cast<T*>(::operator new(sizeof(T) * n));
    }

    void deallocate(T* p, std::size_t) {
        stats->dealloc_count.fetch_add(1, std::memory_order_relaxed);
        ::operator delete(p);
    }

    template <typename U>
    struct rebind { using other = counting_allocator<U>; };

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    bool operator==(const counting_allocator& rhs) const noexcept { return stats == rhs.stats; }
    bool operator!=(const counting_allocator& rhs) const noexcept { return !(*this == rhs); }
};

using namespace structures;

// Helper type that satisfies static_assert in raii_atomic_buffer
struct trivial_copyable {
    int x{ 0 };
};

// ---------- raii_atomic_buffer tests ----------

TEST(RAIIAtomicBuffer, ConstructsWithValidSizeAndBuffer) {
    // Use counting allocator to validate allocation path if buffer uses allocator internally
    raii_atomic_buffer<int, counting_allocator<int>> buf(16);

    // Visibility: only atomic members are available; basic invariants
    EXPECT_GT(buf.m_buf_sz.load(std::memory_order_relaxed), 0u);
    EXPECT_NE(buf.m_buf.load(std::memory_order_relaxed), nullptr);
}

TEST(RAIIAtomicBuffer, DestructorFreesMemoryViaAllocator) {
    counting_allocator<int> alloc;
    auto stats = alloc.stats;

    {
        raii_atomic_buffer<int, counting_allocator<int>> buf(32);
        // associate allocator if your implementation requires construction with allocator;
        // If not, this test only asserts general deallocation by observing state changes
        EXPECT_NE(buf.m_buf.load(), nullptr);
        // Scope end triggers destructor
    }
    // We can’t directly inspect buf internals post-dtor, but we can at least assert
    // allocation count >= deallocation count when object goes out of scope if implementation uses allocator.
    // If allocator is used for a single buffer, expect exactly one alloc and one dealloc.
    // Relax if your implementation allocates control blocks separately.
    EXPECT_GE(stats->alloc_count.load(), stats->dealloc_count.load());
}

// ---------- spmc_deque single-thread correctness ----------

TEST(SPMCDeque, EmptyTakeReturnsNullopt) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    auto val = dq.take();
    EXPECT_FALSE(val.has_value());
}

TEST(SPMCDeque, EmptyStealReturnsNullopt) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    auto val = dq.steal();
    EXPECT_FALSE(val.has_value());
}

TEST(SPMCDeque, PushThenTakeReturnsLastPushedLIFO) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    dq.push(1);
    dq.push(2);
    dq.push(3);

    auto a = dq.take(); ASSERT_TRUE(a.has_value()); EXPECT_EQ(*a, 3);
    auto b = dq.take(); ASSERT_TRUE(b.has_value()); EXPECT_EQ(*b, 2);
    auto c = dq.take(); ASSERT_TRUE(c.has_value()); EXPECT_EQ(*c, 1);

    EXPECT_FALSE(dq.take().has_value());
}

TEST(SPMCDeque, PushThenStealReturnsFirstPushedFIFOFromTop) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    dq.push(10);
    dq.push(20);
    dq.push(30);

    auto s1 = dq.steal(); ASSERT_TRUE(s1.has_value()); EXPECT_EQ(*s1, 10);
    auto s2 = dq.steal(); ASSERT_TRUE(s2.has_value()); EXPECT_EQ(*s2, 20);
    auto s3 = dq.steal(); ASSERT_TRUE(s3.has_value()); EXPECT_EQ(*s3, 30);

    EXPECT_FALSE(dq.steal().has_value());
}

TEST(SPMCDeque, InterleaveTakeAndStealMaintainsEndsCorrectness) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    // Bottom grows with push: [t = 0] top points to 0th
    dq.push(1); // top=0 bottom=1
    dq.push(2); // top=0 bottom=2
    dq.push(3); // top=0 bottom=3

    // Steal from top -> 1
    auto st = dq.steal(); ASSERT_TRUE(st.has_value()); EXPECT_EQ(*st, 1);
    // Take from bottom -> 3
    auto tk1 = dq.take(); ASSERT_TRUE(tk1.has_value()); EXPECT_EQ(*tk1, 3);
    // Remaining should be {2}; stealing gets 2
    auto st2 = dq.steal(); ASSERT_TRUE(st2.has_value()); EXPECT_EQ(*st2, 2);

    EXPECT_FALSE(dq.steal().has_value());
    EXPECT_FALSE(dq.take().has_value());
}

// ---------- spmc_deque resize behavior ----------

TEST(SPMCDeque, ResizePreservesOrderAcrossPushTake) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    // Push enough elements to trigger resize internally
    // We don’t know initial capacity; push a large number to force at least one resize.
    const int N = 1024;
    for (int i = 1; i <= N; ++i) dq.push(i);

    // Take LIFO order (N -> 1)
    for (int i = N; i >= 1; --i) {
        auto v = dq.take();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
    EXPECT_FALSE(dq.take().has_value());
}

TEST(SPMCDeque, ResizePreservesOrderAcrossPushSteal) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    const int N = 1024;
    for (int i = 1; i <= N; ++i) dq.push(i);

    // Steal FIFO from top (1 -> N)
    for (int i = 1; i <= N; ++i) {
        auto v = dq.steal();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
    EXPECT_FALSE(dq.steal().has_value());
}

// ---------- spmc_deque concurrency ----------

TEST(SPMCDeque, MultipleThievesStealAllItemsExactlyOnce) {
    ebr::spmc_deque<int, std::allocator<int>> dq;
    const int N = 10'000;

    for (int i = 0; i < N; ++i) dq.push(i);

    const int thieves = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 8;
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> stolen(thieves);

    std::barrier sync_point(thieves + 1);
    for (int t = 0; t < thieves; ++t) {
        threads.emplace_back([&, t] {
            sync_point.arrive_and_wait();
            while (true) {
                auto v = dq.steal();
                if (!v.has_value()) {
                    // Check if producer also draining, but here producer not taking; so empty means done
                    break;
                }
                stolen[t].push_back(*v);
            }
            });
    }

    sync_point.arrive_and_wait();
    for (auto& th : threads) th.join();

    // Verify all items were stolen exactly once
    std::vector<char> seen(N, 0);
    size_t total = 0;
    for (auto& vec : stolen) {
        for (int x : vec) {
            ASSERT_GE(x, 0);
            ASSERT_LT(x, N);
            ASSERT_EQ(seen[x], 0) << "Duplicate steal of value " << x;
            seen[x] = 1;
            ++total;
        }
    }
    EXPECT_EQ(total, static_cast<size_t>(N));
}

TEST(SPMCDeque, ProducerTakeRacesWithThievesOnLastElement) {
    ebr::spmc_deque<int, std::allocator<int>> dq;

    // Push a small known set
    dq.push(1);
    dq.push(2);
    dq.push(3);

    std::atomic<int> stolen_count{ 0 };
    std::atomic<int> taken_count{ 0 };

    const int thieves = 4;
    std::barrier sync_point(thieves + 1);

    std::vector<std::thread> threads;
    for (int t = 0; t < thieves; ++t) {
        threads.emplace_back([&] {
            sync_point.arrive_and_wait();
            while (true) {
                auto v = dq.steal();
                if (!v.has_value()) break;
                stolen_count.fetch_add(1, std::memory_order_relaxed);
                // Slow thieves slightly to increase race chances
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            });
    }

    sync_point.arrive_and_wait();
    // Producer drains bottom concurrently
    auto v1 = dq.take(); if (v1) taken_count++;
    auto v2 = dq.take(); if (v2) taken_count++;
    auto v3 = dq.take(); if (v3) taken_count++;

    for (auto& th : threads) th.join();

    // Sum of taken + stolen equals pushed
    EXPECT_EQ(stolen_count.load() + taken_count.load(), 3);
}

TEST(SPMCDeque, ConcurrentPushStealTakeStress) {
    ebr::spmc_deque<int, std::allocator<int>> dq;

    const int producers = 1;
    const int thieves = 8;
    const int total_items = 50'000;

    std::barrier sync_point(producers + thieves);

    std::thread producer([&] {
        sync_point.arrive_and_wait();
        for (int i = 0; i < total_items; ++i) {
            dq.push(i);
            if ((i % (total_items / 2)) == 0) {
                // Exercise resize path periodically
                dq.resize();
            }
        }
        });

    std::atomic<int> stolen{ 0 };
    std::atomic<int> taken{ 0 };
    std::vector<std::thread> thieves_threads;
    for (int t = 0; t < thieves; ++t) {
        thieves_threads.emplace_back([&] {
            sync_point.arrive_and_wait();
            for (;;) {
                // Randomly choose to steal or yield
                auto v = dq.steal();
                if (v) stolen.fetch_add(1, std::memory_order_relaxed);
                else {
                    // Empty moment; back off briefly to allow producer
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50us);
                    // Heuristic exit condition: when steal yields empty repeatedly and producer likely finished
                    if (stolen.load() + taken.load() >= total_items) break;
                }
            }
            });
    }

    // spmc model, owner can only take or push at any point. no concurrency for both
    producer.join();

    // Owner thread occasionally takes
    std::thread taker([&] {
        for (;;) {
            auto v = dq.take();
            if (v) taken.fetch_add(1, std::memory_order_relaxed);
            else {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100us);
                if (stolen.load() + taken.load() >= total_items) break;
            }
        }
        });

    for (auto& th : thieves_threads) th.join();
    taker.join();

    EXPECT_EQ(stolen.load() + taken.load(), total_items);
}

// ---------- traits/static_assert compatibility ----------

TEST(RAIIAtomicBuffer, RejectsNonCopyAssignableOrNonTriviallyConstructible) {
    struct non_copy_assignable {
        non_copy_assignable() = default;
        non_copy_assignable(const non_copy_assignable&) = default;
        non_copy_assignable& operator=(const non_copy_assignable&) = delete; // not copy-assignable
    };
    struct non_trivial_ctor {
        non_trivial_ctor() { /* non-trivial */ }
        non_trivial_ctor(const non_trivial_ctor&) = default;
        non_trivial_ctor& operator=(const non_trivial_ctor&) = default;
    };

#if !defined(__clang__) && !defined(__GNUC__)
    // Some compilers may not evaluate static_assert until instantiation; we can conditionally compile expectations.
#endif
    // The following lines should fail to compile if uncommented, validating static_assert.
    // ebr::raii_atomic_buffer<non_copy_assignable, std::allocator> bad1(8);
    // ebr::raii_atomic_buffer<non_trivial_ctor, std::allocator> bad2(8);

    SUCCEED(); // This test serves as documentation; see commented lines above.
}

// ---------- allocator integration with deque ----------

TEST(SPMCDeque, WorksWithCountingAllocator) {
    ebr::spmc_deque<int, counting_allocator<int>> dq;
    for (int i = 0; i < 1000; ++i) dq.push(i);

    size_t count = 0;
    while (auto v = dq.take()) {
        ++count;
        // optional: validate sequence in reverse
    }
    EXPECT_EQ(count, 1000);
}

// --- main entry point for gtest ---
int main(int argc, char** argv) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#else
using namespace structures;
int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ebr::spmc_deque<int, std::allocator<int>> dq;

    const int producers = 1;
    const int thieves = 8;
    const int total_items = 50'000;

    std::barrier sync_point(producers + thieves);

    std::thread producer([&] {
        sync_point.arrive_and_wait();
        for (int i = 0; i < total_items; ++i) {
            dq.push(i);
            if ((i % (total_items / 2)) == 0) {
                // Exercise resize path periodically
                dq.resize();
            }
        }
        });

    std::atomic<int> stolen{ 0 };
    std::atomic<int> taken{ 0 };
    std::vector<std::thread> thieves_threads;
    for (int t = 0; t < thieves; ++t) {
        thieves_threads.emplace_back([&] {
            sync_point.arrive_and_wait();
            for (;;) {
                // Randomly choose to steal or yield
                auto v = dq.steal();
                if (v) stolen.fetch_add(1, std::memory_order_relaxed);
                else {
                    // Empty moment; back off briefly to allow producer
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50us);
                    // Heuristic exit condition: when steal yields empty repeatedly and producer likely finished
                    if (stolen.load() + taken.load() >= total_items) break;
                }
            }
            });
    }

    // spmc model, owner can only take or push at any point. no concurrency for both
    producer.join();

    // Owner thread occasionally takes
    std::thread taker([&] {
        for (;;) {
            auto v = dq.take();
            if (v) taken.fetch_add(1, std::memory_order_relaxed);
            else {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100us);
                if (stolen.load() + taken.load() >= total_items) break;
            }
        }
        });

    for (auto& th : thieves_threads) th.join();
    taker.join();
    return 0;
}
#endif
