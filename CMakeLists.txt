project(Shot2)
cmake_minimum_required(VERSION 3.17)

find_package(LLGL REQUIRED)
set(CMAKE_CXX_STANDARD 20)

add_executable(shot2
    main.cpp
)
target_link_libraries(shot2 PUBLIC LLGL::3D)
