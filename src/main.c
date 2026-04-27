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
    //
    // Initialization
    //
    timer_init();

    if(window_create(800 , 600, "Hello Overtone!")) {
        printf("ERROR! Failed to Create Window\n");
        return EXIT_FAILURE;
    }
    window_set_callback_resize(renderer_signal_framebuffer_resized);

    if(renderer_initialize() == ResultFailure) {
        printf("ERROR! Failed to Initialize Renderer\n");
        return EXIT_FAILURE;
    }

    //
    // Main Loop
    //

    TimeStep startTime = timer_get_timeval();
    uint64_t framecount = 0;

    bool running = true;
    while(running) {
        window_poll_events();
        if(window_should_close()) {
            printf("Window closed, terminating loop\n");
            running = false;
            break;
        }


        //Loop Logic

        //Draw Frame
        if(renderer_draw_frame() != ResultOk) {
            printf("ERROR: Failed to draw frame\n");
            running = false;
        }
        framecount++;
    }
    TimeStep elapsedTime = timer_get_timeval() - startTime;

    printf("Total Frames: %llu, Elapsed Time: %.2fs\n",
            framecount,
            timestep_to_s(elapsedTime));
    printf("Avg Frame: %.2fms (%.2ffps)\n",
            (float) timestep_to_ms(elapsedTime) / framecount,
            (double)framecount / timestep_to_s(elapsedTime)); 
    renderer_wait_idle();

    //
    // Cleanup
    //
    renderer_shutdown();
    window_cleanup();

    return EXIT_SUCCESS;
}
