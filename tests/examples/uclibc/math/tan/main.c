#include "ajit_shim.h"
#include <stdio.h>
#define M_PI 3.14159265358979323846

int b;
void main() {
  //b = (int)tan(M_PI/2.0);
  b = (int)(tan(M_PI/4.0) * 10);
}
