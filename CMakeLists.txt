cmake_minimum_required(VERSION 3.25)

project(rescue-cipher
    VERSION 1.0.0
    DESCRIPTION "Rescue Cipher implementation over Curve25519 base field"
    LANGUAGES CXX
)

# C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Options
option(RESCUE_BUILD_TESTS "Build unit tests" ON)
option(RESCUE_BUILD_BENCHMARKS "Build benchmarks" ON)
option(RESCUE_BUILD_EXAMPLES "Build examples" ON)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# Find required dependencies (OpenSSL only - no GMP!)
find_package(OpenSSL REQUIRED)

# Main library
add_subdirectory(src)

# Tests
if(RESCUE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(RESCUE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Examples
if(RESCUE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS rescue
    EXPORT rescue-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/rescue
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT rescue-targets
    FILE rescue-targets.cmake
    NAMESPACE rescue::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rescue
)

# Package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rescue-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/rescue-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rescue
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/rescue-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/rescue-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/rescue-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rescue
)
