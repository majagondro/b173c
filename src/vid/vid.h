#ifndef B173C_VID_H
#define B173C_VID_H

#include "common.h"

struct gl_state {
	int w, h;
	u_int shader3d, shader2d;
};

void vid_init(void);
void vid_shutdown(void);
void vid_update(void);

void vid_update_viewport(void);
void vid_mouse_grab(bool grab);

void vid_display_frame(void);

void world_renderer_init(void);
void world_render(void);

#endif