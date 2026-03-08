Custom memory management suite optimizing for cache locality. 

Designed for low-latency systems (HFT/Game Engines).

These allocators achieve up to 7x performance gains over the standard library in non-uniform allocation workloads.

# Core Architecture
1. **Arena (Bump)** Allocator: Linear allocation. Ideal for per-frame or per-request lifecycles.
2. **Typed Pool Allocator:** Fixed-size block management using a stack-based freelist.
3. **Segregated Size-Class Pool:** A multi-pool system that routes allocation requests to the smallest viable power-of-two block, minimizing external fragmentation.

# Benchmarks
Executed on a 24-core 3418 MHz CPU with a 30MB L3 Cache. 10k allocations

Uniform Workload
| Allocator | Time (ns) | Speedup | Iterations |
| :--- | :--- | :--- | :--- |
| Standard Malloc | 265,379 | 1.00x | 2,036 |
| Arena (Bump) | 67,347 | 3.94x | 8,960 |
| Typed Pool | 71,965 | 3.69x | 8,960 |
| Untyped Pool | 83,324 | 3.18x | 8,960 |

Non-Uniform (Random) Workload
| Allocator | Time (ns) | Speedup | Iterations |
| :--- | :--- | :--- | :--- |
| Standard Malloc | 277,990 | 1.00x | 1,948 |
| Untyped Pool | 200,913 | 1.38x | 3,446 |
| Arena (Bump) | 38,571 | 7.21x | 18,667 |
