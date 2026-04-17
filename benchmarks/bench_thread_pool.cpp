#include "../include/thread_pool.hpp"
#include <benchmark/benchmark.h>
#include <future>
#include <vector>


static void BM_SubmitOverhead(benchmark::State& state) {
    cortex::ThreadPool pool(4);
    for (auto _ : state) {
        std::vector<std::future<int>> futures; // Changed to future<int>
        futures.reserve(100);
        for (int i = 0; i < 100; ++i) {
            
            futures.push_back(pool.submit_task([]{ return 0; })); 
        }
        for (auto& f : futures) f.get();
    }
}
BENCHMARK(BM_SubmitOverhead);


static void BM_ConcurrentWork(benchmark::State& state) {
    cortex::ThreadPool pool(4);
    for (auto _ : state) {
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
BENCHMARK(BM_ConcurrentWork);


static void BM_StdAsyncBaseline(benchmark::State& state) {
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
BENCHMARK(BM_StdAsyncBaseline);

BENCHMARK_MAIN();