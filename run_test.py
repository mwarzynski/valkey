#!/usr/bin/env python3
import subprocess
import sys

def run_benchmark(num_runs):
    command = "./src/valkey-benchmark -h localhost -p 6379 -t get,set -n 1000000 -c 20"
    results = []
    
    for i in range(num_runs):
        print(f"Running iteration {i+1}/{num_runs}...")
        process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        output = process.stdout
        results.append(output)
        print(output)
        
    return results

def main():
    if len(sys.argv) != 2:
        print("Usage: python run_benchmark.py <num_runs>")
        sys.exit(1)
    
    try:
        num_runs = int(sys.argv[1])
    except ValueError:
        print("Error: num_runs must be an integer")
        sys.exit(1)
    
    results = run_benchmark(num_runs)
    
    with open("benchmark_results.txt", "w") as f:
        for idx, output in enumerate(results, start=1):
            f.write(f"--- Run {idx} ---\n")
            f.write(output)
            f.write("\n\n")
    
    print("Benchmark results saved to benchmark_results.txt")

if __name__ == "__main__":
    main()
