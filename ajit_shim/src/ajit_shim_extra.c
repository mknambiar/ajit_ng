#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>

/* Minimal extras to avoid pulling in uclibc. */

static char* ajit_shim_out_uint(char* out, unsigned long long v, unsigned base, int upper) {
  const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  char tmp[32];
  int i = 0;
  if (base < 2) base = 10;
  do {
    tmp[i++] = digits[v % base];
    v /= base;
  } while (v && i < (int)sizeof(tmp));
  while (i--) {
    *out++ = tmp[i];
  }
  return out;
}

int vsprintf(char* s, const char* fmt, va_list ap) {
  char* out = s;
  const char* p = fmt;
  while (*p) {
    if (*p != '%') {
      *out++ = *p++;
      continue;
    }
    p++; /* skip '%' */
    if (*p == '%') {
      *out++ = '%';
      p++;
      continue;
    }

    int long_flag = 0;
    if (*p == 'l') { long_flag = 1; p++; }
    if (*p == 'l') { long_flag = 2; p++; }

    switch (*p) {
      case 'c': {
        int c = va_arg(ap, int);
        *out++ = (char)c;
        break;
      }
      case 's': {
        const char* str = va_arg(ap, const char*);
        if (!str) str = "(null)";
        while (*str) *out++ = *str++;
        break;
      }
      case 'd':
      case 'i': {
        long long v = long_flag ? va_arg(ap, long) : va_arg(ap, int);
        if (v < 0) { *out++ = '-'; v = -v; }
        out = ajit_shim_out_uint(out, (unsigned long long)v, 10, 0);
        break;
      }
      case 'u': {
        unsigned long long v = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        out = ajit_shim_out_uint(out, v, 10, 0);
        break;
      }
      case 'x':
      case 'X': {
        unsigned long long v = long_flag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        out = ajit_shim_out_uint(out, v, 16, (*p == 'X'));
        break;
      }
      case 'p': {
        unsigned long long v = (unsigned long long)(uintptr_t)va_arg(ap, void*);
        *out++ = '0';
        *out++ = 'x';
        out = ajit_shim_out_uint(out, v, 16, 0);
        break;
      }
      default:
        *out++ = '%';
        if (*p) *out++ = *p;
        break;
    }
    if (*p) p++;
  }
  *out = '\0';
  return (int)(out - s);
}

int sprintf(char* s, const char* fmt, ...) {
  va_list ap;
  int n;
  va_start(ap, fmt);
  n = vsprintf(s, fmt, ap);
  va_end(ap);
  return n;
}

/* File I/O stubs: keep linkable, not functional. */
FILE* fopen(const char* path, const char* mode) { (void)path; (void)mode; return (FILE*)0; }
int fclose(FILE* f) { (void)f; return EOF; }
int fscanf(FILE* f, const char* fmt, ...) { (void)f; (void)fmt; return EOF; }

/* setjmp/longjmp stubs: keep linkable, not functional. */
int setjmp(jmp_buf env) { (void)env; return 0; }
void longjmp(jmp_buf env, int val) { (void)env; (void)val; for(;;){} }

/* exit should be provided by the shim runtime. */
void _exit(int code);
void exit(int code) { _exit(code); }
