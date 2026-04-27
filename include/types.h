// ========================================
// File: include/types.h
// Description: Commonly Used Generic Types
// Author: Nicholas Carpenetti
// Created On: 19-04-2026
// ========================================
#pragma once
#include <stdint.h>

typedef uint32_t u32;

inline u32 min_u32 (u32 a, u32 b) {
    return a < b ? a : b;
}

inline u32 max_u32 (u32 a, u32 b) {
    return a > b ? a : b;
}

inline u32 clamp_u32 (u32 val, u32 min, u32 max) {
    const u32 t = val < min ? min : val;
    return t > max ? max : t;
}

typedef enum Result {
    ResultOk,
    ResultFailure
} Result;

