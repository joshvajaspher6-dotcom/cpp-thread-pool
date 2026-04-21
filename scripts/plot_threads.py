#!/usr/bin/env python3
# scripts/plot_threads.py

import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def plot_results(csv_path, output_path):
    if not os.path.exists(csv_path):
        print(f"❌ Error: {csv_path} not found")
        return
    
    df = pd.read_csv(csv_path)
    
    # Filter out rows with missing/zero data
    df = df[df['submit_overhead_ns'] > 0].copy()
    
    if df.empty:
        print("❌ Error: No valid data found in CSV")
        print("Tip: Check that benchmark output contains expected benchmark names")
        return
    
    # Convert to per-task latency (benchmarks run 100 tasks)
    df['submit_per_task'] = df['submit_overhead_ns'] / 100
    df['work_per_task'] = df['concurrent_work_ns'] / 100
    df['async_per_task'] = df['std_async_ns'] / 100
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # Plot 1: Per-task latency
    ax1.plot(df['threads'], df['submit_per_task'], marker='o', label='Submit Overhead')
    ax1.plot(df['threads'], df['work_per_task'], marker='s', label='Concurrent Work')
    ax1.axhline(y=df['async_per_task'].iloc[0], color='red', linestyle='--', label='std::async baseline')
    ax1.set_xlabel('Thread Count')
    ax1.set_ylabel('Latency per Task (ns)')
    ax1.set_title('Task Latency vs Thread Count')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    ax1.set_xscale('log', base=2)
    
    # Plot 2: Speedup
    df['speedup'] = df['async_per_task'] / df['submit_per_task']
    ax2.plot(df['threads'], df['speedup'], marker='^', color='green')
    ax2.axhline(y=1.0, color='red', linestyle='--', label='Break-even (1x)')
    ax2.set_xlabel('Thread Count')
    ax2.set_ylabel('Speedup vs std::async')
    ax2.set_title('Performance Speedup')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    ax2.set_xscale('log', base=2)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"✅ Plot saved to {output_path}")

if __name__ == "__main__":
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "results/thread_benchmark.csv"
    output_path = sys.argv[2] if len(sys.argv) > 2 else "results/throughput_vs_threads.png"
    plot_results(csv_path, output_path)