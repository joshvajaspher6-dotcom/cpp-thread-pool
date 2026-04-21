#!/bin/bash
# scripts/benchmark_threads.sh
set -e

BUILD_DIR="${1:-../build}"
OUTPUT_DIR="${2:-./results}"
mkdir -p "$OUTPUT_DIR"

echo "threads,submit_overhead_ns,concurrent_work_ns,std_async_ns" > "$OUTPUT_DIR/thread_benchmark.csv"

for threads in 1 2 4 8 16 32; do
    echo "🔍 Running benchmark with $threads threads..."
    
    # Run benchmarks and capture CSV output (skip header lines)
    output=$("$BUILD_DIR/bench_thread_pool" --benchmark_format=csv --benchmark_min_time=0.1s 2>/dev/null | grep "^\"")
    
    # Parse with awk: handles quoted names and scientific notation
    # CSV format: "name",iterations,real_time,cpu_time,time_unit,...
    # We want cpu_time (4th field)
    
    submit_overhead=$(echo "$output" | awk -F',' -v t="$threads" \
        '$1 == "\"BM_SubmitOverhead/"t"\"" {printf "%.0f", $4; exit}')
    
    concurrent_work=$(echo "$output" | awk -F',' -v t="$threads" \
        '$1 == "\"BM_ConcurrentWork/"t"\"" {printf "%.0f", $4; exit}')
    
    std_async=$(echo "$output" | awk -F',' \
        '$1 == "\"BM_StdAsyncBaseline\"" {printf "%.0f", $4; exit}')
    
    # Handle empty values (default to 0 if not found)
    submit_overhead=${submit_overhead:-0}
    concurrent_work=${concurrent_work:-0}
    std_async=${std_async:-0}
    
    echo "$threads,$submit_overhead,$concurrent_work,$std_async" >> "$OUTPUT_DIR/thread_benchmark.csv"
done

echo "✅ Results saved to $OUTPUT_DIR/thread_benchmark.csv"
echo ""
echo "📊 Raw results:"
cat "$OUTPUT_DIR/thread_benchmark.csv"