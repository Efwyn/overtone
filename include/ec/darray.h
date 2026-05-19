// =========================================
// File:        darray.h
// Author:      Morgan Carpenetti
// Description: A Dyanmically resizing array
// Created On:  5/12/2026
// =========================================
#pragma once

#include <assert.h>
#include <stdlib.h> //malloc (replace later?)
#include <stdio.h>
#include <string.h>

#include "ec/types.h"

//TODO: Improve the interface, by removing the need
// for the user to use typeSize directly

typedef struct DArray {
    char*  data;
    u32    typeSize;
    u32    capacity;
    u32    count;
} DArray;

// DArray_create:
// Create a new dynamic array that can hold a given number
//  of items of size [typeSize] (in bytes)
// Arguments:
//      capacity -- capacity for darray in bytes > 0
//      typeSize -- size of each element in darray in bytes > 0
static inline DArray DArray_create(u32 capacity, size_t typeSize) {
    assert(capacity > 0 && "initialCapacity must be > 0");
    assert(typeSize > 0 && "typeSize must be > 0");

    DArray newArray = {
        .typeSize = typeSize,
        .data = calloc(capacity, typeSize),
        .capacity = capacity,
    };
    return newArray;
}

// DArray_free:
// Description: Free the memory allocated by the DArray
// Arguments:
//      &array -- valid, initialized DArray
static inline void DArray_free(DArray* array) {
    assert(array != nullptr && "DArray is null");
    assert(array->data != nullptr && "DArray not initialized");

    if(array->data != nullptr) {
        free(array->data);
        array->data = nullptr;
    }
}

// DArray_push:
// Description: Push a new item onto the end of the array
// Arguments:
//      &array,  -- valid, initialized DArray
//      &item    -- address to item
//      itemSize -- size of the item, sanity check
static inline void DArray_push(DArray* array, void* item, size_t itemSize) {
    assert(array != nullptr && "DArray is null");
    assert(item != nullptr && "item is null");
    assert(itemSize == array->typeSize && "typeSize mismatch!");

    //if full, double the capacity and realloc
    if(array->count == array->capacity) {
        array->data = realloc(array->data, array->typeSize * array->capacity * 2);
        array->capacity *= 2;
    }

    // retrieve the pointer to the end of the darray
    char* dest = array->data + (array->typeSize * array->count);
    array->count++;

    //copy data in
    memcpy(dest, item, array->typeSize);
}

// DArray_at_ptr:
// Description: Retrieve pointer to an item from the 
//   DArray at the given index
// Arguments:
//      &darray  -- valid, initialized DArray
//      index    -- valid index, < darray.count
//      itemSize -- size of the item, sanity check
static inline void* DArray_at_ptr(DArray* array, u32 index, size_t itemSize) {
    assert(array != nullptr && "DArray is null");
    assert(array->data != nullptr && "DArray not initialized");
    assert(index < array->count && "invalid index!");
    assert(itemSize == array->typeSize && "typeSize mismatch!");

    return (char*)array->data + array->typeSize * index;
}
//macro that takes care of the common *(Type*) prefix on every call to DArray_at_ptr
#define DArray_at(Type, DArray, Index) *(Type*)DArray_at_ptr((DArray), (Index), sizeof(Type))
