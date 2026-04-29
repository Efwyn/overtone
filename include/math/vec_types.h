// ====================================
// File:        vec_types.h
// Author:      Morgan Carpenetti
// Description: 
// Created On:  4/28/2026
// ====================================
#pragma once
#include "types.h"

//
// Vec2
//
typedef struct Vec2 {
    f32 x;
    f32 y;
} Vec2;

static inline Vec2 Vec2_add(Vec2 left, Vec2 right) {
    Vec2 sum = {left.x + right.x, left.y + right.y};
    return sum;
}

static inline Vec2 Vec2_scale(Vec2 v, f32 scale) {
    Vec2 scaled = {v.x * scale, v.y * scale};
    return scaled;
}

static inline f32 Vec2_length_2(Vec2 v) {
    return (v.x * v.x) + (v.y * v.y);
}

static inline f32 Vec2_dot(Vec2 a, Vec2 b) {
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

static inline Vec3 Vec3_add(Vec3 left, Vec3 right) {
    Vec3 sum = {left.x + right.x, left.y + right.y, left.z + left.z};
    return sum;
}

static inline Vec3 Vec3_scale(Vec3 v, f32 scale) {
    Vec3 scaled = {v.x * scale, v.y * scale, v.z * scale};
    return scaled;
}

static inline f32 Vec3_length_sq(Vec3 v) {
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

static inline f32 Vec3_dot(Vec3 a, Vec3 b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static inline Vec3 Vec3_cross(Vec3 a, Vec3 b) {
    Vec3 cross  = { a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x };
    return cross;
}
