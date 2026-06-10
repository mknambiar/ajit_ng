#include "ajit_shim.h"

int b;
void main() {
  // b should be 1570
  b = (int)(asinf(1) * 1000);
}
