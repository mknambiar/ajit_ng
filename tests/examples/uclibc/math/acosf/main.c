#include "ajit_shim.h"

int b;
void main() {
  // b should be 1570
  b = (int)(acosf(0) * 1000);
}
