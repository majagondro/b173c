#include "common.h"
#include "net/net.h"
#include <SDL2/SDL.h>
#include "vid/vid.h"
#include "client/console.h"
#include "input.h"
#include "vid/ui.h"
#include "client/client.h"
#include "assets.h"
#include "client/cvar.h"

struct client_state cl = {0};
void *_mem_alloc_impl(size_t sz, const char *file, int line)
{
	void *ptr;

	ptr = calloc(sz, 1);

	if(!ptr) {
		con_printf("%s:%d (%lu): it appears that you've run out of memory. i will help"
			   " out by killing myself, meanwhile you go and clean up"
			   " your computer, buy more ram, or, if the number in parenthesis"
			   " is very big, report this error to the developer.\n", file, line, sz);
		exit(1);
	}

	return ptr;
}

const char *va(const char *fmt, ...)
{
	static char buf[4096]; // fixme buf
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	return buf;
}

const char *utf16toutf8(const char16_t *str, size_t len)
{
    static char buf[4096]; // fixme buf
    memset(buf, 0, sizeof(buf));
    for(size_t i = 0; i < len && i < sizeof(buf); i++) {
        buf[i] = (char) str[i];
    }
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

   cvar_init();

   assets_init();
	in_init();
	con_init();
	net_init();
	vid_init();
	ui_init();
   entity_renderer_init();

	world_init();

	cmd_exec("exec config");
	cmd_exec("exec autoexec");

	while(!cl.done) {
		cl.frametime = calc_frametime();

		in_update();


		phys_timeout -= cl.frametime;
		if(phys_timeout <= 0.0f) {
			net_process();
			phys_timeout = 0.05f; // 20 updates per second
		}

		vid_update();
		ui_draw();
		vid_display_frame();

		if(!cl.active) { // todo: sys_inactivesleep cvar
			SDL_Delay(25);
		}
	}

	net_shutdown();
	in_shutdown();
    entity_renderer_shutdown();
	ui_shutdown();
	vid_shutdown();

	return 0;
}

