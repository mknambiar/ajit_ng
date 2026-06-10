#ifndef AJIT_SHIM_STDLIB_H
#define AJIT_SHIM_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t n);
void free(void* p);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* p, size_t n);

void abort(void) __attribute__((noreturn));
void _exit(int code) __attribute__((noreturn));
void exit(int code) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
