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

typedef struct Texture {
    char* data;
    u32 width;
    u32 height;
    u32 bpp;
} Texture;

Result load_binary_file(const char* filename, BinaryFile* file) {
    FILE* fd;
    if(fopen_s(&fd, filename, "rb") != 0) 
        return ResultFailure;

    //run to the end of the file and count how many bytes to allocate
    if(fseek(fd, 0, SEEK_END) < 0) return ResultFailure;
    size_t size = ftell(fd);
    file->data = malloc(size);

    if(fseek(fd, 0, SEEK_SET) < 0) return ResultFailure;

    if(fread(file->data, size, 1, fd) < 0) return ResultFailure;

    file->size = size;
    fclose(fd);

    return ResultOk;
}

Result create_shader_module(BinaryFile shaderFile, VkDevice device, VkShaderModule* shaderModule) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderFile.size,
        .pCode = (const u32*)shaderFile.data,
    };

    if(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, shaderModule) != VK_SUCCESS) {
        return ResultFailure;
    }
    return ResultOk;
}

//second header which we need starts atg +14
#define BMP_DIB_OFFSET 14

Result load_bmp(Texture* tex, BinaryFile file) {
    //base 14-byte header
    char* fileStart = file.data;
    void* imageDataStart = fileStart + *(u32*)(fileStart + 10); 
    //second header, type can be id'd by size field at beginning
    uint32_t headerSize = *(int*)(fileStart + BMP_DIB_OFFSET);
    switch(headerSize) {
        case 40: { //BITMAPINFOHEADER
            tex->width  = *(int*)(fileStart + BMP_DIB_OFFSET + 4);
            tex->height = *(int*)(fileStart + BMP_DIB_OFFSET + 8);
            tex->bpp    = *(u32*)(fileStart + BMP_DIB_OFFSET + 14);
            break;
        }
        default: {
            printf("Header Type not supported!\n");
            return ResultFailure;
        }
    }
    printf("BMP loaded: %d x %d, %dbpp\n", tex->width, tex->height, tex->bpp);


    tex->data = (char*)calloc(tex->width * tex->height, (tex->bpp >> 3) );
    if(tex->data == nullptr) {
        printf("ERROR: failed to calloc tex->data");
        return ResultFailure;
    }

    memcpy(tex->data, imageDataStart, tex->width * tex->height * (tex->bpp >> 3));

    return ResultOk;
}
