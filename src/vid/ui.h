#ifndef B173C_UI_H
#define B173C_UI_H

#include "common.h"

extern int ui_w, ui_h;

void ui_init(void);
void ui_shutdown(void);

void ui_draw(void);   // called before r_display_frame
void ui_commit(void); // called in r_display_frame

bool ui_drawchar(u_byte c, int x, int y, bool red, int color);
void ui_drawtext(const char *text, int x, int y, bool red);
void ui_printf(int x, int y, bool red, const char *fmt, ...);
int ui_charwidth(u_byte c);
int ui_strwidth(const char *text);

// console.c
void ui_draw_console(void);
// hud.c
void ui_draw_hud(void);
// ui_draw_hud
// ui_draw_scoreboard
// etc...

#endif
