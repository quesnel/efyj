add_executable(app src/main.cpp)

target_link_libraries(app libefyj eigen3 expat threads)

set_property(TARGET app PROPERTY CXX_STANDARD 14)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR})

set_target_properties(app PROPERTIES
  OUTPUT_NAME "efyj-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

install(TARGETS app
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
