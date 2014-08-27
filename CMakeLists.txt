project(efyj)
cmake_minimum_required(VERSION 2.8)
set(efyj_MAJOR 0)
set(efyj_MINOR 5)
set(efyj_PATCH 99)
set(efyj_VERSION "${efyj_MAJOR}.${efyj_MINOR}.${efyj_PATCH}")
set(efyj_STABLEVERSION "${efyj_MAJOR}.${efyj_MINOR}")

if (CMAKE_BUILD_TYPE STREQUAL "")
  set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build" FORCE)
endif ()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")

  if ("${CMAKE_BUILD_TYPE}" EQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" EQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
  endif ()
endif ()

include_directories(. ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})
find_package(PkgConfig)

macro (pkg_config_library LIBNAME pcfile)
  pkg_check_modules(${LIBNAME} REQUIRED ${pcfile})
  include_directories(${${LIBNAME}_INCLUDE_DIRS})
  link_directories(${${LIBNAME}_LIBRARY_DIRS})
  add_definitions(${${LIBNAME}_CFLAGS_OTHER})
  set(efyj_LINK_LIBRARIES ${efyj_LINK_LIBRARIES} ${${LIBNAME}_LIBRARIES})
  set(efyj_STATIC_LINK_LIBRARIES ${efyj_STATIC_LINK_LIBRARIES} ${${LIBNAME}_STATIC_LIBRARIES})
endmacro ()

pkg_config_library(LIBEXPAT expat)
pkg_config_library(LIBEIGEN3 eigen3)
#pkg_config_library(LIBMPI ompi-cxx)

message(STATUS "checking for 'gmp' library")
find_path(GMP_INCLUDE_DIR NAMES gmpxx.h)
if (GMP_INCLUDE_DIR)
  message(STATUS " found 'gmpxx.h' in ${GMP_INCLUDE_DIR}")
  find_library(GMP_LIBRARIES NAMES gmp libgmp)
  if (GMP_LIBRARIES)
    message(STATUS " found ${GMP_LIBRARIES}")
    find_library(GMPXX_LIBRARIES NAMES gmpxx libgmpxx)
    if (GMPXX_LIBRARIES)
      message(STATUS " found ${GMPXX_LIBRARIES}")
      include_directories(${GMP_INCLUDE_DIR})
      set(efyj_LINK_LIBRARIES ${efyj_LINK_LIBRARIES} ${GMP_LIBRARIES})
      set(efyj_LINK_LIBRARIES ${efyj_LINK_LIBRARIES} ${GMPXX_LIBRARIES})
      set(efyj_STATIC_LINK_LIBRARIES ${efyj_STATIC_LINK_LIBRARIES} ${GMP_LIBRARIES})
      set(efyj_STATIC_LINK_LIBRARIES ${efyj_STATIC_LINK_LIBRARIES} ${GMPXX_LIBRARIES})
    else ()
      message(ERROR " can not found gmpxx or libgmpxx")
    endif ()
  else ()
    message(ERROR " can not found gmp or libgmp")
  endif ()
else ()
  message(ERROR " can not found gmpxx.h")
endif ()

