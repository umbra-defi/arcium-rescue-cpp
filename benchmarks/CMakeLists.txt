# Benchmarks configuration

include(FetchContent)

# Google Benchmark
FetchContent_Declare(
    benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

# nlohmann/json for JSON output
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(bench_rescue bench_rescue.cpp)
target_link_libraries(bench_rescue
    PRIVATE
        rescue::rescue
        benchmark::benchmark
        nlohmann_json::nlohmann_json
)
target_compile_features(bench_rescue PRIVATE cxx_std_23)
