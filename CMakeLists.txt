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

add_executable(uniform_bench benchmarks/uniform_data_benchmark.cpp)
add_executable(nonuniform_bench benchmarks/nonuniform_data_benchmark.cpp)
add_executable(stl_contiguous_benchmark benchmarks/stl_contiguous_benchmark.cpp)
target_link_libraries(stl_contiguous_benchmark PRIVATE benchmark::benchmark)
target_link_libraries(uniform_bench PRIVATE benchmark::benchmark)
target_link_libraries(nonuniform_bench PRIVATE benchmark::benchmark)

add_executable(memory_pool_test testing/memorypooltest.cpp)