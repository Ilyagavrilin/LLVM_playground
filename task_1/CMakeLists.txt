cmake_minimum_required(VERSION 3.7)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
add_compile_options(-fPIC)

project(APP)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(APP app.c sim.c start.c)
target_link_libraries(APP ${SDL2_LIBRARIES})