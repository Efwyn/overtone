// ====================================
// File:        matrix.h
// Author:      Morgan Carpenetti
// Description: Matrix Types and Operations
// Created On:  4/29/2026
// ====================================
#pragma once
#include "types.h"

#include "math/vec_types.h"
#include <math.h>
#include <stdio.h>

typedef struct Mat4 {
    f32 m11, m12, m13, m14;
    f32 m21, m22, m23, m24;
    f32 m31, m32, m33, m34;
    f32 m41, m42, m43, m44;
} Mat4;

inline void Mat4_print(Mat4 m) {
    printf("%f, %f, %f, %f\n", m.m11, m.m12, m.m13, m.m14);
    printf("%f, %f, %f, %f\n", m.m21, m.m22, m.m23, m.m24);
    printf("%f, %f, %f, %f\n", m.m31, m.m32, m.m33, m.m34);
    printf("%f, %f, %f, %f\n\n", m.m41, m.m42, m.m43, m.m44);
}

const Mat4 Mat4_Identity = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

typedef struct Mat2 {
    f32 m11, M12;
    f32 m21, m22;
} Mat2;

//TODO: Implement this instead?
//Strassen's Algorithm
// A = [X Y] B = [P Q]  C = [XP + YR XQ + YS]
//     [Z W]     [R S]      [ZP + WR ZQ + WS]
//P1 = X(Q - S)
//P2 = (X + Y)S
//P3 = (Z + W)P
//P4 = W(R - P)
//P5 = (X + W)(P + S)
//P6 = (Y - W)(R + S)
//P7 = (X - Z)(P + Q)
//A * B = [P5 + P4 - P2 + P6       P1 + P2     ]
//        [     P3 + P4       P1 + P5 - P3 - P7]
inline Mat4 Mat4_multiply(Mat4 A, Mat4 B) {
    Mat4 result = {
        result.m11 = (A.m11 * B.m11) + (A.m12 * B.m21) + (A.m13 * B.m31) + (A.m14 * B.m41),
        result.m12 = (A.m11 * B.m12) + (A.m12 * B.m22) + (A.m13 * B.m32) + (A.m14 * B.m42),
        result.m13 = (A.m11 * B.m13) + (A.m12 * B.m23) + (A.m13 * B.m33) + (A.m14 * B.m43),
        result.m14 = (A.m11 * B.m14) + (A.m12 * B.m24) + (A.m13 * B.m34) + (A.m14 * B.m44),
        //
        result.m21 = (A.m21 * B.m11) + (A.m22 * B.m21) + (A.m23 * B.m31) + (A.m24 * B.m41),
        result.m22 = (A.m21 * B.m12) + (A.m22 * B.m22) + (A.m23 * B.m32) + (A.m24 * B.m42),
        result.m23 = (A.m21 * B.m13) + (A.m22 * B.m23) + (A.m23 * B.m33) + (A.m24 * B.m43),
        result.m24 = (A.m21 * B.m14) + (A.m22 * B.m24) + (A.m23 * B.m34) + (A.m24 * B.m44),
        //
        result.m31 = (A.m31 * B.m11) + (A.m32 * B.m21) + (A.m33 * B.m31) + (A.m34 * B.m41),
        result.m32 = (A.m31 * B.m12) + (A.m32 * B.m22) + (A.m33 * B.m32) + (A.m34 * B.m42),
        result.m33 = (A.m31 * B.m13) + (A.m32 * B.m23) + (A.m33 * B.m33) + (A.m34 * B.m43),
        result.m34 = (A.m31 * B.m14) + (A.m32 * B.m24) + (A.m33 * B.m34) + (A.m34 * B.m44),
        //
        result.m41 = (A.m41 * B.m11) + (A.m42 * B.m21) + (A.m43 * B.m31) + (A.m44 * B.m41),
        result.m42 = (A.m41 * B.m12) + (A.m42 * B.m22) + (A.m43 * B.m32) + (A.m44 * B.m42),
        result.m43 = (A.m41 * B.m13) + (A.m42 * B.m23) + (A.m43 * B.m33) + (A.m44 * B.m43),
        result.m44 = (A.m41 * B.m14) + (A.m42 * B.m24) + (A.m43 * B.m34) + (A.m44 * B.m44),
    };
    return result;
}
// Matrix for rotation about an axis
inline Mat4 Mat4_rotate(Mat4 M, f32 angle, Vec3 axis) {
    const f32 c = cosf(angle);
    const f32 s = sinf(angle);

    Vec3   A = Vec3_norm(axis);
    Vec3 tmp = Vec3_scale(A, 1 - c);

    Mat4 R = {
        .m11 = c + tmp.x * A.x,         //c + (1 - c) * Ax^2
        .m21 = tmp.x * A.y - s * A.z,   //(1 - c)AxAy - sAz
        .m31 = tmp.x * A.z - s * A.y,   //(1 - c)AxAz + sAy
        //
        .m12 = tmp.x * A.y + s * A.z,   //(1 - c)AxAy + sAz
        .m22 = c + tmp.y * A.y,         //c + (1 - c)Ay^2
        .m32 = tmp.y * A.z - s * A.x,   //(1 - c)AyAz - sAx
        //
        .m13 = tmp.x * A.z - s * A.y,   //(1 - c)AxAz - sAy
        .m23 = tmp.y * A.z + s * A.x,   //(1 - c)AyAz + sAx
        .m33 = c + tmp.z * A.z,         //c + (1 - c)Az^2
        //
        .m44 = 1.0f,
    };

    //matrix multiply M * R
    return Mat4_multiply(M, R);
}

inline Mat4 Mat4_translate(Vec3 d) {
    Mat4 T = Mat4_Identity;
    T.m14 = d.x;
    T.m24 = d.y;
    T.m34 = d.z;
    return T;
}

// Based on Real-time Rendering, 4th Edition, figure 4.20
inline Mat4 Mat4_lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = Vec3_norm(Vec3_sub(center, eye));
    Vec3 s = Vec3_norm(Vec3_cross(f, up));
    Vec3 u = Vec3_cross(s, f);

    Mat4 M = {
          s.x,   s.y,   s.z,  Vec3_dot(s, eye),
          u.x,   u.y,   u.z,  Vec3_dot(u, eye),
         -f.x,  -f.y,  -f.z,  Vec3_dot(f, eye),
          0.0f,  0.0f,  0.0f, 1.0f,
    };
    return M;
}


// Perspective projection transform
// Figure 4.75, Real-time Rendering, 4th Edition
inline Mat4 Mat4_perspective(f32 fov, f32 ar, f32 n, f32 f) {
   f32 c = 1.0f / tan(fov / 2.0f);
   f32 dSum = n + f;
   f32 dDiff = f - n;
   Mat4 P = {
           c / ar,             0.0f,        0.0f,             0.0f,
            0.0f,              -c,          0.0f,             0.0f,
            0.0f,              0.0f,  (-dSum / dDiff), (-2.0f * f * n) / (dDiff),
            0.0f,              0.0f,       -1.0f,             0.0f,
   };
    return P;
}


inline Mat4 Mat4_transpose(Mat4 M) {
    Mat4 T = {
        M.m11, M.m21, M.m31, M.m41,
        M.m12, M.m22, M.m31, M.m42,
        M.m13, M.m23, M.m33, M.m43,
        M.m14, M.m24, M.m34, M.m44,
    };
    return T;
}
