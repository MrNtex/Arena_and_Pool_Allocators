#include <benchmark/benchmark.h>
#include <vector>
#include "MemoryPool.hpp"

struct Order {
    char type;
    double price;
    int size;

    Order(char t, double p, int s) : type(t), price(p), size(s) {}
};

constexpr int ALLOCATIONS_PER_CYCLE = 1000;

static void BM_StandardMalloc(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Order*> orders;
        orders.reserve(ALLOCATIONS_PER_CYCLE);
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            Order* order = new Order{ 'B', 150.50, 100 };
            benchmark::DoNotOptimize(order);
            orders.push_back(order);
        }
        for (auto* ptr : orders) {
            delete ptr;
        }
    }
}
BENCHMARK(BM_StandardMalloc);

static void BM_ArenaAllocator(benchmark::State& state) {
    Arena<sizeof(Order)* ALLOCATIONS_PER_CYCLE + 1024> arena;
    for (auto _ : state) {
        for (int i = 0; i < ALLOCATIONS_PER_CYCLE; ++i) {
            Order* order = arena.allocate<Order>('B', 150.50, 100);
            benchmark::DoNotOptimize(order);
        }
        arena.reset();
    }
}
BENCHMARK(BM_ArenaAllocator);

BENCHMARK_MAIN();