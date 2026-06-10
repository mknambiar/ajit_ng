
#ifndef AJIT_SHIM_H
#define AJIT_SHIM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
/* ---- math extras (freestanding) ---- */
float sinf(float x);
float cosf(float x);
float sqrtf(float x);

double tan(double x);
float tanf(float x);

double fabs(double x);
float fabsf(float x);

double trunc(double x);
float truncf(float x);

double ceil(double x);
float ceilf(float x);

double sinh(double x);
float sinhf(float x);
double cosh(double x);
float coshf(float x);

double asin(double x);
float asinf(float x);
double acos(double x);
float acosf(float x);

double asinh(double x);
float asinhf(float x);
double acosh(double x);
float acoshf(float x);

double log(double x);
float logf(float x);
double log10(double x);
float log10f(float x);

double pow(double x, double y);
float powf(float x, float y);

#endif
int ajit_shim_uart_putc(int ch);
long write(int fd, const void* buf, unsigned long count);
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
void* memset(void* dst, int c, size_t n);
int memcmp(const void* a, const void* b, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char* s);
void* malloc(size_t n);
void free(void* p);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* p, size_t n);
void _exit(int code) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
int putchar(int ch);
int puts(const char* s);
int printf(const char* fmt, ...);
double sin(double x);
double cos(double x);
double sqrt(double x);
#ifdef __cplusplus
}
/* ---- math extras (freestanding) ---- */
float sinf(float x);
float cosf(float x);
float sqrtf(float x);

double tan(double x);
float tanf(float x);

double fabs(double x);
float fabsf(float x);

double trunc(double x);
float truncf(float x);

double ceil(double x);
float ceilf(float x);

double sinh(double x);
float sinhf(float x);
double cosh(double x);
float coshf(float x);

double asin(double x);
float asinf(float x);
double acos(double x);
float acosf(float x);

double asinh(double x);
float asinhf(float x);
double acosh(double x);
float acoshf(float x);

double log(double x);
float logf(float x);
double log10(double x);
float log10f(float x);

double pow(double x, double y);
float powf(float x, float y);

#endif
/* ---- math extras (freestanding) ---- */
float sinf(float x);
float cosf(float x);
float sqrtf(float x);

double tan(double x);
float tanf(float x);

double fabs(double x);
float fabsf(float x);

double trunc(double x);
float truncf(float x);

double ceil(double x);
float ceilf(float x);

double sinh(double x);
float sinhf(float x);
double cosh(double x);
float coshf(float x);

double asin(double x);
float asinf(float x);
double acos(double x);
float acosf(float x);

double asinh(double x);
float asinhf(float x);
double acosh(double x);
float acoshf(float x);

double log(double x);
float logf(float x);
double log10(double x);
float log10f(float x);

double pow(double x, double y);
float powf(float x, float y);

#endif
