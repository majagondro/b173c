#ifndef B173C_CONSOLE_H
#define B173C_CONSOLE_H

#include "common.h"

#define COLOR_BLACK "\xa7" "0"
#define COLOR_DBLUE "\xa7" "1"
#define COLOR_DGREEN "\xa7" "2"
#define COLOR_DCYAN "\xa7" "3"
#define COLOR_DRED "\xa7" "4"
#define COLOR_DPINK "\xa7" "6"
#define COLOR_DYELLOW "\xa7" "7"
#define COLOR_GRAY "\xa7" "7"
#define COLOR_DGRAY "\xa7" "8"
#define COLOR_BLUE "\xa7" "9"
#define COLOR_GREEN "\xa7" "a"
#define COLOR_CYAN "\xa7" "b"
#define COLOR_RED "\xa7" "c"
#define COLOR_PINK "\xa7" "d"
#define COLOR_YELLOW "\xa7" "e"
#define COLOR_WHITE "\xa7" "f"
#define STYLE_PADDED "\xa7" "p"

typedef struct cvar {
	char *name;
	char *default_value;

	void (*onchange)(void);

	char *string;
	float value;
	int integer;

	struct cvar *next;
} cvar;

struct cmd {
	char *name;
	void (*func)(void);

	struct cmd *next;
};

struct alias {
	char *name;
	char *cmd;
	bool server;

	struct alias *next;
};

extern cvar developer;

extern bool con_opened;
extern int con_scroll;

void con_init(void);
bool con_handle_key(int key, int keymod);

void con_show(void);
void con_hide(void);

void con_printf(char *text, ...);

// cvar/cmd related

void cmd_init(void);
void cmd_exec(char *text, bool from_server);

void cvar_register(cvar *c);
void cmd_register(const char *name, void (*func)(void));
void alias_register(const char *name, const char *command);

void alias_remove(const char *name);

cvar *cvar_find(const char *name);
struct cmd *cmd_find(const char *name);
struct alias *alias_find(const char *name);

bool cvar_set(const char *name, const char *value);
// doesnt call onchange functions
bool cvar_set_silent(const char *name, const char *value);

char *cmd_argv(int i);
int cmd_argc(void);
bool cmd_is_stuffed(void);
char *cmd_args(int from, int to);

#endif
