// ====================================
// File:        timer.c
// Author:      Morgan Carpenetti
// Description: 
// Created On:  4/26/2026
// ====================================
#include "timer.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

uint64_t timer_frequency;

void timer_init() {
    LARGE_INTEGER f;
    if(!QueryPerformanceFrequency(&f)) {
        printf("ERROR: Failed to Create High Frequency Timer!\n");
    }
    timer_frequency = f.QuadPart;
}

TimeStep timer_get_timeval() {
    LARGE_INTEGER ticks;
    if(!QueryPerformanceCounter(&ticks)) {
        return 0;
    }
    ticks.QuadPart *= 1000000;
    ticks.QuadPart /= timer_frequency;
    return ticks.QuadPart;
}

uint64_t timestep_to_ms(TimeStep t) {
    return t / 1000;
}

float timestep_to_s(TimeStep t) {
    return (double)t / 1000000.0f;
}

