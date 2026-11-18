# Needs a reasonably recent CMake.
cmake_minimum_required(VERSION 3.20)

# The project is called AlphaDesk and is written in C++ (CMake calls it CXX).
project(AlphaDesk CXX)

# Use the 2020 version of C++, and refuse to build with an older one.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Write build/compile_commands.json, which tells the editor how the code is
# compiled. Without it the editor assumes old C++ and shows false errors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build with optimisation unless told otherwise. Without this the program
# runs several times slower.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Ask the compiler to warn about code that is legal but probably a mistake.
add_compile_options(-Wall -Wextra)

# Programs live in src/tools, one file each, named after the program.
add_executable(itch_count src/tools/itch_count.cpp)
