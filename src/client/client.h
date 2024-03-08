#ifndef B173C_CLIENT_H
#define B173C_CLIENT_H

#include "common.h"
#include "mathlib.h"
#include "game/world.h"

extern struct client_state {
    struct {
        bool rotated, moved;
        entity *our_ent;
        int our_id;
        long seed;
        long time;
        struct trace_result look_trace;
        bool unstuck;
        vec3_t cam_pos;
    } game;

    enum { cl_disconnected, cl_connecting, cl_connected } state;

    bool done;   // used in the main loop
    bool active; // whether the window is focused

    ulong fps;
    float frametime;
    bool is_physframe;
} cl;

void cl_end_game(void);

#endif
