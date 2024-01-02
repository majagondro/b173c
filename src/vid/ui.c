#include <glad/glad.h>
#include <stdarg.h>
#include <ctype.h>
#include "ui.h"
#include "common.h"
#include "client/console.h"
#include "vid.h"
#include "client/client.h"


#define MAX_CON_CHARS 8192
#define CON_CHAR_SIZE 8

int ui_w, ui_h;

extern cvar vid_width;
extern cvar vid_height;

void onchange_ui_scale(void);
cvar ui_scale = {"ui_scale", "2", onchange_ui_scale};

vec2 glyph_vertices_original[] = {
	// this is 2 because opengl coords is -1 to 1 but i want the origin to be the top left corner
	{0.0f,  0.0f}, {0.0f, 0.0f},
	{0.0f, -2.0f}, {0.0f, 1.0f},
	{2.0f,  0.0f}, {1.0f, 0.0f},
	{2.0f, -2.0f}, {1.0f, 1.0f},
};

vec2 glyph_vertices[] = {
	// this is 2 because opengl coords is -1 to 1 but i want the origin to be the top left corner
	{0.0f,  0.0f}, {0.0f, 0.0f},
	{0.0f, -2.0f}, {0.0f, 1.0f},
	{2.0f,  0.0f}, {1.0f, 0.0f},
	{2.0f, -2.0f}, {1.0f, 1.0f},
};

#include "ext/default.c"
int charwidths[256] = {0};
vec4 fontdata[MAX_CON_CHARS] = {0};
int fontcharcount = 0;
uint fonttex;
uint fontvao, fontvbo = 0, fontdatavbo;

extern struct gl_state gl;

void ui_update_size(int x, int y)
{
	int i;

	ui_w = (int)((float)(x) / ui_scale.value);
	ui_h = (int)((float)(y) / ui_scale.value);

	for(i = 0; i < 8; i += 2) {
		glyph_vertices[i][0] = glyph_vertices_original[i][0] * CON_CHAR_SIZE / (float)(ui_w);
		glyph_vertices[i][1] = glyph_vertices_original[i][1] * CON_CHAR_SIZE / (float)(ui_h);
	}

	if(fontvbo) {
		glBindBuffer(GL_ARRAY_BUFFER, fontvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glyph_vertices), glyph_vertices, GL_STATIC_DRAW);
	}
}

void onchange_ui_scale(void)
{
	ui_update_size(vid_width.integer, vid_height.integer);
}

static void calculate_char_widths(void)
{
	int i;
	for(i = 0; i < 256; i++) {
		u_byte c = i & 255;
		int x, x2, y, y2, alpha_idx, xs, xe;
		bool start_set = false;

		if(c == ' ') {
			charwidths[c] = CON_CHAR_SIZE;
			continue;
		}

		xe = 7;
		xs = 0;

		x = (c % 16) * CON_CHAR_SIZE;
		y = (c / 16) * CON_CHAR_SIZE;
		for(x2 = x; x2 < x + CON_CHAR_SIZE; x2++) {
			bool empty = true;
			for(y2 = y; y2 < y + CON_CHAR_SIZE; y2++) {
				alpha_idx = (y2 * q_conchars.width + x2) * q_conchars.bytes_per_pixel + 3;
				if(q_conchars.pixel_data[alpha_idx] > 0) {
					empty = false;
					break;
				}
			}
			if(!start_set) {
				xs = x2 - x;
				if(!empty) {
					start_set = true;
				}
			} else {
				xe = x2 - x;
				if(empty) {
					break;
				}
			}
		}
		charwidths[c] = xe - xs + 1;
	}
}

void ui_init(void)
{
	cvar_register(&ui_scale);

	// setup font image
	glGenTextures(1, &fonttex);
	glBindTexture(GL_TEXTURE_2D, fonttex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, q_conchars.pixel_data);

	// setup font buffers
	glGenVertexArrays(1, &fontvao);
	glGenBuffers(1, &fontvbo);
	glGenBuffers(1, &fontdatavbo);
	glBindVertexArray(fontvao);
	glBindBuffer(GL_ARRAY_BUFFER, fontvbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glyph_vertices), glyph_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), (void *) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(vec2), (void *) sizeof(vec2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	// instance data
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, fontdatavbo);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void *) 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(2, 1);

	ui_update_size(vid_width.integer, vid_height.integer);

	calculate_char_widths();
}

