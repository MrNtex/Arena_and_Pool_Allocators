#include <benchmark/benchmark.h>
#include "MemoryPool.hpp" 

struct Order {
    char type;
    double price;
    int size;

    Order(char t, double p, int s) : type(t), price(p), size(s) {}
};

constexpr int ALLOCATIONS_PER_CYCLE = 10000;

static void BM_StandardMalloc(benchmark::State& state) {
    for (auto _ : state) {
        Order* orders[ALLOCATIONS_PER_CYCLE];

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            orders[i] = new Order('B', 150.50, 100);
            benchmark::DoNotOptimize(orders[i]);
        }

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            delete orders[i];
        }
    }
}
BENCHMARK(BM_StandardMalloc);

static void BM_ArenaAllocator(benchmark::State& state) {
    Arena<sizeof(Order)* ALLOCATIONS_PER_CYCLE + 1024> arena;

    for (auto _ : state) {
        Order* orders[ALLOCATIONS_PER_CYCLE];

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            orders[i] = arena.allocate<Order>('B', 150.50, 100);
            benchmark::DoNotOptimize(orders[i]);
        }

        arena.reset();
    }
}
BENCHMARK(BM_ArenaAllocator);

static void BM_PoolAllocator(benchmark::State& state) {
    TypedPoolAllocator<Order, ALLOCATIONS_PER_CYCLE> pool;

    for (auto _ : state) {
        Order* orders[ALLOCATIONS_PER_CYCLE];

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            orders[i] = pool.allocate('B', 150.50, 100);
            benchmark::DoNotOptimize(orders[i]);
        }

        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            pool.deallocate(orders[i]);
        }
    }
}
BENCHMARK(BM_PoolAllocator);

BENCHMARK_MAIN();