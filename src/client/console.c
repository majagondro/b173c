#include "vid/ui.h"
#include "console.h"
#include <SDL2/SDL.h>
#include "input.h"
#include "vid/vid.h"
#include "common.h"

#define CONSOLE_MAX_LINES 512
#define CONSOLE_MAX_LINE 256

static char conbuf[1024 * 1024] = {0};
static size_t conbuf_len = 0;
static size_t conbuf_lines = 0;
static char input_line[CONSOLE_MAX_LINE + 1] = {0};
static int input_pos = 0;
static int history_idx = 0;
static int line_count = 0;

bool con_opened = true;
int con_scroll = 0;

void toggleconsole_f(void)
{
	con_opened = !con_opened;
	vid_mouse_grab(!con_opened);
}

void con_show(void)
{
	if(!con_opened) {
		toggleconsole_f();
	}
}

void con_hide(void)
{
	if(con_opened) {
		toggleconsole_f();
	}
}

void con_init(void)
{
	cmd_init();

	cmd_register("toggleconsole", toggleconsole_f);
}

static void con_putinc(u_byte c)
{
	if(input_pos < CONSOLE_MAX_LINE) {
		if(strnlen(input_line, CONSOLE_MAX_LINE) == CONSOLE_MAX_LINE) {
			// dont
		} else {
			memmove(&input_line[input_pos + 1], &input_line[input_pos], CONSOLE_MAX_LINE - input_pos - 1);
			input_line[input_pos++] = c;
		}
	}
}

static void con_submit(void)
{
	con_printf("]%s\n", input_line);

	cmd_exec((char *) input_line, false);

	memset(input_line, 0, CONSOLE_MAX_LINE);
	input_pos = 0;

	// reset history index
	history_idx = 0;

	// scroll back to bottom
	con_scroll = 0;
}

static const char *get_nth_inputted_line(int n)
{
	return NULL;
}

static int calc_max_scroll(void)
{
	int visible_lines = (ui_h / 2 - LINE_HEIGHT_PX) / LINE_HEIGHT_PX;
	int max = line_count - visible_lines;
	if(max < 0)
		max = 0;
	return max;
}

bool con_handle_key(int key, int keymod)
{
	char c;
	int caps_strength;

	if(!con_opened)
		return false;

	caps_strength = ((keymod & KEYMOD_SHIFT) != 0) ^ ((keymod & KEYMOD_CAPSLOCK) != 0);

	if(key >= KEY_A && key <= KEY_Z) {
		if(key == KEY_V && keymod & KEYMOD_CTRL) {
			char *clipboard = SDL_GetClipboardText();
			int i;
			for(i = 0; i < (int) strlen(clipboard); i++)
				con_putinc(clipboard[i]);
			SDL_free(clipboard);
			return true;
		}

		c = (char) ((key - KEY_A) + 'a' - (caps_strength * ('a' - 'A')));
		con_putinc((u_byte) c);
		return true;
	}

	switch(key) {
		case KEY_ESCAPE: {
			toggleconsole_f();
		} break;

		case KEY_BACKSPACE: {
			if (input_pos > 0) {
				if(input_pos == CONSOLE_MAX_LINE)
					input_line[input_pos - 1] = 0;
				else
					memmove(&input_line[input_pos - 1], &input_line[input_pos], CONSOLE_MAX_LINE - input_pos);
				input_pos--;
			}
		} break;

		case KEY_DELETE: {
			if(input_pos < CONSOLE_MAX_LINE) {
				memmove(&input_line[input_pos], &input_line[input_pos + 1], CONSOLE_MAX_LINE - input_pos);
			}
		} break;

		case KEY_RETURN: {
			con_submit();
		} break;

		case KEY_HOME: {
			if(keymod & KEYMOD_CTRL) {
				con_scroll = 0;
			} else {
				input_pos = 0;
			}
		} break;

		case KEY_END: {
			if(keymod & KEYMOD_CTRL) {
				con_scroll = calc_max_scroll();
			} else {
				input_pos = CONSOLE_MAX_LINE;
			}
		} break;

		case KEY_LEFT: {
			if(input_pos > 0) {
				input_pos--;
			}
		} break;

		case KEY_RIGHT: {
			if(input_pos < (int) strnlen(input_line, CONSOLE_MAX_LINE)) {
				input_pos++;
			}
		} break;

		case KEY_DOWN:
			history_idx -= 2;
			if(history_idx < -1)
				history_idx = -1;
			/* fall through */
		case KEY_UP: {
			/*struct conline *l;
			l = get_nth_inputted_line(history_idx);
			if(l == NULL) {
				if(history_idx != 0) {
					history_idx = 0;
					memset(input_line, 0, sizeof(input_line));
					input_pos = 0;
				}
				return true;
			}
			strlcpy(input_line, l->line+1, CONSOLE_MAX_LINE - 1);
			input_pos = strlen(input_line) - 1;
			input_line[input_pos] = 0;
			history_idx++;*/
		} break;

#define checkkey(k, c) case (k): { con_putinc(c); } break
#define checkkey2(k, c_norm, c_shift) case (k): { con_putinc((char)((keymod & KEYMOD_SHIFT) ? (c_shift) : (c_norm))); } break

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

		default: {
			return false;
		}
	}
	return true;
}

