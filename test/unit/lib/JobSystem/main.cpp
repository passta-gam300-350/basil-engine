/******************************************************************************/
/*!
\file   main.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Job system unit tests

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <jobsystem.hpp> 

// Utility: wait until a condition or timeout
template <typename Pred>
bool wait_until(Pred&& pred, int ms = 2000) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(ms))
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

// ------------------ TESTS ------------------

TEST(JobSystem, SingleJobCompletes) {
    JobSystem js(2, 32);

    std::atomic<bool> ran{ false };

    auto job = js.submit(0, {}, [&](JobSystem& sys) -> JobTask {
        ran = true;
        co_return;
        }, std::ref(js));

    ASSERT_TRUE(wait_until([&] { return ran.load(); }));
    ASSERT_TRUE(js.is_done(job));
}

TEST(JobSystem, MultipleIndependentJobs) {
    JobSystem js(4, 1000);

    std::atomic<int> counter{ 0 };
    const int N = 1000;

    std::vector<JobID> jobs;
    for (int i = 0; i < N; ++i) {
        jobs.push_back(js.submit(i % 4, {}, [&](JobSystem& sys, int id) -> JobTask {
            counter.fetch_add(1);
            co_return;
            }, std::ref(js), i));
    }

    ASSERT_TRUE(wait_until([&] { return counter.load() == N; }));
    for (auto j : jobs) ASSERT_TRUE(js.is_done(j));
}

TEST(JobSystem, JobDependencies) {
    JobSystem js(4, 64);

    std::atomic<bool> aDone{ false }, bDone{ false }, cDone{ false };

    auto jobA = js.submit(0, {}, [&](JobSystem& sys) -> JobTask {
        aDone = true;
        co_return;
        }, std::ref(js));

    auto jobB = js.submit(1, {}, [&](JobSystem& sys) -> JobTask {
        bDone = true;
        co_return;
        }, std::ref(js));

    auto jobC = js.submit(2, { jobA, jobB }, [&](JobSystem& sys) -> JobTask {
        EXPECT_TRUE(aDone.load());
        EXPECT_TRUE(bDone.load());
        cDone = true;
        co_return;
        }, std::ref(js));

    ASSERT_TRUE(wait_until([&] { return cDone.load(); }));
    ASSERT_TRUE(js.is_done(jobC));
}

TEST(JobSystem, CoroutineYielding) {
    JobSystem js(2, 32);

    std::atomic<int> steps{ 0 };

    auto job = js.submit(0, {}, [&](JobSystem& sys) -> JobTask {
        steps.fetch_add(1);
        co_await sys.yield();
        steps.fetch_add(1);
        co_return;
        }, std::ref(js));

    ASSERT_TRUE(wait_until([&] { return steps.load() == 2; }));
}

TEST(JobSystem, ScheduleOnDifferentWorker) {
    JobSystem js(2, 32);

    std::atomic<int> ranOn{ -1 };

    auto job = js.submit(0, {}, [&](JobSystem& sys) -> JobTask {
        co_await sys.schedule_on(1);
        ranOn = 1;
        co_return;
        }, std::ref(js));

    ASSERT_TRUE(wait_until([&] { return ranOn.load() == 1; }));
}

TEST(JobSystem, WorkStealing) {
    JobSystem js(4, 128);

    std::atomic<int> counter{ 0 };
    const int N = 20;

    // Submit all jobs to worker 0, others should steal
    for (int i = 0; i < N; ++i) {
        js.submit(0, {}, [&](JobSystem& sys, int id) -> JobTask {
            counter.fetch_add(1);
            co_return;
            }, std::ref(js), i);
    }

    ASSERT_TRUE(wait_until([&] { return counter.load() == N; }));
}

// A helper coroutine function that takes its own copies of i and dep
void chain_step(int my_i, std::atomic<int>& last) {
    // At this point, all static dependencies have been resolved by the scheduler
    EXPECT_EQ(last.load(), my_i - 1);
    last = my_i;
}

TEST(JobSystem, ChainOfDependencies) {
    JobSystem js(8, 500);

    const int N = 500;
    std::atomic<int> last{ 0 };

    // First job in the chain
    JobID prev = js.submit(0, {}, [&](JobSystem& sys) -> JobTask {
        last = 1;
        co_return;
        }, std::ref(js));

    // Subsequent jobs depend on the previous one
    for (int i = 2; i <= N; ++i) {
        prev = js.submit(i % 8, { prev }, [](int my_i, std::atomic<int>& last) -> JobTask {
            chain_step(my_i, last);
            co_return;
            }, i, std::ref(last));
    }

    js.wait_for(prev);

    ASSERT_TRUE(wait_until([&] { return last.load() == N; }));
}


TEST(JobSystem, ManyJobsStress) {
    JobSystem js(8, 2048);

    const int N = 1000;
    std::atomic<int> counter{ 0 };

    for (int i = 0; i < N; ++i) {
        js.submit(i % 8, {}, [&](JobSystem& sys) -> JobTask {
            counter.fetch_add(1);
            co_return;
            }, std::ref(js));
    }

    ASSERT_TRUE(wait_until([&] { return counter.load() == N; }, 5000));
}

// --- main entry point for gtest ---
int main(int argc, char** argv) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
