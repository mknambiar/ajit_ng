#include "ajit_shim.h"
#define M_PI 3.14159265358979323846
int b;
int main(){
  b = (int)sin(M_PI/2.0);
  return 1;
}
