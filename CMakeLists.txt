cmake_minimum_required(VERSION 3.14)
project(DisksProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -O3 -march=native -m64")

add_executable(DisksProject main.cpp Mesh.cpp Shader.cpp utils.cpp Program.cpp Scene.cpp Camera.cpp include/Camera.h)
target_link_libraries(DisksProject GL GLEW glut pthread)

# Copy shaders to binary directory
file(GLOB shds RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} shaders/* )
foreach (s ${shds})
    configure_file(${s} ${s} COPYONLY)
endforeach ()

# Copy models to binary directory
file(GLOB meshes RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} models/* )
foreach (s ${meshes})
    configure_file(${s} ${s} COPYONLY)
endforeach ()
