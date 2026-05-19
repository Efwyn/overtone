// ====================================
// File:        vec_types.h
// Author:      Morgan Carpenetti
// Description: 
// Created On:  4/28/2026
// ====================================
#pragma once
#include "ec/types.h"
#include <math.h>
#include <stdio.h>

//
// Vec2
//
typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

inline void Vec2_print(Vec2 v) {
    printf("<%f, %f>\n", v.x, v.y);
}

inline bool Vec2_cmp(Vec2 left, Vec2 right) {
    return left.x == right.x && left.y == right.y;
}

inline Vec2 Vec2_add(Vec2 left, Vec2 right) {
    Vec2 sum = { left.x + right.x, left.y + right.y };
    return sum;
}

inline Vec2 Vec2_sub(Vec2 left, Vec2 right) {
    Vec2 result = { left.x - right.x, left.y - right.y };
    return result;
}

inline Vec2 Vec2_scale(Vec2 v, f32 scale) {
    Vec2 scaled = { v.x * scale, v.y * scale };
    return scaled;
}

inline f32 Vec2_len_sq(Vec2 v) {
    return (v.x * v.x) + (v.y * v.y);
}

inline f32 Vec2_len(Vec2 v) {
    return sqrtf((v.x * v.x) + (v.y * v.y));
}

inline Vec2 Vec2_norm(Vec2 v) {
    f32 length = Vec2_len(v);
    return Vec2_scale(v, 1.0f / length);
}

inline f32 Vec2_dot(Vec2 a, Vec2 b) {
    return (a.x * b.x) + (a.y * b.y);
}

//
// Vec3
//
typedef struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

inline void Vec3_print(Vec3 v) {
    printf("<%f, %f, %f>\n", v.x, v.y, v.z);
}

inline bool Vec3_cmp(Vec3 left, Vec3 right) {
    return left.x == right.x &&
           left.y == right.y &&
           left.z == right.z;
}

inline Vec3 Vec3_add(Vec3 left, Vec3 right) {
    Vec3 sum = {left.x + right.x,
                left.y + right.y,
                left.z + right.z   };
    return sum;
}

inline Vec3 Vec3_sub(Vec3 left, Vec3 right) {
    Vec3 result = {left.x - right.x,
                   left.y - right.y,
                   left.z - right.z };
    return result;
}

inline Vec3 Vec3_scale(Vec3 v, f32 scale) {
    Vec3 scaled = { v.x * scale,
                    v.y * scale,
                    v.z * scale };
    return scaled;
}

inline f32 Vec3_len_sq(Vec3 v) {
    return (v.x * v.x) +
           (v.y * v.y) +
           (v.z * v.z);
}

inline f32 Vec3_len(Vec3 v) {
    return sqrtf((v.x * v.x) +
                 (v.y * v.y) +
                 (v.z * v.z));
}

inline Vec3 Vec3_norm(Vec3 v) {
    f32 length = Vec3_len(v);
    return Vec3_scale(v, 1.0f / length);
}

inline f32 Vec3_dot(Vec3 a, Vec3 b) {
    return (a.x * b.x) +
           (a.y * b.y) +
           (a.z * b.z);
}

inline Vec3 Vec3_cross(Vec3 a, Vec3 b) {
    Vec3 cross  = { a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x };
    return cross;
}

//
// Vec4
//
typedef struct Vec4 {
    f32 x, y, z, w;
} Vec4;

inline void Vec4_print(Vec4 v) {
    printf("<%f, %f, %f, %f>\n", v.x, v.y, v.z, v.w);
}
