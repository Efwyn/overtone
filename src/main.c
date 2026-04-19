// ========================================
// File: main.c
// Description: Overtone: A Game Engine Written in C
// Author: Morgan Carpenetti
// Created On: 19-04-2026
// ========================================
#include "types.h"
#include "window.h"

#include <stdio.h>

int main(int argc, char** argv) {
    Window window;
    if(window_create(&window, 640, 480, "Hello Overtone!") == ResultFailure) {
        printf("ERROR! Failed to Create Window\n");
        return EXIT_FAILURE;
    }

    window_show(&window);

    bool running = true;
    while(running) {
        if(window_poll_events())
            running = false;

        //Loop Logic
    }

    window_cleanup(&window);

    return EXIT_SUCCESS;
}

