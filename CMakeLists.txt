cmake_minimum_required(VERSION 3.16)

project(SnakeSFML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(SnakeSFML
    src/main.cpp
)

target_link_libraries(SnakeSFML PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(MSVC)
    target_compile_options(SnakeSFML PRIVATE /W4 /permissive-)
else()
    target_compile_options(SnakeSFML PRIVATE -Wall -Wextra -Wpedantic)
endif()
