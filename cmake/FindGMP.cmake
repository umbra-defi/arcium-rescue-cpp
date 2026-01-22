# FindGMP.cmake
# Find the GNU Multiple Precision Arithmetic Library
#
# This module defines:
#   GMP_FOUND        - True if GMP was found
#   GMP_INCLUDE_DIRS - Include directories for GMP
#   GMP_LIBRARIES    - Libraries to link against
#   GMP::GMP         - Imported target for GMP
#   GMP::GMPXX       - Imported target for GMP C++ bindings

include(FindPackageHandleStandardArgs)

# Try to find GMP using pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GMP QUIET gmp)
endif()

# Find the include directory
find_path(GMP_INCLUDE_DIR
    NAMES gmp.h
    HINTS
        ${PC_GMP_INCLUDEDIR}
        ${PC_GMP_INCLUDE_DIRS}
        ENV GMP_ROOT
    PATH_SUFFIXES include
)

# Find the GMP library
find_library(GMP_LIBRARY
    NAMES gmp libgmp
    HINTS
        ${PC_GMP_LIBDIR}
        ${PC_GMP_LIBRARY_DIRS}
        ENV GMP_ROOT
    PATH_SUFFIXES lib lib64
)

# Find the GMP C++ library
find_library(GMPXX_LIBRARY
    NAMES gmpxx libgmpxx
    HINTS
        ${PC_GMP_LIBDIR}
        ${PC_GMP_LIBRARY_DIRS}
        ENV GMP_ROOT
    PATH_SUFFIXES lib lib64
)

# Get the version if possible
if(GMP_INCLUDE_DIR AND EXISTS "${GMP_INCLUDE_DIR}/gmp.h")
    file(STRINGS "${GMP_INCLUDE_DIR}/gmp.h" GMP_VERSION_MAJOR_LINE
        REGEX "^#define __GNU_MP_VERSION[ \t]+[0-9]+$")
    file(STRINGS "${GMP_INCLUDE_DIR}/gmp.h" GMP_VERSION_MINOR_LINE
        REGEX "^#define __GNU_MP_VERSION_MINOR[ \t]+[0-9]+$")
    file(STRINGS "${GMP_INCLUDE_DIR}/gmp.h" GMP_VERSION_PATCH_LINE
        REGEX "^#define __GNU_MP_VERSION_PATCHLEVEL[ \t]+[0-9]+$")
    
    string(REGEX REPLACE "^#define __GNU_MP_VERSION[ \t]+([0-9]+)$" "\\1"
        GMP_VERSION_MAJOR "${GMP_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define __GNU_MP_VERSION_MINOR[ \t]+([0-9]+)$" "\\1"
        GMP_VERSION_MINOR "${GMP_VERSION_MINOR_LINE}")
    string(REGEX REPLACE "^#define __GNU_MP_VERSION_PATCHLEVEL[ \t]+([0-9]+)$" "\\1"
        GMP_VERSION_PATCH "${GMP_VERSION_PATCH_LINE}")
    
    set(GMP_VERSION "${GMP_VERSION_MAJOR}.${GMP_VERSION_MINOR}.${GMP_VERSION_PATCH}")
endif()

# Handle standard find_package arguments
find_package_handle_standard_args(GMP
    REQUIRED_VARS GMP_LIBRARY GMP_INCLUDE_DIR
    VERSION_VAR GMP_VERSION
)

# Set output variables
if(GMP_FOUND)
    set(GMP_INCLUDE_DIRS ${GMP_INCLUDE_DIR})
    set(GMP_LIBRARIES ${GMP_LIBRARY})
    
    if(GMPXX_LIBRARY)
        list(APPEND GMP_LIBRARIES ${GMPXX_LIBRARY})
    endif()
    
    # Create imported target for GMP C library
    if(NOT TARGET GMP::GMP)
        add_library(GMP::GMP UNKNOWN IMPORTED)
        set_target_properties(GMP::GMP PROPERTIES
            IMPORTED_LOCATION "${GMP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
        )
    endif()
    
    # Create imported target for GMP C++ library
    if(GMPXX_LIBRARY AND NOT TARGET GMP::GMPXX)
        add_library(GMP::GMPXX UNKNOWN IMPORTED)
        set_target_properties(GMP::GMPXX PROPERTIES
            IMPORTED_LOCATION "${GMPXX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES GMP::GMP
        )
    endif()
endif()

# Hide internal variables
mark_as_advanced(
    GMP_INCLUDE_DIR
    GMP_LIBRARY
    GMPXX_LIBRARY
)
