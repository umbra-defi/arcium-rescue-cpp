# Examples configuration

add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage
    PRIVATE
        rescue::rescue
)
target_compile_features(basic_usage PRIVATE cxx_std_23)
