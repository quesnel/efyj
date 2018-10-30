project(libefyj VERSION 0.6.0 LANGUAGES CXX)

option(WITH_DEBUG "enable maximium debug code. [default: ON]" ON)
option(WITH_LOG "enable log message. [default: ON]" ON)

set(header_files
  include/efyj/efyj.hpp
  include/efyj/matrix.hpp)

set(source_files
  src/adjustment.cpp
  src/adjustment.hpp
  src/dynarray.hpp
  src/efyj.cpp
  src/efyj.hpp
  src/model.cpp
  src/model.hpp
  src/options.cpp
  src/options.hpp
  src/post.hpp
  src/prediction.cpp
  src/prediction.hpp
  src/prediction-thread.cpp
  src/prediction-thread.hpp
  src/private.hpp
  src/solver-stack.cpp
  src/solver-stack.hpp
  src/utils.hpp)

add_library(libefyj SHARED ${header_files} ${source_files})
add_library(libstaticefyj STATIC ${header_files} ${source_files})

target_include_directories(libefyj PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src ${CMAKE_BINARY_DIR})
target_include_directories(libstaticefyj PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src ${CMAKE_BINARY_DIR})

target_link_libraries(libefyj threads
  $<$<BOOL:${NLOPT_FOUND}>:nlopt>)
target_link_libraries(libstaticefyj threads
  $<$<BOOL:${NLOPT_FOUND}>:nlopt>)

set_target_properties(libefyj PROPERTIES
  OUTPUT_NAME "libefyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  POSITION_INDEPENDENT_CODE ON
  COMPILE_DEFINITIONS "EFYJ_DLL"
  DEFINE_SYMBOL "libefyj_EXPORTS"
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON)
set_target_properties(libstaticefyj PROPERTIES
  OUTPUT_NAME "libefyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON)

target_compile_definitions(libefyj
  PRIVATE
  $<$<BOOL:${WITH_DEBUG}>:EFYJ_ENABLE_DEBUG>
  $<$<BOOL:${WITH_LOG}>:EFYJ_ENABLE_LOG>
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
  VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${PROJECT_VERSION_MINOR}
  VERSION_PATCH=${PROJECT_VERSION_PATCH}
  VERSION_TWEAK=${PROJECT_VERSION_TWEAK})
target_compile_definitions(libstaticefyj
  PRIVATE
  $<$<BOOL:${WITH_DEBUG}>:EFYJ_ENABLE_DEBUG>
  $<$<BOOL:${WITH_LOG}>:EFYJ_ENABLE_LOG>
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
  VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${PROJECT_VERSION_MINOR}
  VERSION_PATCH=${PROJECT_VERSION_PATCH}
  VERSION_TWEAK=${PROJECT_VERSION_TWEAK})

install(TARGETS libefyj
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS libstaticefyj
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)

# install(DIRECTORY include/efyj DESTINATION
#         "include/efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# This makes the project importable from the build directory
export(TARGETS libefyj FILE LibEFYjConfig.cmake)

add_executable(testlib test/test_basic.cpp)

set_target_properties(testlib PROPERTIES
  COMPILE_DEFINITIONS EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/")

target_include_directories(testlib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(testlib PRIVATE EASTL_EASTDC_VSNPRINTF=1)

target_link_libraries(testlib libstaticefyj expat threads ${EASTL_LIBRARY})

set_property(TARGET testlib PROPERTY CXX_STANDARD 14)

configure_file(efyj.pc.in "${CMAKE_BINARY_DIR}/efyj.pc" @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/efyj.pc"
  DESTINATION "lib/pkgconfig")

# Configure the version.hpp.in to produce the VLE_VERSION macro.

math(EXPR EFYJ_MACRO_VERSION "${PROJECT_VERSION_MAJOR} * 100000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")

configure_file(include/efyj/version.hpp.in
  ${CMAKE_BINARY_DIR}/version.hpp @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/version.hpp DESTINATION
  "include/efyj")

add_test(testlib testlib)
