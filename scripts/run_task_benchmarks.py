
import subprocess
import csv
import io
import os
import sys
import matplotlib.pyplot as plt
import pandas as pd

def main():
 
    script_dir = os.path.dirname(os.path.realpath(__file__))
    bin_path = os.path.join(script_dir, '..', 'build', 'bench_thread_pool')
    
    if not os.path.exists(bin_path):
        print(f"❌ Binary not found at: {bin_path}")
        sys.exit(1)

    print(f"🚀 Running Benchmarks: {bin_path}...")
    try:
       
        result = subprocess.run(
            [bin_path, '--benchmark_format=csv', '--benchmark_min_time=0.1s'],
            capture_output=True, text=True, check=True
        )
        output = result.stdout
    except subprocess.CalledProcessError as e:
        print(f"❌ Benchmark failed: {e.stderr}")
        sys.exit(1)

   
    print("🔍 Parsing results...")
    data = {}
    reader = csv.reader(io.StringIO(output))
    for row in reader:
        if len(row) >= 4:
            name = row[0].strip().strip('"')
            try:
                cpu_time = float(row[3]) # 4th column is CPU time in ns
                data[name] = cpu_time
            except ValueError:
                continue

    # 3. Calculate Metrics
    print("📊 Calculating metrics...")
    results = []
    
   
    tasks = [
        ("Tiny", 0, "BM_TinyTasks", "BM_StdAsyncTiny"),
        ("Medium", 1000, "BM_MediumTasks", "BM_StdAsyncMedium"),
        ("Large", 1000000, "BM_LargeTasks", "BM_StdAsyncLarge")
    ]
    
    threads_list = [1, 4, 8, 12]

    for label, ns, bench_name, async_name in tasks:
        async_time = data.get(async_name, 0)
        
        for t in threads_list:
            key = f"{bench_name}/{t}"
            cpu_time = data.get(key, 0)
            
            if cpu_time > 0:
               
                throughput = (100 * 1_000_000_000) / cpu_time
                latency = cpu_time / 100
                overhead = ((cpu_time - async_time) / async_time) * 100 if async_time > 0 else 0
                
                results.append({
                    'task_size_ns': ns, 
                    'threads': t, 
                    'throughput': throughput, 
                    'latency': latency,
                    'overhead': overhead
                })
                print(f"  ✅ {label} / {t}T: Latency={latency:.0f}ns, Overhead={overhead:.1f}%")
            else:
                print(f"  ⚠️ Missing data for {key}")

  
    out_dir = os.path.join(script_dir, '..', 'results')
    os.makedirs(out_dir, exist_ok=True)
    csv_path = os.path.join(out_dir, 'task_size_benchmark.csv')
    
    df = pd.DataFrame(results)
    df.to_csv(csv_path, index=False)
    print(f"\n✅ CSV saved to {csv_path}")

    print("📈 Generating plot...")
    fig, ax = plt.subplots(figsize=(10, 6))
    
    for label, group in df.groupby('task_size_ns'):
        
        name = {0: 'Tiny', 1000: 'Medium', 1000000: 'Large'}[label]
        ax.plot(group['threads'], group['latency'], marker='o', label=f'{name} Task Latency')
        
    ax.set_xlabel('Threads')
    ax.set_ylabel('Latency per Task (ns)')
    ax.set_title('Task Latency vs Thread Count by Task Size')
    ax.legend()
    ax.grid(True, alpha=0.3)
    
    plot_path = os.path.join(out_dir, 'task_size_analysis.png')
    plt.savefig(plot_path, dpi=150)
    print(f"✅ Plot saved to {plot_path}")

if __name__ == "__main__":
    main()