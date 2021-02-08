pybind11_add_module(pyefyj MODULE pyefyj.cpp)

target_include_directories(pyefyj PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:lib/include>
  PRIVATE expat::expat lib/src ${CMAKE_BINARY_DIR})

target_link_libraries(pyefyj  PRIVATE libefyj
  Threads::Threads 
  expat::expat
  pybind11::module
  pybind11::lto
  pybind11::windows_extras)

set_target_properties(pyefyj PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON)

target_compile_definitions(pyefyj
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

pybind11_extension(pyefyj)
pybind11_strip(pyefyj)