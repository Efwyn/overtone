// ====================================
// File:        load.c
// Author:      Morgan Carpenetti
// Description: 
// Created On:  5/7/2026
// ====================================
#include "ec/load.h"
#include <stdint.h>

Result load_binary_file(const char* filename, BinaryFile* file) {
    FILE* fd;
    if(fopen_s(&fd, filename, "rb") != 0) {
        printf("File open failed!, (%s)\n", filename);
        return ResultFailure;
    }

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

//second header which we need starts at +14
#define DIB 14
//retrieve memory at an address + offset and cast to given type
#define EXTRACT_VAL(Type, Address, Offset) *(Type*)((Address) + (Offset))

Result load_img_bmp(Image* img, BinaryFile file) {
    //base 14-byte header
    char* fileStart = file.data;
    char* imageData = fileStart + EXTRACT_VAL(u32, fileStart, 10);

    //second header, type can be id'd by size field at beginning
    u32 headerSize = EXTRACT_VAL(u32, fileStart, DIB);
    u32 bpp = 0;
    switch(headerSize) {
        case 40: { //BITMAPINFOHEADER
            img->width  = EXTRACT_VAL(u32, fileStart, DIB + 4);
            img->height = EXTRACT_VAL(u32, fileStart, DIB + 8);
            bpp         = EXTRACT_VAL(u32, fileStart, DIB + 14);
            break;
        }
        default: {
            printf("BMP Header Type not supported! (size of %d)\n", headerSize);
            return ResultFailure;
        }
    }
    printf("[LOAD] BMP: %d x %d, %dbpp\n", img->width, img->height, bpp);

    if(bpp < 24) {
        printf("BMP files with bpp < 24 not supported!\n");
        return ResultFailure;
    }


    // Output will always be 32 bit pixels, RGBA
    u32 pixelCount = img->width * img->height;
    img->data = (char*)calloc(pixelCount, 4);
    if(img->data == nullptr) {
        printf("ERROR: failed to alloc data for BMP file");
        return ResultFailure;
    }

    if(bpp == 32) {
        //BGRA order
        for(u32 i = 0; i < pixelCount; i++) {
            (img->data)[4 * i + 0] = imageData[4 * i + 2];
            (img->data)[4 * i + 1] = imageData[4 * i + 1];
            (img->data)[4 * i + 2] = imageData[4 * i + 0];
            (img->data)[4 * i + 3] = imageData[4 * i + 3];
        }
    }
    else if(bpp == 24) {
        //BGR order
        for(u32 i = 0; i < pixelCount; i++) {
            (img->data)[4 * i + 0] = imageData[3 * i + 2];
            (img->data)[4 * i + 1] = imageData[3 * i + 1];
            (img->data)[4 * i + 2] = imageData[3 * i + 0];
        }
    }

    return ResultOk;
}

Result load_image(const char* filename, Image* image) {
    BinaryFile file;
    load_binary_file(filename, &file);

    // Figure out file type by extension and verify
    if(file.data[0] == 'B' && file.data[1] == 'M') {
        printf("BMP file detected!\n");
        if(load_img_bmp(image, file) != ResultOk) {
            printf("Failed to load image!\n");
            free(file.data);
            return ResultFailure;
        }
   } else if((uint8_t)file.data[0] == 0x89 && !strncmp(file.data + 1, "PNG", 3)) {
        printf("PNG not supported yet!\n");
        free(file.data);
        return ResultFailure;
   } else {
        printf("Filetype not supported!\n");
        free(file.data);
        return ResultFailure;
    }

    return ResultOk;
}

