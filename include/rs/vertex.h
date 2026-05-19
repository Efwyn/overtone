// ====================================
// File:        vertex.h
// Author:      Morgan Carpenetti
// Description: 
// Created On:  5/11/2026
// ====================================
#pragma once

#include "ec/math/vec_types.h"

typedef struct Vertex { 
    Vec3 pos;
    Vec2 texCoord;
} Vertex;

inline bool Vertex_cmp(Vertex left, Vertex right) {
    return Vec3_cmp(left.pos, right.pos) &&
           Vec2_cmp(left.texCoord, right.texCoord);
}

