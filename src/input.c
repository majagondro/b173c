#include <SDL2/SDL.h>
#include "input.h"
#include "vid/vid.h"
#include "client/console.h"
#include "client/client.h"
#include "vid/ui.h"

extern SDL_Window *window_handle;
struct key_status input_keys[512] = {0};

static int capslock_keymod = 0;

struct keyname {
	char *name;
	int keynum;
};

struct {
	u_byte forward : 1;
	u_byte back : 1;
	u_byte left : 1;
	u_byte right : 1;
	u_byte attack : 1;
	u_byte jump : 1;
	u_byte sneak : 1;
} inkeys;

cvar sensitivity = {"sensitivity", "2.5"};

void forwarddown_f(void) { inkeys.forward = 1; }
void forwardup_f(void) { inkeys.forward = 0; }
void backdown_f(void) { inkeys.back = 1; }
void backup_f(void) { inkeys.back = 0; }
void leftdown_f(void) { inkeys.left = 1; }
void leftup_f(void) { inkeys.left = 0; }
void rightdown_f(void) { inkeys.right = 1; }
void rightup_f(void) { inkeys.right = 0; }
void jumpdown_f(void) { inkeys.jump = 1; }
void jumpup_f(void) { inkeys.jump = 0; }
void sneakdown_f(void) { inkeys.sneak = 1; }
void sneakup_f(void) { inkeys.sneak = 0; }
void attackdown_f(void) { inkeys.attack = 1; }
void attackup_f(void) { inkeys.attack = 0; }

struct keyname keynames[] = {
	{"TAB",        KEY_TAB},
	{"ENTER",      KEY_RETURN},
	{"ESCAPE",     KEY_ESCAPE},
	{"SPACE",      KEY_SPACE},
	{"BACKSPACE",  KEY_BACKSPACE},
	{"UPARROW",    KEY_UP},
	{"DOWNARROW",  KEY_DOWN},
	{"LEFTARROW",  KEY_LEFT},
	{"RIGHTARROW", KEY_RIGHT},

	{"LALT",       KEY_LALT},
	{"RALT",       KEY_RALT},
	{"ALT",        KEY_LALT},
	{"LCTRL",      KEY_LCTRL},
	{"RCTRL",      KEY_RCTRL},
	{"CTRL",       KEY_LCTRL},
	{"LSHIFT",     KEY_LSHIFT},
	{"RSHIFT",     KEY_RSHIFT},
	{"SHIFT",      KEY_LSHIFT},

	{"F1",         KEY_F1},
	{"F2",         KEY_F2},
	{"F3",         KEY_F3},
	{"F4",         KEY_F4},
	{"F5",         KEY_F5},
	{"F6",         KEY_F6},
	{"F7",         KEY_F7},
	{"F8",         KEY_F8},
	{"F9",         KEY_F9},
	{"F10",        KEY_F10},
	{"F11",        KEY_F11},
	{"F12",        KEY_F12},

	{"INS",        KEY_INSERT},
	{"DEL",        KEY_DELETE},
	{"PGDN",       KEY_PAGEDOWN},
	{"PGUP",       KEY_PAGEUP},
	{"HOME",       KEY_HOME},
	{"END",        KEY_END},

	{"MOUSE1",     KEY_MOUSE1},
	{"MOUSE2",     KEY_MOUSE2},
	{"MOUSE3",     KEY_MOUSE3},
	{"MWHEELUP",   KEY_MOUSEWHEELUP},
	{"MWHEELDOWN", KEY_MOUSEWHEELDOWN},

	{"PAUSE",      KEY_PAUSE},

	{"SEMICOLON",  KEY_SEMICOLON},

	// todo: add more

	{NULL,         0}
};

int key_from_name(const char *name)
{
	struct keyname *kn;

	if (!name || !name[0])
		return -1;
	if (name[1] == 0) {
		if(name[0] >= 'A' && name[0] <= 'Z') {
			return name[0] - 'A' + KEY_A;
		} else if(name[0] >= 'a' && name[0] <= 'z') {
			return name[0] - 'a' + KEY_A;
		} else if(name[0] == '0') {
			return KEY_0;
		} else if(name[0] >= '1' && name[0] <= '9') {
			return name[0] - '1' + KEY_1;
		} else {
			return -1;
		}
	}

	for (kn = keynames; kn->name != NULL; kn++)
		if (!strcasecmp(name, kn->name))
			return kn->keynum;

	return -1;
}

const char *name_from_key(int key)
{
	struct keyname *kn;
	static char tmp[2] = {0};

	if(key >= KEY_A && key <= KEY_Z) {
		tmp[0] = (char) (key + 'A' - KEY_A);
		return tmp;
	} else if(key >= KEY_1 && key <= KEY_9) {
		tmp[0] = (char) (key + '1' - KEY_1);
		return tmp;
	} else if(key == KEY_0) {
		tmp[0] = '0';
		return tmp;
	}

	for (kn = keynames; kn->name != NULL; kn++)
		if(kn->keynum == key)
			return kn->name;

	return NULL;
}

