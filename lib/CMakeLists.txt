project(libefyj VERSION 0.1.0 LANGUAGES CXX C)

add_library(libefyj INTERFACE)

target_include_directories(libefyj INTERFACE
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:include>
)

install(DIRECTORY include/efyj DESTINATION
        "include/efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# This makes the project importable from the build directory
export(TARGETS libefyj FILE LibEFYjConfig.cmake)

add_executable(testlib test/test_basic.cpp)

set_target_properties(testlib PROPERTIES
  COMPILE_DEFINITIONS EXAMPLES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/test\")

target_include_directories(testlib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(testlib libefyj eigen3 expat threads)

set_property(TARGET testlib PROPERTY CXX_STANDARD 14)

configure_file(efyj.pc.in "${CMAKE_BINARY_DIR}/efyj.pc" @ONLY)

install(FILES "${CMAKE_BINARY_DIR}/efyj.pc"
  DESTINATION "lib/pkgconfig")

add_test(testlib testlib)
