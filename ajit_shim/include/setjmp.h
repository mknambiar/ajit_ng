#ifndef AJIT_SHIM_SETJMP_H
#define AJIT_SHIM_SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int jmp_buf[1];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#ifdef __cplusplus
}
#endif

#endif
