#ifndef B173C_CHAR16_H
#define B173C_CHAR16_H

#include <stddef.h>

typedef short char16;

// prints the string8 into a temporary string16 buffer
// dont use too often
char16 *c16(const char *s);

// reverse of c16
char *c8(const char16 *s);

// prints the string8 into a heap-allocated buffer
char16 *c16h(const char *s);

size_t c16strlen(const char16 *s);

// print the string16 to stdout
void c16puts(const char16 *s);

#endif
