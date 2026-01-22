# CompilerWarnings.cmake
# Configure compiler warnings for the project

# Function to set project warnings
function(set_project_warnings target_name)
    set(CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    set(MSVC_WARNINGS
        /W4
        /w14242  # conversion from 'type1' to 'type2', possible loss of data
        /w14254  # operator: conversion from 'type1' to 'type2', possible loss of data
        /w14263  # member function does not override any base class virtual member function
        /w14265  # class has virtual functions, but destructor is not virtual
        /w14287  # unsigned/negative constant mismatch
        /we4289  # nonstandard extension: 'var' : loop control variable declared in the for-loop is used outside the for-loop scope
        /w14296  # expression is always false
        /w14311  # pointer truncation from 'type' to 'type'
        /w14545  # expression before comma evaluates to a function which is missing an argument list
        /w14546  # function call before comma missing argument list
        /w14547  # operator before comma has no effect; expected operator with side-effect
        /w14549  # operator before comma has no effect; did you intend 'operator'?
        /w14555  # expression has no effect; expected expression with side-effect
        /w14619  # pragma warning: there is no warning number 'number'
        /w14640  # Enable warning on thread un-safe static member initialization
        /w14826  # Conversion from 'type1' to 'type2' is sign-extended
        /w14905  # wide string literal cast to 'LPSTR'
        /w14906  # string literal cast to 'LPWSTR'
        /w14928  # illegal copy-initialization
        /permissive-
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    elseif(MSVC)
        set(PROJECT_WARNINGS ${MSVC_WARNINGS})
    else()
        message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}, no warnings set")
        set(PROJECT_WARNINGS "")
    endif()

    # Apply warnings to target
    target_compile_options(${target_name} PRIVATE ${PROJECT_WARNINGS})
endfunction()

# Security hardening flags
function(set_security_flags target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
        )
        
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target_name} PRIVATE
                -fPIE
            )
            target_link_options(${target_name} PRIVATE
                -pie
            )
        endif()
    endif()
endfunction()

# Sanitizer options (for development/testing)
function(enable_sanitizers target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
        option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
        option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
        
        if(ENABLE_ASAN)
            target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()
        
        if(ENABLE_UBSAN)
            target_compile_options(${target_name} PRIVATE -fsanitize=undefined)
            target_link_options(${target_name} PRIVATE -fsanitize=undefined)
        endif()
        
        if(ENABLE_TSAN)
            target_compile_options(${target_name} PRIVATE -fsanitize=thread)
            target_link_options(${target_name} PRIVATE -fsanitize=thread)
        endif()
    endif()
endfunction()
