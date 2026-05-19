// ====================================
// File:        load.c
// Author:      Morgan Carpenetti
// Description: 
// Created On:  5/7/2026
// ====================================
#include "ec/load.h"
#include "ec/darray.h"
#include "ec/math/vec_types.h"
#include "rs/vertex.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

//A list of unique vertices
typedef struct VertList {
    Vertex* verts;
    u32 count;
    u32 capacity;
} VertList;

VertList VertList_create(u32 capacity) {
    VertList vl = {};
    vl.verts = calloc(capacity, sizeof(Vertex));
    vl.capacity = capacity;
    return vl;
}

size_t VertList_find(VertList* vl, Vertex v) {
    for(size_t i = 0; i < vl->count; i++) {
        if(Vertex_cmp(vl->verts[i], v)) {
            return i;
        }
    }
    return ~0u;
}

size_t VertList_insert(VertList* vl, Vertex v) {
    size_t vertIdx = VertList_find(vl, v);
    if(vertIdx == ~0u) { //not found, add to the end
        if(vl->count == vl->capacity) {
            printf("More unique Vertices than expected, expanding!\n");
            vl->capacity *= 2;
            vl->verts = realloc(vl->verts, sizeof(Vertex) * vl->capacity);
        }
        vertIdx = vl->count;
        vl->verts[vertIdx] = v;
        vl->count++;
        return vertIdx;
    }
    return vertIdx;
}


//vertex format
// option 1: f v1 v2 v3 (positions only)
// option 2: f v1/vt1 v2/vt2 v3/vt3 ... (position, texCoord)
// option 3: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ... (position, texCoord, normal)
// option 4: f v1//vn1 v2//vn2 v3//vn3 ... (position, normal)

Vertex parse_vertex(char* substr, DArray* positions, DArray* texCoords) {
    Vertex vert = {};
    int vi = 0, vti = 0, vni = 0;
    char *v1, *v2, *v3, *context;
    u32 slashCount = 0;
    for(u32 i = 0; substr[i]; i++) {
        if(substr[i] == '/') slashCount++;
    }
    if(slashCount > 2) {
        printf("Invalid vertex, >2 '/' characters! (%s)\n", substr);
        printf("Invalid vertex Description, > 2 slashes\n");
        return vert;
    }

    //first token is always the position
    v1 = strtok_s(substr, "/", &context);
    vi = atoi(v1);


    if(slashCount == 1) {                   // Type 2: v1/vt1 (pos, uv)
        v2 = strtok_s(nullptr, "/", &context);
        vti = atoi(v2);
    }
    else if(slashCount == 2) {
        v2 = strtok_s(nullptr, "/", &context);
        v3 = strtok_s(nullptr, "/", &context);
        if(v3 != nullptr) {                 // Type 3: {v1/vt1/vn1} (pos, uv, norm) 
            vti = atoi(v2);
            vni = atoi(v3);
        }
        else {                              // Type 4: {v1//vn1} (pos, norm)
            vni = atoi(v2);
        }
    }

    //construct the vertex
    //positive values of vi/vti/vni are 1-indexed
    //negative values are distance from max
    u32 posidx = 0; u32 uvidx = 0; //u32 normidx = 0;
    if(vi == 0) {
        printf("Invalid vertex position index!\n");
        return vert;
    }

    posidx = (vi > 0) ? vi - 1 : positions->count + vi;
    if(posidx >= positions->count) {
        printf("Invalid vertex position index!\n");
        return vert;
    }
    vert.pos = DArray_at(Vec3, positions, posidx);

    if(vti != 0) {
        uvidx = (vti > 0) ? vti - 1 : texCoords->count + vti;
        if(uvidx >= texCoords->count) {
            printf("Invalid texCoord index!\n");
            return vert;
        }
        vert.texCoord = DArray_at(Vec2, texCoords, uvidx);
    }

    if(vni != 0) {
        /*
        normidx = (vi > 0) ? vni - 1 : normals->count + vni;
        if(normidx >= normals->count) {
            printf("Invalid normal index!\n");
            return vert;
        }
        .normal = *(Vec3*)DArrayAt(&normals, vni),
        */
    }
    return vert;
}


Result load_obj_file(const char* filename, MeshData* meshData) {
    FILE* fd;
    if(fopen_s(&fd, filename, "r") != 0) {
        printf("[LOAD]: File open failed!, (%s)\n", filename);
        return ResultFailure;
    }
    printf("[LOAD]: Obj File \"%s\"\n", filename);

    DArray positions = DArray_create(128, sizeof(Vec3));
    DArray texCoords = DArray_create(128, sizeof(Vec2));
    //DArray normals  = DArray_reate(128, sizeof(Vec3));
    VertList vl = VertList_create(4096);
    DArray indices   = DArray_create(128, sizeof(u32));

    #define OBJ_MAX_LINE 128
    char line[OBJ_MAX_LINE];
    char* context = nullptr;
    char* token = nullptr;

    while(fgets(line, OBJ_MAX_LINE, fd) != NULL) {
        token = strtok_s(line, " ", &context);
        if(token == nullptr) continue;

        if(token[0] == 'o') { // o {objectName}
            token = strtok_s(nullptr, " ", &context);
            //printf("object: %s", token);
        }
        else if(strcmp(token, "v") == 0) { // vertex positions
            // v {pos.x} {pos.y} {pos.z}
            Vec3 pos = {
                .x = atof(strtok_s(nullptr, " ", &context)),
                .y = atof(strtok_s(nullptr, " ", &context)),
                .z = atof(strtok_s(nullptr, " ", &context)),
            };
            DArray_push(&positions, &pos, sizeof(pos));
        }
        else if(strcmp(token, "vt") == 0) { // texture coordinates
            // vt {texCoord.x} {texCoord.y}
            Vec2 texCoord = {
                .x = atof(strtok_s(nullptr, " ", &context)),
                .y = atof(strtok_s(nullptr, " ", &context)),
            };
            DArray_push(&texCoords, &texCoord, sizeof(texCoord));
        }
        else if(strcmp(token, "f") == 0) { // faces
            // f {vi}/{vti}/{vni} x 3+
            while((token = strtok_s(nullptr, " \n", &context)) != nullptr) {
                //naiive implementation, lots of duplicate vertices
                Vertex vert = parse_vertex(token, &positions, &texCoords);
                u32 idx = VertList_insert(&vl, vert);
                DArray_push(&indices, &idx, sizeof(idx));
            }
        }
        else {
            //printf("x\n");
        }
    }

    //transfer memory ownership to the mesh struct
    meshData->vertices = vl.verts;
    meshData->vertexCount = vl.count;
    meshData->indices = (u32*)indices.data;
    meshData->indexCount = indices.count;

    printf("[LOAD]: %d Vertices, %d Indices\n", meshData->vertexCount, meshData->indexCount);
    DArray_free(&positions);
    DArray_free(&texCoords);
    fclose(fd);

    return ResultOk;
}
