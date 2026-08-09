# -----------------------------
# Standalone Unit Tests (Isolated Build)
# -----------------------------
# This CMakeLists.txt can be built independently without the main project
# Usage: cd tests && cmake && make

cmake_minimum_required(VERSION 3.15)
project(nitro_tests C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(file_test
  unit/file_test.cpp
)

# Basic test includes (no llama, no curl dependencies)
target_include_directories(file_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link only standard libraries (no external deps)
target_link_libraries(file_test PRIVATE
  stdc++fs
)

set_target_properties(file_test PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Add test discovery
enable_testing()
add_test(NAME file_test COMMAND file_test)

