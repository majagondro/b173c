#include "common.h"
#include "net.h"
#include <SDL2/SDL.h>

void *_B_malloc(size_t sz, const char *file, int line)
{
	void *ptr;

	ptr = malloc(sz);

	if(!ptr) {
		printf("%s:%d (%lu): it appears that you've run out of memory. i will help"
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

int main(void)
{
	net_init();
	while(1) {
		float dt = calc_frametime();
		net_process(dt);
	}

	return 0;
}

