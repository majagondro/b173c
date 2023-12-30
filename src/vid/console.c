#include "vid/ui.h"
#include "console.h"
#include <SDL2/SDL.h>
#include "input.h"
#include "vid/vid.h"

#define CONSOLE_MAX_LINES 512

struct conline {
	char line[256];
	int len;
	struct conline *next, *prev;
} *conlines = NULL;

#define MAX_INPUT_LEN 256
static char input_line[MAX_INPUT_LEN+1] = {0};
static int input_pos = 0;
static int history_idx = 0;

static ulong flash_tick = 0;
static u_byte flash_char = '|';
static bool flash = false;

bool con_opened = true;
int con_scroll = 0;

void toggleconsole_f(void)
{
	con_opened = !con_opened;
	vid_mouse_grab(!con_opened);
}

void con_show(void)
{
	if(!con_opened)
		toggleconsole_f();
}

void con_hide(void)
{
	if(con_opened)
		toggleconsole_f();
}

void con_init(void)
{
	conlines = B_malloc(sizeof(*conlines));
	memset(conlines, 0, sizeof(*conlines));

	cmd_init();

	cmd_register("toggleconsole", toggleconsole_f);
}

static void con_putinc(u_byte c, bool red)
{
	if(input_pos < MAX_INPUT_LEN) {
		if(strnlen(input_line, MAX_INPUT_LEN) == MAX_INPUT_LEN) {
			// dont
		} else {
			memmove(&input_line[input_pos + 1], &input_line[input_pos], MAX_INPUT_LEN - input_pos - 1);
			input_line[input_pos++] = red ? REDCHAR(c) : c;
		}
	}
}

static void free_excess_lines(void)
{
	int count = 0;
	struct conline *l = conlines;

	while(l->next != NULL) {
		l = l->next;
		count++;
	}

	while(count-- > CONSOLE_MAX_LINES) {
		l = l->prev;
		B_free(l->next);
		l->next = NULL;
	}
}

static void add_line(struct conline *l)
{
	conlines->prev = l;
	l->next = conlines;
	conlines = l;
	free_excess_lines();
}

static void con_submit(void)
{
	con_printf("]%s\n", input_line);

	cmd_exec((char *) input_line, false);

	memset(input_line, 0, MAX_INPUT_LEN);
	input_pos = 0;

	// reset history index
	history_idx = 0;

	// scroll back to bottom
	con_scroll = 0;
}

static struct conline *get_nth_inputted_line(int n)
{
	struct conline *c;
	int i;

	if(!conlines)
		return NULL;

	c = conlines;
	for(i = 0; i < n+1 && c != NULL;) {
		if(c->line[0] == ']' && c->line[1] != '\0') {
			i++;
			if(i >= n+1) {
				break;
			}
		}
		c = c->next;
	}
	return c;
}

static int calc_max_scroll(void)
{
	return 0;
}

