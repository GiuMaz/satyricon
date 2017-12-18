cmake_minimum_required (VERSION 3.1)

project (satyricon VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE include/utils )

add_library(satyricon STATIC
    src/satyricon/dimacs_parser.cpp
    src/satyricon/sat_solver.cpp
    src/satyricon/clause.cpp
    src/satyricon/vsids.cpp
    src/satyricon/literal.cpp)

target_include_directories(satyricon PUBLIC include/satyricon )
target_link_libraries(satyricon PRIVATE utils)

add_executable(solver solver/solver.cpp)
target_link_libraries(solver PRIVATE satyricon utils)

add_executable(pigeon_hole pigeon_hole/pigeon.cpp)
target_link_libraries(pigeon_hole PRIVATE utils)

