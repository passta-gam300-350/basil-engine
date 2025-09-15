// test_jobsystem_gtest.cpp

#include "jobsystem.hpp"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// Helper: wait up to timeout for future to become ready
template<typename T>
bool wait_ready(std::future<T>& fut,
    std::chrono::milliseconds timeout = 500ms)
{
    return fut.wait_for(timeout) == std::future_status::ready;
}

//==============================================================================
// 1) Basic value-returning job
//==============================================================================
TEST(JobSystemTest, BasicValueJob) {
    JobSystem js(2);
    auto [h, f] = js.schedule([](int a, int b) { return a + b; }, 6, 7);
    EXPECT_TRUE(wait_ready(f));
    EXPECT_EQ(f.get(), 13);
    js.shutdown();
}

//==============================================================================
// 2) Void-returning job
//==============================================================================
TEST(JobSystemTest, VoidJob) {
    JobSystem js(2);
    bool flag = false;
    auto [h, f] = js.schedule([&] { flag = true; });
    EXPECT_TRUE(wait_ready(f));
    // No return value; just ensure completion
    f.get();
    EXPECT_TRUE(flag);
    js.shutdown();
}

//==============================================================================
// 3) Single dependency (order guarantee)
//==============================================================================
TEST(JobSystemTest, SingleDependency) {
    JobSystem js(4);
    bool flag = false;

    auto [hA, fA] = js.schedule([] { return true; });
    auto [hB, fB] = js.scheduleWithDeps(
        { hA },
        [&flag, &fA] { flag = fA.get(); }
    );

    EXPECT_TRUE(wait_ready(fB));
    fB.get();
    EXPECT_TRUE(flag);
    js.shutdown();
}

//==============================================================================
// 4) Multiple dependencies (all prerequisites must finish first)
//==============================================================================
TEST(JobSystemTest, MultipleDependencies) {
    JobSystem js(4);
    bool flag = false;

    auto [h1, f1] = js.schedule([] { return 3; });
    auto [h2, f2] = js.schedule([] { return 3; });

    auto [h3, f3] = js.scheduleWithDeps(
        { h1, h2 },
        [&flag, &f1, &f2] { flag = f1.get() == f2.get(); }
    );

    EXPECT_TRUE(wait_ready(f3));
    f3.get();
    EXPECT_TRUE(flag);
    js.shutdown();
}

//==============================================================================
// 5) Deep dependency chain
//==============================================================================
TEST(JobSystemTest, DeepDependencyChain) {
    const int DEPTH = 50;
    JobSystem js(4);

    // Kick off A0
    JobHandle hPrev;
    std::future<int> fPrev;
    {
        auto [h0, f0] = js.schedule([] { return 1; });
        hPrev = h0;
        fPrev = std::move(f0);
    }

    // Build chain A1..A(DEPTH-1)
    for (int i = 1; i < DEPTH; ++i) {
        int prevVal = fPrev.get();  // capture before scheduling next
        auto [hNext, fNext] = js.scheduleWithDeps(
            { hPrev },
            [prevVal] { return prevVal + 1; }
        );
        hPrev = hNext;
        fPrev = std::move(fNext);
    }

    // Final result must be DEPTH
    EXPECT_EQ(fPrev.get(), DEPTH);
    js.shutdown();
}

//==============================================================================
// 6) Branching dependencies (fan-out + fan-in)
//==============================================================================
TEST(JobSystemTest, BranchingDependencies) {
    JobSystem js(4);

    // Root job
    auto [hRoot, fRoot] = js.schedule([] { return 10; });
    int rootVal = fRoot.get();

    // Fan-out
    auto [hA, fA] = js.scheduleWithDeps(
        { hRoot },
        [rootVal] { return rootVal + 1; }
    );
    auto [hB, fB] = js.scheduleWithDeps(
        { hRoot },
        [rootVal] { return rootVal + 2; }
    );
    int valA = fA.get();
    int valB = fB.get();

    // Fan-in
    auto [hC, fC] = js.scheduleWithDeps(
        { hA, hB },
        [valA, valB] { return valA * valB; }
    );
    EXPECT_EQ(fC.get(), (rootVal + 1) * (rootVal + 2));

    js.shutdown();
}

//==============================================================================
// 7) Nested scheduling (jobs that spawn jobs)
//==============================================================================
TEST(JobSystemTest, NestedScheduling) {
    JobSystem js(4);

    auto [hTop, fTop] = js.scheduleWithDeps({}, [&js] {
        // This runs in a worker thread
        auto [hSub, fSub] = js.schedule([] { return 99; });
        return fSub.get() + 1;
        });

    EXPECT_EQ(fTop.get(), 100);
    js.shutdown();
}

//==============================================================================
// 8) Stress test (many small jobs)
//==============================================================================
TEST(JobSystemTest, StressTest) {
    const int N = 10000;
    JobSystem js(8);

    std::atomic<int> counter{ 0 };
    std::vector<std::future<void>> futures;
    futures.reserve(N);

    for (int i = 0; i < N; ++i) {
        auto [h, f] = js.schedule(
            [&counter] { counter.fetch_add(1, std::memory_order_relaxed); }
        );
        futures.push_back(std::move(f));
    }

    for (auto& f : futures) {
        EXPECT_TRUE(wait_ready(f, 5000ms));
        f.get();
    }

    EXPECT_EQ(counter.load(), N);
    js.shutdown();
}

//==============================================================================
// 9) Shutdown drains remaining jobs
//==============================================================================
TEST(JobSystemTest, ShutdownDrainsJobs) {
    JobSystem js(2);

    auto [h, f] = js.schedule([] {
        std::this_thread::sleep_for(10ms);
        return 123;
        });

    // Call shutdown before waiting on future
    js.shutdown();

    // The shutdown should drain and run this job
    EXPECT_EQ(f.get(), 123);
}

//==============================================================================
// main()
//==============================================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
