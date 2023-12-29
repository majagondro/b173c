#ifndef B173C_COMMON_H
#define B173C_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdio.h>
#include "char16.h"

typedef char byte;
typedef unsigned char u_byte;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef char *string8;
typedef char16 *string16;

void *_B_malloc(size_t sz, const char *file, int line);
#define B_malloc(size) _B_malloc(size, __FILE__, __LINE__)
#define B_free(p) free(p), p = NULL

#endif
