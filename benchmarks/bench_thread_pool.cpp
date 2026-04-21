// benchmarks/bench_thread_pool.cpp
#include "../include/thread_pool.hpp"
#include <benchmark/benchmark.h>
#include <future>
#include <vector>
#include <chrono>

static void busy_wait_ns(int64_t ns) {
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < std::chrono::nanoseconds(ns)) {
        benchmark::DoNotOptimize(start);
    }
}

static void BM_TinyTasks(benchmark::State& state) {
    int nt = static_cast<int>(state.range(0));
    for (auto _ : state) {
        cortex::ThreadPool pool(nt);
        std::vector<std::future<int>> futs;
        futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(pool.submit_task([]{return 42;}));
        int s=0; for(auto&f:futs) s+=f.get();
        benchmark::DoNotOptimize(s);
    }
}

static void BM_MediumTasks(benchmark::State& state) {
    int nt = static_cast<int>(state.range(0));
    for (auto _ : state) {
        cortex::ThreadPool pool(nt);
        std::vector<std::future<int>> futs;
        futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(pool.submit_task([i]{ busy_wait_ns(1000); return i*2; }));
        int s=0; for(auto&f:futs) s+=f.get();
        benchmark::DoNotOptimize(s);
    }
}

static void BM_LargeTasks(benchmark::State& state) {
    int nt = static_cast<int>(state.range(0));
    for (auto _ : state) {
        cortex::ThreadPool pool(nt);
        std::vector<std::future<int>> futs;
        futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(pool.submit_task([i]{ busy_wait_ns(1000000); return i*3; }));
        int s=0; for(auto&f:futs) s+=f.get();
        benchmark::DoNotOptimize(s);
    }
}

static void BM_StdAsyncTiny(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(std::async(std::launch::async, []{return 42;}));
        int s=0; for(auto&f:futs) s+=f.get(); benchmark::DoNotOptimize(s);
    }
}
static void BM_StdAsyncMedium(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(std::async(std::launch::async, [i]{ busy_wait_ns(1000); return i*2; }));
        int s=0; for(auto&f:futs) s+=f.get(); benchmark::DoNotOptimize(s);
    }
}
static void BM_StdAsyncLarge(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(std::async(std::launch::async, [i]{ busy_wait_ns(1000000); return i*3; }));
        int s=0; for(auto&f:futs) s+=f.get(); benchmark::DoNotOptimize(s);
    }
}

// ✅ Day 25: Thread Count Scaling Benchmarks

// Submit overhead: empty tasks, many threads
static void BM_SubmitOverhead(benchmark::State& state) {
    int nt = static_cast<int>(state.range(0));
    for (auto _ : state) {
        cortex::ThreadPool pool(nt);
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(pool.submit_task([]{return 42;}));
        int s=0; for(auto&f:futs) s+=f.get();
        benchmark::DoNotOptimize(s);
    }
}

// Concurrent work: small compute, many threads
static void BM_ConcurrentWork(benchmark::State& state) {
    int nt = static_cast<int>(state.range(0));
    for (auto _ : state) {
        cortex::ThreadPool pool(nt);
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(pool.submit_task([i]{ 
            benchmark::DoNotOptimize(i*2); 
            return i*2; 
        }));
        int s=0; for(auto&f:futs) s+=f.get();
        benchmark::DoNotOptimize(s);
    }
}

// Single std::async baseline for Day 25
static void BM_StdAsyncBaseline(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::future<int>> futs; futs.reserve(100);
        for(int i=0;i<100;++i) futs.push_back(std::async(std::launch::async, []{return 42;}));
        int s=0; for(auto&f:futs) s+=f.get(); benchmark::DoNotOptimize(s);
    }
}

// Register Day 25 benchmarks with MORE thread counts (1→32)
BENCHMARK(BM_SubmitOverhead)->Args({1})->Args({2})->Args({4})->Args({8})->Args({16})->Args({32})->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ConcurrentWork)->Args({1})->Args({2})->Args({4})->Args({8})->Args({16})->Args({32})->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdAsyncBaseline)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_TinyTasks)->Arg(1)->Arg(4)->Arg(8)->Arg(12)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MediumTasks)->Arg(1)->Arg(4)->Arg(8)->Arg(12)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_LargeTasks)->Arg(1)->Arg(4)->Arg(8)->Arg(12)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdAsyncTiny)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdAsyncMedium)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_StdAsyncLarge)->Unit(benchmark::kNanosecond);
BENCHMARK_MAIN();
