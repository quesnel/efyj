cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(efyj VERSION 0.6.0 LANGUAGES CXX)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(DisablePython "Disable python wrapper BUILD_RPATH [OFF]" OFF)

if (UNIX)
  set(BUILD_SHARED_LIBS ON)
endif()

if (WIN32)
  find_package(expat CONFIG REQUIRED)
else ()
  find_package(EXPAT REQUIRED)
  add_library(expat::expat INTERFACE IMPORTED)
  target_link_libraries(expat::expat INTERFACE EXPAT::EXPAT)
endif ()

find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_definitions(-DFMT_HEADER_ONLY)
include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)

if (NOT DisablePython)
	add_subdirectory(external/pybind11)
endif ()

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)

