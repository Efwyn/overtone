// ====================================
// File:        load.h
// Author:      Morgan Carpenetti
// Description: Methods for loading 
//  files of different types
// Created On:  5/5/2026
// ====================================
#pragma once

#include "ec/types.h"

#include <stdlib.h>         //calloc
#include <stdio.h>          //printf
#include <string.h>         //memcpy
#include <vulkan/vulkan.h>  //shader module

typedef struct BinaryFile {
    char* data;
    size_t size;
} BinaryFile;

typedef struct Image {
    char* data;
    u32 width;
    u32 height;
} Image;

Result load_binary_file(const char* filename, BinaryFile* file);
Result create_shader_module(BinaryFile shaderFile, VkDevice device, VkShaderModule* shaderModule);
Result load_img_bmp(Image* img, BinaryFile file);
Result load_image(const char* filename, Image* img);
