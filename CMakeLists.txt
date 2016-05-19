cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(efyj VERSION 0.1 LANGUAGES C CXX)

find_package(Boost REQUIRED)
add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

find_package(Eigen3 REQUIRED)
add_library(eigen3 INTERFACE IMPORTED)
set_property(TARGET eigen3 PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${EIGEN3_INCLUDE_DIR})

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

enable_testing()
add_subdirectory(lib)
add_subdirectory(app)
