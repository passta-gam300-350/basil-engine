#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <random>

// Include your hash?table implementation header:
#include "tempht.hpp"

static constexpr int NUM_THREADS = 8;
static constexpr int OPS_PER_THREAD = 100000;
static constexpr int KEY_RANGE = 10000;

class HashTableTest : public ::testing::Test {
protected:
    // Table with exposed debug APIs:
    //   bucket_count(): current bucket array size
    //   force_hazard_scan(): run one global SMR scan
    LockFreeHashTable<int> ht{ /*initial_capacity=*/2 };

    // Barrier to synchronize thread start
    std::atomic<int> ready{ 0 };
    void wait_for_all(int n) {
        if (++ready == n) return;
        while (ready < n) { /* spin */ }
    }
};


// 1. Single?Thread Sanity
TEST_F(HashTableTest, SingleThreadBasic) {
    EXPECT_FALSE(ht.contains(42));
    EXPECT_TRUE(ht.insert(42));
    EXPECT_TRUE(ht.contains(42));
    EXPECT_FALSE(ht.insert(42));  // duplicate
    EXPECT_TRUE(ht.remove(42));
    EXPECT_FALSE(ht.contains(42));
    EXPECT_FALSE(ht.remove(42));  // already gone
}

// 2. Disjoint Inserts
TEST_F(HashTableTest, ConcurrentDisjointInserts) {
    const int perThread = 1000;
    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            wait_for_all(NUM_THREADS);
            int base = t * perThread;
            for (int i = 0; i < perThread; ++i) {
                EXPECT_TRUE(ht.insert(base + i));
            }
            });
    }
    for (auto& th : threads) th.join();

    // Verify
    for (int t = 0; t < NUM_THREADS; ++t) {
        int base = t * perThread;
        for (int i = 0; i < perThread; ++i) {
            EXPECT_TRUE(ht.contains(base + i));
        }
    }
}

// 3. Disjoint Removes
TEST_F(HashTableTest, ConcurrentDisjointRemoves) {
    const int totalKeys = NUM_THREADS * 500;
    // Prepopulate
    for (int i = 0; i < totalKeys; ++i) {
        ASSERT_TRUE(ht.insert(i));
    }

    std::vector<std::thread> threads;
    const int perThread = totalKeys / NUM_THREADS;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            wait_for_all(NUM_THREADS);
            int start = t * perThread;
            for (int i = 0; i < perThread; ++i) {
                EXPECT_TRUE(ht.remove(start + i));
            }
            });
    }
    for (auto& th : threads) th.join();

    // Verify none remain
    for (int i = 0; i < totalKeys; ++i) {
        EXPECT_FALSE(ht.contains(i));
    }
}

// 4. Mixed Operations on Hot Key Set
TEST_F(HashTableTest, MixedOperationsHighContention) {
    std::atomic<bool> keepRunning{ true };
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&]() {
            wait_for_all(NUM_THREADS);
            std::mt19937_64 rnd(std::random_device{}());
            std::uniform_int_distribution<int> keyDist(0, KEY_RANGE - 1);
            std::uniform_int_distribution<int> opDist(0, 2);

            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                int key = keyDist(rnd);
                switch (opDist(rnd)) {
                case 0: ht.insert(key); break;
                case 1: ht.remove(key); break;
                case 2: ht.contains(key); break;
                }
            }
            });
    }
    for (auto& th : threads) th.join();

    // At least no crash; optionally spot?check a few keys:
    for (int k = 0; k < 100; ++k) {
        ht.contains(k);
    }
}

// 5. Resize Stress
TEST_F(HashTableTest, ForceResizing) {
    const int totalInserts = 200000;
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            wait_for_all(NUM_THREADS);
            int start = t * (totalInserts / NUM_THREADS);
            int end = start + (totalInserts / NUM_THREADS);
            for (int i = start; i < end; ++i) {
                ht.insert(i);
            }
            });
    }
    for (auto& th : threads) th.join();

    // Table should have grown
    EXPECT_GT(ht.bucket_count(), 2u);

    // Verify all keys
    for (int i = 0; i < totalInserts; ++i) {
        EXPECT_TRUE(ht.contains(i));
    }
}

// 6. SMR Reclamation Verification
TEST_F(HashTableTest, HazardPointerReclaim) {
    // Rapid insert/remove churn on small key range
    const int CYCLES = 50000;
    for (int i = 0; i < CYCLES; ++i) {
        ht.insert(i % 100);
        ht.remove(i % 100);
    }
    // Force a global scan
    ht.force_hazard_scan();
    // The test?only hook returns number of pending retired nodes
    size_t pending = ht.debug_retired_count<Node>();
    // Should not grow without bound
    EXPECT_LE(pending, ht.retire_threshold());
}

// 7. Combined Grow + Reclaim
TEST_F(HashTableTest, CombinedResizeAndReclaim) {
    // Mix large?scale inserts triggering resize and periodic clears
    const int INS = 50000;
    const int MOD = 10000;
    std::thread inserter([&]() {
        for (int i = 0; i < INS; ++i) {
            ht.insert(i);
            if (i % MOD == 0) {
                // periodically remove half
                for (int j = 0; j < i / 2; ++j)
                    ht.remove(j);
                ht.force_hazard_scan();
            }
        }
        });
    inserter.join();

    // Final reclaim
    ht.force_hazard_scan();
    EXPECT_LE(ht.debug_retired_count<Node>(), ht.retire_threshold());
}


//==============================================================================
// main()
//==============================================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
