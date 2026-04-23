cmake_minimum_required(VERSION 3.40)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

find_package(Vulkan REQUIRED)

project(overtone
        VERSION 0.1
        DESCRIPTION "A Game Engine Written in C"
        LANGUAGES C)

add_executable(hello src/main.c src/window.c src/renderer/renderer.c)
target_include_directories(hello PUBLIC ./include ./include/renderer)
target_link_libraries(hello PRIVATE Vulkan::Vulkan)

if(MSVC)
    target_compile_options(hello PRIVATE /W4 /WX)
else()
    target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
