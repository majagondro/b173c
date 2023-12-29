#include <ctype.h>
#include "vid/console.h"
#include "common.h"

cvar *cvarlist = NULL;
struct cmd *cmdlist = NULL;
struct alias *aliaslist = NULL;

static char *$argv[64];
static int $argc = 0;

static char *emptystr = "";

cvar developer = {"developer", "0"};

// cmds.c
void cmds_register(void);
void cmd_init(void)
{
	cvar_register(&developer);
	cmds_register();

	cmd_exec("exec config", false);
	cmd_exec("exec autoexec", false);
}

char *cmd_argv(int i)
{
	if(i < 0 || i >= $argc)
		return emptystr;
	return $argv[i];
}

int cmd_argc(void)
{
	return $argc;
}

char *cmd_args(int from, int to)
{
	static char buf[4096]; // :|
	int i;

	if(from < 0)
		from = 0;
	if(to > cmd_argc())
		to = cmd_argc();

	memset(buf, 0, sizeof(buf));
	for (i = from; i < to; i++) {
		strlcat(buf, cmd_argv(i), sizeof(buf));
		if(i != to - 1) {
			strlcat(buf, " ", sizeof(buf));
		}
	}

	return buf;
}

void cvar_register(cvar *c)
{
	if(c->name == NULL)
		return;
	if(c->default_value == NULL)
		return;

	c->next = cvarlist;
	cvarlist = c;
	c->string = NULL;

	cvar_set(c->name, c->default_value);
}

void cmd_register(const char *name, void (*func)(void))
{
	struct cmd *c;

	if(!name)
		return;
	if(!func)
		return;

	c = malloc(sizeof(*c));

	c->name = malloc(strlen(name) + 1);
	strcpy(c->name, name);
	c->func = func;

	c->next = cmdlist;
	cmdlist = c;
}

void alias_register(const char *name, const char *command)
{
	struct alias *a;

	if(!name)
		return;
	if(!command)
		return;

	if(alias_find(name))
		alias_remove(name);

	a = malloc(sizeof(*a));

	a->name = malloc(strlen(name) + 1);
	strcpy(a->name, name);

	a->cmd = malloc(strlen(command) + 1);
	strcpy(a->cmd, command);

	a->server = cmd_is_stuffed();

	a->next = aliaslist;
	aliaslist = a;
}

void alias_remove(const char *name)
{
	struct alias *a = NULL, *prev = NULL;

	if(name == NULL)
		return;

	for(a = aliaslist; a != NULL; prev = a, a = a->next)
		if(strcmp(a->name, name) == 0)
			break;

	if(a == NULL)
		return;

	if(prev != NULL)
		prev->next = a->next;
	else
		aliaslist = a->next;

	free(a->name);
	free(a->cmd);
	free(a);
}

cvar *cvar_find(const char *name)
{
	cvar *cvar;

	for(cvar = cvarlist; cvar != NULL; cvar = cvar->next)
		if(strcmp(name, cvar->name) == 0)
			return cvar;

	return NULL;
}

struct cmd *cmd_find(const char *name)
{
	struct cmd *cmd = NULL;

	for(cmd = cmdlist; cmd != NULL; cmd = cmd->next)
		if(strcmp(name, cmd->name) == 0)
			return cmd;

	return NULL;
}

struct alias *alias_find(const char *name)
{
	struct alias *a = NULL;

	for(a = aliaslist; a != NULL; a = a->next)
		if(strcmp(name, a->name) == 0)
			return a;

	return NULL;
}

bool cvar_set_silent(const char *name, const char *value)
{
	cvar *cvar;
	size_t len;

	if (name == NULL)
		return false;
	if (value == NULL)
		return false;
	if (!(cvar = cvar_find(name)))
		return false;

	len = strlen(value);

	free(cvar->string);
	cvar->string = malloc(len + 1);
	memcpy(cvar->string, value, len);
	cvar->string[len] = 0;

	cvar->value = strtof(value, NULL);
	cvar->integer = (int) strtol(value, NULL, 10);

	return true;
}

bool cvar_set(const char *name, const char *value)
{
	cvar *cvar;

	if(!cvar_set_silent(name, value))
		return false;

	cvar = cvar_find(name);
	if(cvar->onchange)
		cvar->onchange();

	return true;
}