message(STATUS "checking for a C++ compiler with MPI support")
find_package(MPI REQUIRED)
if (NOT MPI_CXX_FOUND)
  message(ERROR " failed to found a C++ compiler with MPI support.
  Please assign MPI_CXX_COMPILER to the MPI wrapper (mpicc, etc.) and
  rerun CMake. If this fails, set both MPI_CXX_LIBRARIES and
  MPI_CXX_INCLUDE_PATH and rerun CMake.")
else()
  message(STATUS " MPI support enabled")
endif ()


### # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
## efyj-x.y.pc
#

if (WIN32)
  set(efyj_PKGCONFIG_PREFIXPATH "/target")
  set(efyj_PKGCONFIG_LIBDIR "bin")
else()
  set(efyj_PKGCONFIG_PREFIXPATH "${CMAKE_INSTALL_PREFIX}")
  set(efyj_PKGCONFIG_LIBDIR "lib")
endif()

configure_file(efyj.pc.in ${PROJECT_BINARY_DIR}/efyj-${efyj_STABLEVERSION}.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/efyj-${efyj_STABLEVERSION}.pc
  DESTINATION lib/pkgconfig)

### # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
## Core library
#

set(efyj_CORE_HDRS log.hpp model.hpp print.hpp problem.hpp solver.hpp
  utils.hpp visibility.hpp)

set(efyj_CORE_SRCS log.cpp model.cpp model-io.cpp print.cpp
  problem.cpp solver.cpp solver-basic.cpp solver-bigmem.cpp
  solver-hash.cpp)

add_library(efyj_corelib SHARED ${efyj_CORE_SRCS})
add_library(efyj_static_corelib STATIC ${efyj_CORE_SRCS})

set_target_properties(efyj_corelib PROPERTIES
  VERSION 0
  OUTPUT_NAME "efyj-${efyj_STABLEVERSION}"
  DEFINE_SYMBOL "efyj_EXPORTS"
  COMPILE_DEFINITIONS "efyj_DLL"
  COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")

if (WIN32)
  target_link_libraries(efyj_corelib ${efyj_STATIC_LINK_LIBRARIES})
else ()
  target_link_libraries(efyj_corelib ${efyj_LINK_LIBRARIES})
endif ()

set_target_properties(efyj_static_corelib PROPERTIES
  VERSION 0
  OUTPUT_NAME "efyj-${efyj_STABLEVERSION}"
  DEFINE_SYMBOL "efyj_EXPORTS"
  COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")

install(FILES ${efyj_CORE_HDRS}
  DESTINATION include/efyj-${efyj_STABLEVERSION})

install(TARGETS efyj_corelib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(TARGETS efyj_static_corelib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

### # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
## Executable
#

set(efyj_MAIN_SRCS main.cpp)

add_executable(efyj ${efyj_MAIN_SRCS})

if (MPI_CXX_FOUND)
  include_directories(${MPI_CXX_INCLUDE_PATH})
  target_link_libraries(efyj ${MPI_CXX_LIBRARIES})

  if (MPI_CXX_COMPILE_FLAGS)
    set_target_properties(efyj PROPERTIES COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS}")
  endif ()

  if (MPI_CXX_LINK_FLAGS)
    set_target_properties(efyj PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS}")
  endif ()
endif ()

if (WIN32)
  target_link_libraries(efyj efyj_static_corelib ${efyj_STATIC_LINK_LIBRARIES})
  set_target_properties(efyj PROPERTIES
    COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")
else ()
  target_link_libraries(efyj efyj_corelib ${efyj_LINK_LIBRARIES})
  set_target_properties(efyj PROPERTIES
    VERSION ${efyj_STABLEVERSION}
    COMPILE_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")
endif ()


install(TARGETS efyj DESTINATION bin)

### # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
## Testing
#

enable_testing()

message(STATUS "checking for 'catch.hpp'")
find_path(CATCH_INCLUDE_DIR catch.hpp PATHS /usr/include /usr/local/include ENV CATCH_INCLUDE_DIR)

if (CATCH_INCLUDE_DIR)
  message(STATUS " found 'catch.hpp' in ${CATCH_INCLUDE_DIR}")
  include_directories(${CATCH_INCLUDE_DIR})

  add_executable(test_basic tests/test_basic.cpp)
  target_link_libraries(test_basic efyj_corelib ${efyj_LINK_LIBRARIES})
  set_target_properties(test_basic PROPERTIES
    COMPILE_DEFINITIONS EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/tests")
  add_test(NAME test_basic COMMAND test_basic)
else ()
  message(STATUS " not found catch.hpp. Unit test disabled")
endif ()


### # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
## CPack
#

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EFYj - Multi-Attribute Decision Making")
set(CPACK_PACKAGE_VENDOR "INRA")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_NAME "efyj")
set(CPACK_PACKAGE_VERSION_MAJOR "${efyj_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${efyj_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${efyj_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "efyj ${efyj_MAJOR}.${efyj_MINOR}")
set(CPACK_STRIP_FILES "1")
set(CPACK_PACKAGE_CONTACT "Gauthier Quesnel <gauthier.quesnel@toulouse.inra.fr>")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY "1")

if (WIN32)
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY "0")
  set (CPACK_GENERATOR "ZIP;NSIS")
  set (CPACK_SOURCE_GENERATOR "ZIP")
else ()
  set (CPACK_GENERATOR "TGZ;DEB;RPM")
  set (CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
endif ()

set(CPACK_SOURCE_PACKAGE_FILE_NAME "efyj-${efyj_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES
  "/\\\\.git/"
  "\\\\.#"
  "/#"
  "/\\\\.gitignore$"
  "/\\\\.gitattributes$"
  "/build-debug-clang/"
  "/build-debug-gcc/"
  "/build-release-clang/"
  "/build-release-gcc/"
  "/build-cross/"
  "/coverage/"
  )

if (WIN32 AND NOT UNIX)
  set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}\\\\efyj.ico")
  set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}\\\\efyj.ico")
  set(CPACK_PACKAGE_ICON "${PROJECT_SOURCE_DIR}\\\\efyj.bmp")
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\efyj.exe")
  set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} EFYj ${efyj_VERSION}")
  set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\https://github.com/quesnel/efyj")
  set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\https://github.com/quesnel/efyj")
  set(CPACK_NSIS_CONTACT "gauthier.quesnel@toulouse.inra.fr")
  set(CPACK_NSIS_MODIFY_PATH ON)
else ()
  set(CPACK_STRIP_FILES "bin/efyj")
endif ()

INCLUDE(CPack)
