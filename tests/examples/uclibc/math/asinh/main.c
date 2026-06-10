#include "ajit_shim.h"

int b;
void main() {
  // b should be 277
  b = (int)(asinh(8) * 100);
}