void bind_f(void)
{
	int key;
	char *cmd;

	if (cmd_argc() != 2 && cmd_argc() != 3) {
		con_printf("usage: %s <key> [command]\n", cmd_argv(0));
		return;
	}

	key = key_from_name(cmd_argv(1));

	if(key == -1) {
		con_printf("invalid key\n");
		return;
	}

	if(cmd_argc() == 3) {
		cmd = cmd_argv(2);
		key_bind(key, cmd);
	} else {
		con_printf("%s : \"%s\"\n", cmd_argv(1), input_keys[key].binding);
	}
}

void bindlist_f(void)
{
	int maxwidth = 0;
	for(int i = 0; i < (int) (sizeof(input_keys) / sizeof(input_keys[0])); i++) {
		if(input_keys[i].binding) {
			int w = ui_strwidth(name_from_key(i));
			if(w > maxwidth) {
				maxwidth = w;
			}
		}
	}

	for(int i = 0; i < (int) (sizeof(input_keys) / sizeof(input_keys[0])); i++) {
		if(input_keys[i].binding) {
			const char *name = name_from_key(i);
			int px = maxwidth - ui_strwidth(name);
			con_printf("%s"COLOR_PADPX"%d : \"%s\"\n", name, px, input_keys[i].binding);
		}
	}
}

void unbind_f(void)
{
	int key;

	if (cmd_argc() != 2) {
		con_printf("usage: %s <key>\n", cmd_argv(0));
		return;
	}

	key = key_from_name(cmd_argv(1));

	if (key == -1) {
		con_printf("invalid key\n");
		return;
	}

	input_keys[key].binding = NULL;
}

void in_init(void)
{
	cmd_register("+forward", forwarddown_f);
	cmd_register("-forward", forwardup_f);
	cmd_register("+back", backdown_f);
	cmd_register("-back", backup_f);
	cmd_register("+left", leftdown_f);
	cmd_register("-left", leftup_f);
	cmd_register("+right", rightdown_f);
	cmd_register("-right", rightup_f);
	cmd_register("+attack", attackdown_f);
	cmd_register("-attack", attackup_f);
	cmd_register("+jump", jumpdown_f);
	cmd_register("-jump", jumpup_f);
	cmd_register("+sneak", sneakdown_f);
	cmd_register("-sneak", sneakup_f);

	cmd_register("bind", bind_f);
	cmd_register("bindlist", bindlist_f);
	cmd_register("unbind", unbind_f);

	cvar_register(&sensitivity);

	key_bind(KEY_ESCAPE, "toggleconsole");
}


static void handle_mouse(int x, int y, int scroll)
{
	if(con_opened) {
		con_scroll += scroll;
		if(con_scroll < 0)
			con_scroll = 0;
		return;
	}

	cl.game.rot[0] += ((float)(y)) * 0.022f * sensitivity.value;
	cl.game.rot[1] -= ((float)(x)) * 0.022f * sensitivity.value;

	// limit camera angles
	cl.game.rot[1] = fmodf(cl.game.rot[1], 360.0f);
	if(cl.game.rot[0] > 90.0f)
		cl.game.rot[0] = 90.0f;
	if(cl.game.rot[0] < -90.0f)
		cl.game.rot[0] = -90.0f;

	cl.game.rotated = true;
}

