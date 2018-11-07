cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(efyj VERSION 0.6.0 LANGUAGES CXX)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(EXPAT REQUIRED)
find_package(Threads REQUIRED)
# find_package(EASTL REQUIRED)

set(EASTL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/EASTL)
include_directories (${EASTL_ROOT_DIR}/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAAssert/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EABase/include/Common)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAMain/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAStdC/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EATest/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAThread/include)

if (WI32)
  set(EASTL_LIBRARY debug ${EASTL_ROOT_DIR}/out/Debug/EASTL.lib
    optimized ${EASTL_ROOT_DIR}/out/Release/EASTL.lib)
  add_custom_target(NatVis SOURCES ${EASTL_ROOT_DIR}/doc/EASTL.natvis)
else ()
  set(EASTL_LIBRARY debug ${EASTL_ROOT_DIR}/out/Debug/libEASTL.a
    optimized ${EASTL_ROOT_DIR}/out/Release/libEASTL.a)
endif ()

add_definitions(-DFMT_HEADER_ONLY)
include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)
