project(libefyj VERSION 0.1.0.0 LANGUAGES CXX C)

## TODO src/prediction-mpi.cpp

add_library(libefyj SHARED src/context.cpp src/cstream.cpp
  src/exception.cpp src/main.cpp src/model.cpp src/options.cpp
  src/post.hpp src/prediction.cpp src/prediction.hpp
  src/prediction-thread.cpp src/problem.cpp src/solver.cpp
  src/solver-stack.hpp src/utils.cpp src/utils.hpp)

target_include_directories(libefyj PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

# If we have compiler requirements for this library, list them
# here
# target_compile_features(lib
#   PUBLIC cxx_auto_type
#   PRIVATE cxx_variadic_templates)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
check_cxx_compiler_flag("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)

if (COMPILER_SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX1Y)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++14/1y "
    "spport. Please use a different C++ compiler.")
endif()

# target_compile_features(lib
#  PUBLIC cxx_constexpr cxx_auto_type cxx_variadic_templates)

target_link_libraries(libefyj boost eigen3 expat threads)

set_target_properties(libefyj PROPERTIES
  OUTPUT_NAME "efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

install(TARGETS libefyj
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include/efyj)

# This makes the project importable from the build directory
export(TARGETS libefyj FILE LibEFYjConfig.cmake)

add_executable(testlib test/test_basic.cpp)

set_target_properties(testlib PROPERTIES
  COMPILE_DEFINITIONS EXAMPLES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test\")

target_link_libraries(testlib libefyj boost eigen3 expat threads)

add_test(testlib testlib)