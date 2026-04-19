cmake_minimum_required(VERSION 3.40)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

project(overtone
        VERSION 0.1
        DESCRIPTION "A Game Engine Written in C"
        LANGUAGES C)

add_executable(hello src/main.c)
