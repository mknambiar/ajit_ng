
#include "ajit_shim.h"
int ajit_shim_uart_putc(int ch) { return ch; }
long write(int fd, const void* buf, unsigned long count) {
  const unsigned char* p = (const unsigned char*)buf;
  for (unsigned long i=0;i<count;i++) ajit_shim_uart_putc(p[i]);
  return (long)count;
}
void* memcpy(void* d,const void*s,size_t n){unsigned char*D=d;const unsigned char*S=s;for(size_t i=0;i<n;i++)D[i]=S[i];return d;}
void* memmove(void* d,const void*s,size_t n){unsigned char*D=d;const unsigned char*S=s;if(D<S){for(size_t i=0;i<n;i++)D[i]=S[i];}else{for(size_t i=n;i;i--)D[i-1]=S[i-1];}return d;}
void* memset(void* d,int c,size_t n){unsigned char*D=d;for(size_t i=0;i<n;i++)D[i]=(unsigned char)c;return d;}
int memcmp(const void*a,const void*b,size_t n){const unsigned char*A=a,*B=b;for(size_t i=0;i<n;i++)if(A[i]!=B[i])return A[i]<B[i]?-1:1;return 0;}
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}
char* strcpy(char* d,const char*s){char*D=d;while((*D++=*s++)!='\0'){}return d;}
size_t strlen(const char*s){size_t n=0;while(s&&s[n])n++;return n;}
void* malloc(size_t n){static unsigned char h[65536];static size_t o; if(!n||o+n>sizeof(h))return 0;void*p=&h[o];o+=(n+7)&~7;return p;}
void free(void*p){(void)p;}
void* calloc(size_t m,size_t s){size_t n=m*s;void*p=malloc(n);if(p)memset(p,0,n);return p;}
void* realloc(void*p,size_t n){(void)p;return malloc(n);}
void _exit(int c){(void)c;for(;;){}}
void abort(void){for(;;){}}
int putchar(int ch){return ajit_shim_uart_putc(ch);}
int puts(const char*s){long n=strlen(s);write(1,s,n);write(1,"\n",1);return (int)(n+1);}
int printf(const char*fmt,...){const char*p=fmt;while(*p){if(*p=='%'&&p[1]=='%'){ajit_shim_uart_putc('%');p+=2;}else{ajit_shim_uart_putc(*p++);} }return 0;}
double sin(double x){double y=x;double x2=x*x;y-=x*x2/6;y+=x*x2*x2/120;return y;}
double cos(double x){double y=1;double x2=x*x;y-=x2/2;y+=x2*x2/24;return y;}
double sqrt(double x){if(x<=0)return 0;double g=x;for(int i=0;i<10;i++)g=0.5*(g+x/g);return g;}


/* -------------------- math extras (freestanding) --------------------
   Goal: linkability + reasonable approximations, no libc/libm dependency.
   Accuracy is modest; suitable for simulator bring-up and non-critical tests.
*/

static double ajit_shim_absd(double x) { return x < 0.0 ? -x : x; }

double fabs(double x) { return ajit_shim_absd(x); }
float fabsf(float x) { return (float)fabs((double)x); }

double trunc(double x) {
  /* trunc toward zero */
  long i = (long)x;
  return (double)i;
}
float truncf(float x) { return (float)trunc((double)x); }

double ceil(double x) {
  long i = (long)x;
  double di = (double)i;
  if (di == x) return x;
  return (x > 0.0) ? (double)(i + 1) : di;
}
float ceilf(float x) { return (float)ceil((double)x); }

double tan(double x) {
  double c = cos(x);
  if (c == 0.0) return (x >= 0.0) ? 1.0e308 : -1.0e308;
  return sin(x) / c;
}
float tanf(float x) { return (float)tan((double)x); }

float sinf(float x) { return (float)sin((double)x); }
float cosf(float x) { return (float)cos((double)x); }
float sqrtf(float x) { return (float)sqrt((double)x); }

static double ajit_shim_exp(double x) {
  /* simple exp via range reduction + Taylor; good enough for small |x| */
  if (x == 0.0) return 1.0;
  if (x > 40.0) x = 40.0;
  if (x < -40.0) x = -40.0;

  const double ln2 = 0.6931471805599453;
  int k = (int)(x / ln2);
  double r = x - (double)k * ln2;

  double term = 1.0;
  double sum = 1.0;
  for (int n = 1; n <= 18; n++) {
    term *= r / (double)n;
    sum += term;
  }

  if (k > 0) {
    for (int i = 0; i < k; i++) sum *= 2.0;
  } else if (k < 0) {
    for (int i = 0; i < -k; i++) sum *= 0.5;
  }
  return sum;
}

