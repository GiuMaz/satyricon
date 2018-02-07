cmake_minimum_required (VERSION 3.1)

project (satyricon VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_library(satyricon STATIC
    src/dimacs_parser.cpp
    src/sat_solver.cpp)

target_include_directories(satyricon PUBLIC include)

add_executable(solver solver/solver.cpp)
target_link_libraries(solver PRIVATE satyricon)

