#ifndef B173C_CLIENT_H
#define B173C_CLIENT_H

#include "common.h"
#include "mathlib.h"

extern struct client_state {
	struct {
		vec3 rot;
		vec3 pos;
		float stance;
		bool rotated, moved;
		int our_id;
	} game;

	enum { cl_disconnected, cl_connecting, cl_connected } state;

	bool done, active;

	u_long fps;
	u_long time;
	float frametime;
} cl;

#endif
