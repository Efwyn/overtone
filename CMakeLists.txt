cmake_minimum_required(VERSION 3.40)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 23)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

find_package(Vulkan REQUIRED)

project(overtone
        VERSION 0.1
        DESCRIPTION "A Game Engine Written in C"
        LANGUAGES C)

# https://thatonegamedev.com/cpp/cmake/how-to-compile-shaders-with-cmake/
function(add_shaders TARGET_NAME)
    set(SHADER_SOURCE_FILES ${ARGN})
    set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

    set(ENTRY_POINTS -entry vertMain -entry fragMain)

    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "No shader source files to add")
    endif ()

    add_custom_command(
            OUTPUT ${SHADER_OUT_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUT_DIR}
    )

    set(SHADER_COMMANDS)
    set(SHADER_PRODUCTS)

    foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
        cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
        cmake_path(GET SHADER_SOURCE STEM SHADER_NAME)

        list(APPEND SHADER_COMMANDS COMMAND)
        list(APPEND SHADER_COMMANDS "${SLANGC_EXECUTABLE}")
        list(APPEND SHADER_COMMANDS "${SHADER_SOURCE}")
        list(APPEND SHADER_COMMANDS -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name)
        list(APPEND SHADER_COMMANDS "${ENTRY_POINTS}")
        list(APPEND SHADER_COMMANDS "-o")
        list(APPEND SHADER_COMMANDS "${SHADER_OUT_DIR}/${SHADER_NAME}.spv")

        list(APPEND SHADER_PRODUCTS ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
    endforeach ()

   add_custom_target(
           ${TARGET_NAME} ALL
           ${SHADER_COMMANDS}
           COMMENT "Compiling Shaders for [${TARGET_NAME}]"
           SOURCES ${SHADER_SOURCE_FILES}
           BYPRODUCTS ${SHADER_PRODUCTS}
   )
endfunction()

add_executable(hello src/main.c src/window.c src/timer.c src/renderer/renderer.c)
target_include_directories(hello PUBLIC ./include ./include/renderer)
target_link_libraries(hello PRIVATE Vulkan::Vulkan)

add_shaders(hello_shaders shaders/triangle.slang)
add_dependencies(hello hello_shaders)

if(MSVC)
    target_compile_options(hello PRIVATE /W4 /WX)
else()
    target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