void ui_shutdown(void)
{

}

void ui_draw(void)
{
	ui_draw_console();
}

static char *statestr(void)
{
	switch(cl.state) {
		case cl_connecting:
			return "connecting";
		case cl_disconnected:
			return "disconnected";
		case cl_connected:
			return "connected";
	}
	return "?";
}

void ui_commit(void)
{
	if(developer.integer) {
		extern int wr_total_faces;
		extern int wr_draw_calls;

		ui_printf(8, ui_h - 48, "3d: %d draw calls / %d faces (avg %d)", wr_draw_calls, wr_total_faces, wr_total_faces / (wr_draw_calls+1));
		ui_printf(8, ui_h - 40, "net: %s", statestr());

		ui_printf(8, ui_h - 32, "%f %f %f / %f %f %f",
				  cl.game.pos[0], cl.game.pos[1], cl.game.pos[2],
				  cl.game.rot[0], cl.game.rot[1], cl.game.rot[2]);

		ui_printf(8, ui_h - 16, "%lu/%.0f fps", cl.fps, 1 / cl.frametime);

		ui_printf(40, ui_h - 24, "/%d chars used", MAX_CON_CHARS);
		ui_printf(8, ui_h - 24, "%4d", fontcharcount + 4);
	}

	glBindBuffer(GL_ARRAY_BUFFER, fontdatavbo);
	// todo: update only parts of the buffer that were changed
	glBufferData(GL_ARRAY_BUFFER, fontcharcount * sizeof(vec4), fontdata, GL_DYNAMIC_DRAW);

	glBindVertexArray(fontvao);
	glBindTexture(GL_TEXTURE_2D, fonttex);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, fontcharcount);

	fontcharcount = 0;
}

int ui_charwidth(u_byte c)
{
	return charwidths[c];
}

int ui_strwidth(const char *text)
{
	int w = 0;
	if(!text)
		return 0;
	while(*text)
		w += ui_charwidth(*text++);
	return w;
}

bool ui_drawchar(u_byte c, int x, int y, int color)
{
	int b = c;

	if(color >= 0) // draw shadow
		ui_drawchar(c, x+1, y+1, -color);

	if(fontcharcount > MAX_CON_CHARS - 10) {
		con_printf("font count is @ %d!!!! not drawing any text anymore\n", fontcharcount);
		return false;
	}

	if(x < -CON_CHAR_SIZE || x > ui_w)
		return false;
	if(y < -CON_CHAR_SIZE || y > ui_h)
		return false;

	fontdata[fontcharcount][0] = ((float)x) / ((float)ui_w);
	fontdata[fontcharcount][1] = ((float)y) / ((float)ui_h);
	fontdata[fontcharcount][2] = (float)b;
	fontdata[fontcharcount][3] = (float)color;

	fontcharcount++;

	return true;
}

void ui_drawtext(const char *text, int x, int y)
{
	int color = 0xf;
	bool read_color_code = false;
	bool invisible = false;

	// skip characters outside the screen
	while(x < -CON_CHAR_SIZE && *text) {
		x += ui_charwidth(*text);
		text++;
	}

	if(y == -CON_CHAR_SIZE || y > ui_h)
		return; // cant be seen anyway

	while(*text) {
		if(read_color_code) {
			switch(*text) {
				case 'i': {
					invisible = true;
					read_color_code = false;
					text++;
					continue;
				} break;
				case 'p': {
					read_color_code = false;
					text++;
					x += strtol(text, &text, 10);
					continue;
				} break;
			}

			if(isdigit(*text))
				color = *text - '0';
			else
				color = *text - 'a' + 0xa;

			if(color < 0x0 || color > 0xf)
				color = 0xf;

			invisible = false;
			read_color_code = false;
			text++;
			continue;
		} else if(*text == '\xa7') {
			read_color_code = true;
			text++;
			continue;
		}

		if(!invisible) {
			if(!ui_drawchar(*text, x, y, color)) {
				break;
			}
		}

		x += ui_charwidth(*text++);
	}
}

void ui_printf(int x, int y, const char *fmt, ...)
{
	va_list va;
	char buf[4096];

	va_start(va, fmt);
	vsnprintf(buf, 4096, fmt, va);
	va_end(va);

	ui_drawtext(buf, x, y);
}
