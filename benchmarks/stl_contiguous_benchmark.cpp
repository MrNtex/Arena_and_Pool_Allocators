#include <vector>
#include "MemoryPool.hpp"
#include "benchmark/benchmark.h"

struct Order {
    char type;
    double price;
    int size;

    Order(char t, double p, int s) : type(t), price(p), size(s) {}
};

constexpr int ALLOCATIONS_PER_CYCLE = 10000;

static void BM_StdVector_OSAllocator(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Order> vec;
        vec.reserve(ALLOCATIONS_PER_CYCLE);

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            vec.emplace_back('B', 150.50, 100);
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_StdVector_OSAllocator);

static void BM_StdVector_ArenaAllocator(benchmark::State& state) {
    constexpr std::size_t ArenaSize = sizeof(Order) * ALLOCATIONS_PER_CYCLE + 1024;
    Arena<ArenaSize> arena;
    STLArenaAllocator<Order, ArenaSize> alloc(&arena);

    for (auto _ : state) {
        {
            std::vector<Order, STLArenaAllocator<Order, ArenaSize>> vec(alloc);
            vec.reserve(ALLOCATIONS_PER_CYCLE);

            for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
                vec.emplace_back('B', 150.50, 100);
            }
            benchmark::DoNotOptimize(vec);
        }
        arena.reset();
    }
}
BENCHMARK(BM_StdVector_ArenaAllocator);

BENCHMARK_MAIN();