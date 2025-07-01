cmake_minimum_required(VERSION 3.31)
project(sevensegmentdisplay)

set(CMAKE_CXX_STANDARD 26)

find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

include_directories(/home/cat/CLionProjects/sevensegmentdisplay/headers)

add_executable(sevensegmentdisplay src/Main.cpp src/Renderer.cpp src/glad.c)

target_include_directories(sevensegmentdisplay PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_link_libraries(sevensegmentdisplay PRIVATE glfw glm fontconfig)