add_executable(app src/main.cpp)

target_link_libraries(app libefyj Threads::Threads expat::expat ${EASTL_LIBRARY})

set_property(TARGET app PROPERTY CXX_STANDARD 17)

add_definitions(-DFMT_HEADER_ONLY)

include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR})

target_compile_definitions(app PRIVATE EASTL_EASTDC_VSNPRINTF=1)

set_target_properties(app PROPERTIES
  OUTPUT_NAME "efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

install(TARGETS app
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