static void recalc_num_lines(void)
{
	conbuf_lines = 0;
	for(size_t i = 0; i < sizeof(conbuf); i++) {
		if(conbuf[i] == '\n') {
			conbuf_lines++;
		}
	}
}

void con_printf(char *fmt, ...)
{
	va_list va;
	size_t n;

	if(!fmt)
		return;

	va_start(va, fmt);
	n = (size_t) vsnprintf(conbuf + conbuf_len, sizeof(conbuf), fmt, va);
	printf("%s", conbuf + conbuf_len);
	conbuf_len += n + 1;

	va_end(va);

	if(n >= sizeof(conbuf)) {
		// free old stuff
		// todo reprint again
		size_t nb = sizeof(conbuf) / 2;
		memmove(conbuf, conbuf + nb, nb);
		conbuf_len = nb;
		conbuf_len += snprintf(conbuf, sizeof(conbuf), "console buffer truncated\n");
	}

	recalc_num_lines();
}

static void draw_input_line(int y)
{
	// todo: always show cursor while the user is typing
	static ulong blink_tick = 0;
	static u_byte blink_char = '|';
	static bool blink = false;

	if(blink)
		ui_printf(1, y, "]%.*s%c%s", input_pos, input_line, blink_char, input_line + input_pos);
	else
		ui_printf(1, y, "]%s", input_line);

	if(SDL_GetTicks64() - blink_tick > 800) {
		blink_tick = SDL_GetTicks64();
		blink = !blink;
	}
}
static char *textwidthwordmax(char *text, int maxwidth)
{
	int w = 0;
	char *lastword = text;
	while(*text && w < maxwidth) {
		w += ui_charwidth(*text);
		if(isspace(*text)) {
			lastword = text;
		}
		text++;
	}
	return w > 0 ? lastword + 1 : NULL;
}

static void drawwrap(int x, int *y, char *p, int *scroll)
{
	char *end;
	char old;

	if(*y < -LINE_HEIGHT_PX || *y >= ui_h)
		return;

	// todo: calculate proper y and go down instead of up? thus removing recursion
	end = textwidthwordmax(p, ui_w);

	if(!end)
		// drawtext p?
		return;

	if(ui_strwidth(end) > ui_w) {
		drawwrap(x, y, end, scroll);
	} else {
		if(*scroll <= 0) {
			ui_drawtext(end, x, *y);
			*y -= LINE_HEIGHT_PX;
		}
		(*scroll)--;
	}

	if(*scroll <= 0) {
		old = *end;
		*end = 0;
		ui_drawtext(p, x, *y);
		*end = old;
		*y -= LINE_HEIGHT_PX;
	} else {
		(*scroll)--;
	}
}

void ui_draw_console(void)
{
	int y = ui_h / 2;
	int scroll = con_scroll;
	int num_lines_that_fit = y / LINE_HEIGHT_PX;
	char *p;

	if(!con_opened)
		return;
	if(conbuf_len <= 0)
		return;

	draw_input_line(y);
	y -= LINE_HEIGHT_PX;

	p = conbuf + conbuf_len - 2;
	for(int i = 0; i < num_lines_that_fit + con_scroll; i++) {
		while(*p)
			p--;
		p++;
		if(ui_strwidth(p) >= ui_w - 8) {
			drawwrap(1, &y, p, &scroll);
		} else if(scroll <= 0) {
			ui_printf(1, y, p);
			y -= LINE_HEIGHT_PX;
		}
		scroll--;
		if(p == conbuf) {
			break;
		}
		if(y <= -LINE_HEIGHT_PX) {
			break;
		}
		p -= 2;
	}
}
