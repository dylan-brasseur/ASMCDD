cmake_minimum_required(VERSION 3.14)
project(DisksProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -O3 -march=native -m64 -fopenmp -D_FORTIFY_SOURCE=2")

add_executable(DisksProject main.cpp src/Mesh.cpp src/Shader.cpp src/utils.cpp src/Program.cpp src/Scene.cpp src/Camera.cpp src/LinePlot.cpp src/ASMCDD.cpp)
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

# Copy examples to binary directory
file(GLOB expl RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} examples/* )
foreach (s ${expl})
    configure_file(${s} ${s} COPYONLY)
endforeach ()

# Copy configs to binary directory
file(GLOB cfgs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} configs/* )
foreach (s ${cfgs})
    configure_file(${s} ${s} COPYONLY)
endforeach ()
