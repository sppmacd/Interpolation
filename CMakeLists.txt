project(Interpolation)
cmake_minimum_required(VERSION 3.17)

find_package(LLGL REQUIRED)
set(CMAKE_CXX_STANDARD 20)

add_executable(interpolation main.cpp)
target_link_libraries(interpolation PUBLIC LLGL::3D)
