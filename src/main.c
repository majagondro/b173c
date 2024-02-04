#include "common.h"
#include "net/net.h"
#include <SDL2/SDL.h>
#include "vid/vid.h"
#include "client/console.h"
#include "input.h"
#include "vid/ui.h"
#include "client/client.h"
#include "assets.h"

struct client_state cl = {0};
void *_mem_alloc_impl(size_t sz, const char *file, int line)
{
	void *ptr;

	ptr = calloc(sz, 1);

	if(!ptr) {
		con_printf("%s:%d (%lu): it appears that you've run out of memory. i will help"
			   " out by killing myself, meanwhile you go and clean up"
			   " your computer, buy more ram or, if the number in parenthesis"
			   " is very big, report this error to the developer.\n", file, line, sz);
		exit(1);
	}

	return ptr;
}

const char *va(const char *fmt, ...)
{
	static char buf[4096];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	return buf;
}

float calc_frametime(void)
{
	static ulong now = 0;
	static ulong then = 0;

	then = now;
	now = SDL_GetPerformanceCounter();

	return ((float)(now - then) / (float)SDL_GetPerformanceFrequency());
}

int main(void)
{
	float phys_timeout = 0.0f;

	assets_init();
	in_init();
	con_init();
	net_init();
	vid_init();
	ui_init();

	world_init();

	cmd_exec("exec config", false);
	cmd_exec("exec autoexec", false);

	con_printf("lorem ipsum dolor sit amet.\n");

	while(!cl.done) {
		cl.frametime = calc_frametime();

		in_update();

		phys_timeout -= cl.frametime;
		if(phys_timeout <= 0.0f) {
			net_process();

			world_set_time(world_get_time() + 1); // client prediction of time

			phys_timeout = 0.05f; // 20 u/s
		}

		vid_update();
		ui_draw();
		vid_display_frame();
	}

	net_shutdown();
	in_shutdown();
	ui_shutdown();
	vid_shutdown();

	return 0;
}

