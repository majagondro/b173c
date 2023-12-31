#include <SDL2/SDL.h>
#include <glad/glad.h>
#include "vid.h"
#include "shaders.h"
#include "client/console.h"
#include "ui.h"
#include "client/client.h"
#include "game/world.h"

SDL_Window *window_handle;
SDL_GLContext glcontext;

ulong frames_drawn = 0;
ulong last_check_tick = 0; // for fps because its a little more stable and looks nicer :)
ulong then = 0, now = 0; // for frametime

cvar vid_width  = {"vid_width", "1280"};
cvar vid_height = {"vid_height", "720"};

struct gl_state gl;

#define check_shader_compile(shader) check_shader_compile_impl(shader, #shader)

static bool check_shader_compile_impl(uint h, const char *name)
{
	int ok, loglen;
	char *log;

	glGetShaderiv(h, GL_COMPILE_STATUS, &ok);
	if(!ok) {
		glGetShaderiv(h, GL_INFO_LOG_LENGTH, &loglen);
		log = B_malloc(loglen + 1);
		memset(log, 0, loglen + 1);
		glGetShaderInfoLog(h, loglen, &loglen, log);
		con_printf("shader '%s' failed to compile: %s\n", name, log);
		glDeleteShader(h);
	}

	return ok;
}

static uint check_program_compile(uint h, uint h_vs, uint h_fs)
{
	int ok, loglen;
	char *log;

	glGetProgramiv(h, GL_LINK_STATUS, &ok);
	if(!ok) {
		glGetProgramiv(h, GL_INFO_LOG_LENGTH, &loglen);
		log = B_malloc(loglen + 1);
		memset(log, 0, loglen + 1);
		glGetProgramInfoLog(h, loglen, &loglen, log);
		con_printf("shader program failed to compile:\n%s\n", log);
		glDeleteProgram(h);
		glDeleteShader(h_vs);
		glDeleteShader(h_fs);
		return 0;
	}

	glDetachShader(h, h_vs);
	glDetachShader(h, h_fs);
	glDeleteShader(h_vs);
	glDeleteShader(h_fs);

	return h;
}

static uint load_shader(const char *vs, const char *fs, const char *gs)
{
	uint h_vs, h_fs, h_gs, h_prog;

	// vertex shader
	h_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(h_vs, 1, &vs, NULL);
	glCompileShader(h_vs);
	if(!check_shader_compile(h_vs))
		return 0;

	// fragment shader
	h_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(h_fs, 1, &fs, NULL);
	glCompileShader(h_fs);
	if(!check_shader_compile(h_fs))
		return 0;

	// geometry shader
	if(gs) {
		h_gs = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(h_gs, 1, &gs, NULL);
		glCompileShader(h_gs);
		if(!check_shader_compile(h_gs)) {
			return 0;
		}
	}

	h_prog = glCreateProgram();
	glAttachShader(h_prog, h_vs);
	glAttachShader(h_prog, h_fs);
	if(gs) {
		glAttachShader(h_prog, h_gs);
	}
	glLinkProgram(h_prog);
	return check_program_compile(h_prog, h_vs, h_fs);
}

void vid_init(void)
{
	int flags, ok;
	int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;

	cvar_register(&vid_height);
	cvar_register(&vid_width);

	/* ----- SDL INIT ----- */
	ok = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	if(ok < 0)
		con_printf("SDL error: %s", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, true);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_SHOWN;
	window_handle = SDL_CreateWindow("b173c", x, y, vid_width.integer, vid_height.integer, flags);
	if(!window_handle)
		con_printf("Failed to create window: %s", SDL_GetError());

	/* ----- OPENGL INIT ----- */
	glcontext = SDL_GL_CreateContext(window_handle);
	if(!glcontext)
		con_printf("Failed to create OpenGL context: %s", SDL_GetError());

	ok = gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);
	if(!ok)
		con_printf("Failed to initialize GLAD");

	// load shaders
	gl.shader3d = load_shader(shader_vertex, shader_fragment, shader_geometry);
	gl.shader2d = load_shader(shader_vertex2d, shader_fragment2d, NULL);

	SDL_GL_SetSwapInterval(0);

	now = SDL_GetPerformanceCounter();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	world_renderer_init();
}

void vid_shutdown(void)
{
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window_handle);

	SDL_Quit();

	world_renderer_shutdown();

	glDeleteProgram(gl.shader3d);
	glDeleteProgram(gl.shader2d);
	SDL_DestroyWindow(window_handle);
}

void ui_update_size(int, int);
void vid_update_viewport(void)
{
	SDL_GetWindowSize(window_handle, &gl.w, &gl.h);
	glViewport(0, 0, gl.w, gl.h);
	cvar_set("vid_width", va("%d", gl.w));
	cvar_set("vid_height", va("%d", gl.h));
	cvar_find("ui_scale")->onchange(); // hack as fack
}

void vid_mouse_grab(bool grab)
{
	SDL_SetWindowGrab(window_handle, grab ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
}

void vid_update(void)
{
	if(SDL_GetRelativeMouseMode()) {
		SDL_WarpMouseInWindow(window_handle, gl.w / 2, gl.h / 2);
	}
}

void vid_display_frame(void)
{
	int e;
	float *clearcolor = world_get_sky_color();
	glClearColor(clearcolor[0], clearcolor[1], clearcolor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* draw 3d stuff */
	glUseProgram(gl.shader3d);
	world_render();

	e = glGetError();
	if(e) {
		con_printf("gl_error: %d\n", e);
		exit(1);
	}

	/* draw 2d stuff */
	glPolygonMode(GL_FRONT_AND_BACK,  GL_FILL);
	glUseProgram(gl.shader2d);
	ui_commit();

	e = glGetError();
	if(e) {
		con_printf("gl_error: %d\n", e);
		exit(1);
	}

	SDL_GL_SwapWindow(window_handle);

	frames_drawn++;
	if(SDL_GetTicks64() - last_check_tick > 1000) {
		cl.fps = frames_drawn;
		last_check_tick = SDL_GetTicks64();
		frames_drawn = 0;
	}
}
