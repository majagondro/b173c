#ifndef B173C_CONSOLE_H
#define B173C_CONSOLE_H

#include "common.h"

#define REDCHAR(c) (c + 127)

#define CVAR_NONE     (0)
#define CVAR_USERINFO (1 << 0)

typedef struct cvar {
	char *name;
	char *default_value;
	int flags;
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
