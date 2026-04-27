// ========================================
// File: main.c
// Description: Overtone: A Game Engine Written in C
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ========================================
#include "types.h"
#include "window.h"
#include "renderer/renderer.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h> //EXIT_SUCCESS/EXIT_FAILURE


int main() {
    TimeStep startTime, endTime, elapsedTime;
    timer_init();
    //
    // Initialization
    //
    Window window;
    if(window_create(&window, 800 , 600, "Hello Overtone!") == ResultFailure) {
        printf("ERROR! Failed to Create Window\n");
        return EXIT_FAILURE;
    }

    window_show(&window);

    if(renderer_initialize(&window) == ResultFailure) {
        printf("ERROR! Failed to Initialize Renderer\n");
        return EXIT_FAILURE;
    }

    //
    // Main Loop
    //

    startTime = timer_get_timeval();
    uint64_t framecount = 0;

    bool running = true;
    while(running) {
        if(window_poll_events())
            running = false;


        //Loop Logic

        //Draw Frame
        if(renderer_draw_frame() != ResultOk) {
            printf("ERROR: Failed to draw frame\n");
            running = false;
        }
        framecount++;
    }
    endTime = timer_get_timeval();
    elapsedTime = endTime - startTime;

    printf("Total Frames: %llu, Elapsed Time: %.2fs\n", framecount, timestep_to_s(elapsedTime));
    printf("Avg Frame: %.2fms (%.2ffps)\n", (float) timestep_to_ms(elapsedTime) / framecount, (double)framecount / timestep_to_s(elapsedTime)); 
    renderer_wait_idle();

    //
    // Cleanup
    //
    renderer_shutdown();
    window_cleanup(&window);

    return EXIT_SUCCESS;
}
