#include "common.h"
#include "net/net.h"
#include <SDL2/SDL.h>
#include "vid/vid.h"
#include "vid/console.h"
#include "input.h"
#include "vid/ui.h"
#include "client/client.h"

struct client_state cl = {0};

void *_B_malloc(size_t sz, const char *file, int line)
{
	void *ptr;

	ptr = malloc(sz);

	if(!ptr) {
		con_printf("%s:%d (%lu): it appears that you've run out of memory. i will help"
			   " out by killing myself, meanwhile you go and clean up"
			   " your computer, buy more ram or, if the number in parenthesis"
			   " is very big, report this error to the developer.\n", file, line, sz);
		exit(1);
	}

	return ptr;
}

float calc_frametime(void)
{
	static u_long now = 0;
	static u_long then = 0;

	then = now;
	now = SDL_GetPerformanceCounter();

	return ((float)(now - then) / (float)SDL_GetPerformanceFrequency());
}

void app_shutdown(void)
{
	net_shutdown();
	in_shutdown();
	ui_shutdown();
	vid_shutdown();
}

int main(void)
{
	u_long last_time = 0;
	in_init();

	con_init();

	net_init();

	vid_init();
	ui_init();

	while(!cl.done) {
		cl.frametime = calc_frametime();

		// maybe put input into tick_timer if too
		in_update();

		if(last_time - SDL_GetTicks64() >= 50) {
			net_process();
			last_time = SDL_GetTicks64();
		}

		vid_update();
		ui_draw();
		vid_display_frame();
	}

	app_shutdown();

	return 0;
}

