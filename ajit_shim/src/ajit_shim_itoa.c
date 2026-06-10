#include "ajit_shim_itoa.h"

void ajit_shim_u32_to_dec(char *buf, uint32_t v) {
  char tmp[11];
  int i = 0;

  if (v == 0) {
    buf[0] = '0';
    buf[1] = 0;
    return;
  }

  while (v > 0) {
    uint32_t q = v / 10;
    uint32_t r = v - (q * 10);
    tmp[i++] = (char)('0' + r);
    v = q;
  }

  for (int j = 0; j < i; j++) {
    buf[j] = tmp[i - 1 - j];
  }
  buf[i] = 0;
}
