cmake_minimum_required (VERSION 3.1)

project (satyricon VERSION 0.2 LANGUAGES CXX)

add_library(satyricon STATIC src/satyricon/dimacs_parser.cpp)

target_include_directories(satyricon PUBLIC include/satyricon )
target_compile_features(satyricon PRIVATE cxx_range_for)

add_library(utils INTERFACE)
target_include_directories(utils INTERFACE include/utils )

add_executable(solver solver/solver.cpp)
target_compile_features(solver PRIVATE cxx_range_for)
target_link_libraries(solver PRIVATE satyricon utils)

add_executable(pigeon_hole pigeon_hole/pigeon.cpp)
target_link_libraries(pigeon_hole PRIVATE utils)
target_compile_features(pigeon_hole PRIVATE cxx_range_for)

