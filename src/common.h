#ifndef B173C_COMMON_H
#define B173C_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdio.h>
#include <uchar.h>
#include <sys/types.h>

#define attr(a) __attribute__((__##a##__))

#define BIN_FMT "%c%c%c%c%c%c%c%c"
#define BIN_BYTE(b)         \
  ((b) & 0x80 ? '1' : '0'), \
  ((b) & 0x40 ? '1' : '0'), \
  ((b) & 0x20 ? '1' : '0'), \
  ((b) & 0x10 ? '1' : '0'), \
  ((b) & 0x08 ? '1' : '0'), \
  ((b) & 0x04 ? '1' : '0'), \
  ((b) & 0x02 ? '1' : '0'), \
  ((b) & 0x01 ? '1' : '0')


#ifdef _WIN32
#define byte char
#else
typedef char byte;
#endif
typedef unsigned char ubyte;

void *_mem_alloc_impl(size_t sz, const char *file, int line);
#define mem_alloc(size) _mem_alloc_impl(size, __FILE__, __LINE__)
#define mem_free(p)  \
    do {             \
        if(p)        \
            free(p); \
        (p) = NULL;  \
    } while(0)

void con_printf(char *text, ...);

// does a sprintf into a temp static buffer
const char *va(const char *fmt, ...);

// USES A STATIC BUFFER!!!!
const char *utf16toutf8(const char16_t *str, size_t len);

typedef enum {
    ERR_OK,
    ERR_FATAL,
    ERR_NETWORK
} errcode;

#define swap(a, b) do {             \
    __typeof__(a) __swap_tmp = (a); \
    (a) = (b);                      \
    (b) = __swap_tmp;               \
} while(0)

size_t strlcpy(char *dst, const char *src, size_t dsize);
size_t strlcat(char *dst, const char *src, size_t dsize);

#endif
