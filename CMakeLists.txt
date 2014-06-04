project(efyj)
cmake_minimum_required(VERSION 2.8)
set(efyj_MAJOR 0)
set(efyj_MINOR 1)
set(efyj_PATCH 0)
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
  pkg_check_modules(${LIBNAME} ${pcfile})
  include_directories(${${LIBNAME}_INCLUDE_DIRS})
  link_directories(${${LIBNAME}_LIBRARY_DIRS})
  add_definitions(${${LIBNAME}_CFLAGS_OTHER})
  set(efyj_LINK_LIBRARIES ${efyj_LINK_LIBRARIES} ${${LIBNAME}_LIBRARIES})
  set(efyj_STATIC_LINK_LIBRARIES ${efyj_STATIC_LINK_LIBRARIES} ${${LIBNAME}_STATIC_LIBRARIES})
endmacro ()

pkg_config_library(LIBEXPAT expat)
pkg_config_library(LIBEIGEN3 eigen3)

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

set(efyj_CORE_HDRS dbg.hpp io.hpp model.hpp print.hpp solver.hpp
  utils.hpp visibility.hpp)

set(efyj_CORE_SRCS io.cpp model.cpp print.cpp solver-basic.cpp
  solver-bigmem.cpp solver-hash.cpp)

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
