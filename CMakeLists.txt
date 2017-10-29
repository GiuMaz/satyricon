cmake_minimum_required (VERSION 3.1)

project (satyricon VERSION 0.1 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 11)
include_directories(include)
file(GLOB CPP_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(satyricon ${CPP_SRCS} )

add_executable(solver solver/solver.cpp)
add_executable(pigeon_hole pigeon_hole/pigeon.cpp)

target_link_libraries(solver satyricon)