bool con_handle_key(int key, int keymod)
{
	char c;
	int caps_strength = 0;
	bool red = keymod & KEYMOD_ALT;

	if(!con_opened)
		return false;

	caps_strength = ((keymod & KEYMOD_SHIFT) != 0) ^ ((keymod & KEYMOD_CAPSLOCK) != 0);

	if(key >= KEY_A && key <= KEY_Z) {
		if(key == KEY_V && keymod & KEYMOD_CTRL) {
			char *clipboard = SDL_GetClipboardText();
			int i;
			for(i = 0; i < (int) strlen(clipboard); i++)
				con_putinc(clipboard[i], false);
			SDL_free(clipboard);
			return true;
		}

		c = (char) ((key - KEY_A) + 'a' - (caps_strength * ('a' - 'A')));
		con_putinc((u_byte) c, red);
		return true;
	}

	switch(key) {
		case KEY_ESCAPE: {
			toggleconsole_f();
		} break;

		case KEY_BACKSPACE: {
			if(keymod & KEYMOD_CTRL) {
				con_putinc(127, red);
			} else {
				if (input_pos > 0) {
					if(input_pos == MAX_INPUT_LEN) {
						input_line[input_pos - 1] = 0;
					} else {
						memmove(&input_line[input_pos - 1], &input_line[input_pos], MAX_INPUT_LEN - input_pos);
					}
					input_pos--;
				}
			}
		} break;

		case KEY_DELETE: {
			if(input_pos < MAX_INPUT_LEN) {
				memmove(&input_line[input_pos], &input_line[input_pos + 1], MAX_INPUT_LEN - input_pos);
			}
		} break;

		case KEY_RETURN: {
			con_submit();
		} break;

		case KEY_HOME:
			if(keymod & KEYMOD_CTRL) {
				con_scroll = 0;
			} else {
				input_pos = 0;
			}
			break;

		case KEY_END:
			if(keymod & KEYMOD_CTRL) {
				con_scroll = calc_max_scroll();
			} else {
				input_pos = MAX_INPUT_LEN;
			}
			break;

		case KEY_LEFT:
			if(input_pos > 0)
				input_pos--;
			break;
		case KEY_RIGHT:
			if(input_pos < (int) strnlen(input_line, MAX_INPUT_LEN))
				input_pos++;
			break;

		case KEY_DOWN:
			history_idx -= 2;
			if(history_idx < -1)
				history_idx = -1;
		case KEY_UP: {
			struct conline *l;
			l = get_nth_inputted_line(history_idx);
			if(l == NULL) {
				if(history_idx != 0) {
					history_idx = 0;
					memset(input_line, 0, sizeof(input_line));
					input_pos = 0;
				}
				return true;
			}
			strlcpy(input_line, l->line+1, 255);
			input_pos = strlen(input_line) - 1;
			input_line[input_pos] = 0;
			history_idx++;
		} break;

		// get ready.....

#define checkkey(k, c) case (k): { con_putinc(c, red); } break
#define checkkey2(k, c_norm, c_shift) case (k): { con_putinc((char)((keymod & KEYMOD_SHIFT) ? (c_shift) : (c_norm)), red); } break

		checkkey(KEY_SPACE, ' ');
		checkkey2(KEY_1, '1', '!');
		checkkey2(KEY_2, '2', '@');
		checkkey2(KEY_3, '3', '#');
		checkkey2(KEY_4, '4', '$');
		checkkey2(KEY_5, '5', '%');
		checkkey2(KEY_6, '6', '^');
		checkkey2(KEY_7, '7', '&');
		checkkey2(KEY_8, '8', '*');
		checkkey2(KEY_9, '9', '(');
		checkkey2(KEY_0, '0', ')');
		checkkey2(KEY_GRAVE, '`', '~');
		checkkey2(KEY_MINUS, '-', '_');
		checkkey2(KEY_EQUALS, '=', '+');
		checkkey2(KEY_LEFTBRACKET, '[', '{');
		checkkey2(KEY_RIGHTBRACKET, ']', '}');
		checkkey2(KEY_BACKSLASH, '\\', '|');
		checkkey2(KEY_SEMICOLON, ';', ':');
		checkkey2(KEY_APOSTROPHE, '\'', '"');
		checkkey2(KEY_COMMA, ',', '<');
		checkkey2(KEY_PERIOD, '.', '>');
		checkkey2(KEY_SLASH, '/', '?');

		default:
			return false;
	}
	return true;
}

void con_printf(char *fmt, ...)
{
	struct conline *l;
	char *text, *p;
	va_list va;
	size_t len = strlen(fmt) + 2048; // additional space for expansions

	text = B_malloc(len + 1);
	memset(text, 0, len + 1);
	p = text;

	va_start(va, fmt);
	vsnprintf(text, len, fmt, va);
	va_end(va);

	printf("%s", text);

	while(*p != 0) {
		if(conlines->len < 256) {
			if(*p == '\n') // the font has a symbol for \n, dont display it
				conlines->line[conlines->len++] = ' ';
			else
				conlines->line[conlines->len++] = *p;
		} else {
			// TODO: word wrapping
		}

		if(*p == '\n') {
			l = B_malloc(sizeof(*l));
			memset(l, 0, sizeof(*l));
			add_line(l);
			p++;
			continue;
		}
		p++;
	}

	B_free(text);
}

static void draw_input_line(int y)
{
	if(flash)
		ui_printf(0, y, false, "]%.*s%c%s", input_pos, input_line, flash_char, input_line + input_pos);
	else
		ui_printf(0, y, false, "]%s", input_line);

	if(SDL_GetTicks64() - flash_tick > 400) {
		flash = !flash;
		flash_tick = SDL_GetTicks64();
	}
}

void ui_draw_console(void)
{
	int y = (ui_h / 16) * 8 - 8;
	struct conline *l = conlines;
	int scroll = con_scroll;

	if(!con_opened)
		return;

	draw_input_line(y + 8);

	while(scroll-- >= 0 && l != NULL)
		l = l->next;

	if(con_scroll > 0) {
		ui_printf(0, y, false, "%s", "...");
		l = l->next;
		y -= 8;
	}

	for(; y >= 0; y -= 8) {
		if(l != NULL) {
			if(l->line[0] != '\0')
				ui_printf(0, y, false, "%s", l->line);
			else
				y += 8;
			l = l->next;
		} else {
			ui_printf(0, y, false, "%c", ' ');
		}
	}

}
