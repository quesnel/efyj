cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(efyj VERSION 0.6.0 LANGUAGES CXX)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(EXPAT REQUIRED)
find_package(Threads REQUIRED)
find_package(EASTL REQUIRED)

message(STATUS "EASTL includes: ${EASTL_INCLUDE_DIRS}")
message(STATUS "EASTL libraries: ${EASTL_LIBRARIES}")

add_definitions(-DFMT_HEADER_ONLY)
include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)
