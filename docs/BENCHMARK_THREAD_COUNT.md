# Thread Count Benchmark Results

## Overview
This benchmark measures how the thread pool's performance scales as we increase the number of worker threads from 1 to 32, using empty task submission and small compute workloads.

## Test Setup
| Parameter | Value |
|-----------|-------|
| **CPU** | 12-core (6P+6E), 4.0 GHz |
| **Compiler** | GCC 15.2.1 with `-O3` |
| **Workload** | 100 tasks per iteration |
| **Benchmark Duration** | 0.1s minimum per test |
| **Metrics** | CPU time (ns), throughput (tasks/sec), speedup vs `std::async` |

## Benchmarks
| Benchmark | Description |
|-----------|-------------|
| `BM_SubmitOverhead` | Empty lambda tasks: `[](){ return 42; }` |
| `BM_ConcurrentWork` | Small compute: `[](){ return i*2; }` |
| `BM_StdAsyncBaseline` | System baseline using `std::async(std::launch::async, ...)` |

## Results Summary

| Threads | Submit Latency (ns/task) | Work Latency (ns/task) | Speedup vs std::async |
|---------|-------------------------|----------------------|---------------------|
| **1** | ~2,000 | ~1,700 | **14.5x** ✅ |
| **2** | ~2,600 | ~2,700 | **11.1x** ✅ |
| **4** | ~2,800 | ~2,800 | **10.1x** ✅ |
| **8** | ~4,200 | ~4,400 | **6.7x** ✅ |
| **16** | ~7,200 | ~6,500 | **4.0x** ⚠️ |
| **32** | ~11,700 | ~11,000 | **2.4x** ⚠️ |

> **Note**: `std::async` baseline latency: ~26,000 ns/task

## Key Findings

### 1. Peak Performance at 1-4 Threads
- Maximum speedup: **14.5x** over `std::async` at 1 thread
- Latency remains under **3,000 ns/task** up to 4 threads
- Thread pool overhead is minimal in this range

### 2. Contention Threshold at 8+ Threads
- Latency increases **2.5x** from 4→8 threads (2,800ns → 4,200ns)
- Speedup drops from 10x to 6.7x
- **Cause**: Lock contention on shared task queue

### 3. Diminishing Returns at 16-32 Threads
- At 32 threads, latency is **5.8x higher** than at 1 thread
- Speedup still positive (2.4x) but barely worth the overhead
- Thread pool still outperforms `std::async` baseline

### 4. Always Better Than std::async
- Even at worst case (32 threads), pool is **2.4x faster**
- Thread reuse eliminates thread creation overhead
- Pool overhead never exceeds baseline

## Graph Analysis

### Left Plot: Task Latency vs Thread Count

Latency (ns)
   ^
   |                                    std::async baseline (~26,000ns)
26k|----------------------------------------------------------------------------
   |
   |                                    • (32T, ~11,700ns)
10k|                           • (16T)
   |                    • (8T)
 3k|        • (4T)  • (2T)  • (1T)
   +--------------------------------------------------------------------------> Threads (log scale)
     1    2    4    8    16   32


- **Blue line (Submit Overhead)**: Rises from ~2,000ns to ~10,000ns
- **Orange line (Concurrent Work)**: Similar trajectory
- **Red dashed line (std::async)**: Flat at ~26,000ns (constant overhead)
- **Key insight**: Pool latency stays well below baseline across all thread counts

### Right Plot: Performance Speedup


Speedup (x)
   ^
15 | • (1T: 14.5x)
   |
10 |       • (4T: 10.1x)
   |
 5 |                • (8T: 6.7x)
   |                          • (16T: 4.0x)
 1 |------------------------------------ Break-even (1x)
   |                                    • (32T: 2.4x)
   +--------------------------------------------------------------------------> Threads (log scale)
     1    2    4    8    16   32


     - **Green line**: Starts at 14.5x, drops to 2.4x at 32 threads
- **Red dashed line**: Break-even point (1x speedup)
- **Key insight**: Speedup degrades logarithmically, never crosses break-even

## Recommendations

### ✅ When to Use This Thread Pool
1. **For low-latency workloads**: Use **4-8 threads** for optimal balance
2. **For throughput workloads**: Use **8-12 threads** (matches your CPU's P-cores)
3. **For mixed workloads**: Use `std::thread::hardware_concurrency()` (12 on this system)

### ⚠️ When to Be Cautious
1. **Avoid >16 threads** unless tasks are I/O-bound or very large (>1ms)
2. **For tiny tasks (<100ns)**: Consider batching 10-100 tasks together
3. **For latency-critical code**: Profile with your specific workload

### 🔧 Optimization Opportunities
The rising latency curve indicates **lock contention**. Potential fixes:
1. **Work stealing**: Reduce contention with per-thread local queues
2. **Lock-free queues**: Replace mutex with atomic operations
3. **Batch submission**: Submit multiple tasks per lock acquisition

## Conclusion
The thread pool provides **excellent performance** (10-14x speedup) at 1-4 threads and remains **superior to std::async** at all thread counts. The optimal configuration for this 12-core CPU is **8 threads**, balancing latency and parallelism.

---

**Benchmark Date**: 2026-04-21  
**Test Binary**: `build/bench_thread_pool`  
**Script**: `scripts/benchmark_threads.py`  
**Results**: `results/thread_benchmark.csv`, `results/thread_benchmark.png`