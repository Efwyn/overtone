// ====================================
// File:        vector_test.c
// Author:      Morgan Carpenetti
// Description: 
// Created On:  5/1/2026
// ====================================
#include <stdio.h>

#include "ec/types.h"
#include "ec/math/vec_types.h"

bool Vec3_cmp_check () {
    Vec3 v1 = { 1.0f, 2.0f, 3.0f};
    Vec3 v2 = { 1.0f, 0.0f, 0.0f};
    Vec3 v3 = { 0.0f, 2.0f, 0.0f};
    Vec3 v4 = { 0.0f, 0.0f, 3.0f};
    return Vec3_cmp(v1, v1) &&
          !Vec3_cmp(v1, v2) &&
          !Vec3_cmp(v1, v3) &&
          !Vec3_cmp(v1, v4);
}

bool Vec3_add_check() {
    Vec3 v1 =     { 4.0f, 4.0f, 4.0f };
    Vec3 v2 =     { 1.0f, 2.0f, 3.0f };
    Vec3 answer = { 5.0f, 6.0f, 7.0f };
    return Vec3_cmp(Vec3_add(v1, v2), answer);
}

bool Vec3_sub_check() {
    Vec3 v1 =     { 4.0f, 4.0f, 4.0f };
    Vec3 v2 =     { 1.0f, 2.0f, 3.0f };
    Vec3 answer = { 3.0f, 2.0f, 1.0f };
    return Vec3_cmp(Vec3_sub(v1, v2), answer);
}

bool Vec3_scale_check() {
    Vec3 base =     {  1.0f, 2.0f, 3.0f };
    f32 scale =        4.0f;
    Vec3 answer =   {  4.0f, 8.0f, 12.0f };
    Vec3 inverted = { -1.0f, -2.0f, -3.0f };
    return Vec3_cmp(Vec3_scale(base, scale), answer) &&
           Vec3_cmp(Vec3_scale(base, -1.0f), inverted);
}

bool Vec3_len_sq_check() {
    Vec3 v = { 2.0f, -3.0f, 4.0f };
    f32 len_sq = 29.0f;
    return(Vec3_len_sq(v) == len_sq);
}

bool Vec3_len_check() {
    Vec3 v1 = { 3.0f, -4.0f, 0.0f };
    Vec3 v2 = { 0.0f, -4.0f, -3.0f };
    Vec3 v3 = { -3.0f, 0.0f, 4.0f };

    f32 len = 5.0f;
    return Vec3_len(v1) == len &&
           Vec3_len(v2) == len &&
           Vec3_len(v3) == len; 
}

bool Vec3_norm_check() {
    Vec3 v1 = { -5.0f, 0.0f, 0.0f };
    Vec3 v2 = { 0.0f, -10.0f,0.0f };
    Vec3 v3 = { 0.0f, 0.0f, -4.0f };

    Vec3 a1 = { -1.0f, 0.0f, 0.0f };
    Vec3 a2 = { 0.0f, -1.0f, 0.0f };
    Vec3 a3 = { 0.0f, 0.0f, -1.0f };

    return Vec3_cmp(Vec3_norm(v1), a1) &&
           Vec3_cmp(Vec3_norm(v2), a2) &&
           Vec3_cmp(Vec3_norm(v3), a3);
}

bool Vec3_dot_check() {
    Vec3 v1 = { -1.0, 2.0, -3.0 };
    Vec3 v2 = {  4.0, -5.0, 6.0 };
    f32  result = -32.0f;
    return Vec3_dot(v1, v2) == result;
}

bool Vec3_cross_check() {
    Vec3 i = { 1.0f, 0.0f, 0.0f };
    Vec3 j = { 0.0f, 1.0f, 0.0f };
    Vec3 k = { 0.0f, 0.0f, 1.0f };

    //i x j = k
    //j x k = i
    //k x i = j
    return Vec3_cmp(Vec3_cross(i, j), k) &&
           Vec3_cmp(Vec3_cross(j, k), i) &&
           Vec3_cmp(Vec3_cross(k, i), j);
}

int main() {

    if(Vec3_cmp_check()) {
        printf("[Ok]: Vec3_cmp_check\n");
    } else {
        printf("[Fail]: Vec3_cmp_check\n");
    }

    if(Vec3_add_check()) {
        printf("[Ok]: Vec3_add_check\n");
    } else {
        printf("[Fail]: Vec3_add_check\n");
    }

    if(Vec3_sub_check()) {
        printf("[Ok]: Vec3_sub_check\n");
    } else {
        printf("[Fail]: Vec3_sub_check\n");
    }

    if(Vec3_scale_check()) {
        printf("[Ok]: Vec3_scale_check\n");
    } else {
        printf("[Fail]: Vec3_scale_check\n");
    }

    if(Vec3_len_sq_check()) {
        printf("[Ok]: Vec3_len_sq_check\n");
    } else {
        printf("[Fail]: Vec3_len_sq_check\n");
    }

    if(Vec3_len_check()) {
        printf("[Ok]: Vec3_len_check\n");
    } else {
        printf("[Fail]: Vec3_len_check\n");
    }

    if(Vec3_norm_check()) {
        printf("[Ok]: Vec3_norm_check\n");
    } else {
        printf("[Fail]: Vec3_norm_check\n");
    }

    if(Vec3_dot_check()) {
        printf("[Ok]: Vec3_dot_check\n");
    } else {
        printf("[Fail]: Vec3_dot_check\n");
    }

    if(Vec3_cross_check()) {
        printf("[Ok]: Vec3_cross_check\n");
    } else {
        printf("[Fail]: Vec3_cross_check\n");
    }

    return 0;
}
