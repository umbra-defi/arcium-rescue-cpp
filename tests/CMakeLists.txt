# Tests configuration

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable testing
include(GoogleTest)

# Helper function to create test executables
function(add_rescue_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME}
        PRIVATE
            rescue::rescue
            GTest::gtest_main
            GTest::gmock
    )
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_23)
    gtest_discover_tests(${TEST_NAME})
endfunction()

# Add test executables
add_rescue_test(test_field)
add_rescue_test(test_constant_time)
add_rescue_test(test_matrix)
add_rescue_test(test_rescue_permutation)
add_rescue_test(test_rescue_hash)
add_rescue_test(test_rescue_cipher)
