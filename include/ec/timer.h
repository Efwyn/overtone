// ====================================
// File:        timer.h
// Author:      Morgan Carpenetti
// Description: A simple timer using QueryPerformanceCounter
// Created On:  4/26/2026
// ====================================
#pragma once

#include <stdint.h>
// Timestep represents an amount of time elapsed in microseconds
typedef uint64_t TimeStep;

#define TIMESTEP(stepName) TimeStep stepName = timer_get_timeval();

void timer_init();

TimeStep timer_get_timeval();

uint64_t timestep_to_ms(TimeStep t);
float    timestep_to_s(TimeStep t);