static char *copystr(char *s)
{
	char *s2;
	size_t len = strlen(s);
	s2 = malloc(len + 1);
	s2[len] = 0;
	memcpy(s2, s, len);
	return s2;
}

static void cmd_cvar(void)
{
	char *name = cmd_argv(0);
	cvar *c = cvar_find(name);
	size_t len;

	if(c == NULL) {
		con_printf("cmd_cvar: WTF?!\n");
		exit(1);
	}

	if(cmd_argc() == 1) {
		// display info
		len = strlen(name);
		con_printf("%*s : default value is \"%s\"\n", len, name, c->string);
		con_printf("%*s   current value is \"%s\"\n", len, "", c->string);
	} else {
		// set
		cvar_set(name, cmd_argv(1));
	}

}

char *tokenize(char *text)
{
	static char *totok;
	static char *p, *start;
	static char oldchar;
	static bool done;

	bool quoted = false;
	bool scanning = false;

	if(text) {
		totok = text;
		p = totok;
		start = p;
		oldchar = 0;
		done = false;
	}

	if(oldchar != 0) {
		*p = oldchar;
		oldchar = 0;
		start = ++p;
		scanning = false;
	}

	while(*p) {
		if(isspace(*p)) {
			if(scanning && !quoted) {
				oldchar = *p;
				*p = 0;
				return start;
			} else {
				p++;
				if(!quoted)
					start = p;
				continue;
			}
		} else if(*p == '"') {
			if(quoted) {
				oldchar = *p;
				*p = 0;
				return start;
			} else {
				if(scanning) {
					oldchar = *p;
					*p = 0;
					return start;
				} else {
					quoted = true;
					start = ++p;
					continue;
				}
			}
		} else {
			scanning = true;
			p++;
		}
	}

	if(done) {
		return NULL;
	} else {
		done = true;
		if(*start == 0)
			return NULL;
		return start;
	}
}

void cmd_exec_impl(char *text, bool from_server);
void cmd_exec(char *text, bool from_server)
{
	bool quoted;
	char buf[2048] = {0};
	int len = 0;

	while(*text) {
		if(*text == '"') {
			quoted = !quoted;
		} else if(*text == ';') {
			if(!quoted) {
				// dont bother executing if its empty
				if(buf[0] != 0)
					cmd_exec_impl(buf, from_server);
				memset(buf, 0, sizeof(buf));
				len = 0;
				text++;
				continue;
			}
		}
		buf[len++] = *text++;
	}

	if(buf[0] != 0) {
		cmd_exec_impl(buf, from_server);
	}
}

static bool stuffed = false;
bool cmd_is_stuffed(void) { return stuffed; }

static int recurse_protection = 0;

void cmd_exec_impl(char *text, bool from_server)
{
	char *tok;
	char *totok;
	struct cmd *cmd;
	cvar *cvar;
	struct alias *alias;
	int i;
	char buf[4096] = {0};

	stuffed = from_server;

// reset old args
	for(i = 0; i < $argc; i++)
		free($argv[i]);
	$argc = 0;

// make new args
	// new var because strtok modifies the string
	totok = malloc(4096);
	memset(totok, 0, 4096);
	strcpy(totok, text);
	for(tok = tokenize(totok); tok != NULL; tok = tokenize(NULL))
		$argv[$argc++] = copystr(tok);
	free(totok);

	if($argc == 0)
		return;

// try to find cmd
	cmd = cmd_find($argv[0]);
	if(cmd != NULL) {
		cmd->func();
		return;
	}

// not found, so alias
	alias = alias_find($argv[0]);
	if(alias != NULL) {
		if(recurse_protection < 1000) {
			if($argc > 1) {
				snprintf(buf, sizeof(buf), "%s %s", alias->cmd, cmd_args(1, cmd_argc()));
				cmd_exec(buf, from_server);
			} else {
				cmd_exec(alias->cmd, from_server);
			}
			recurse_protection++;
		} else {
			con_printf("recursing alias detected - stopping execution\n");
			recurse_protection = 0;
		}
		return;
	}

// not found, so cvar
	cvar = cvar_find($argv[0]);
	if(cvar != NULL) {
		cmd_cvar();
		return;
	}

// nothing was found
	con_printf("unknown command \"%s\"\n", $argv[0]);
}
