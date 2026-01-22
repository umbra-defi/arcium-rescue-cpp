cmake_minimum_required(VERSION 3.16)
project(rescue_interop_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add parent cmake module path for FindGMP.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Find dependencies
find_package(GMP REQUIRED)
find_package(OpenSSL REQUIRED)

# Set up the rescue library manually
set(RESCUE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(RESCUE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../build/src/librescue.a)

add_executable(test_interop test_interop.cpp)
target_include_directories(test_interop PRIVATE ${RESCUE_INCLUDE_DIR})
target_link_libraries(test_interop PRIVATE 
    ${RESCUE_LIBRARY}
    GMP::GMP
    GMP::GMPXX
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Copy test vectors to build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test_vectors_js.json 
               ${CMAKE_CURRENT_BINARY_DIR}/test_vectors_js.json COPYONLY)
