#ifndef B173C_COMMON_H
#define B173C_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdio.h>

_Static_assert(__STDC_UTF_16__ == 1, "WHAT THE FUCK");

#define attr(a) __attribute__((__##a##__))

typedef char byte;
typedef unsigned char ubyte;

void *_mem_alloc_impl(size_t sz, const char *file, int line);
#define mem_alloc(size) _mem_alloc_impl(size, __FILE__, __LINE__)
#define mem_free(p) do { if(p) free(p); p = NULL; } while(0)

void con_printf(char *text, ...);

// does a sprintf into a temp static buffer
const char *va(const char *fmt, ...);

#endif
