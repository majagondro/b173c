#include "char16.h"
#include "common.h"

char16 *c16(const char *s)
{
	static char16 buf[4096]; // .....
	int i;
	for(i = 0; s && i < (int) strlen(s) && i < 4096; i++)
		buf[i] = s[i];
	buf[i] = 0;
	return buf;
}

char *c8(const char16 *s)
{
	static char buf[4096];
	int i;
	for(i = 0; s && i < (int) c16strlen(s) && i < 4096; i++) {
		buf[i] = s[i] >> 8;
	}
	buf[i] = 0;
	return buf;
}

char16 *c16h(const char *s)
{
	char16 *buf;
	int i;
	buf = B_malloc(strlen(s) * sizeof(char16));
	for(i = 0; s && i < (int) strlen(s); i++)
		buf[i] = s[i];
	buf[i] = 0;
	return buf;
}

size_t c16strlen(const char16 *s)
{
	size_t len = 0;
	if(!s)
		return 0;
	while(*s) {
		len++;
		s++;
	}
	return len;
}

void c16puts(const char16 *s)
{
	if(!s)
		return;
	while(*s) {
		putchar(*s++ >> 8);
	}
}

void c16toc8(char16 *s, char *dest)
{
	int i;
	for(i = 0; i < (int) c16strlen(s); i++) {
		dest[i] = s[i];
	}
}