double exp(double x) { return ajit_shim_exp(x); }
float expf(float x) { return (float)exp((double)x); }

double sinh(double x) {
  double ex = ajit_shim_exp(x);
  double emx = ajit_shim_exp(-x);
  return 0.5 * (ex - emx);
}
float sinhf(float x) { return (float)sinh((double)x); }

double cosh(double x) {
  double ex = ajit_shim_exp(x);
  double emx = ajit_shim_exp(-x);
  return 0.5 * (ex + emx);
}
float coshf(float x) { return (float)cosh((double)x); }

static double ajit_shim_ln(double x) {
  if (x <= 0.0) return 0.0/0.0; /* NaN */
  int k = 0;
  double m = x;
  while (m >= 1.0) { m *= 0.5; k++; if (k > 1024) break; }
  while (m < 0.5) { m *= 2.0; k--; if (k < -1024) break; }

  double y = (m - 1.0) / (m + 1.0);
  double y2 = y*y;
  double term = y;
  double sum = 0.0;
  for (int n = 1; n <= 19; n += 2) {
    sum += term / (double)n;
    term *= y2;
  }
  const double ln2 = 0.6931471805599453;
  return 2.0*sum + (double)k * ln2;
}

double log(double x) { return ajit_shim_ln(x); }
float logf(float x) { return (float)log((double)x); }

double log10(double x) {
  const double inv_ln10 = 0.4342944819032518;
  return log(x) * inv_ln10;
}
float log10f(float x) { return (float)log10((double)x); }

static double ajit_shim_atan(double x) {
  const double pi_over_2 = 1.5707963267948966;
  double ax = ajit_shim_absd(x);
  if (ax > 1.0) {
    double r = pi_over_2 - ajit_shim_atan(1.0/ax);
    return (x < 0.0) ? -r : r;
  }
  double x2 = x*x;
  double p = x * (1.0 - x2*(0.3333333333333333 - x2*(0.2 - x2*(0.14285714285714285))));
  return p;
}

double atan(double x) { return ajit_shim_atan(x); }
float atanf(float x) { return (float)atan((double)x); }

double asinh(double x) {
  double ax = ajit_shim_absd(x);
  double t = ax + sqrt(ax*ax + 1.0);
  double l = log(t);
  return (x < 0.0) ? -l : l;
}
float asinhf(float x) { return (float)asinh((double)x); }

double acosh(double x) {
  if (x < 1.0) return 0.0/0.0;
  if (x > 1.0e308) {
    const double ln2 = 0.6931471805599453;
    return log(x) + ln2;
  }
  return log(x + sqrt(x - 1.0) * sqrt(x + 1.0));
}
float acoshf(float x) { return (float)acosh((double)x); }

double asin(double x) {
  if (x > 1.0 || x < -1.0) return 0.0/0.0;
  double t = sqrt(1.0 - x*x);
  if (t == 0.0) return (x > 0.0) ? 1.5707963267948966 : -1.5707963267948966;
  return ajit_shim_atan(x / t);
}
float asinf(float x) { return (float)asin((double)x); }
long double asinl(long double x) { return (long double)asin((double)x); }

double acos(double x) {
  if (x > 1.0 || x < -1.0) return 0.0/0.0;
  return 1.5707963267948966 - asin(x);
}
float acosf(float x) { return (float)acos((double)x); }
long double acosl(long double x) { return (long double)acos((double)x); }

static double ajit_shim_pow_int(double x, long long n) {
  double r = 1.0;
  double b = x;
  long long e = n;
  if (e < 0) { e = -e; b = 1.0 / b; }
  while (e) {
    if (e & 1) r *= b;
    b *= b;
    e >>= 1;
  }
  return r;
}

double pow(double x, double y) {
  long yi = (long)y;
  double dy = y - (double)yi;
  if (ajit_shim_absd(dy) < 1e-12) {
    return ajit_shim_pow_int(x, (long long)yi);
  }
  if (x <= 0.0) {
    return 0.0/0.0;
  }
  return ajit_shim_exp(y * log(x));
}
float powf(float x, float y) { return (float)pow((double)x, (double)y); }
