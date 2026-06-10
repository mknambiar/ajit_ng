#ifndef AJIT_SHIM_STDIO_H
#define AJIT_SHIM_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AjitShimFile FILE;

#ifndef EOF
#define EOF (-1)
#endif

int printf(const char* fmt, ...);
int sprintf(char* s, const char* fmt, ...);
int vsprintf(char* s, const char* fmt, va_list ap);
int puts(const char* s);
int putchar(int ch);

FILE* fopen(const char* path, const char* mode);
int fclose(FILE* f);
int fscanf(FILE* f, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
