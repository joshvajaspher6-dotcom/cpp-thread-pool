#!/usr/bin/env python3
import subprocess
import os
import sys
import matplotlib.pyplot as plt
import pandas as pd

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bin_path = os.path.join(script_dir, '..', 'build', 'bench_thread_pool')
    
    if not os.path.exists(bin_path):
        print(f"❌ Binary not found: {bin_path}")
        sys.exit(1)

    print("🚀 Running Thread Count Benchmarks...")
    try:
        result = subprocess.run(
            [bin_path, '--benchmark_format=csv', '--benchmark_min_time=0.1s'],
            capture_output=True, text=True, check=True
        )
        output = result.stdout
    except subprocess.CalledProcessError as e:
        print(f"❌ Benchmark failed: {e.stderr}")
        sys.exit(1)

    # 🔍 DEBUG: Show exactly what the binary outputs
    lines = output.strip().split('\n')
    print(f"📦 Received {len(lines)} lines of CSV output")
    for line in lines[:5]:
        print(f"  > {line}")

    # Parse safely (Google Benchmark CSV: "name",iterations,real_time,cpu_time,time_unit,...)
    data = {}
    for line in lines:
        if not line.startswith('"'): continue  # Skip header/empty lines
        parts = line.split(',')
        if len(parts) < 4: continue
        
        name = parts[0].strip('"')
        try:
            cpu_time = float(parts[3])  # Column 3 is cpu_time in ns
            data[name] = cpu_time
        except ValueError:
            pass

    print(f"🔑 Parsed {len(data)} benchmarks: {list(data.keys())}")

    # Extract metrics
    results = []
    for t in [1, 2, 4, 8, 16, 32]:
        submit_key = f"BM_SubmitOverhead/{t}"
        work_key = f"BM_ConcurrentWork/{t}"
        async_key = "BM_StdAsyncBaseline"

        submit_time = data.get(submit_key, 0)
        work_time = data.get(work_key, 0)
        async_time = data.get(async_key, 0)

        if submit_time > 0 or work_time > 0:
            submit_lat = submit_time / 100
            work_lat = work_time / 100
            speedup = (async_time / 100) / submit_lat if submit_lat > 0 else 0
            results.append({
                'threads': t, 'submit_latency': submit_lat,
                'work_latency': work_lat, 'speedup': speedup
            })
            print(f"  ✅ {t}T: Latency={submit_lat:.0f}ns, Speedup={speedup:.1f}x")
        else:
            print(f"  ⚠️ Missing data for {t}T (Expected key: {submit_key})")

    if not results:
        print("❌ No data parsed! Check the 'Parsed benchmarks' list above.")
        sys.exit(1)

    # Save & Plot
    df = pd.DataFrame(results)
    out_dir = os.path.join(script_dir, '..', 'results')
    os.makedirs(out_dir, exist_ok=True)
    df.to_csv(os.path.join(out_dir, 'thread_benchmark.csv'), index=False)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    ax1.plot(df['threads'], df['submit_latency'], marker='o', label='Submit')
    ax1.plot(df['threads'], df['work_latency'], marker='s', label='Work')
    ax1.set_xlabel('Threads'); ax1.set_ylabel('Latency (ns)')
    ax1.set_title('Latency vs Threads'); ax1.legend(); ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log', base=2)

    ax2.plot(df['threads'], df['speedup'], marker='^', color='green')
    ax2.axhline(y=1.0, color='red', linestyle='--', label='Break-even (1x)')
    ax2.set_xlabel('Threads'); ax2.set_ylabel('Speedup vs std::async')
    ax2.set_title('Speedup'); ax2.legend(); ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log', base=2)

    plt.tight_layout()
    plt.savefig(os.path.join(out_dir, 'thread_benchmark.png'), dpi=150)
    print(f"✅ Saved to results/thread_benchmark.csv & .png")

if __name__ == "__main__":
    main()