cmake_minimum_required(VERSION 3.16)
project(rescue_interop_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Find dependencies
find_package(OpenSSL REQUIRED)

# Set up the rescue library manually
set(RESCUE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(RESCUE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../build/src/librescue.a)

# Test interop executable
add_executable(test_interop test_interop.cpp)
target_include_directories(test_interop PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(test_interop PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# 100k benchmark executable
add_executable(benchmark_100k benchmark_100k.cpp)
target_include_directories(benchmark_100k PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(benchmark_100k PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Chunked benchmark executable
add_executable(chunk_benchmark_cpp chunk_benchmark_cpp.cpp)
target_include_directories(chunk_benchmark_cpp PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(chunk_benchmark_cpp PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Debug comparison executable
add_executable(debug_compare debug_compare.cpp)
target_include_directories(debug_compare PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(debug_compare PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
)

# Debug hash parameters
add_executable(debug_hash debug_hash.cpp)
target_include_directories(debug_hash PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(debug_hash PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
)

# Test SHAKE256
add_executable(test_shake test_shake.cpp)
target_include_directories(test_shake PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(test_shake PRIVATE 
    ${RESCUE_LIBRARY}
    OpenSSL::Crypto
)
