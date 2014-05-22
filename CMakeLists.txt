project(efyj)
cmake_minimum_required(VERSION 2.8)

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
endmacro ()

pkg_config_library(LIBEXPAT expat)

set(efyj_MAIN_SRCS main.cpp)
set(efyj_CORE_SRCS dbg.hpp io.cpp io.hpp model.cpp model.hpp print.hpp
  utils.hpp)

add_library(efyj_corelib STATIC ${efyj_CORE_SRCS})

add_executable(efyj ${efyj_MAIN_SRCS})
target_link_libraries(efyj efyj_corelib ${efyj_LINK_LIBRARIES})

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