static void handle_keys(void)
{
	int key, keymod = 0;

	if (input_keys[KEY_CAPSLOCK].just_pressed)
		capslock_keymod = !capslock_keymod;

	keymod |= input_keys[KEY_LSHIFT].pressed ? KEYMOD_SHIFT : 0;
	keymod |= input_keys[KEY_RSHIFT].pressed ? KEYMOD_SHIFT : 0;
	keymod |= input_keys[KEY_LCTRL].pressed ? KEYMOD_CTRL : 0;
	keymod |= input_keys[KEY_RCTRL].pressed ? KEYMOD_CTRL : 0;
	keymod |= input_keys[KEY_LALT].pressed ? KEYMOD_ALT : 0;
	keymod |= input_keys[KEY_RALT].pressed ? KEYMOD_ALT : 0;
	keymod |= capslock_keymod ? KEYMOD_CAPSLOCK : 0;

	// notify console
	for (key = 0; key < KEY_NUM; key++) {
		if (con_opened && input_keys[key].echo && (key < KEY_LCTRL || key > KEY_RGUI))
			if(con_handle_key(key, keymod))
				continue;

		// check for keybinds
		if (!input_keys[key].binding)
			continue;

		if (input_keys[key].just_pressed) {
			cmd_exec(input_keys[key].binding, false);
		} else if (input_keys[key].just_released && input_keys[key].binding[0] == '+') {
			input_keys[key].binding[0] = '-';
			cmd_exec(input_keys[key].binding, false);
			input_keys[key].binding[0] = '+';
		}
	}


	if(cl.state == cl_connected && !con_opened) {
		//fixmee rmme
		float spd = 20.0f;
		vec3 fwd, side, up;
		cam_angles(fwd, side, up, cl.game.rot[1], 0.0f);
		fwd[1] = 0.0f;

		if(inkeys.forward) {
			cl.game.pos[0] += fwd[0] * cl.frametime * spd;
			cl.game.pos[2] += fwd[2] * cl.frametime * spd;
			cl.game.moved = true;
		} else if(inkeys.back) {
			cl.game.pos[0] -= fwd[0] * cl.frametime * spd;
			cl.game.pos[2] -= fwd[2] * cl.frametime * spd;
			cl.game.moved = true;
		}

		if(inkeys.left) {
			cl.game.pos[0] -= side[0] * cl.frametime * spd;
			cl.game.pos[2] -= side[2] * cl.frametime * spd;
			cl.game.moved = true;
		} else if(inkeys.right) {
			cl.game.pos[0] += side[0] * cl.frametime * spd;
			cl.game.pos[2] += side[2] * cl.frametime * spd;
			cl.game.moved = true;
		}

		if(inkeys.jump) {
			cl.game.pos[1] += cl.frametime * spd;
			cl.game.stance += cl.frametime * spd;
			cl.game.moved = true;
		} else if(inkeys.sneak) {
			cl.game.pos[1] -= cl.frametime * spd;
			cl.game.stance -= cl.frametime * spd;
			cl.game.moved = true;
		}
	}

}

void in_update(void)
{
	SDL_Event e;
	int i;

	for (i = 0; i < KEY_NUM; i++) {
		input_keys[i].just_pressed = false;
		input_keys[i].just_released = false;
		input_keys[i].echo = false;
	}

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT: {
				cl.done = true;
			} break;

			case SDL_KEYDOWN: {
				input_keys[e.key.keysym.scancode].pressed = true;
				input_keys[e.key.keysym.scancode].echo = true;
				if (!e.key.repeat)
					input_keys[e.key.keysym.scancode].just_pressed = true;
			} break;

			case SDL_KEYUP: {
				input_keys[e.key.keysym.scancode].pressed = false;
				if (!e.key.repeat)
					input_keys[e.key.keysym.scancode].just_released = true;
			} break;

			case SDL_MOUSEBUTTONDOWN: {
				switch(e.button.button) {
					case SDL_BUTTON_LEFT:
						i = KEY_MOUSE1;
						break;
					case SDL_BUTTON_RIGHT:
						i = KEY_MOUSE2;
						break;
					case SDL_BUTTON_MIDDLE:
						i = KEY_MOUSE3;
						break;
				}
				input_keys[i].pressed = true;
				input_keys[i].just_pressed = true;
			} break;

			case SDL_MOUSEBUTTONUP: {
				switch(e.button.button) {
					case SDL_BUTTON_LEFT:
						i = KEY_MOUSE1;
						break;
					case SDL_BUTTON_RIGHT:
						i = KEY_MOUSE2;
						break;
					case SDL_BUTTON_MIDDLE:
						i = KEY_MOUSE3;
						break;
				}
				input_keys[i].pressed = false;
				input_keys[i].just_released = true;
			} break;

			case SDL_MOUSEMOTION: {
				handle_mouse(e.motion.xrel, e.motion.yrel, 0);
			} break;

			case SDL_MOUSEWHEEL: {
				handle_mouse(0, 0, e.wheel.y);
			} break;

			case SDL_WINDOWEVENT: {
				if (e.window.event >= SDL_WINDOWEVENT_RESIZED && e.window.event <= SDL_WINDOWEVENT_MAXIMIZED)
					vid_update_viewport();
				switch (e.window.event) {
					case SDL_WINDOWEVENT_FOCUS_LOST:
					case SDL_WINDOWEVENT_MINIMIZED:
						cl.active = false;
						break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
					case SDL_WINDOWEVENT_RESTORED:
						cl.active = true;
						break;
				}
			} break;
		}
	}

	handle_keys();
}

void in_shutdown(void)
{

}

void key_bind(int key, char *bind)
{
	char *tmp;
	size_t len;

	len = strlen(bind);
	tmp = B_malloc(len + 1);
	strncpy(tmp, bind, len);
	tmp[len] = 0;

	if (input_keys[key].binding)
		B_free(input_keys[key].binding);

	input_keys[key].binding = tmp;
}
