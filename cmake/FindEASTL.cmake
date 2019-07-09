# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindEASTL
# ----------
#
# Try to find EASTL
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``EASTL::EASTL``, if
# EASTL has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   EASTL_FOUND          - True if EASTL was found
#   EASTL_INCLUDE_DIRS   - include directories for EASTL
#   EASTL_LIBRARIES      - link against this library to use EASTL
#
# The module will also define two cache variables::
#
#   EASTL_INCLUDE_DIR    - the EASTL include directory
#   EASTL_LIBRARY        - the path to the EASTL library
#

find_path(EASTL_ROOT_DIR
  NAMES include/EASTL/version.h
  HINTS ENV EASTL_ROOTDIR
  PATHS ${PROJECT_SOURCE_DIR}/external/EASTL)

find_path(EASTL_INCLUDE_DIR
  NAMES EASTL/version.h
  HINTS ENV EASTL_ROOTDIR
  PATHS ${PROJECT_SOURCE_DIR}/external/EASTL/include)

find_library(EASTL_LIBRARY_DEBUG
  NAMES EASTL
  HINTS ENV EASTL_ROOTDIR
  PATHS ${PROJECT_SOURCE_DIR}/external/EASTL/build/Debug)

find_library(EASTL_LIBRARY_RELEASE
  NAMES EASTL
  HINTS ENV EASTL_ROOTDIR
  PATHS ${PROJECT_SOURCE_DIR}/external/EASTL/build/Release)

include(SelectLibraryConfigurations)
select_library_configurations(EASTL)

message(STATUS "EASTL_LIBRARY_DEBUG: ${EASTL_LIBRARY_DEBUG}")
message(STATUS "EASTL_LIBRARY_RELEASE: ${EASTL_LIBRARY_RELEASE}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EASTL
  FOUND_VAR EASTL_FOUND
  REQUIRED_VARS
  EASTL_LIBRARY_DEBUG
  EASTL_LIBRARY_RELEASE
  EASTL_INCLUDE_DIR)

if (EASTL_FOUND)
  set(EASTL_INCLUDE_DIRS
    ${EASTL_INCLUDE_DIR}
    ${EASTL_ROOT_DIR}/test/packages/EABase/include/Common)

  if (NOT EASTL_LIBRARIES)
    set(EASTL_DEBUG_LIBRARIES ${EASTL_LIBRARY_DEBUG})
    set(EASTL_RELEASE_LIBRARIES ${EASTL_LIBRARY_RELEASE})
    set(EASTL_LIBRARIES ${EASTL_LIBRARY_RELEASE})
  endif ()

  if (NOT TARGET EASTL::EASTL)
    add_library(EASTL::EASTL UNKNOWN IMPORTED)
    set_target_properties(EASTL::EASTL PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${EASTL_INCLUDE_DIRS}")

    if (EASTL_LIBRARY_RELEASE)
      set_property(TARGET EASTL::EASTL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(EASTL::EASTL PROPERTIES
        IMPORTED_LOCATION_RELEASE "${EASTL_LIBRARY_RELEASE}")
    endif()

    if (EASTL_LIBRARY_DEBUG)
      set_property(TARGET EASTL::EASTL APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)

      set_target_properties(EASTL::EASTL PROPERTIES
        IMPORTED_LOCATION_DEBUG "${EASTL_LIBRARY_DEBUG}")
    endif()

    if (NOT EASTL_LIBRARY_RELEASE AND NOT EASTL_LIBRARY_DEBUG)
      set_property(TARGET EASTL::EASTL APPEND PROPERTY
        IMPORTED_LOCATION "${EASTL_LIBRARY}")
    endif ()

    set_target_properties(EASTL::EASTL PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      INTERFACE_INCLUDE_DIRECTORIES "${EASTL_INCLUDE_DIRS}")

    set_property(TARGET EASTL::EASTL APPEND PROPERTY
      INTERFACE_COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG;EA_DEBUG;EA_ASSERT_ENABLED>)
  endif ()
endif ()

mark_as_advanced(EASTL EASTL_LIBRARY_DEBUG EASTL_LIBRARY_RELEASE EASTL_INCLUDE_DIR)
