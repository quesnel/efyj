project(libefyj VERSION 0.6.0 LANGUAGES CXX)

option(WITH_DEBUG "enable maximium debug code. [default: ON]" ON)
option(WITH_LOG "enable log message. [default: ON]" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)

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

add_library(libefyj ${header_files} ${source_files})

target_include_directories(libefyj PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src ${CMAKE_BINARY_DIR})

target_compile_options(libefyj PRIVATE  
  $<$<CXX_COMPILER_ID:MSVC>:
      /Zc:__cplusplus
      $<$<CONFIG:Debug>:/Od /W3 /Zi /analyze>>)

target_link_libraries(libefyj
  Threads::Threads expat::expat fmt::fmt
  $<$<BOOL:${NLOPT_FOUND}>:nlopt>)

set_target_properties(libefyj PROPERTIES
  OUTPUT_NAME "efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  PUBLIC_HEADER "${header_files}")

target_compile_definitions(libefyj
  PRIVATE
  $<$<BOOL:${BUILD_SHARED_LIBS}>:EFYJ_DLL>
  $<$<BOOL:${WITH_DEBUG}>:EFYJ_ENABLE_DEBUG>
  $<$<BOOL:${WITH_LOG}>:EFYJ_ENABLE_LOG>
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
  VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${PROJECT_VERSION_MINOR}
  VERSION_PATCH=${PROJECT_VERSION_PATCH}
  VERSION_TWEAK=${PROJECT_VERSION_TWEAK})

install(TARGETS libefyj
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}/efyj)

export(TARGETS libefyj FILE LibEFYjConfig.cmake)

add_executable(testlib test/test_basic.cpp)

target_link_libraries(testlib libefyj Threads::Threads expat::expat fmt::fmt)

set_property(TARGET testlib PROPERTY CXX_STANDARD 17)

set_target_properties(testlib PROPERTIES
  COMPILE_DEFINITIONS EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/")

target_include_directories(testlib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)

target_compile_definitions(testlib PRIVATE
  PRIVATE
  $<$<BOOL:${WITH_DEBUG}>:EFYJ_ENABLE_DEBUG>
  $<$<BOOL:${WITH_LOG}>:EFYJ_ENABLE_LOG>
  $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
  $<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_WARNINGS>
  VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  VERSION_MINOR=${PROJECT_VERSION_MINOR}
  VERSION_PATCH=${PROJECT_VERSION_PATCH}
  VERSION_TWEAK=${PROJECT_VERSION_TWEAK})

configure_file(efyj.pc.in ${CMAKE_BINARY_DIR}/efyj.pc @ONLY)
install(FILES
  ${CMAKE_BINARY_DIR}/efyj.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  RENAME efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.pc)

math(EXPR EFYJ_MACRO_VERSION
  "${PROJECT_VERSION_MAJOR} * 100000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")

configure_file(include/efyj/version.hpp.in
  ${CMAKE_BINARY_DIR}/version.hpp @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/version.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

if (NOT DisablePython)
	add_subdirectory(python)
endif ()

add_test(testlib testlib)
