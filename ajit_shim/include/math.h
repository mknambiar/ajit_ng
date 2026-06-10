#ifndef AJIT_SHIM_MATH_H
#define AJIT_SHIM_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

double sin(double x);
double cos(double x);
double tan(double x);
double sqrt(double x);
double fabs(double x);
double trunc(double x);
double ceil(double x);
double sinh(double x);
double cosh(double x);
double atan(double x);
double exp(double x);
double asin(double x);
double acos(double x);
double asinh(double x);
double acosh(double x);
long double asinl(long double x);
long double acosl(long double x);
double log(double x);
double log10(double x);
double pow(double x, double y);

float sinf(float x);
float cosf(float x);
float tanf(float x);
float sqrtf(float x);
float fabsf(float x);
float truncf(float x);
float ceilf(float x);
float sinhf(float x);
float coshf(float x);
float atanf(float x);
float expf(float x);
float asinf(float x);
float acosf(float x);
float asinhf(float x);
float acoshf(float x);
float logf(float x);
float log10f(float x);
float powf(float x, float y);

#ifdef __cplusplus
}
#endif

#endif
