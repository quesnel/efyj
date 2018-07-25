cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(efyj VERSION 0.1.0 LANGUAGES CXX)

find_package(EXPAT REQUIRED)
add_library(expat INTERFACE IMPORTED)
set_property(TARGET expat PROPERTY
  INTERFACE_LINK_LIBRARIES ${EXPAT_LIBRARIES})
set_property(TARGET expat PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${EXPAT_INCLUDE_DIRS})

find_package(Threads REQUIRED)
add_library(threads INTERFACE IMPORTED)
set_property(TARGET threads PROPERTY
  INTERFACE_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DFMT_HEADER_ONLY)
include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)

set(EASTL_ROOT_DIR ${PROJECT_SOURCE_DIR}/external/EASTL)
include_directories (${EASTL_ROOT_DIR}/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAAssert/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EABase/include/Common)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAMain/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAStdC/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EATest/include)
include_directories (${EASTL_ROOT_DIR}/test/packages/EAThread/include)

if (WIN32)
	set(EASTL_LIBRARY debug ${EASTL_ROOT_DIR}/build/Debug/EASTL.lib
	  optimized ${EASTL_ROOT_DIR}/build/Release/EASTL.lib)
else ()
	set(EASTL_LIBRARY debug ${EASTL_ROOT_DIR}/build/Debug/libEASTL.a
	  optimized ${EASTL_ROOT_DIR}/build/Release/libEASTL.a)
endif ()

add_custom_target(NatVis SOURCES ${EASTL_ROOT_DIR}/doc/EASTL.natvis)

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)
