#include <random>
#include <vector>
#include <benchmark/benchmark.h>
#include "MemoryPool.hpp" 

constexpr int ALLOCATIONS_PER_CYCLE = 10000;

std::vector<std::size_t> GetRandomSizes(int count) {
    std::vector<std::size_t> sizes;
    std::size_t class_options[] = { 8, 16, 64, 256 };

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 3);

    for (int i = 0; i < count; ++i) {
        sizes.push_back(class_options[dist(rng)]);
    }
    return sizes;
}

static const auto G_SIZES = GetRandomSizes(ALLOCATIONS_PER_CYCLE);

static void BM_StandardMalloc_Random(benchmark::State& state) {
    for (auto _ : state) {
        void* ptrs[ALLOCATIONS_PER_CYCLE];
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            ptrs[i] = ::operator new(G_SIZES[i]);
            benchmark::DoNotOptimize(ptrs[i]);
        }
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            ::operator delete(ptrs[i]);
        }
    }
}
BENCHMARK(BM_StandardMalloc_Random);

static void BM_Arena_Random(benchmark::State& state) {
    Arena<256 * ALLOCATIONS_PER_CYCLE + 4096> arena;

    for (auto _ : state) {
        void* ptrs[ALLOCATIONS_PER_CYCLE];
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            ptrs[i] = arena.allocate_raw(G_SIZES[i], alignof(std::max_align_t));
            benchmark::DoNotOptimize(ptrs[i]);
        }
        arena.reset();
    }
}
BENCHMARK(BM_Arena_Random);

static void BM_UntypedPool_Random(benchmark::State& state) {
    MemoryPool<ALLOCATIONS_PER_CYCLE> pool;
    for (auto _ : state) {
        void* ptrs[ALLOCATIONS_PER_CYCLE];
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            ptrs[i] = pool.allocate(G_SIZES[i]);
            benchmark::DoNotOptimize(ptrs[i]);
        }
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            pool.deallocate(ptrs[i], G_SIZES[i]);
        }
    }
}
BENCHMARK(BM_UntypedPool_Random);

BENCHMARK_MAIN();