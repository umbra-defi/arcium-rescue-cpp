# Source files for the rescue library

add_library(rescue
    uint256.cpp
    field.cpp
    matrix.cpp
    utils.cpp
    rescue_desc.cpp
    rescue_hash.cpp
    rescue_cipher.cpp
)

# Add alias for cleaner linking
add_library(rescue::rescue ALIAS rescue)

# Include directories
target_include_directories(rescue
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies (OpenSSL only - no GMP!)
target_link_libraries(rescue
    PRIVATE
        OpenSSL::Crypto
)

# Set compile features
target_compile_features(rescue PUBLIC cxx_std_23)

# Apply warnings
set_project_warnings(rescue)

# Apply security flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_security_flags(rescue)
endif()

# Set properties
set_target_properties(rescue PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
