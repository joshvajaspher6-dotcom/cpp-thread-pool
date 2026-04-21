// benchmarks/bench_thread_pool.cpp

#include "../include/thread_pool.hpp"
#include <benchmark/benchmark.h>
#include <future>
#include <vector>

// ✅ Benchmark functions (no extra args needed)
static void BM_SubmitOverhead(benchmark::State& state) {
    // Thread count will be set via ->Arg() below
    int num_threads = static_cast<int>(state.range(0));
    
    for (auto _ : state) {
        cortex::ThreadPool pool(num_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(100);
        
        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit_task([]{ return 0; }));
        }
        for (auto& f : futures) f.get();
    }
}

static void BM_ConcurrentWork(benchmark::State& state) {
    int num_threads = static_cast<int>(state.range(0));
    
    for (auto _ : state) {
        cortex::ThreadPool pool(num_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(100);
        
        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit_task([i]{ return i * 2; }));
        }
        int sum = 0;
        for (auto& f : futures) sum += f.get();
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_StdAsyncBaseline(benchmark::State& state) {
    // std::async doesn't use a pool, so we ignore thread count here
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(100);
        
        for (int i = 0; i < 100; ++i) {
            futures.push_back(std::async(std::launch::async, [i]{ return i * 2; }));
        }
        int sum = 0;
        for (auto& f : futures) sum += f.get();
        benchmark::DoNotOptimize(sum);
    }
}

// ✅ Register benchmarks with predefined thread counts
// This creates variants: BM_SubmitOverhead/1, BM_SubmitOverhead/2, etc.
BENCHMARK(BM_SubmitOverhead)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kNanosecond)
    ->Name("BM_SubmitOverhead");

BENCHMARK(BM_ConcurrentWork)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32)
    ->Unit(benchmark::kNanosecond)
    ->Name("BM_ConcurrentWork");

BENCHMARK(BM_StdAsyncBaseline)
    ->Unit(benchmark::kNanosecond)
    ->Name("BM_StdAsyncBaseline");

BENCHMARK_MAIN();