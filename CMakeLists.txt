cmake_minimum_required(VERSION 3.14)
project(ArenaAllocatorBenchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

add_executable(arena_bench benchmarks/arena_benchmark.cpp)
target_link_libraries(arena_bench PRIVATE benchmark::benchmark)

add_executable(memory_pool_test testing/memorypooltest.cpp)